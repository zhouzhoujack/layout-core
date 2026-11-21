#include "geoCellGraphUtils.h"

namespace geo
{

// ---------------------------------------------------------------------
//  CellCounter implementation

CellCounter::CellCounter (const geo::Layout *cell_graph)
  : m_cache (), mp_cell_graph (cell_graph)
{
  //  .. nothing yet ..
}

CellCounter::CellCounter (const geo::Layout *cell_graph, geo::cell_index_type starting_cell)
  : m_cache (), mp_cell_graph (cell_graph)
{
  cell_graph->cell (starting_cell).collect_called_cells (m_selection);
  m_selection.insert (starting_cell);
}

size_t 
CellCounter::weight (geo::cell_index_type ci)
{
  cache_t::const_iterator c = m_cache.find (ci);

  if (c != m_cache.end ()) {
    return c->second;
  } else if (! m_selection.empty () && m_selection.find (ci) == m_selection.end ()) {
    return 0;
  } else {

    const geo::Cell *cell = & mp_cell_graph->cell (ci);
    size_t count = 0;

    for (geo::Cell::parent_inst_iterator p = cell->begin_parent_insts (); ! p.at_end (); ++p) {
      if (m_selection.empty () || m_selection.find (p->parent_cell_index ()) != m_selection.end ()) {
        count += weight (p->parent_cell_index ()) * p->child_inst ().size ();
      }
    }

    if (count == 0) {
      count = 1;  // top cells have multiplicity 1
    }

    m_cache.insert (std::make_pair (ci, count));
    return count;

  }
}

}

