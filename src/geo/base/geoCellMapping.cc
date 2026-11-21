#include "geoLayout.h"
#include "geoCellGraphUtils.h"
#include "geoCellMapping.h"
#include "geoLayoutUtils.h"
#include "utilsLog.h"
#include "utilsTimer.h"

#include <memory>
#include <map>
#include <set>
#include <vector>

namespace geo
{

// -------------------------------------------------------------------------------------
//  Some utility class: an iterator for cell instances delivered sorted by cell index

struct SortedCellIndexIterator
{
  typedef geo::cell_index_type value_type;
  typedef size_t difference_type;
  typedef size_t pointer;
  typedef size_t reference;
  typedef std::random_access_iterator_tag iterator_category;

  SortedCellIndexIterator ()
    : mp_cell (0), m_n (0)
  { 
    // .. nothing yet ..
  }

  SortedCellIndexIterator (const geo::Cell &cell, size_t n)
    : mp_cell (&cell), m_n (n)
  { 
    // .. nothing yet ..
  }

  geo::cell_index_type operator*() const
  {
    return mp_cell->sorted_inst_ptr (m_n).cell_index ();
  }

  size_t operator-(const SortedCellIndexIterator &d) const
  {
    return m_n - d.m_n;
  }

  SortedCellIndexIterator &operator++() 
  {
    ++m_n;
    return *this;
  }

  SortedCellIndexIterator &operator+=(size_t n) 
  {
    m_n += n;
    return *this;
  }

  SortedCellIndexIterator &operator--()
  {
    --m_n;
    return *this;
  }

  SortedCellIndexIterator &operator-=(size_t n)
  {
    m_n -= n;
    return *this;
  }

  bool operator==(const SortedCellIndexIterator &d) const
  {
    return m_n == d.m_n;
  }

  bool operator!=(const SortedCellIndexIterator &d) const
  {
    return m_n != d.m_n;
  }

  bool operator<(const SortedCellIndexIterator &d) const
  {
    return m_n < d.m_n;
  }

  geo::Instance instance () const 
  {
    return mp_cell->sorted_inst_ptr (m_n);
  }

private:
  const geo::Cell *mp_cell;
  size_t m_n;
};

// -------------------------------------------------------------------------------------
//  Some utility class: a compare function for a instance set of two cells in the context
//  of two layouts and two initial cells.

class InstanceSetCompareFunction 
{
public:
  typedef std::multiset<geo::ICplxTrans, geo::trans_less_func<geo::ICplxTrans> > trans_set_t;

  InstanceSetCompareFunction (const geo::Layout &layout_a, geo::cell_index_type initial_cell_a, const geo::Layout &layout_b, geo::cell_index_type initial_cell_b)
    : m_layout_a (layout_a), m_initial_cell_a (initial_cell_a),
      m_layout_b (layout_b), m_initial_cell_b (initial_cell_b),
      m_cell_a (std::numeric_limits<geo::cell_index_type>::max ()),
      m_repr_set (false)
  {
    // ..
  }

  bool compare (geo::cell_index_type cell_a, const std::set<geo::cell_index_type> &selection_cone_a, geo::cell_index_type cell_b, const std::set<geo::cell_index_type> &selection_cone_b)
  {
    if (cell_a != m_cell_a) {

      m_cell_a = cell_a;

      m_callers_a.clear ();
      m_layout_a.cell (cell_a).collect_caller_cells (m_callers_a, selection_cone_a, -1);
      m_callers_a.insert (cell_a);

      m_trans.clear ();
      insert (m_layout_a, m_initial_cell_a, m_cell_a, m_callers_a, m_trans, geo::ICplxTrans ());

    }

    std::set<geo::cell_index_type> callers_b;
    m_layout_b.cell (cell_b).collect_caller_cells (callers_b, selection_cone_b, -1);
    callers_b.insert (cell_b);

    m_repr_set = false;

    std::map<geo::cell_index_type, geo::ICplxTrans>::const_iterator r = m_repr.find (cell_b);
    if (r != m_repr.end ()) {
      m_repr_set = true;
      if (m_trans.find (r->second) == m_trans.end ()) {
        return false;
      }
    }
    
    trans_set_t trans (m_trans);

    double mag = m_layout_b.dbu () / m_layout_a.dbu ();
    if (! compare (m_layout_b, m_initial_cell_b, cell_b, callers_b, trans, geo::ICplxTrans (mag), geo::ICplxTrans (1.0 / mag))) {
      return false;
    }

    return trans.empty ();
  }

private:
  const geo::Layout &m_layout_a;
  geo::cell_index_type m_initial_cell_a;
  const geo::Layout &m_layout_b;
  geo::cell_index_type m_initial_cell_b;
  geo::cell_index_type m_cell_a;
  std::set<geo::cell_index_type> m_callers_a;
  trans_set_t m_trans;
  std::map<geo::cell_index_type, geo::ICplxTrans> m_repr;
  bool m_repr_set;

  void insert (const geo::Layout &layout, geo::cell_index_type current_cell, geo::cell_index_type cell, const std::set<geo::cell_index_type> &cone, trans_set_t &trans, const geo::ICplxTrans &current_trans)
  {
    if (current_cell == cell) {
      trans.insert (current_trans);
    } else {

      const geo::Cell &cc = layout.cell (current_cell);
      size_t instances = cc.cell_instances ();
      SortedCellIndexIterator begin (cc, 0);
      SortedCellIndexIterator end (cc, instances);

      SortedCellIndexIterator i = begin;
      for (std::set<geo::cell_index_type>::const_iterator c = cone.begin (); c != cone.end () && i != end; ++c) {
        if (*i <= *c) {
          for (i = std::lower_bound (i, end, *c); i != end && *i == *c; ++i) {
            for (geo::CellInstArray::iterator arr = i.instance ().begin (); ! arr.at_end (); ++arr) {
              insert (layout, *c, cell, cone, trans, current_trans * i.instance ().complex_trans (*arr));
            }
          }
        }
      }

    }
  }

  bool compare (const geo::Layout &layout, geo::cell_index_type current_cell, geo::cell_index_type cell, const std::set<geo::cell_index_type> &cone, trans_set_t &trans, const geo::ICplxTrans &current_trans, const geo::ICplxTrans &local_trans)
  {
    if (current_cell == cell) {

      geo::ICplxTrans eff_trans (current_trans * local_trans);

      if (! m_repr_set) {
        m_repr_set = true;
        m_repr.insert (std::make_pair (cell, eff_trans));
      }

      trans_set_t::iterator t = trans.find (eff_trans);
      if (t == trans.end ()) {
        return false;
      } else {
        trans.erase (t);
        return true;
      }

    } else {

      const geo::Cell &cc = layout.cell (current_cell);
      size_t instances = cc.cell_instances ();
      SortedCellIndexIterator begin (cc, 0);
      SortedCellIndexIterator end (cc, instances);

      SortedCellIndexIterator i = begin;
      for (std::set<geo::cell_index_type>::const_iterator c = cone.begin (); c != cone.end () && i != end; ++c) {
        if (*i <= *c) {
          for (i = std::lower_bound (i, end, *c); i != end && *i == *c; ++i) {
            for (geo::CellInstArray::iterator arr = i.instance ().begin (); ! arr.at_end (); ++arr) {
              if (! compare (layout, *c, cell, cone, trans, current_trans * i.instance ().complex_trans (*arr), local_trans)) {
                return false;
              }
            }
          }
        }
      }

      return true;

    }
  }
};

// -------------------------------------------------------------------------------------
//  CellMapping implementation

// -----------------------------------------------------------------------------
//  CellMapping implementation

CellMapping::CellMapping ()
{
  // .. nothing yet ..
}

void CellMapping::clear ()
{
  m_b2a_mapping.clear ();
}

std::vector<geo::cell_index_type> CellMapping::source_cells () const
{
  std::vector<geo::cell_index_type> s;
  s.reserve (m_b2a_mapping.size ());
  for (iterator m = begin (); m != end (); ++m) {
    s.push_back (m->first);
  }
  return s;
}


void 
CellMapping::create_single_mapping (const geo::Layout & /*layout_a*/, geo::cell_index_type cell_index_a, const geo::Layout & /*layout_b*/, geo::cell_index_type cell_index_b)
{
  clear ();
  map (cell_index_b, cell_index_a);
}

void
CellMapping::create_multi_mapping (const geo::Layout & /*layout_a*/, const std::vector<geo::cell_index_type> &cell_index_a, const geo::Layout & /*layout_b*/, const std::vector<geo::cell_index_type> &cell_index_b)
{
  clear ();
  if (cell_index_a.size () != cell_index_b.size ()) {
    throw geo::Exception (geo::to_string (tr ("cell index arrays for A and B cells must have same length in 'create_multi_mapping'")));
  }
  for (std::vector<geo::cell_index_type>::const_iterator ia = cell_index_a.begin (), ib = cell_index_b.begin (); ia != cell_index_a.end (); ++ia, ++ib) {
    map (*ib, *ia);
  }
}

void
CellMapping::create_from_names (const geo::Layout &layout_a, geo::cell_index_type cell_index_a, const geo::Layout &layout_b, geo::cell_index_type cell_index_b)
{
  clear ();

  std::set<geo::cell_index_type> called_b;
  layout_b.cell (cell_index_b).collect_called_cells (called_b);

  map (cell_index_b, cell_index_a);

  for (std::set<geo::cell_index_type>::const_iterator b = called_b.begin (); b != called_b.end (); ++b) {
    std::pair<bool, geo::cell_index_type> ac = layout_a.cell_by_name (layout_b.cell_name (*b));
    if (ac.first) {
      map (*b, ac.second);
    }
  }
}

std::vector<geo::cell_index_type> 
CellMapping::create_missing_mapping (geo::Layout &layout_a, const geo::Layout &layout_b, const std::vector<geo::cell_index_type> &cell_index_b, const std::set<geo::cell_index_type> *exclude_cells, const std::set<geo::cell_index_type> *include_cells)
{
  std::vector<geo::cell_index_type> new_cells;
  do_create_missing_mapping (layout_a, layout_b, cell_index_b, exclude_cells, include_cells, &new_cells, 0);
  return new_cells;
}

std::vector<std::pair<geo::cell_index_type, geo::cell_index_type> >
CellMapping::create_missing_mapping2 (geo::Layout &layout_a, const geo::Layout &layout_b, const std::vector<geo::cell_index_type> &cell_index_b, const std::set<geo::cell_index_type> *exclude_cells, const std::set<geo::cell_index_type> *include_cells)
{
  std::vector<std::pair<geo::cell_index_type, geo::cell_index_type> > cell_pairs;
  do_create_missing_mapping (layout_a, layout_b, cell_index_b, exclude_cells, include_cells, 0, &cell_pairs);
  return cell_pairs;
}

void
CellMapping::do_create_missing_mapping (geo::Layout &layout_a, const geo::Layout &layout_b, const std::vector<geo::cell_index_type> &cell_index_b, const std::set<geo::cell_index_type> *exclude_cells, const std::set<geo::cell_index_type> *include_cells, std::vector<geo::cell_index_type> *new_cells_ptr, std::vector<std::pair<geo::cell_index_type, geo::cell_index_type> > *mapped_pairs)
{
  std::vector<geo::cell_index_type> new_cells_int;
  std::vector<geo::cell_index_type> &new_cells = *(new_cells_ptr ? new_cells_ptr : &new_cells_int);
  std::vector<geo::cell_index_type> new_cells_b;

  std::set<geo::cell_index_type> called_b;
  for (std::vector<geo::cell_index_type>::const_iterator i = cell_index_b.begin (); i != cell_index_b.end (); ++i) {
    layout_b.cell (*i).collect_called_cells (called_b);
    called_b.insert (*i);
  }

  for (std::set<geo::cell_index_type>::const_iterator b = called_b.begin (); b != called_b.end (); ++b) {
    if (m_b2a_mapping.find (*b) == m_b2a_mapping.end ()
        && (! exclude_cells || exclude_cells->find (*b) == exclude_cells->end ())
        && (! include_cells || include_cells->find (*b) != include_cells->end ())) {

      geo::cell_index_type new_cell = layout_a.add_cell (layout_b.cell_name (*b));
      new_cells.push_back (new_cell);
      new_cells_b.push_back (*b);

      if (mapped_pairs) {
        mapped_pairs->push_back (std::make_pair (*b, new_cell));
      }

      map (*b, new_cell);

    }
  }

  if (! new_cells.empty ()) {

    geo::PropertyMapper pm (layout_a, layout_b);

    //  Note: this avoids frequent cell index table rebuilds if source and target layout are identical
    layout_a.start_changes ();

    //  Create instances for the new cells in layout A according to their instantiation in layout B 
    double mag = layout_b.dbu () / layout_a.dbu ();
    for (size_t i = 0; i < new_cells.size (); ++i) {

      const geo::Cell &b = layout_b.cell (new_cells_b [i]);
      for (geo::Cell::parent_inst_iterator pb = b.begin_parent_insts (); ! pb.at_end (); ++pb) {

        if (called_b.find (pb->parent_cell_index ()) != called_b.end ()) {

          geo::Cell &pa = layout_a.cell (m_b2a_mapping [pb->parent_cell_index ()]);

          geo::Instance bi = pb->child_inst ();

          geo::CellInstArray bci = bi.cell_inst ();
          bci.object ().cell_index (new_cells [i]);
          bci.transform_into (geo::ICplxTrans (mag), &layout_a.array_repository ());

          if (bi.has_prop_id ()) {
            pa.insert (geo::CellInstArrayWithProperties (bci, pm (bi.prop_id ())));
          } else {
            pa.insert (bci);
          }

        }

      }

    }

    //  Note: must be there because of start_changes
    layout_a.end_changes ();

  }
}

void 
CellMapping::create_from_geometry (const geo::Layout &layout_a, geo::cell_index_type cell_index_a, const geo::Layout &layout_b, geo::cell_index_type cell_index_b)
{
  geo::SelfTimer timer (geo::verbosity () >= 11, geo::to_string (tr ("Cell mapping")));

  if (geo::verbosity () >= 20) {
    geo::info << "Cell mapping - first step: mapping instance count and instance identity";
  }

  clear ();

  geo::CellCounter cc_a (&layout_a, cell_index_a);
  geo::CellCounter cc_b (&layout_b, cell_index_b);

  std::multimap<size_t, geo::cell_index_type> cm_b;
  for (geo::CellCounter::selection_iterator c = cc_b.begin (); c != cc_b.end (); ++c) {
    cm_b.insert (std::make_pair (*c == cell_index_b ? 0 : cc_b.weight (*c), *c));
  }

  std::multimap<size_t, geo::cell_index_type> cm_a;
  for (geo::CellCounter::selection_iterator c = cc_a.begin (); c != cc_a.end (); ++c) {
    cm_a.insert (std::make_pair (*c == cell_index_a ? 0 : cc_a.weight (*c), *c));
  }

  std::map <geo::cell_index_type, std::vector<geo::cell_index_type> > candidates; // key = index(a), value = indices(b)

  InstanceSetCompareFunction cmp (layout_a, cell_index_a, layout_b, cell_index_b);

  std::multimap<size_t, geo::cell_index_type>::const_iterator a = cm_a.begin (), b = cm_b.begin ();
  while (a != cm_a.end () && b != cm_b.end ()) {

    size_t w = a->first;
    while (b != cm_b.end () && b->first < w) {
      ++b;
    }

    if (b == cm_b.end ()) {
      break;
    } else if (b->first > w) {
      candidates.insert (std::make_pair (a->second, std::vector<geo::cell_index_type> ()));
      ++a;
    } else {

      if (geo::verbosity () >= 30) {
        size_t na = 0, nb = 0;
        for (std::multimap<size_t, geo::cell_index_type>::const_iterator aa = a; aa != cm_a.end () && aa->first == w; ++aa) {
          ++na;
        }
        for (std::multimap<size_t, geo::cell_index_type>::const_iterator bb = b; bb != cm_b.end () && bb->first == w; ++bb) {
          ++nb;
        }
        geo::info << "Multiplicity group (" << w << " instances) - " << na << " vs. " << nb << " cells";
      }

      unsigned int g = 0;
      std::map <unsigned int, std::vector <geo::cell_index_type> > b_group;
      std::map <geo::cell_index_type, unsigned int> b_group_of_cell;

      while (a != cm_a.end () && a->first == w) {

        candidates.insert (std::make_pair (a->second, std::vector <geo::cell_index_type> ()));

        std::set <unsigned int> groups_taken;

        std::multimap<size_t, geo::cell_index_type>::const_iterator bb = b;
        while (bb != cm_b.end () && bb->first == w) {

          std::map <geo::cell_index_type, unsigned int>::const_iterator bg = b_group_of_cell.find (bb->second);
          if (bg != b_group_of_cell.end ()) {

            if (groups_taken.find (bg->second) == groups_taken.end ()) {
              if (cmp.compare (a->second, cc_a.selection (), bb->second, cc_b.selection ())) {
                candidates [a->second] = b_group [bg->second];
                groups_taken.insert (bg->second);
              }
            }

          } else {

            if (cmp.compare (a->second, cc_a.selection (), bb->second, cc_b.selection ())) {
              candidates [a->second].push_back (bb->second);
              b_group_of_cell.insert (std::make_pair (bb->second, g));
              b_group.insert (std::make_pair (g, std::vector <geo::cell_index_type> ())).first->second.push_back (bb->second);
            }

          }

          ++bb;

        }

        if (geo::verbosity () >= 40) {
          geo::info << "Checked cell " << layout_a.cell_name (a->second) << ": " << candidates [a->second].size () << " candidates remaining.";
        }
          
        ++a;
        ++g;

      }

      while (b != cm_b.end () && b->second == w) {
        ++b;
      }

    }

  }

  while (a != cm_a.end ()) {
    candidates.insert (std::make_pair (a->second, std::vector<geo::cell_index_type> ()));
    ++a;
  }

  if (geo::verbosity () >= 40) {
    geo::info << "Mapping candidates:";
    dump_mapping (candidates, layout_a, layout_b);
  }
    
  for (std::map <geo::cell_index_type, std::vector<geo::cell_index_type> >::const_iterator cand = candidates.begin (); cand != candidates.end (); ++cand) {
    extract_unique (cand, m_b2a_mapping, layout_a, layout_b);
  }

  int iteration = 0;

  bool reduction = true;
  while (reduction) {

    reduction = false;
    ++iteration;

    if (geo::verbosity () >= 20) {
      geo::info << "Cell mapping - iteration " << iteration << ": cross-instance cone reduction";
    }

    // This map stores that layout_b cells with the corresponding layout_a cell for such cells which 
    // have their mapping reduced to a unique one 
    std::map <geo::cell_index_type, std::pair<geo::cell_index_type, int> > unique_candidates;

    std::vector<geo::cell_index_type> refined_cand;

    for (std::map <geo::cell_index_type, std::vector<geo::cell_index_type> >::iterator cand = candidates.begin (); cand != candidates.end (); ++cand) {

      if (cand->second.size () > 1) {

        refined_cand.clear ();
        refined_cand.insert (refined_cand.end (), cand->second.begin (), cand->second.end ());

        if (geo::verbosity () >= 50) {
          geo::info << "--- Cell: " << layout_a.cell_name (cand->first);
          geo::info << "Before reduction: " << geo::noendl; 
          for (size_t i = 0; i < refined_cand.size (); ++i) { 
            geo::info << " " << layout_b.cell_name(refined_cand[i]) << geo::noendl; 
          } 
          geo::info << "";
        }

        std::set<geo::cell_index_type> callers;
        layout_a.cell (cand->first).collect_caller_cells (callers, cc_a.selection (), -1);

        for (std::set<geo::cell_index_type>::const_iterator c = callers.begin (); c != callers.end () && refined_cand.size () > 0; ++c) {

          if (*c != cell_index_a) {

            const std::vector<geo::cell_index_type> &others = candidates.find (*c)->second;
            if (others.size () == 1) {

              std::set<geo::cell_index_type> cross_cone_b;
              layout_b.cell (others.front ()).collect_called_cells (cross_cone_b);

              std::vector<geo::cell_index_type>::iterator cout = refined_cand.begin ();
              for (std::vector<geo::cell_index_type>::const_iterator cc = refined_cand.begin (); cc != refined_cand.end (); ++cc) {
                if (cross_cone_b.find (*cc) != cross_cone_b.end ()) {
                  *cout++ = *cc;
                }
              }

              if (geo::verbosity () >= 50 && cout != refined_cand.end ()) {
                geo::info << "Reduction because of caller mapping: " << layout_a.cell_name (*c) << " <-> " << layout_b.cell_name (others[0]);
                geo::info << "  -> " << geo::noendl; 
                for (size_t i = 0; i < size_t (cout - refined_cand.begin ()); ++i) { 
                  geo::info << " " << layout_b.cell_name(refined_cand[i]) << geo::noendl; 
                } 
                geo::info << "";
              }

              refined_cand.erase (cout, refined_cand.end ());

            }

          }

        }

        if (refined_cand.size () > 0) {

          std::set<geo::cell_index_type> called;
          layout_a.cell (cand->first).collect_called_cells (called);

          for (std::set<geo::cell_index_type>::const_iterator c = called.begin (); c != called.end () && refined_cand.size () > 0; ++c) {

            const std::vector<geo::cell_index_type> &others = candidates.find (*c)->second;
            if (others.size () == 1) {

              std::set<geo::cell_index_type> cross_cone_b;
              layout_b.cell (others.front ()).collect_caller_cells (cross_cone_b, cc_b.selection (), -1);

              std::vector<geo::cell_index_type>::iterator cout = refined_cand.begin ();
              for (std::vector<geo::cell_index_type>::const_iterator cc = refined_cand.begin (); cc != refined_cand.end (); ++cc) {
                if (cross_cone_b.find (*cc) != cross_cone_b.end ()) {
                  *cout++ = *cc;
                }
              }

              if (geo::verbosity () >= 50 && cout != refined_cand.end ()) {
                geo::info << "Reduction because of callee mapping: " << layout_a.cell_name (*c) << " <-> " << layout_b.cell_name (others[0]);
                geo::info << "  -> " << geo::noendl; 
                for (size_t i = 0; i < size_t (cout - refined_cand.begin ()); ++i) { 
                  geo::info << " " << layout_b.cell_name(refined_cand[i]) << geo::noendl; 
                } 
                geo::info << "";
              }

              refined_cand.erase (cout, refined_cand.end ());

            }

          }

          if (refined_cand.size () == 1) {

            //  The remaining cell is a candidate for layout_b to layout_a mapping
            geo::cell_index_type cb = refined_cand[0];
            geo::cell_index_type ca = cand->first;

            std::map <geo::cell_index_type, std::pair<geo::cell_index_type, int> >::iterator uc = unique_candidates.find (cb);
            if (uc != unique_candidates.end ()) {
              if (uc->second.first != ca) {
                int ed = geo::edit_distance (layout_a.cell_name (ca), layout_b.cell_name (cb));
                if (ed < uc->second.second) {
                  uc->second = std::make_pair (ca, ed);
                  if (geo::verbosity () >= 40) {
                    geo::info << "Choosing " << layout_b.cell_name (cb) << " (layout_b) as new unique mapping for " << layout_a.cell_name (ca) << " (layout_a)";
                  }
                }
              }
            } else {
              int ed = geo::edit_distance (layout_a.cell_name (ca), layout_b.cell_name (cb));
              unique_candidates.insert (std::make_pair (cb, std::make_pair (ca, ed)));
              if (geo::verbosity () >= 40) {
                geo::info << "Choosing " << layout_b.cell_name (cb) << " (layout_b) as unique mapping for " << layout_a.cell_name (ca) << " (layout_a)";
              }
            }

          }

        }

      }

    }

    //  realize the proposed unique mapping
    for (std::map <geo::cell_index_type, std::pair<geo::cell_index_type, int> >::const_iterator uc = unique_candidates.begin (); uc != unique_candidates.end (); ++uc) {

      std::map <geo::cell_index_type, std::vector<geo::cell_index_type> >::iterator cand = candidates.find (uc->second.first);
      tl_assert (cand != candidates.end ());
      cand->second.clear ();
      cand->second.push_back (uc->first);
      reduction = true;
      extract_unique (cand, m_b2a_mapping, layout_a, layout_b);

    }

    if (geo::verbosity () >= 40) {
      geo::info << "Further refined candidates:";
      dump_mapping (candidates, layout_a, layout_b);
    }
    
    if (geo::verbosity () >= 20) {
      geo::info << "Cell mapping - iteration " << iteration << ": removal of uniquely mapped cells on B side";
    }

    for (std::map <geo::cell_index_type, std::vector<geo::cell_index_type> >::iterator cand = candidates.begin (); cand != candidates.end (); ++cand) {

      if (cand->second.size () > 1) {

        std::vector<geo::cell_index_type> refined_cand;
        for (std::vector<geo::cell_index_type>::const_iterator c = cand->second.begin (); c != cand->second.end (); ++c) {
          std::map<geo::cell_index_type, geo::cell_index_type>::const_iterator um = m_b2a_mapping.find (*c);
          if (um == m_b2a_mapping.end () || um->second == cand->first) {
            refined_cand.push_back (*c);
          }
        }

        if (refined_cand.size () < cand->second.size ()) {
          reduction = true;
          cand->second.swap (refined_cand);
          extract_unique (cand, m_b2a_mapping, layout_a, layout_b);
        }

      }

    }
    
    if (geo::verbosity () >= 40) {
      geo::info << "After reduction of mapped cells on b side:";
      dump_mapping (candidates, layout_a, layout_b);
    }
    
  }

  if (geo::verbosity () >= 20) {

    int total = 0;
    int not_mapped = 0;
    int unique = 0;
    int non_unique = 0;
    int alternatives = 0;

    for (std::map <geo::cell_index_type, std::vector<geo::cell_index_type> >::iterator cand = candidates.begin (); cand != candidates.end (); ++cand) {
      ++total;
      if (cand->second.size () == 0) {
        ++not_mapped;
      } else if (cand->second.size () == 1) {
        ++unique;
      } else {
        ++non_unique;
        alternatives += (int) cand->second.size ();
      }
    }

    geo::info << "Geometry mapping statistics:";
    geo::info << "  Total cells = " << total;
    geo::info << "  Not mapped = " << not_mapped;
    geo::info << "  Unique = " << unique;
    geo::info << "  Non unique = " << non_unique << " (total " << alternatives << " of alternatives)";

  }

  //  Resolve mapping according to string match

  if (geo::verbosity () >= 20) {
    geo::info << "Cell mapping - string mapping as last resort";
  }

  for (std::map <geo::cell_index_type, std::vector<geo::cell_index_type> >::iterator cand = candidates.begin (); cand != candidates.end (); ++cand) {

    if (cand->second.size () > 1) {

      std::string cn_a (layout_a.cell_name (cand->first));

      int min_ed = std::numeric_limits<int>::max ();
      geo::cell_index_type min_ed_ci;

      for (std::vector<geo::cell_index_type>::const_iterator c = cand->second.begin (); c != cand->second.end (); ++c) {

        if (m_b2a_mapping.find (*c) == m_b2a_mapping.end ()) {

          int ed = geo::edit_distance (cn_a, layout_b.cell_name (*c));
          if (ed < min_ed) {
            min_ed = ed;
            min_ed_ci = *c;
          }

        }

      }

      cand->second.clear ();
      if (min_ed < std::numeric_limits<int>::max ()) {
        cand->second.push_back (min_ed_ci);
        extract_unique (cand, m_b2a_mapping, layout_a, layout_b);
      }

    }

  }

  if (geo::verbosity () >= 20) {

    int total = 0;
    int not_mapped = 0;
    int unique = 0;
    int non_unique = 0;
    int alternatives = 0;

    for (std::map <geo::cell_index_type, std::vector<geo::cell_index_type> >::iterator cand = candidates.begin (); cand != candidates.end (); ++cand) {
      ++total;
      if (cand->second.size () == 0) {
        if (geo::verbosity () >= 30) {
          geo::info << "Unmapped cell: " << layout_a.cell_name (cand->first);
        }
        ++not_mapped;
      } else if (cand->second.size () == 1) {
        ++unique;
      } else {
        ++non_unique;
        alternatives += (int) cand->second.size ();
      }
    }

    geo::info << "Final mapping statistics:";
    geo::info << "  Total cells = " << total;
    geo::info << "  Not mapped = " << not_mapped;
    geo::info << "  Unique = " << unique;
    geo::info << "  Non unique = " << non_unique << " (total " << alternatives << " of alternatives)";

  }
}

void 
CellMapping::extract_unique (std::map <geo::cell_index_type, std::vector<geo::cell_index_type> >::const_iterator cand, 
                             std::map<geo::cell_index_type, geo::cell_index_type> &unique_mapping,
                             const geo::Layout &layout_a, const geo::Layout &layout_b)
{
  if (cand->second.size () == 1) {

    if (geo::verbosity () >= 20) {
      geo::info << "  (U) " << layout_a.cell_name (cand->first) << " -> " << layout_b.cell_name (cand->second.front ()) << " (" << cand->first << " -> " << cand->second.front () << ")";
    }
    unique_mapping.insert (std::make_pair (cand->second.front (), cand->first));

  } else if (geo::verbosity () >= 30) {

    geo::info << "      " << layout_a.cell_name (cand->first) << " ->" << geo::noendl;
    int n = 5;
    for (std::vector<geo::cell_index_type>::const_iterator c = cand->second.begin (); c != cand->second.end () && --n > 0; ++c) {
      geo::info << " " << layout_b.cell_name (*c) << geo::noendl;
    }
    if (n == 0) {
      geo::info << " ..";
    } else {
      geo::info << "";
    }

  }
}

void 
CellMapping::dump_mapping (const std::map <geo::cell_index_type, std::vector<geo::cell_index_type> > &candidates, 
                           const geo::Layout &layout_a, const geo::Layout &layout_b)
{
  for (std::map <geo::cell_index_type, std::vector<geo::cell_index_type> >::const_iterator cand = candidates.begin (); cand != candidates.end (); ++cand) {
    geo::info << "  " << layout_a.cell_name (cand->first) << " ->" << geo::noendl;
    int n = 5;
    for (std::vector<geo::cell_index_type>::const_iterator c = cand->second.begin (); c != cand->second.end () && --n > 0; ++c) {
      geo::info << " " << layout_b.cell_name (*c) << geo::noendl;
    }
    if (n == 0) {
      geo::info << " ..";
    } else {
      geo::info << "";
    }
  }
}

std::pair<bool, geo::cell_index_type> 
CellMapping::cell_mapping_pair (geo::cell_index_type cell_index_b) const
{
  std::map <geo::cell_index_type, geo::cell_index_type>::const_iterator m = m_b2a_mapping.find (cell_index_b);
  if (m == m_b2a_mapping.end ()) {
    return std::make_pair (false, 0);
  } else {
    return std::make_pair (true, m->second);
  }
}

bool 
CellMapping::has_mapping (geo::cell_index_type cell_index_b) const
{
  std::map <geo::cell_index_type, geo::cell_index_type>::const_iterator m = m_b2a_mapping.find (cell_index_b);
  return (m != m_b2a_mapping.end ());
}

geo::cell_index_type 
CellMapping::cell_mapping (geo::cell_index_type cell_index_b) const
{
  std::map <geo::cell_index_type, geo::cell_index_type>::const_iterator m = m_b2a_mapping.find (cell_index_b);
  tl_assert (m != m_b2a_mapping.end ());
  return m->second;
}

}

