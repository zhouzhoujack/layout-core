#include "geoLayoutUtils.h"
#include "geoCellVariants.h"
#include "geoRegionUtils.h"
#include "utilsProgress.h"

namespace geo
{

// ------------------------------------------------------------------------------------
//  DirectLayerMapping implementation

DirectLayerMapping::DirectLayerMapping (geo::Layout *target_layout)
  : ImportLayerMapping (), mp_layout (target_layout), m_initialized (false)
{
  // .. nothing yet ..
}

std::pair <bool, unsigned int> 
DirectLayerMapping::map_layer (const LayerProperties &lprops)
{
  if (! m_initialized) {
    for (geo::Layout::layer_iterator l = mp_layout->begin_layers (); l != mp_layout->end_layers (); ++l) {
      m_lmap.insert (std::make_pair (*(*l).second, (*l).first));
    }
    m_initialized = true;
  }

  std::map <geo::LayerProperties, unsigned int, geo::LPLogicalLessFunc>::const_iterator lm = m_lmap.find (lprops);
  if (lm != m_lmap.end ()) {
    return std::make_pair (true, lm->second);
  } else {
    return std::make_pair (true, m_lmap.insert (std::make_pair (lprops, mp_layout->insert_layer (lprops))).first->second);
  }
}

// ------------------------------------------------------------------------------------
//  PropertyMapper implementation

PropertyMapper::PropertyMapper (geo::Layout &target, const geo::Layout &source)
  : mp_target (&target), mp_source (&source)
{
  //  .. nothing yet ..
}

/**
 *  @brief Instantiate a property mapper for mapping of property ids from the source to the target layout
 *
 *  This version does not specify a certain source or target layout. These must be set with the
 *  set_source or set_target methods.
 */
PropertyMapper::PropertyMapper ()
  : mp_target (0), mp_source (0)
{
  //  .. nothing yet ..
}

/**
 *  @brief Specify the source layout
 */
void 
PropertyMapper::set_source (const geo::Layout &source)
{
  if (&source != mp_source) {
    m_prop_id_map.clear ();
    mp_source = &source;
  }
}

/**
 *  @brief Specify the target layout
 */
void 
PropertyMapper::set_target (geo::Layout &target)
{
  if (&target != mp_target) {
    m_prop_id_map.clear ();
    mp_target = &target;
  }
}

/**
 *  @brief The actual mapping function
 */
geo::Layout::properties_id_type 
PropertyMapper::operator() (geo::Layout::properties_id_type source_id)
{
  if (source_id == 0 || mp_source == mp_target) {
    return source_id;
  }

  tl_assert (mp_source != 0);
  tl_assert (mp_target != 0);

  std::map <geo::Layout::properties_id_type, geo::Layout::properties_id_type>::const_iterator p = m_prop_id_map.find (source_id);

  if (p == m_prop_id_map.end ()) {
    geo::Layout::properties_id_type new_id = mp_target->properties_repository ().translate (mp_source->properties_repository (), source_id);
    m_prop_id_map.insert (std::make_pair (source_id, new_id));
    return new_id;
  } else {
    return p->second;
  }
}

// ------------------------------------------------------------------------------------
//  merge_layouts implementation

static void
collect_cells_to_copy (const geo::Layout &source,
                       const std::vector<geo::cell_index_type> &source_cells,
                       const std::map<geo::cell_index_type, geo::cell_index_type> &cell_mapping,
                       std::set<geo::cell_index_type> &all_top_level_cells,
                       std::set<geo::cell_index_type> &all_cells_to_copy)
{
  std::vector<geo::cell_index_type> dropped_cells;

  for (std::map<geo::cell_index_type, geo::cell_index_type>::const_iterator m = cell_mapping.begin (); m != cell_mapping.end (); ++m) {
    if (m->second == DropCell) {
      dropped_cells.push_back (m->first);
    }
  }

  for (std::vector<geo::cell_index_type>::const_iterator src = source_cells.begin (); src != source_cells.end (); ++src) {

    all_cells_to_copy.insert (*src);
    all_top_level_cells.insert (*src);

    //  feed the excluded cells into the "all_cells_to_copy" cache. This will make "collect_called_cells" not
    //  dive into their hierarchy. We will later delete them there.
    all_cells_to_copy.insert (dropped_cells.begin (), dropped_cells.end ());

    source.cell (*src).collect_called_cells (all_cells_to_copy);

    for (std::vector<geo::cell_index_type>::const_iterator i = dropped_cells.begin (); i != dropped_cells.end (); ++i) {
      all_cells_to_copy.erase (*i);
      all_top_level_cells.erase (*i);
    }

  }
}

void 
merge_layouts (geo::Layout &target, 
               const geo::Layout &source, 
               const geo::ICplxTrans &trans,
               const std::vector<geo::cell_index_type> &source_cells, 
               const std::map<geo::cell_index_type, geo::cell_index_type> &cell_mapping,
               const std::map<unsigned int, unsigned int> &layer_mapping,
               std::map<geo::cell_index_type, geo::cell_index_type> *final_cell_mapping)
{
  //  collect all called cells and all top level cells
  std::set<geo::cell_index_type> all_top_level_cells;
  std::set<geo::cell_index_type> all_cells_to_copy;

  collect_cells_to_copy (source, source_cells, cell_mapping, all_top_level_cells, all_cells_to_copy);

  //  identify all new cells and create new ones
  std::map<geo::cell_index_type, geo::cell_index_type> new_cell_mapping;
  for (std::set<geo::cell_index_type>::const_iterator c = all_cells_to_copy.begin (); c != all_cells_to_copy.end (); ++c) {
    if (cell_mapping.find (*c) == cell_mapping.end ()) {
      new_cell_mapping.insert (std::make_pair (*c, target.add_cell (source.cell_name (*c))));
    }
  }

  if (final_cell_mapping) {
    for (std::map<geo::cell_index_type, geo::cell_index_type>::const_iterator m = cell_mapping.begin (); m != cell_mapping.end (); ++m) {
      if (m->second != DropCell) {
        final_cell_mapping->insert (*m);
      }
    }
    final_cell_mapping->insert (new_cell_mapping.begin (), new_cell_mapping.end ());
  }

  //  provide the property mapper
  geo::PropertyMapper pm (target, source);

  geo::RelativeProgress progress (geo::to_string (tr ("Merge cells")), all_cells_to_copy.size (), 1);

  //  actually to the mapping
  for (std::set<geo::cell_index_type>::const_iterator c = all_cells_to_copy.begin (); c != all_cells_to_copy.end (); ++c) {

    ++progress;

    geo::cell_index_type target_cell_index = 0;
    std::map<geo::cell_index_type, geo::cell_index_type>::const_iterator cm = cell_mapping.find (*c);
    if (cm == cell_mapping.end ()) {
      target_cell_index = new_cell_mapping[*c];
    } else {
      target_cell_index = cm->second;
    }

    const geo::Cell &source_cell = source.cell (*c);
    geo::Cell &target_cell = target.cell (target_cell_index);

    //  NOTE: this implementation employs the safe but cumbersome "local transformation" feature.
    //  This means, all cells are transformed according to the given transformation and their
    //  references are transformed to account for that effect. This will lead to somewhat strange
    //  local modifications.

    //  copy and transform the shapes
    for (std::map<unsigned int, unsigned int>::const_iterator lm = layer_mapping.begin (); lm != layer_mapping.end (); ++lm) {
      target_cell.shapes (lm->second).insert_transformed (source_cell.shapes (lm->first), trans, pm);
    }

    //  copy the instances
    for (geo::Cell::const_iterator inst = source_cell.begin (); !inst.at_end (); ++inst) {

      //  only copy instances for new cells ..
      std::map<geo::cell_index_type, geo::cell_index_type>::const_iterator nc = new_cell_mapping.find (inst->cell_index ());
      if (nc != new_cell_mapping.end ()) {

        geo::CellInstArray new_inst_array (inst->cell_inst ());
        new_inst_array.transform_into (trans, 0 /*no array repository*/);

        new_inst_array.object ().cell_index (nc->second);

        if (inst->has_prop_id ()) {
          target_cell.insert (geo::object_with_properties<geo::CellInstArray> (new_inst_array, pm (inst->prop_id ())));
        } else {
          target_cell.insert (new_inst_array);
        }

      }

    }

  }

}

static void 
copy_or_propagate_shapes (geo::Layout &target, 
                          const geo::Layout &source, 
                          const geo::ICplxTrans &trans,
                          const geo::ICplxTrans &propagate_trans,
                          geo::PropertyMapper &pm,
                          geo::cell_index_type source_cell_index,
                          geo::cell_index_type source_parent_cell_index,
                          unsigned int target_layer, unsigned int source_layer,
                          const std::set<geo::cell_index_type> &all_cells_to_copy,
                          const std::map<geo::cell_index_type, geo::cell_index_type> &cell_mapping,
                          const ShapesTransformer *transformer)
{
  const geo::Cell &source_cell = source.cell (source_cell_index);
  const geo::Cell &source_parent_cell = source.cell (source_parent_cell_index);

  std::map<geo::cell_index_type, geo::cell_index_type>::const_iterator cm = cell_mapping.find (source_parent_cell_index);
  if (cm == cell_mapping.end ()) {

    for (geo::Cell::parent_inst_iterator p = source_parent_cell.begin_parent_insts (); ! p.at_end (); ++p) {

      if (all_cells_to_copy.find (p->parent_cell_index ()) != all_cells_to_copy.end ()) {
        const geo::CellInstArray &cell_inst = p->child_inst ().cell_inst ();
        for (geo::CellInstArray::iterator a = cell_inst.begin (); ! a.at_end (); ++a) {
          geo::ICplxTrans t = geo::ICplxTrans (cell_inst.complex_trans (*a)) * propagate_trans;
          copy_or_propagate_shapes (target, source, trans, t, pm, source_cell_index, p->parent_cell_index (), target_layer, source_layer, all_cells_to_copy, cell_mapping, transformer);
        }
      }

    }

  } else if (cm->second != DropCell) {

    geo::Cell &target_cell = target.cell (cm->second);
    transformer->insert_transformed (target_cell.shapes (target_layer), source_cell.shapes (source_layer), trans * propagate_trans, pm);
  }
}

static void
copy_or_move_shapes (geo::Layout &target,
                     geo::Layout &source,
                     const geo::ICplxTrans &trans,
                     const std::vector<geo::cell_index_type> &source_cells,
                     const std::map<geo::cell_index_type, geo::cell_index_type> &cell_mapping,
                     const std::map<unsigned int, unsigned int> &layer_mapping,
                     const ShapesTransformer *transformer,
                     bool move
                    )
{
  //  collect all called cells and all top level cells
  std::set<geo::cell_index_type> all_top_level_cells;
  std::set<geo::cell_index_type> all_cells_to_copy;

  collect_cells_to_copy (source, source_cells, cell_mapping, all_top_level_cells, all_cells_to_copy);

  //  provide the property mapper
  geo::PropertyMapper pm (target, source);

  geo::RelativeProgress progress (geo::to_string (tr ("Merge cells")), all_cells_to_copy.size () * layer_mapping.size (), 1);

  //  and copy
  for (std::set<geo::cell_index_type>::const_iterator c = all_cells_to_copy.begin (); c != all_cells_to_copy.end (); ++c) {
    for (std::map<unsigned int, unsigned int>::const_iterator lm = layer_mapping.begin (); lm != layer_mapping.end (); ++lm) {
      ++progress;
      copy_or_propagate_shapes (target, source, trans, geo::ICplxTrans (), pm, *c, *c, lm->second, lm->first, all_cells_to_copy, cell_mapping, transformer);
      if (move) {
        source.cell (*c).shapes (lm->first).clear ();
      }
    }
  }
}

namespace
{
  class StandardShapesTransformer
    : public ShapesTransformer
  {
  public:
    void insert_transformed (Shapes &into, const Shapes &from, const ICplxTrans &trans, PropertyMapper &pm) const
    {
      into.insert_transformed (from, trans, pm);
    }
  };
}

void
copy_shapes (geo::Layout &target, 
             const geo::Layout &source, 
             const geo::ICplxTrans &trans,
             const std::vector<geo::cell_index_type> &source_cells, 
             const std::map<geo::cell_index_type, geo::cell_index_type> &cell_mapping,
             const std::map<unsigned int, unsigned int> &layer_mapping,
             const ShapesTransformer *transformer)
{
  StandardShapesTransformer st;
  if (! transformer) {
    transformer = &st;
  }
  copy_or_move_shapes (target, const_cast<geo::Layout &> (source), trans, source_cells, cell_mapping, layer_mapping, transformer, false);
}

void 
move_shapes (geo::Layout &target, 
             geo::Layout &source, 
             const geo::ICplxTrans &trans,
             const std::vector<geo::cell_index_type> &source_cells, 
             const std::map<geo::cell_index_type, geo::cell_index_type> &cell_mapping,
             const std::map<unsigned int, unsigned int> &layer_mapping,
             const ShapesTransformer *transformer)
{
  StandardShapesTransformer st;
  if (! transformer) {
    transformer = &st;
  }
  copy_or_move_shapes (target, source, trans, source_cells, cell_mapping, layer_mapping, transformer, true);
}

// ------------------------------------------------------------
//  Implementation of "find_layout_context"

static std::pair<bool, geo::ICplxTrans> 
find_layout_context (const geo::Layout &layout, geo::cell_index_type from, geo::cell_index_type to, std::set <geo::cell_index_type> &visited, const geo::ICplxTrans &trans) 
{
  const geo::Cell &cell = layout.cell (from);
  for (geo::Cell::parent_inst_iterator p = cell.begin_parent_insts (); ! p.at_end (); ++p) {

    if (p->parent_cell_index () == to) {

      return std::make_pair (true, geo::ICplxTrans (p->child_inst ().complex_trans ()) * trans);

    } else if (visited.find (p->parent_cell_index ()) == visited.end ()) {

      visited.insert (p->parent_cell_index ());

      std::pair<bool, geo::ICplxTrans> context = find_layout_context (layout, p->parent_cell_index (), to, visited, geo::ICplxTrans (p->child_inst ().complex_trans ()) * trans);
      if (context.first) {
        return context;
      }

    }

  }

  return std::pair<bool, geo::ICplxTrans> (false, geo::ICplxTrans ());
}

std::pair<bool, geo::ICplxTrans> 
find_layout_context (const geo::Layout &layout, geo::cell_index_type from, geo::cell_index_type to)
{
  if (from == to) {
    return std::make_pair (true, geo::ICplxTrans ());
  } else {
    std::set <geo::cell_index_type> v;
    return find_layout_context (layout, from, to, v, geo::ICplxTrans ());
  }
}

// ------------------------------------------------------------
//  Implementation of ContextCache

ContextCache::ContextCache (const geo::Layout *layout)
  : mp_layout (layout)
{
  //  .. nothing yet ..
}

const std::pair<bool, geo::ICplxTrans> &
ContextCache::find_layout_context (geo::cell_index_type from, geo::cell_index_type to)
{
  if (! mp_layout) {
    static std::pair<bool, geo::ICplxTrans> nothing (false, geo::ICplxTrans ());
    return nothing;
  }

  std::map<std::pair<geo::cell_index_type, geo::cell_index_type>, std::pair<bool, geo::ICplxTrans> >::iterator c = m_cache.find (std::make_pair (from, to));
  if (c == m_cache.end ()) {
    c = m_cache.insert (std::make_pair (std::make_pair (from, to), std::make_pair (false, geo::ICplxTrans ()))).first;
    c->second = geo::find_layout_context (*mp_layout, from, to);
  }
  return c->second;
}

// ------------------------------------------------------------
//  Scale and snap a layout

void
scale_and_snap (geo::Layout &layout, geo::Cell &cell, geo::Coord g, geo::Coord m, geo::Coord d)
{
  if (g < 0) {
    throw geo::Exception (geo::to_string (tr ("Snapping requires a positive grid value")));
  }

  if (m <= 0 || d <= 0) {
    throw geo::Exception (geo::to_string (tr ("Scale and snap requires positive and non-null magnification or divisor values")));
  }

  if (! g && m == d) {
    return;
  }

  geo::cell_variants_collector<geo::ScaleAndGridReducer> vars (geo::ScaleAndGridReducer (g, m, d));

  vars.collect (layout, cell);
  vars.separate_variants (layout, cell);

  std::set<geo::cell_index_type> called_cells;
  cell.collect_called_cells (called_cells);
  called_cells.insert (cell.cell_index ());

  geo::LayoutLocker layout_locker (&layout);
  layout.update ();

  std::vector<geo::Point> heap;

  unsigned int work_layer = layout.insert_layer ();

  for (geo::Layout::iterator c = layout.begin (); c != layout.end (); ++c) {

    if (called_cells.find (c->cell_index ()) == called_cells.end ()) {
      continue;
    }

    const std::map<geo::ICplxTrans, size_t> &v = vars.variants (c->cell_index ());
    tl_assert (v.size () == size_t (1));
    geo::ICplxTrans tr = v.begin ()->first;

    //  NOTE: tr_disp is already multiplied with mag, so it can be an integer
    geo::Vector tr_disp = tr.disp ();

    tr.disp (geo::Vector ());
    geo::ICplxTrans trinv = tr.inverted ();

    for (geo::Layout::layer_iterator l = layout.begin_layers (); l != layout.end_layers (); ++l) {

      geo::Shapes &s = c->shapes ((*l).first);
      geo::Shapes &out = c->shapes (work_layer);

      for (geo::Shapes::shape_iterator si = s.begin (geo::ShapeIterator::Polygons | geo::ShapeIterator::Paths | geo::ShapeIterator::Boxes); ! si.at_end (); ++si) {

        geo::Polygon poly;
        si->polygon (poly);
        poly.transform (tr);
        poly = scaled_and_snapped_polygon (poly, g, m, d, tr_disp.x (), g, m, d, tr_disp.y (), heap);
        poly.transform (trinv);
        out.insert (poly);

      }

      for (geo::Shapes::shape_iterator si = s.begin (geo::ShapeIterator::Texts); ! si.at_end (); ++si) {

        geo::Text text;
        si->text (text);
        text.transform (tr);
        text.trans (geo::Trans (text.trans ().rot (), scaled_and_snapped_vector (text.trans ().disp (), g, m, d, tr_disp.x (), g, m, d, tr_disp.y ())));
        text.transform (trinv);
        out.insert (text);

      }

      s.swap (out);
      out.clear ();

    }

    //  Snap instance placements to grid and magnify
    //  NOTE: we can modify the instances because the ScaleAndGridReducer marked every cell with children
    //  as a variant cell (an effect of ScaleAndGridReducer::want_variants(cell) == true where cells have children).
    //  Variant cells are not copied blindly back to the original layout.

    std::list<geo::CellInstArray> new_insts;

    for (geo::Cell::const_iterator inst = c->begin (); ! inst.at_end (); ++inst) {

      const geo::CellInstArray &ia = inst->cell_inst ();
      for (geo::CellInstArray::iterator i = ia.begin (); ! i.at_end (); ++i) {

        geo::Trans ti (*i);
        geo::Vector ti_disp = ti.disp ();
        ti_disp.transform (tr);
        ti_disp = scaled_and_snapped_vector (ti_disp, g, m, d, tr_disp.x (), g, m, d, tr_disp.y ());
        ti_disp.transform (trinv);
        ti.disp (ti_disp);

        if (ia.is_complex ()) {
          new_insts.push_back (geo::CellInstArray (ia.object (), ia.complex_trans (ti)));
        } else {
          new_insts.push_back (geo::CellInstArray (ia.object (), ti));
        }

      }

    }

    c->clear_insts ();

    for (std::list<geo::CellInstArray>::const_iterator i = new_insts.begin (); i != new_insts.end (); ++i) {
      c->insert (*i);
    }

  }
}

}

