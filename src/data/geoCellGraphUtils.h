#ifndef HDR_geoCellGraphUtils
#define HDR_geoCellGraphUtils


#include "geoLayout.h"

#include <map>
#include <set>

namespace geo
{

/**
 *  @brief A cell multiplicity generator
 *
 *  This class delivers the multiplicity for a cell (number of "as if flat" instances
 *  of the cell in all top cells. It is instantiated with a reference to a cell graph object.
 *  This object caches cell counts for multiple cells. It is efficient to instantiate
 *  this object once and use it as often as possible.
 */

class CellCounter 
{
public:
  typedef std::map <geo::cell_index_type, size_t> cache_t;
  typedef std::set <geo::cell_index_type> selection_t;
  typedef selection_t::const_iterator selection_iterator;

  /** 
   *  @brief Instantiate a counter object with a reference to the given cell graph
   */
  CellCounter (const geo::Layout *cell_graph);

  /** 
   *  @brief Instantiate a counter object with a reference to the given cell graph
   *
   *  This version allows one to specify a initial (starting) cell where only the cell tree below the
   *  staring cell is considered. Multiplicity refers to the number of instances below the
   *  initial cell.
   */
  CellCounter (const geo::Layout *cell_graph, geo::cell_index_type starting_cell);

  /**
   *  @brief Determine the instance count of the cell with index "ci"
   *
   *  The instance count is the number of "flat" instances of the cell in all
   *  top cells of the graph. A top cell has a multiplicity of 1.
   */
  size_t weight (geo::cell_index_type ci);

  /**
   *  @brief Begin iterator for the cells in the selection
   *
   *  The iterator pair delivers all selected cells (only applicable if an initial cell is specified).
   */
  selection_iterator begin () const 
  {
    return m_selection.begin ();
  }
  
  /**
   *  @brief End iterator for the cells in the selection
   */
  selection_iterator end () const 
  {
    return m_selection.end ();
  }

  /**
   *  @brief Get the selection cone
   */
  const selection_t &selection () const
  {
    return m_selection;
  }

private:
  cache_t m_cache;
  selection_t m_selection;
  const geo::Layout *mp_cell_graph;
};

/**
 *  @brief A generic cell instance statistics generator
 *
 *  This class provides a way to efficiently run a cell instance statistics over a 
 *  cell graph or a sub tree of the graph.
 *
 *  The Value specifies the statistics object that is collected over the hierarchy.
 *  It must provide the following methods:
 *    - ctor (const geo::Layout &layout, const geo::Cell &cell);
 *    - Value transformed (const geo::CellInstArray &trans) const;
 *    - add (const Value &other);
 *
 *  The ctor is supposed to create a value for the cell representing "no instance".
 *  The transformed method is supposed to transform a value from a parent to a child cell.
 *  The instance specifies the parent instance which transforms into the child cell.
 *  The add method is supposed to add value for a cell (but a different instance) to *this. 
 *
 *  The cell counter can be implemented with this value class:
 *
 *    class CellCountValue {
 *      CellCountValue (const geo::Layout &, const geo::Cell &) 
 *        : m_count (0) { }
 *      CellCountValue (size_t count) 
 *        : m_count (count) { }
 *      CellCountValue transformed (const geo::CellInstArray &trans) { 
 *        return CellCountValue (std::max (m_count, 1) * trans.size ()); 
 *      }
 *      void add (const CellCountValue &other) {
 *        m_count += other.m_count;
 *      }
 *    };
 *
 *  This will deliver count 0 for the top cell.
 */
template <class Value>
class InstanceStatistics 
{
public:
  typedef std::map <geo::cell_index_type, Value> cache_t;
  typedef std::set <geo::cell_index_type> selection_t;
  typedef selection_t::const_iterator selection_iterator;

  /** 
   *  @brief Instantiate a counter object with a reference to the given cell graph
   */
  InstanceStatistics (const geo::Layout *layout)
    : m_cache (), mp_layout (layout)
  {
    //  .. nothing yet ..
  }

  /** 
   *  @brief Instantiate a counter object with a reference to the given cell graph
   *
   *  This version allows one to specify a initial (starting) cell where only the cell tree below the
   *  staring cell is considered. Multiplicity refers to the number of instances below the
   *  initial cell.
   */
  InstanceStatistics (const geo::Layout *layout, geo::cell_index_type starting_cell)
    : m_cache (), mp_layout (layout)
  {
    layout->cell (starting_cell).collect_called_cells (m_selection);
    m_selection.insert (starting_cell);
  }

  /**
   *  @brief Determine the value of the cell with index "ci"
   *
   *  The value is the collected value of all instances of the cell in all
   *  top cells of the graph. The top cell delivers the default value (default ctor of Value).
   */
  Value value (geo::cell_index_type ci)
  {
    typename cache_t::const_iterator c = m_cache.find (ci);

    if (c != m_cache.end ()) {
      return c->second;
    } else if (! m_selection.empty () && m_selection.find (ci) == m_selection.end ()) {
      return Value (*mp_layout, mp_layout->cell (ci));
    } else {

      const geo::Cell *cell = & mp_layout->cell (ci);

      Value res (*mp_layout, *cell);

      for (geo::Cell::parent_inst_iterator p = cell->begin_parent_insts (); ! p.at_end (); ++p) {
        if (m_selection.empty () || m_selection.find (p->parent_cell_index ()) != m_selection.end ()) {
          res.add (value (p->parent_cell_index ()).transformed (p->child_inst ().cell_inst ()));
        }
      }

      m_cache.insert (std::make_pair (ci, res));
      return res;

    }
  }

  /**
   *  @brief Begin iterator for the cells in the selection
   *
   *  The iterator pair delivers all selected cells (only applicable if an initial cell is specified).
   */
  selection_iterator begin () const 
  {
    return m_selection.begin ();
  }
  
  /**
   *  @brief End iterator for the cells in the selection
   */
  selection_iterator end () const 
  {
    return m_selection.end ();
  }

  /**
   *  @brief Get the selection cone
   */
  const selection_t &selection () const
  {
    return m_selection;
  }

private:
  cache_t m_cache;
  selection_t m_selection;
  const geo::Layout *mp_layout;
};


}  // namespace geo

#endif

