#include "geoRecursiveShapeIterator.h"
#include "geoHierarchyBuilder.h"
#include "geoClip.h"
#include "geoRegion.h"
#include "geoPolygonTools.h"

namespace geo
{

static HierarchyBuilderShapeInserter def_inserter;

static HierarchyBuilder::cell_map_type null_map;
static HierarchyBuilder::cell_map_type::const_iterator null_iterator = null_map.end ();

// -------------------------------------------------------------------------------------------

int
compare_iterators_with_respect_to_target_hierarchy (const geo::RecursiveShapeIterator &iter1, const geo::RecursiveShapeIterator &iter2)
{
  if ((iter1.layout () == 0) != (iter2.layout () == 0)) {
    return (iter1.layout () == 0) < (iter2.layout () == 0) ? -1 : 1;
  }
  if ((iter1.top_cell () == 0) != (iter2.top_cell () == 0)) {
    return (iter1.top_cell () == 0) < (iter2.top_cell () == 0) ? -1 : 1;
  }

  //  basic source (layout, top_cell) needs to be the same of course
  if (iter1.layout () != iter2.layout ()) {
    //  NOTE: pointer compare :-(
    return iter1.layout () < iter2.layout () ? -1 : 1;
  }
  if (iter1.top_cell ()) {
    if (iter1.top_cell ()->cell_index () != iter2.top_cell ()->cell_index ()) {
      return iter1.top_cell ()->cell_index () < iter2.top_cell ()->cell_index () ? -1 : 1;
    }
  }

  //  max depth controls the main hierarchical appearance
  if (iter1.max_depth () != iter2.max_depth ()) {
    return iter1.max_depth () < iter2.max_depth () ? -1 : 1;
  }

  //  take potential selection of cells into account
  if (iter1.disables () != iter2.disables ()) {
    return iter1.disables () < iter2.disables () ? -1 : 1;
  }
  if (iter1.enables () != iter2.enables ()) {
    return iter1.enables () < iter2.enables () ? -1 : 1;
  }

  //  compare global transformations
  if (! iter1.global_trans ().equal (iter2.global_trans ())) {
    return iter1.global_trans ().less (iter2.global_trans ()) ? -1 : 1;
  }

  //  if a region is set, the hierarchical appearance is the same only if the layers and
  //  complex region are identical
  if ((iter1.region () == geo::Box::world ()) != (iter2.region () == geo::Box::world ())) {
    return (iter1.region () == geo::Box::world ()) < (iter2.region () == geo::Box::world ()) ? -1 : 1;
  }

  if (iter1.region () != geo::Box::world ()) {
    if (iter1.has_complex_region () != iter2.has_complex_region ()) {
      return iter1.has_complex_region () < iter2.has_complex_region () ? -1 : 1;
    }
    if (iter1.has_complex_region () && iter1.complex_region () != iter2.complex_region ()) {
      return iter1.complex_region () < iter2.complex_region () ? -1 : 1;
    }
    if (iter1.region () != iter2.region ()) {
      return iter1.region () < iter2.region () ? -1 : 1;
    }
    if (iter1.multiple_layers () != iter2.multiple_layers ()) {
      return iter1.multiple_layers () < iter2.multiple_layers () ? -1 : 1;
    }
    if (iter1.multiple_layers ()) {
      if (iter1.layers () != iter2.layers ()) {
        return iter1.layers () < iter2.layers () ? -1 : 1;
      }
    } else {
      if (iter1.layer () != iter2.layer ()) {
        return iter1.layer () < iter2.layer () ? -1 : 1;
      }
    }
  }

  return 0;
}

// -------------------------------------------------------------------------------------------

/**
 *  @brief Computes the clip variant (a box set) from a cell bbox, a region and a complex region (optional)
 */
static std::pair<bool, std::set<geo::Box> > compute_clip_variant (const geo::Box &cell_bbox, const geo::ICplxTrans &trans, const geo::Box &region, const geo::RecursiveShapeReceiver::box_tree_type *complex_region)
{
  if (region == geo::Box::world ()) {
    return std::make_pair (true, std::set<geo::Box> ());
  }

  geo::ICplxTrans trans_inv (trans.inverted ());
  geo::Box region_in_cell = region.transformed (trans_inv);

  std::set<geo::Box> clip_variant;
  if (! cell_bbox.overlaps (region_in_cell)) {
    //  an empty clip variant should not happen, but who knows
    return std::make_pair (false, std::set<geo::Box> ());
  }

  geo::Box rect_box = region_in_cell & cell_bbox;

  if (complex_region) {

    for (geo::RecursiveShapeReceiver::box_tree_type::overlapping_iterator cr = complex_region->begin_overlapping (region, geo::box_convert<geo::Box> ()); ! cr.at_end (); ++cr) {
      geo::Box cr_in_cell = (*cr).transformed (trans_inv);
      if (rect_box.overlaps (cr_in_cell)) {
        clip_variant.insert (rect_box * cr_in_cell);
      }
    }

    if (clip_variant.empty ()) {
      //  an empty clip variant should not happen, but who knows
      return std::make_pair (false, std::set<geo::Box> ());
    }

  } else {
    clip_variant.insert (rect_box);
  }

  return std::make_pair (true, clip_variant);
}

HierarchyBuilder::HierarchyBuilder (geo::Layout *target, unsigned int target_layer, const geo::ICplxTrans &trans, HierarchyBuilderShapeReceiver *pipe)
  : mp_target (target), m_initial_pass (true), m_cm_new_entry (false), m_target_layer (target_layer), m_trans (trans)
{
  set_shape_receiver (pipe);
}

HierarchyBuilder::HierarchyBuilder (geo::Layout *target, const geo::ICplxTrans &trans, HierarchyBuilderShapeReceiver *pipe)
  : mp_target (target), m_initial_pass (true), m_cm_new_entry (false), m_target_layer (0), m_trans (trans)
{
  set_shape_receiver (pipe);
}

HierarchyBuilder::~HierarchyBuilder ()
{
  //  .. nothing yet ..
}

void
HierarchyBuilder::set_shape_receiver (HierarchyBuilderShapeReceiver *pipe)
{
  mp_pipe = pipe ? pipe : &def_inserter;
}

void
HierarchyBuilder::reset ()
{
  m_initial_pass = true;
  mp_initial_cell = 0;

  m_cells_to_be_filled.clear ();
  m_cell_map.clear ();
  m_cells_seen.clear ();
  m_cell_stack.clear ();
  m_cm_entry = null_iterator;
  m_cm_new_entry = false;
}

void
HierarchyBuilder::register_variant (geo::cell_index_type non_var, geo::cell_index_type var)
{
  //  non_var (despite it's name) may be a variant created previously.
  variant_to_original_target_map_type::const_iterator v = m_variants_to_original_target_map.find (non_var);
  if (v != m_variants_to_original_target_map.end ()) {
    non_var = v->second;
  }

  m_original_targets_to_variants_map [non_var].push_back (var);
  m_variants_to_original_target_map.insert (std::make_pair (var, non_var));
}

void
HierarchyBuilder::unregister_variant (geo::cell_index_type var)
{
  variant_to_original_target_map_type::iterator v = m_variants_to_original_target_map.find (var);
  if (v == m_variants_to_original_target_map.end ()) {
    return;
  }

  original_target_to_variants_map_type::iterator rv = m_original_targets_to_variants_map.find (v->second);
  tl_assert (rv != m_original_targets_to_variants_map.end ());

  std::vector<geo::cell_index_type> &vv = rv->second;
  std::vector<geo::cell_index_type>::iterator ri = std::find (vv.begin (), vv.end (), var);
  tl_assert (ri != vv.end ());
  vv.erase (ri);

  if (vv.empty ()) {
    m_original_targets_to_variants_map.erase (rv);
  }

  m_variants_to_original_target_map.erase (v);
}

geo::cell_index_type
HierarchyBuilder::original_target_for_variant (geo::cell_index_type ci) const
{
  variant_to_original_target_map_type::const_iterator v = m_variants_to_original_target_map.find (ci);
  if (v != m_variants_to_original_target_map.end ()) {
    return v->second;
  } else {
    return ci;
  }
}

void
HierarchyBuilder::begin (const RecursiveShapeIterator *iter)
{
  if (m_initial_pass) {
    m_source = *iter;
  } else {
    tl_assert (compare_iterators_with_respect_to_target_hierarchy (m_source, *iter) == 0);
  }

  m_cell_stack.clear ();
  m_cells_seen.clear ();

  if (! iter->layout () || ! iter->top_cell ()) {
    return;
  }

  CellMapKey key (iter->top_cell ()->cell_index (), false, std::set<geo::Box> ());
  m_cm_entry = m_cell_map.find (key);

  if (m_cm_entry == m_cell_map.end ()) {
    geo::cell_index_type new_top_index = mp_target->add_cell (iter->layout ()->cell_name (key.original_cell));
    m_cm_entry = m_cell_map.insert (std::make_pair (key, new_top_index)).first;
  }

  geo::Cell &new_top = mp_target->cell (m_cm_entry->second);
  m_cells_seen.insert (key);

  //  NOTE: we consider the top cell "new" if it does not have instances.
  //  We can do so as the recursive shape iterator will always deliver all instances
  //  and not a partial set of instances.
  m_cm_new_entry = new_top.begin ().at_end ();
  m_cell_stack.push_back (std::make_pair (m_cm_new_entry, std::vector<geo::Cell *> ()));
  m_cell_stack.back ().second.push_back (&new_top);
}

void
HierarchyBuilder::end (const RecursiveShapeIterator *iter)
{
  tl_assert (! iter->layout () || ! iter->top_cell () || m_cell_stack.size () == 1);

  m_initial_pass = false;
  m_cells_seen.clear ();
  mp_initial_cell = m_cell_stack.empty () ? 0 : m_cell_stack.front ().second.front ();
  m_cell_stack.clear ();
  m_cm_entry = null_iterator;
  m_cm_new_entry = false;
}

void
HierarchyBuilder::enter_cell (const RecursiveShapeIterator * /*iter*/, const geo::Cell * /*cell*/, const geo::Box & /*region*/, const box_tree_type * /*complex_region*/)
{
  tl_assert (m_cm_entry != m_cell_map.end () && m_cm_entry != null_iterator);

  m_cells_seen.insert (m_cm_entry->first);

  bool new_cell = (m_cells_to_be_filled.find (m_cm_entry->second) != m_cells_to_be_filled.end ());
  if (new_cell) {
    m_cells_to_be_filled.erase (m_cm_entry->second);
  }

  m_cell_stack.push_back (std::make_pair (new_cell, std::vector<geo::Cell *> ()));

  original_target_to_variants_map_type::const_iterator v = m_original_targets_to_variants_map.find (m_cm_entry->second);
  if (v != m_original_targets_to_variants_map.end ()) {
    for (std::vector<geo::cell_index_type>::const_iterator i = v->second.begin (); i != v->second.end (); ++i) {
      m_cell_stack.back ().second.push_back (&mp_target->cell (*i));
    }
  } else {
    m_cell_stack.back ().second.push_back (&mp_target->cell (m_cm_entry->second));
  }
}

void
HierarchyBuilder::leave_cell (const RecursiveShapeIterator * /*iter*/, const geo::Cell * /*cell*/)
{
  m_cell_stack.pop_back ();
}

geo::cell_index_type
HierarchyBuilder::make_cell_variant (const HierarchyBuilder::CellMapKey &key, const std::string &cell_name)
{
  m_cm_entry = m_cell_map.find (key);
  m_cm_new_entry = false;

  geo::cell_index_type new_cell;

  if (m_cm_entry == m_cell_map.end ()) {

    std::string cn = cell_name;
    if (! key.clip_region.empty ()) {
      cn += "$CLIP_VAR";
    }
    if (key.inactive) {
      cn += "$DIS";
    }
    new_cell = mp_target->add_cell (cn.c_str ());
    m_cm_entry = m_cell_map.insert (std::make_pair (key, new_cell)).first;
    m_cm_new_entry = true;
    m_cells_to_be_filled.insert (new_cell);

  } else {
    new_cell = m_cm_entry->second;
  }

  return new_cell;
}

HierarchyBuilder::new_inst_mode
HierarchyBuilder::new_inst (const RecursiveShapeIterator *iter, const geo::CellInstArray &inst, const geo::ICplxTrans &always_apply, const geo::Box & /*region*/, const box_tree_type * /*complex_region*/, bool all)
{
  if (all) {

    CellMapKey key (inst.object ().cell_index (), iter->is_child_inactive (inst.object ().cell_index ()), std::set<geo::Box> ());
    geo::cell_index_type new_cell = make_cell_variant (key, iter->layout ()->cell_name (inst.object ().cell_index ()));

    //  for new cells, create this instance
    if (m_cell_stack.back ().first) {
      geo::CellInstArray new_inst (inst, &mp_target->array_repository ());
      new_inst.object () = geo::CellInst (new_cell);
      new_inst.transform (always_apply);
      new_inst.transform_into (m_trans);
      for (std::vector<geo::Cell *>::const_iterator c = m_cell_stack.back ().second.begin (); c != m_cell_stack.back ().second.end (); ++c) {
        (*c)->insert (new_inst);
      }
    }

    //  To see the cell once, use NI_single. If we did see the cell already, skip the whole instance array.
    return (m_cells_seen.find (key) == m_cells_seen.end ()) ? NI_single : NI_skip;

  } else {

    //  Iterate by instance array members
    return NI_all;

  }
}

bool
HierarchyBuilder::new_inst_member (const RecursiveShapeIterator *iter, const geo::CellInstArray &inst, const geo::ICplxTrans &always_apply, const geo::ICplxTrans &trans, const geo::Box &region, const box_tree_type *complex_region, bool all)
{
  if (all) {

    return true;

  } else {

    geo::Box cell_bbox = iter->layout ()->cell (inst.object ().cell_index ()).bbox ();
    std::pair<bool, std::set<geo::Box> > clip_variant = compute_clip_variant (cell_bbox, trans, region, complex_region);
    if (! clip_variant.first) {
      return false;
    }

    CellMapKey key (inst.object ().cell_index (), iter->is_child_inactive (inst.object ().cell_index ()), clip_variant.second);
    geo::cell_index_type new_cell = make_cell_variant (key, iter->layout ()->cell_name (inst.object ().cell_index ()));

    //  for a new cell, create this instance
    if (m_cell_stack.back ().first) {
      geo::CellInstArray new_inst (geo::CellInst (new_cell), always_apply * trans);
      new_inst.transform_into (m_trans);
      for (std::vector<geo::Cell *>::const_iterator c = m_cell_stack.back ().second.begin (); c != m_cell_stack.back ().second.end (); ++c) {
        (*c)->insert (new_inst);
      }
    }

    return (m_cells_seen.find (key) == m_cells_seen.end ());

  }
}

void
HierarchyBuilder::shape (const RecursiveShapeIterator * /*iter*/, const geo::Shape &shape, const geo::ICplxTrans &apply_always, const geo::ICplxTrans & /*trans*/, const geo::Box &region, const box_tree_type *complex_region)
{
  for (std::vector<geo::Cell *>::const_iterator c = m_cell_stack.back ().second.begin (); c != m_cell_stack.back ().second.end (); ++c) {
    geo::Shapes &shapes = (*c)->shapes (m_target_layer);
    mp_pipe->push (shape, m_trans * apply_always, region, complex_region, &shapes);
  }
}

// ---------------------------------------------------------------------------------------------

ClippingHierarchyBuilderShapeReceiver::ClippingHierarchyBuilderShapeReceiver (HierarchyBuilderShapeReceiver *pipe)
  : mp_pipe (pipe ? pipe : &def_inserter)
{
  //  .. nothing yet ..
}

void
ClippingHierarchyBuilderShapeReceiver::push (const geo::Shape &shape, const geo::ICplxTrans &trans, const geo::Box &region, const geo::RecursiveShapeReceiver::box_tree_type *complex_region, geo::Shapes *target)
{
  static geo::Box world = geo::Box::world ();

  if (region == world || is_inside (shape.bbox (), region, complex_region)) {

    mp_pipe->push (shape, trans, world, 0, target);

  } else if (! is_outside (shape.bbox (), region, complex_region)) {

    //  clip the shape if required
    if (shape.is_text () || shape.is_edge () || shape.is_edge_pair ()) {
      mp_pipe->push (shape, trans, world, 0, target);
    } else if (shape.is_box ()) {
      insert_clipped (shape.box (), trans, region, complex_region, target);
    } else if (shape.is_polygon () || shape.is_simple_polygon () || shape.is_path ()) {
      geo::Polygon poly;
      shape.polygon (poly);
      insert_clipped (poly, trans, region, complex_region, target);
    }

  }
}

void
ClippingHierarchyBuilderShapeReceiver::push (const geo::Box &shape, const geo::ICplxTrans &trans, const geo::Box &region, const geo::RecursiveShapeReceiver::box_tree_type *complex_region, geo::Shapes *target)
{
  static geo::Box world = geo::Box::world ();

  if (! complex_region) {
    geo::Box r = shape & region;
    if (! r.empty()) {
      mp_pipe->push (r, trans, world, 0, target);
    }
  } else {
    insert_clipped (shape, trans, region, complex_region, target);
  }
}

void
ClippingHierarchyBuilderShapeReceiver::push (const geo::Polygon &shape, const geo::ICplxTrans &trans, const geo::Box &region, const geo::RecursiveShapeReceiver::box_tree_type *complex_region, geo::Shapes *target)
{
  static geo::Box world = geo::Box::world ();

  if (region == world || (shape.box ().inside (region) && ! complex_region)) {
    mp_pipe->push (shape, trans, world, 0, target);
  } else {
    insert_clipped (shape, trans, region, complex_region, target);
  }
}

bool
ClippingHierarchyBuilderShapeReceiver::is_inside (const geo::Box &box, const geo::Box &region, const geo::RecursiveShapeReceiver::box_tree_type *complex_region)
{
  if (region == geo::Box::world ()) {
    return true;
  }

  if (box.inside (region)) {

    geo::Box rect_box = region & box;

    if (complex_region) {

      //  TODO: this is not a real test for being inside a complex region
      for (geo::RecursiveShapeReceiver::box_tree_type::overlapping_iterator cr = complex_region->begin_overlapping (rect_box, geo::box_convert<geo::Box> ()); ! cr.at_end (); ++cr) {
        if (rect_box.inside (*cr)) {
          return true;
        }
      }

    }

  }

  return false;
}

bool
ClippingHierarchyBuilderShapeReceiver::is_outside (const geo::Box &box, const geo::Box &region, const geo::RecursiveShapeReceiver::box_tree_type *complex_region)
{
  if (region == geo::Box::world ()) {
    return false;
  }

  if (box.overlaps (region)) {

    geo::Box rect_box = region & box;

    if (complex_region) {
      for (geo::RecursiveShapeReceiver::box_tree_type::overlapping_iterator cr = complex_region->begin_overlapping (rect_box, geo::box_convert<geo::Box> ()); ! cr.at_end (); ++cr) {
        if (rect_box.overlaps (*cr)) {
          return false;
        }
      }
    } else {
      return false;
    }

  }

  return true;
}

void
ClippingHierarchyBuilderShapeReceiver::insert_clipped (const geo::Box &box, const geo::ICplxTrans &trans, const geo::Box &region, const geo::RecursiveShapeReceiver::box_tree_type *complex_region, geo::Shapes *target)
{
  geo::Box bb = box & region;
  static geo::Box world = geo::Box::world ();

  if (complex_region) {
    for (geo::RecursiveShapeReceiver::box_tree_type::overlapping_iterator cr = complex_region->begin_overlapping (bb, geo::box_convert<geo::Box> ()); ! cr.at_end (); ++cr) {
      geo::Box bc = *cr & bb;
      if (! bc.empty ()) {
        mp_pipe->push (bc, trans, world, 0, target);
      }
    }
  } else if (! bb.empty ()) {
    mp_pipe->push (bb, trans, world, 0, target);
  }
}

void
ClippingHierarchyBuilderShapeReceiver::insert_clipped (const geo::Polygon &poly, const geo::ICplxTrans &trans, const geo::Box &region, const geo::RecursiveShapeReceiver::box_tree_type *complex_region, geo::Shapes *target)
{
  std::vector<geo::Polygon> clipped_poly;
  static geo::Box world = geo::Box::world ();

  if (complex_region) {
    //  TODO: is this good way to clip a polygon at a complex boundary?
    for (geo::RecursiveShapeReceiver::box_tree_type::overlapping_iterator cr = complex_region->begin_overlapping (region, geo::box_convert<geo::Box> ()); ! cr.at_end (); ++cr) {
      geo::clip_poly (poly, *cr & region, clipped_poly);
    }
  } else {
    geo::clip_poly (poly, region, clipped_poly);
  }

  for (std::vector<geo::Polygon>::const_iterator p = clipped_poly.begin (); p != clipped_poly.end (); ++p) {
    mp_pipe->push (*p, trans, world, 0, target);
  }
}

// ---------------------------------------------------------------------------------------------

ReducingHierarchyBuilderShapeReceiver::ReducingHierarchyBuilderShapeReceiver (HierarchyBuilderShapeReceiver *pipe, double area_ratio, size_t max_vertex_count, bool reject_odd_polygons)
  : mp_pipe (pipe ? pipe : &def_inserter), m_area_ratio (area_ratio), m_max_vertex_count (max_vertex_count), m_reject_odd_polygons (reject_odd_polygons)
{
  //  .. nothing yet ..
}

void
ReducingHierarchyBuilderShapeReceiver::push (const geo::Shape &shape, const geo::ICplxTrans &trans, const geo::Box &region, const geo::RecursiveShapeReceiver::box_tree_type *complex_region, geo::Shapes *target)
{
  if (shape.is_text () || shape.is_edge () || shape.is_edge_pair ()) {
    mp_pipe->push (shape, trans, region, complex_region, target);
  } else if (shape.is_box ()) {
    mp_pipe->push (shape.box (), trans, region, complex_region, target);
  } else if (shape.is_polygon () || shape.is_simple_polygon () || shape.is_path ()) {
    geo::Polygon poly;
    shape.polygon (poly);
    reduce (poly, trans, region, complex_region, target);
  }
}

void
ReducingHierarchyBuilderShapeReceiver::push (const geo::Box &shape, const geo::ICplxTrans &trans, const geo::Box &region, const geo::RecursiveShapeReceiver::box_tree_type *complex_region, geo::Shapes *target)
{
  mp_pipe->push (shape, trans, region, complex_region, target);
}

void
ReducingHierarchyBuilderShapeReceiver::push (const geo::Polygon &shape, const geo::ICplxTrans &trans, const geo::Box &region, const geo::RecursiveShapeReceiver::box_tree_type *complex_region, geo::Shapes *target)
{
  reduce (shape, trans, region, complex_region, target);
}

void
ReducingHierarchyBuilderShapeReceiver::reduce (const geo::Polygon &poly, const geo::ICplxTrans &trans, const geo::Box &region, const geo::RecursiveShapeReceiver::box_tree_type *complex_region, geo::Shapes *target, bool check)
{
  if (check && m_reject_odd_polygons && is_non_orientable_polygon (poly)) {
    //  non-orientable polygons generate an error
    if (target->cell () && target->cell ()->layout ()) {
      throw geo::Exception (geo::to_string (tr ("Non-orientable polygon encountered: %s in cell %s")), poly.to_string (), target->cell ()->layout ()->cell_name (target->cell ()->cell_index ()));
    } else {
      throw geo::Exception (geo::to_string (tr ("Non-orientable polygon encountered: %s")), poly.to_string ());
    }
  }

  //  NOTE: only halfmanhattan polygons are guaranteed not to generate grid snap artefacts when splitting.
  //  This is important to maintain the connection integrity of shape clusters.
  if (poly.is_halfmanhattan () && ((m_max_vertex_count >= 4 && poly.vertices () > m_max_vertex_count) || (m_area_ratio > 2.0 && poly.area_ratio () > m_area_ratio))) {

    std::vector <geo::Polygon> split_polygons;
    geo::split_polygon (poly, split_polygons);

    for (std::vector <geo::Polygon>::const_iterator sp = split_polygons.begin (); sp != split_polygons.end (); ++sp) {
      reduce (*sp, trans, region, complex_region, target, false);
    }

  } else {
    mp_pipe->push (poly, trans, region, complex_region, target);
  }
}

// ---------------------------------------------------------------------------------------------

PolygonReferenceHierarchyBuilderShapeReceiver::PolygonReferenceHierarchyBuilderShapeReceiver (geo::Layout *layout, int text_enlargement, const geo::Variant &text_prop_name)
  : mp_layout (layout), m_text_enlargement (text_enlargement), m_make_text_prop (false), m_text_prop_id (0)
{
  if (! text_prop_name.is_nil ()) {
    m_text_prop_id = layout->properties_repository ().prop_name_id (text_prop_name);
    m_make_text_prop = true;
  }
}

void PolygonReferenceHierarchyBuilderShapeReceiver::push (const geo::Shape &shape, const geo::ICplxTrans &trans, const geo::Box &, const geo::RecursiveShapeReceiver::box_tree_type *, geo::Shapes *target)
{
  if (shape.is_box () || shape.is_polygon () || shape.is_simple_polygon () || shape.is_path ()) {

    geo::Polygon poly;
    shape.polygon (poly);
    if (! trans.is_unity ()) {
      poly.transform (trans);
    }

    //  NOTE: as this is a specialized receiver for the purpose of building region
    //  representations we don't need empty polygons here
    if (poly.area2 () > 0) {
      target->insert (geo::PolygonRef (poly, mp_layout->shape_repository ()));
    }

  } else if (shape.is_text () && m_text_enlargement >= 0) {

    geo::Polygon poly (shape.text_trans () * geo::Box (-m_text_enlargement, -m_text_enlargement, m_text_enlargement, m_text_enlargement));
    if (! trans.is_unity ()) {
      poly.transform (trans);
    }
    geo::PolygonRef pref (poly, mp_layout->shape_repository ());

    if (m_make_text_prop) {

      geo::PropertiesRepository::properties_set ps;
      ps.insert (std::make_pair (m_text_prop_id, geo::Variant (shape.text_string ())));
      geo::properties_id_type pid = mp_layout->properties_repository ().properties_id (ps);

      target->insert (geo::PolygonRefWithProperties (pref, pid));

    } else {
      target->insert (pref);
    }

  }
}

void PolygonReferenceHierarchyBuilderShapeReceiver::push (const geo::Box &shape, const geo::ICplxTrans &trans, const geo::Box &, const geo::RecursiveShapeReceiver::box_tree_type *, geo::Shapes *target)
{
  if (shape.area () > 0) {
    target->insert (geo::PolygonRef (geo::Polygon (shape.transformed (trans)), mp_layout->shape_repository ()));
  }
}

void PolygonReferenceHierarchyBuilderShapeReceiver::push (const geo::Polygon &shape, const geo::ICplxTrans &trans, const geo::Box &, const geo::RecursiveShapeReceiver::box_tree_type *, geo::Shapes *target)
{
  if (shape.area2 () > 0) {
    target->insert (geo::PolygonRef (shape.transformed (trans), mp_layout->shape_repository ()));
  }
}

// ---------------------------------------------------------------------------------------------

EdgeBuildingHierarchyBuilderShapeReceiver::EdgeBuildingHierarchyBuilderShapeReceiver (bool as_edges)
  : m_as_edges (as_edges)
{
  //  .. nothing yet ..
}

void EdgeBuildingHierarchyBuilderShapeReceiver::push (const geo::Shape &shape, const geo::ICplxTrans &trans, const geo::Box &region, const geo::RecursiveShapeReceiver::box_tree_type *complex_region, geo::Shapes *target)
{
  if (m_as_edges && (shape.is_polygon () || shape.is_simple_polygon () || shape.is_path ())) {
    geo::Polygon poly;
    shape.polygon (poly);
    push (poly, trans, region, complex_region, target);
  } else if (m_as_edges && shape.is_box ()) {
    push (shape.box (), trans, region, complex_region, target);
  } else if (shape.is_edge ()) {
    target->insert (shape.edge ());
  }
}

void EdgeBuildingHierarchyBuilderShapeReceiver::push (const geo::Box &box, const geo::ICplxTrans &trans, const geo::Box &, const geo::RecursiveShapeReceiver::box_tree_type *, geo::Shapes *target)
{
  if (m_as_edges && ! box.empty ()) {
    target->insert (geo::Edge (box.p1 (), box.upper_left ()).transformed (trans));
    target->insert (geo::Edge (box.upper_left (), box.p2 ()).transformed (trans));
    target->insert (geo::Edge (box.p2 (), box.lower_right ()).transformed (trans));
    target->insert (geo::Edge (box.lower_right (), box.p1 ()).transformed (trans));
  }
}

void EdgeBuildingHierarchyBuilderShapeReceiver::push (const geo::Polygon &poly, const geo::ICplxTrans &trans, const geo::Box &, const geo::RecursiveShapeReceiver::box_tree_type *, geo::Shapes *target)
{
  if (m_as_edges) {
    for (geo::Polygon::polygon_edge_iterator e = poly.begin_edge (); ! e.at_end (); ++e) {
      target->insert ((*e).transformed (trans));
    }
  }
}

// ---------------------------------------------------------------------------------------------

EdgePairBuildingHierarchyBuilderShapeReceiver::EdgePairBuildingHierarchyBuilderShapeReceiver ()
{
  //  .. nothing yet ..
}

void EdgePairBuildingHierarchyBuilderShapeReceiver::push (const geo::Shape &shape, const geo::ICplxTrans &trans, const geo::Box & /*region*/, const geo::RecursiveShapeReceiver::box_tree_type * /*complex_region*/, geo::Shapes *target)
{
  if (shape.is_edge_pair ()) {
    target->insert (shape.edge_pair ().transformed (trans));
  }
}

// ---------------------------------------------------------------------------------------------

TextBuildingHierarchyBuilderShapeReceiver::TextBuildingHierarchyBuilderShapeReceiver (geo::Layout *layout)
  : mp_layout (layout)
{
  //  .. nothing yet ..
}

void TextBuildingHierarchyBuilderShapeReceiver::push (const geo::Shape &shape, const geo::ICplxTrans &trans, const geo::Box & /*region*/, const geo::RecursiveShapeReceiver::box_tree_type * /*complex_region*/, geo::Shapes *target)
{
  if (shape.is_text ()) {
    //  NOTE: we intentionally skip all the text attributes (font etc.) here because in the context
    //  of a text collections we're only interested in the locations.
    geo::Text t (shape.text_string (), shape.text_trans ());
    target->insert (geo::TextRef (t.transformed (trans), mp_layout->shape_repository ()));
  }
}

}
