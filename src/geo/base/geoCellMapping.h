#ifndef HDR_geoCellMapping
#define HDR_geoCellMapping


#include "geoTypes.h"

#include <map>
#include <vector>
#include <set>

namespace geo
{

class Layout;

/**
 *  @brief A cell mapping
 *
 *  The cell mapping is basically a table that lists corresponding cells in a layout "A"
 *  for cells in layout "B". "B" is considered the source layout while "A" is the target layout.
 *
 *  Cell mappings can be created using a strict geometrical mapping algorithm or 
 *  using name equivalence.
 *
 *  Cell mappings play a role in copy and compare operations.
 */
class CellMapping
{
public:
  typedef std::map <geo::cell_index_type, geo::cell_index_type>::const_iterator iterator;

  /**
   *  @brief Constructor - creates an empty mapping
   */
  CellMapping ();

  /**
   *  @brief Clear the mapping
   */
  void clear ();

  /**
   *  @brief Create a single cell mapping 
   *
   *  A single cell mapping will not map the child cells. When used for hierarchical copy this will basically 
   *  flatten the cell on copy.
   */
  void create_single_mapping (const geo::Layout &layout_a, geo::cell_index_type cell_index_a, const geo::Layout &layout_b, geo::cell_index_type cell_index_b);

  /**
   *  @brief Create a single cell full mapping
   *
   *  This will map the given cell plus create new cells for the child cells of cell_b in layout_a. The
   *  instance tree will be copied over from layout_b to layout_a.
   */
  std::vector<geo::cell_index_type> create_single_mapping_full (geo::Layout &layout_a, geo::cell_index_type cell_index_a, const geo::Layout &layout_b, geo::cell_index_type cell_index_b)
  {
    create_single_mapping (layout_a, cell_index_a, layout_b, cell_index_b);
    return create_missing_mapping (layout_a, layout_b, cell_index_b);
  }

  /**
   *  @brief Create a multi-cell mapping
   *
   *  The mapping created by this function will map the cells of cell_indexes_b to cell_indexes_a. Both arrays need to have
   *  the same length.
   *  This cell mapping will not map the child cells. When used for hierarchical copy this will basically
   *  flatten the cells on copy.
   */
  void create_multi_mapping (const geo::Layout &layout_a, const std::vector<geo::cell_index_type> &cell_indexes_a, const geo::Layout &layout_b, const std::vector<geo::cell_index_type> &cell_indexes_b);

  /**
   *  @brief Create a single cell full mapping
   *
   *  The mapping created by this function will map the cells of cell_indexes_a to cell_indexes_b. Both arrays need to have
   *  the same length. In addition, new cells for the child cells of all cell_indexes_b cells are created in layout_a and
   *  included in the mapping.
   */
  std::vector<geo::cell_index_type> create_multi_mapping_full (geo::Layout &layout_a, const std::vector<geo::cell_index_type> &cell_indexes_a, const geo::Layout &layout_b, const std::vector<geo::cell_index_type> &cell_indexes_b)
  {
    create_multi_mapping (layout_a, cell_indexes_a, layout_b, cell_indexes_b);
    return create_missing_mapping (layout_a, layout_b, cell_indexes_b);
  }

  /**
   *  @brief Create a mapping for layout_b (with initial/top cell cell_index_b) to layout_a (with initial/top cell cell_index_a)
   */
  void create_from_geometry (const geo::Layout &layout_a, geo::cell_index_type cell_index_a, const geo::Layout &layout_b, geo::cell_index_type cell_index_b);

  /**
   *  @brief Create a mapping for layout_b (with initial/top cell cell_index_b) to layout_a (with initial/top cell cell_index_a)
   *
   *  This version creates a full mapping, i.e. cells with no direct corresponding cell in layout A are created there plus
   *  their instances are copied over.
   *
   *  @return A list of cell indexes of the newly created cells.
   */
  std::vector<geo::cell_index_type> create_from_geometry_full (geo::Layout &layout_a, geo::cell_index_type cell_index_a, const geo::Layout &layout_b, geo::cell_index_type cell_index_b)
  {
    create_from_geometry (layout_a, cell_index_a, layout_b, cell_index_b);
    return create_missing_mapping (layout_a, layout_b, cell_index_b);
  }

  /**
   *  @brief Create a mapping for layout_b (with initial/top cell cell_index_b) to layout_a (with initial/top cell cell_index_a)
   */
  void create_from_names (const geo::Layout &layout_a, geo::cell_index_type cell_index_a, const geo::Layout &layout_b, geo::cell_index_type cell_index_b);

  /**
   *  @brief Create a mapping for layout_b (with initial/top cell cell_index_b) to layout_a (with initial/top cell cell_index_a)
   *
   *  This version creates a full mapping, i.e. cells with no direct corresponding cell in layout A are created there plus
   *  their instances are copied over.
   *
   *  @return A list of cell indexes of the newly created cells.
   */
  std::vector<geo::cell_index_type> create_from_names_full (geo::Layout &layout_a, geo::cell_index_type cell_index_a, const geo::Layout &layout_b, geo::cell_index_type cell_index_b)
  {
    create_from_names (layout_a, cell_index_a, layout_b, cell_index_b);
    return create_missing_mapping (layout_a, layout_b, cell_index_b);
  }

  /**
   *  @brief Determine cell mapping to a layout_b cell to the corresponding layout_a cell.
   *
   *  @param cell_index_b The index of the cell in layout_b whose mapping is requested.
   *  @return First: true, if a unique mapping is given, Second: the cell index in layout_a.
   */
  std::pair<bool, geo::cell_index_type> cell_mapping_pair (geo::cell_index_type cell_index_b) const;

  /**
   *  @brief Determine if a cell layout_b has a mapping to a layout_a cell.
   *
   *  @param cell_index_b The index of the cell in layout_b whose mapping is requested.
   *  @return true, if the cell has a mapping
   */
  bool has_mapping (geo::cell_index_type cell_index_b) const;

  /**
   *  @brief Add a cell mapping
   *
   *  @param cell_index_b The index of the cell in layout_a (the source of the mapping)
   *  @param cell_index_a The index of the cell in layout_a (the target of the mapping)
   */
  void map (geo::cell_index_type cell_index_b, geo::cell_index_type cell_index_a)
  {
    m_b2a_mapping.insert (std::make_pair (cell_index_b, 0)).first->second = cell_index_a;
  }

  /**
   *  @brief Determine cell mapping to a layout_b cell to the corresponding layout_a cell.
   *
   *  @param cell_index_b The index of the cell in layout_b whose mapping is requested.
   *  @return the cell index in layout_a.
   */
  geo::cell_index_type cell_mapping (geo::cell_index_type cell_index_b) const;

  /**
   *  @brief Begin iterator for the b to a cell mapping
   */
  iterator begin () const
  {
    return m_b2a_mapping.begin ();
  }

  /**
   *  @brief End iterator for the b to a cell mapping
   */
  iterator end () const
  {
    return m_b2a_mapping.end ();
  }

  /**
   *  @brief Gets the source cells
   */
  std::vector<geo::cell_index_type> source_cells () const;

  /**
   *  @brief Access to the mapping table
   */
  const std::map <geo::cell_index_type, geo::cell_index_type> &table () const 
  {
    return m_b2a_mapping;
  }

  /**
   *  @brief Creates mappings for all cells not mapped yet
   *
   *  When constructing a cell mapping by explicit mapping (map (a, b)), some child cells of the
   *  mapped other cells may be left unmapped.
   *  This method allows creating mappings for these missing cells by adding
   *  new cells and the corresponding instances into the target layout_a.
   *
   *  If given, "exclude_cells" can specify a list of cells not to map.
   *
   *  If given, "include_cells" can specify a list of cells which are included in the
   *  cell tree to create. Cells not in the "include_cells" list are ignored.
   *
   *  The returned vector lists the new cells.
   */
  std::vector<geo::cell_index_type> create_missing_mapping (geo::Layout &layout_a, const geo::Layout &layout_b, const std::vector<geo::cell_index_type> &cell_index_b, const std::set<geo::cell_index_type> *exclude_cells = 0, const std::set<geo::cell_index_type> *include_cells = 0);

  /**
   *  @brief A convenience version of create_missing_mappings which takes a single cell for cell_index_a and cell_index_b.
   */
  std::vector<geo::cell_index_type> create_missing_mapping (geo::Layout &layout_a, const geo::Layout &layout_b, geo::cell_index_type cell_index_b, const std::set<geo::cell_index_type> *exclude_cells = 0, const std::set<geo::cell_index_type> *include_cells = 0)
  {
    std::vector<geo::cell_index_type> cib;
    cib.reserve (1);
    cib.push_back (cell_index_b);
    return create_missing_mapping (layout_a, layout_b, cib, exclude_cells, include_cells);
  }

  /**
   *  @brief Like create_missing_mapping, but returns the newly mapped pairs
   *
   *  The first cell index of the pair is the old cell in layout_a, the second cell index
   *  is the new cell in layout_b.
   */
  std::vector<std::pair<geo::cell_index_type, geo::cell_index_type> > create_missing_mapping2 (geo::Layout &layout_a, const geo::Layout &layout_b, const std::vector<geo::cell_index_type> &cell_index_b, const std::set<geo::cell_index_type> *exclude_cells = 0, const std::set<geo::cell_index_type> *include_cells = 0);

  /**
   *  @brief Like create_missing_mapping, but returns the newly mapped pairs (single-cell version)
   *
   *  The first cell index of the pair is the old cell in layout_a, the second cell index
   *  is the new cell in layout_b.
   */
  std::vector<std::pair<geo::cell_index_type, geo::cell_index_type> > create_missing_mapping2 (geo::Layout &layout_a, const geo::Layout &layout_b, geo::cell_index_type cell_index_b, const std::set<geo::cell_index_type> *exclude_cells = 0, const std::set<geo::cell_index_type> *include_cells = 0)
  {
    std::vector<geo::cell_index_type> cib;
    cib.reserve (1);
    cib.push_back (cell_index_b);
    return create_missing_mapping2 (layout_a, layout_b, cib, exclude_cells, include_cells);
  }

private:
  void extract_unique (std::map <geo::cell_index_type, std::vector<geo::cell_index_type> >::const_iterator cand, 
                       std::map<geo::cell_index_type, geo::cell_index_type> &unique_mapping,
                       const geo::Layout &layout_a, const geo::Layout &layout_b);

  void dump_mapping (const std::map <geo::cell_index_type, std::vector<geo::cell_index_type> > &candidates, 
                     const geo::Layout &layout_a, const geo::Layout &layout_b);

  void do_create_missing_mapping (geo::Layout &layout_a, const geo::Layout &layout_b, const std::vector<geo::cell_index_type> &cell_index_b, const std::set<geo::cell_index_type> *exclude_cells, const std::set<geo::cell_index_type> *include_cells, std::vector<geo::cell_index_type> *new_cells, std::vector<std::pair<geo::cell_index_type, geo::cell_index_type> > *mapped_pairs);

  std::map <geo::cell_index_type, geo::cell_index_type> m_b2a_mapping;
};

}

#endif


