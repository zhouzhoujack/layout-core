#include "geoCellVariants.h"
#include "geoRegionUtils.h"
#include "utilsUtils.h"

namespace geo
{

// ------------------------------------------------------------------------------------------

geo::ICplxTrans OrientationReducer::reduce (const geo::ICplxTrans &trans) const
{
  geo::ICplxTrans res (trans);
  res.disp (geo::Vector ());
  res.mag (1.0);
  return res;
}

geo::Trans OrientationReducer::reduce (const geo::Trans &trans) const
{
  return geo::Trans (trans.fp_trans ());
}

// ------------------------------------------------------------------------------------------

geo::ICplxTrans MagnificationReducer::reduce (const geo::ICplxTrans &trans) const
{
  return geo::ICplxTrans (trans.mag ());
}

geo::Trans MagnificationReducer::reduce (const geo::Trans &) const
{
  return geo::Trans ();
}

// ------------------------------------------------------------------------------------------

geo::ICplxTrans XYAnisotropyAndMagnificationReducer::reduce (const geo::ICplxTrans &trans) const
{
  double a = trans.angle ();
  if (a > 180.0 - geo::epsilon) {
    a -= 180.0;
  }
  return geo::ICplxTrans (trans.mag (), a, false, geo::Vector ());
}

geo::Trans XYAnisotropyAndMagnificationReducer::reduce (const geo::Trans &trans) const
{
  return geo::Trans (trans.angle () % 2, false, geo::Vector ());
}

// ------------------------------------------------------------------------------------------

geo::ICplxTrans MagnificationAndOrientationReducer::reduce (const geo::ICplxTrans &trans) const
{
  geo::ICplxTrans res (trans);
  res.disp (geo::Vector ());
  return res;
}

geo::Trans MagnificationAndOrientationReducer::reduce (const geo::Trans &trans) const
{
  return geo::Trans (trans.fp_trans ());
}

// ------------------------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//  CellVariants implementation

GridReducer::GridReducer (geo::Coord grid)
  : m_grid (grid)
{
  //  .. nothing yet ..
}

geo::ICplxTrans GridReducer::reduce (const geo::ICplxTrans &trans) const
{
  //  NOTE: we need to keep magnification, angle and mirror so when combining the
  //  reduced transformations, the result will be equivalent to reducing the combined
  //  transformation.
  geo::ICplxTrans res (trans);
  res.disp (geo::Vector (trans.disp ().x () - snap_to_grid (trans.disp ().x (), m_grid), trans.disp ().y () - snap_to_grid (trans.disp ().y (), m_grid)));
  return res;
}

geo::Trans GridReducer::reduce (const geo::Trans &trans) const
{
  geo::Trans res (trans);
  res.disp (geo::Vector (trans.disp ().x () - snap_to_grid (trans.disp ().x (), m_grid), trans.disp ().y () - snap_to_grid (trans.disp ().y (), m_grid)));
  return res;
}

// ------------------------------------------------------------------------------------------

ScaleAndGridReducer::ScaleAndGridReducer (geo::Coord grid, geo::Coord mult, geo::Coord div)
  : m_mult (mult), m_grid (int64_t (grid) * int64_t (div))
{
  //  .. nothing yet ..
}

geo::ICplxTrans ScaleAndGridReducer::reduce_trans (const geo::ICplxTrans &trans) const
{
  geo::ICplxTrans res (trans);
  int64_t dx = int64_t (trans.disp ().x ()) * m_mult;
  int64_t dy = int64_t (trans.disp ().y ()) * m_mult;
  res.disp (geo::Vector (geo::Coord (dx - snap_to_grid (dx, m_grid)), geo::Coord (dy - snap_to_grid (dy, m_grid))));
  return res;
}

geo::Trans ScaleAndGridReducer::reduce_trans (const geo::Trans &trans) const
{
  geo::Trans res (trans);
  int64_t dx = int64_t (trans.disp ().x ()) * m_mult;
  int64_t dy = int64_t (trans.disp ().y ()) * m_mult;
  res.disp (geo::Vector (geo::Coord (dx - snap_to_grid (dx, m_grid)), geo::Coord (dy - snap_to_grid (dy, m_grid))));
  return res;
}

geo::ICplxTrans ScaleAndGridReducer::reduce (const geo::ICplxTrans &trans) const
{
  geo::ICplxTrans res (trans);
  int64_t dx = int64_t (trans.disp ().x ());
  int64_t dy = int64_t (trans.disp ().y ());
  res.disp (geo::Vector (geo::Coord (dx - snap_to_grid (dx, m_grid)), geo::Coord (dy - snap_to_grid (dy, m_grid))));
  return res;
}

geo::Trans ScaleAndGridReducer::reduce (const geo::Trans &trans) const
{
  geo::Trans res (trans);
  int64_t dx = int64_t (trans.disp ().x ());
  int64_t dy = int64_t (trans.disp ().y ());
  res.disp (geo::Vector (geo::Coord (dx - snap_to_grid (dx, m_grid)), geo::Coord (dy - snap_to_grid (dy, m_grid))));
  return res;
}

// ------------------------------------------------------------------------------------------

VariantsCollectorBase::VariantsCollectorBase ()
  : mp_red ()
{
  //  .. nothing yet ..
}

VariantsCollectorBase::VariantsCollectorBase (const TransformationReducer *red)
  : mp_red (red)
{
  //  .. nothing yet ..
}

void
VariantsCollectorBase::collect (const geo::Layout &layout, const geo::Cell &top_cell)
{
  tl_assert (mp_red != 0);

  //  The top cell gets a "variant" with unit transformation
  m_variants [top_cell.cell_index ()].insert (std::make_pair (geo::ICplxTrans (), 1));

  std::set<geo::cell_index_type> called;
  top_cell.collect_called_cells (called);

  for (geo::Layout::top_down_const_iterator c = layout.begin_top_down (); c != layout.end_top_down (); ++c) {

    if (called.find (*c) == called.end ()) {
      continue;
    }

    //  collect the parent variants per parent cell

    std::map<geo::cell_index_type, std::map<geo::ICplxTrans, size_t> > variants_per_parent_cell;
    for (geo::Cell::parent_inst_iterator pi = layout.cell (*c).begin_parent_insts (); ! pi.at_end (); ++pi) {
      std::map<geo::ICplxTrans, size_t> &variants = variants_per_parent_cell [pi->inst ().object ().cell_index ()];
      add_variant (variants, pi->child_inst ().cell_inst (), mp_red->is_translation_invariant ());
    }

    //  compute the resulting variants

    std::map<geo::ICplxTrans, size_t> &new_variants = m_variants [*c];

    for (std::map<geo::cell_index_type, std::map<geo::ICplxTrans, size_t> >::const_iterator pv = variants_per_parent_cell.begin (); pv != variants_per_parent_cell.end (); ++pv) {
      product (variants (pv->first), pv->second, new_variants);
    }

  }
}

void
VariantsCollectorBase::separate_variants (geo::Layout &layout, geo::Cell &top_cell, std::map<geo::cell_index_type, std::map<geo::ICplxTrans, geo::cell_index_type> > *var_table)
{
  tl_assert (mp_red != 0);

  geo::LayoutLocker locker (&layout);

  std::set<geo::cell_index_type> called;
  top_cell.collect_called_cells (called);
  called.insert (top_cell.cell_index ());

  //  create new cells for the variants

  std::map<geo::cell_index_type, std::map<geo::ICplxTrans, geo::cell_index_type> > var_table_intern;
  if (! var_table) {
    var_table = &var_table_intern;
  }

  for (geo::Layout::bottom_up_const_iterator c = layout.begin_bottom_up (); c != layout.end_bottom_up (); ++c) {

    if (called.find (*c) == called.end ()) {
      continue;
    }

    geo::Cell &cell = layout.cell (*c);

    std::map<geo::ICplxTrans, size_t> &vv = m_variants [*c];
    if (vv.size () > 1) {

      std::map<geo::ICplxTrans, geo::cell_index_type> &vt = (*var_table) [*c];

      std::vector<geo::CellInstArrayWithProperties> inst;
      inst.reserve (cell.cell_instances ());

      for (geo::Cell::const_iterator i = cell.begin (); ! i.at_end (); ++i) {
        inst.push_back (geo::CellInstArrayWithProperties (i->cell_inst (), i->prop_id ()));
      }

      cell.clear_insts ();

      int index = 0;
      for (std::map<geo::ICplxTrans, size_t>::const_iterator v = vv.begin (); v != vv.end (); ++v, ++index) {

        geo::cell_index_type ci_var;

        if (v != vv.begin ()) {

          std::string var_name = layout.cell_name (*c);
          var_name += "$VAR" + geo::to_string (index);

          ci_var = layout.add_cell (var_name.c_str ());
          copy_shapes (layout, ci_var, *c);

          //  a new entry for the variant
          m_variants [ci_var].insert (*v);

        } else {
          ci_var = *c;
        }

        vt.insert (std::make_pair (v->first, ci_var));
        create_var_instances (layout.cell (ci_var), inst, v->first, *var_table, mp_red->is_translation_invariant ());

      }

      //  correct the first (remaining) entry
      std::pair<geo::ICplxTrans, size_t> v1 = *vt.begin ();
      vv.clear ();
      vv.insert (v1);

    } else {

      //  if the children of this cell are separated, map the instances to the new variants
      bool needs_update = false;
      for (geo::Cell::child_cell_iterator cc = cell.begin_child_cells (); ! cc.at_end () && ! needs_update; ++cc) {
        if (var_table->find (*cc) != var_table->end ()) {
          needs_update = true;
        }
      }

      if (needs_update) {

        std::vector<geo::CellInstArrayWithProperties> inst;
        inst.reserve (cell.cell_instances ());

        for (geo::Cell::const_iterator i = cell.begin (); ! i.at_end (); ++i) {
          inst.push_back (geo::CellInstArrayWithProperties (i->cell_inst (), i->prop_id ()));
        }

        cell.clear_insts ();
        create_var_instances (cell, inst, vv.begin ()->first, *var_table, mp_red->is_translation_invariant ());

      }

    }

  }
}

void
VariantsCollectorBase::commit_shapes (geo::Layout &layout, geo::Cell &top_cell, unsigned int layer, std::map<geo::cell_index_type, std::map<geo::ICplxTrans, geo::Shapes> > &to_commit)
{
  tl_assert (mp_red != 0);

  if (to_commit.empty ()) {
    return;
  }

  //  NOTE: this implementation suffers from accumulation of propagated shapes: we add more levels of propagated
  //  shapes if required. We don't clean up, because we do not know when a shape collection stops being required.

  geo::LayoutLocker locker (&layout);

  std::set<geo::cell_index_type> called;
  top_cell.collect_called_cells (called);
  called.insert (top_cell.cell_index ());

  for (geo::Layout::bottom_up_const_iterator c = layout.begin_bottom_up (); c != layout.end_bottom_up (); ++c) {

    if (called.find (*c) == called.end ()) {
      continue;
    }

    geo::Cell &cell = layout.cell (*c);

    std::map<geo::ICplxTrans, size_t> &vvc = m_variants [*c];
    if (vvc.size () > 1) {

      for (std::map<geo::ICplxTrans, size_t>::const_iterator vc = vvc.begin (); vc != vvc.end (); ++vc) {

        for (geo::Cell::const_iterator i = cell.begin (); ! i.at_end (); ++i) {

          std::map<geo::cell_index_type, std::map<geo::ICplxTrans, geo::Shapes> >::const_iterator tc = to_commit.find (i->cell_index ());
          if (tc != to_commit.end ()) {

            const std::map<geo::ICplxTrans, geo::Shapes> &vt = tc->second;

            //  NOTE: this will add one more commit slot for propagation ... but we don't clean up.
            //  When would a cleanup happen?
            std::map<geo::ICplxTrans, geo::Shapes> &propagated = to_commit [*c];

            for (geo::CellInstArray::iterator ia = i->begin (); ! ia.at_end (); ++ia) {

              geo::ICplxTrans t = i->complex_trans (*ia);
              geo::ICplxTrans rt = mp_red->reduce (vc->first * mp_red->reduce_trans (t));
              std::map<geo::ICplxTrans, geo::Shapes>::const_iterator v = vt.find (rt);
              if (v != vt.end ()) {

                geo::Shapes &ps = propagated [vc->first];
                geo::ident_map<geo::Layout::properties_id_type> pm;

                for (geo::Shapes::shape_iterator si = v->second.begin (geo::ShapeIterator::All); ! si.at_end (); ++si) {
                  ps.insert (*si, t, pm);
                }

              }

            }

          }

        }

      }

    } else {

      //  single variant -> we can commit any shapes we have kept for this cell directly to the cell

      std::map<geo::cell_index_type, std::map<geo::ICplxTrans, geo::Shapes> >::iterator l = to_commit.find (*c);
      if (l != to_commit.end ()) {
        tl_assert (l->second.size () == 1);
        cell.shapes (layer).insert (l->second.begin ()->second);
        to_commit.erase (l);
      }

      //  for child cells, pull everything that needs to be committed to the parent

      for (geo::Cell::const_iterator i = cell.begin (); ! i.at_end (); ++i) {

        std::map<geo::cell_index_type, std::map<geo::ICplxTrans, geo::Shapes> >::const_iterator tc = to_commit.find (i->cell_index ());
        if (tc != to_commit.end ()) {

          const std::map<geo::ICplxTrans, geo::Shapes> &vt = tc->second;

          for (geo::CellInstArray::iterator ia = i->begin (); ! ia.at_end (); ++ia) {

            geo::ICplxTrans t = i->complex_trans (*ia);
            geo::ICplxTrans rt = mp_red->reduce (vvc.begin ()->first * mp_red->reduce_trans (t));
            std::map<geo::ICplxTrans, geo::Shapes>::const_iterator v = vt.find (rt);

            if (v != vt.end ()) {

              geo::ident_map<geo::Layout::properties_id_type> pm;

              for (geo::Shapes::shape_iterator si = v->second.begin (geo::ShapeIterator::All); ! si.at_end (); ++si) {
                cell.shapes (layer).insert (*si, t, pm);
              }

            }

          }

        }

      }

    }

  }
}

const std::map<geo::ICplxTrans, size_t> &
VariantsCollectorBase::variants (geo::cell_index_type ci) const
{
  std::map<geo::cell_index_type, std::map<geo::ICplxTrans, size_t> >::const_iterator v = m_variants.find (ci);
  static std::map<geo::ICplxTrans, size_t> empty_set;
  if (v == m_variants.end ()) {
    return empty_set;
  } else {
    return v->second;
  }
}

bool
VariantsCollectorBase::has_variants () const
{
  for (std::map<geo::cell_index_type, std::map<geo::ICplxTrans, size_t> >::const_iterator i = m_variants.begin (); i != m_variants.end (); ++i) {
    if (i->second.size () > 1) {
      return true;
    }
  }
  return false;
}

void
VariantsCollectorBase::add_variant (std::map<geo::ICplxTrans, size_t> &variants, const geo::CellInstArray &inst, bool tl_invariant) const
{
  if (tl_invariant) {
    add_variant_tl_invariant (variants, inst);
  } else {
    add_variant_non_tl_invariant (variants, inst);
  }
}

void
VariantsCollectorBase::add_variant_non_tl_invariant (std::map<geo::ICplxTrans, size_t> &variants, const geo::CellInstArray &inst) const
{
  if (inst.is_complex ()) {
    for (geo::CellInstArray::iterator i = inst.begin (); ! i.at_end (); ++i) {
      variants [mp_red->reduce_trans (inst.complex_trans (*i))] += 1;
    }
  } else {
    for (geo::CellInstArray::iterator i = inst.begin (); ! i.at_end (); ++i) {
      variants [geo::ICplxTrans (mp_red->reduce_trans (*i))] += 1;
    }
  }
}

void
VariantsCollectorBase::add_variant_tl_invariant (std::map<geo::ICplxTrans, size_t> &variants, const geo::CellInstArray &inst) const
{
  if (inst.is_complex ()) {
    variants [mp_red->reduce_trans (inst.complex_trans ())] += inst.size ();
  } else {
    variants [geo::ICplxTrans (mp_red->reduce_trans (inst.front ()))] += inst.size ();
  }
}

void
VariantsCollectorBase::product (const std::map<geo::ICplxTrans, size_t> &v1, const std::map<geo::ICplxTrans, size_t> &v2, std::map<geo::ICplxTrans, size_t> &prod) const
{
  for (std::map<geo::ICplxTrans, size_t>::const_iterator i = v1.begin (); i != v1.end (); ++i) {
    for (std::map<geo::ICplxTrans, size_t>::const_iterator j = v2.begin (); j != v2.end (); ++j) {
      prod [mp_red->reduce (i->first * j->first)] += i->second * j->second;
    }
  }
}

void
VariantsCollectorBase::copy_shapes (geo::Layout &layout, geo::cell_index_type ci_to, geo::cell_index_type ci_from)
{
  geo::Cell &to = layout.cell (ci_to);
  const geo::Cell &from = layout.cell (ci_from);
  for (geo::Layout::layer_iterator li = layout.begin_layers (); li != layout.end_layers (); ++li) {
    to.shapes ((*li).first) = from.shapes ((*li).first);
  }
}

void
VariantsCollectorBase::create_var_instances (geo::Cell &in_cell, std::vector<geo::CellInstArrayWithProperties> &inst, const geo::ICplxTrans &for_var, const std::map<geo::cell_index_type, std::map<geo::ICplxTrans, geo::cell_index_type> > &var_table, bool tl_invariant) const
{
  if (tl_invariant) {
    create_var_instances_tl_invariant (in_cell, inst, for_var, var_table);
  } else {
    create_var_instances_non_tl_invariant (in_cell, inst, for_var, var_table);
  }
}

void
VariantsCollectorBase::create_var_instances_non_tl_invariant (geo::Cell &in_cell, std::vector<geo::CellInstArrayWithProperties> &inst, const geo::ICplxTrans &for_var, const std::map<geo::cell_index_type, std::map<geo::ICplxTrans, geo::cell_index_type> > &var_table) const
{
  for (std::vector<geo::CellInstArrayWithProperties>::const_iterator i = inst.begin (); i != inst.end (); ++i) {

    std::map<geo::cell_index_type, std::map<geo::ICplxTrans, geo::cell_index_type> >::const_iterator f = var_table.find (i->object ().cell_index ());
    if (f == var_table.end ()) {

       in_cell.insert (*i);

    } else {

      const std::map<geo::ICplxTrans, geo::cell_index_type> &vt = f->second;

      for (geo::CellInstArray::iterator ia = i->begin (); ! ia.at_end (); ++ia) {

        geo::ICplxTrans rt = mp_red->reduce (for_var * mp_red->reduce_trans (i->complex_trans (*ia)));
        std::map<geo::ICplxTrans, geo::cell_index_type>::const_iterator v = vt.find (rt);
        tl_assert (v != vt.end ());

        in_cell.insert (geo::CellInstArrayWithProperties (geo::CellInstArray (geo::CellInst (v->second), i->complex_trans (*ia)), i->properties_id ()));

      }

    }

  }
}

void
VariantsCollectorBase::create_var_instances_tl_invariant (geo::Cell &in_cell, std::vector<geo::CellInstArrayWithProperties> &inst, const geo::ICplxTrans &for_var, const std::map<geo::cell_index_type, std::map<geo::ICplxTrans, geo::cell_index_type> > &var_table) const
{
  for (std::vector<geo::CellInstArrayWithProperties>::const_iterator i = inst.begin (); i != inst.end (); ++i) {

    std::map<geo::cell_index_type, std::map<geo::ICplxTrans, geo::cell_index_type> >::const_iterator f = var_table.find (i->object ().cell_index ());
    if (f == var_table.end ()) {

       in_cell.insert (*i);

    } else {

      const std::map<geo::ICplxTrans, geo::cell_index_type> &vt = f->second;

      std::map<geo::ICplxTrans, geo::cell_index_type>::const_iterator v;

      geo::ICplxTrans rt = mp_red->reduce (for_var * mp_red->reduce_trans (i->complex_trans ()));
      v = vt.find (rt);
      tl_assert (v != vt.end ());

      geo::CellInstArrayWithProperties new_inst = *i;
      new_inst.object ().cell_index (v->second);
      in_cell.insert (new_inst);

    }

  }
}

}

