#ifndef HDR_geoClipboardData
#define HDR_geoClipboardData

#include "geoCommon.h"

#include "geoLayout.h"
#include "geoLayoutUtils.h"

#include <map>
#include <vector>

namespace geo
{

/**
 *  @brief A receiver for insert events of the clipboard data object
 */
class DB_PUBLIC ClipboardDataInsertReceiver 
{
public:
  ClipboardDataInsertReceiver () 
  {
    //  .. nothing yet ..
  }

  virtual ~ClipboardDataInsertReceiver () 
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief This method is called when a shape is inserted
   *
   *  @param cell The index of the cell where the shape is inserted
   *  @param layer The layer where the shape is inserted
   *  @param shape The (new) shape that was inserted
   */
  virtual void shape_inserted (geo::cell_index_type /*cell*/, int /*layer*/, const geo::Shape & /*shape*/) 
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief This method is called when an instance is inserted
   *
   *  @param cell The index of the cell where the shape is inserted
   *  @param shape The (new) instance that was inserted
   */
  virtual void instance_inserted (geo::cell_index_type /*cell*/, const geo::Instance & /*instance*/) 
  {
    //  .. nothing yet ..
  }
};

/**
 *  @brief A container for the clipboard data 
 *
 *  This is basically a layout object enhanced with some special data to 
 *  represent the data on the clipboard.
 */
class DB_PUBLIC ClipboardData
{
public:
  /**
   *  @brief Default ctor
   */
  ClipboardData ();

  /**
   *  @brief dtor
   */
  ~ClipboardData ();

  /**
   *  @brief Add a shape to the clipboard data
   *
   *  It is assumed that all add operations are made from the same layout object for 
   *  one ClipboardData object.
   */
  void add (const geo::Layout &layout, unsigned int layer, const geo::Shape &shape);

  /**
   *  @brief Add a transformed shape to the clipboard data, transformed with the given transformation
   *
   *  It is assumed that all add operations are made from the same layout object for 
   *  one ClipboardData object.
   */
  void add (const geo::Layout &layout, unsigned int layer, const geo::Shape &shape, const geo::ICplxTrans &trans);

  /**
   *  @brief Add an instance to the clipboard data
   *
   *  Depending on the mode, not only the instance but the cell which is instantiated 
   *  is added to the clipboard data as well.
   *  It is assumed that all add operations are made from the same layout object for 
   *  one ClipboardData object.
   *
   *  @param mode 0, if to copy just the instance, 1 to copy the cell as well (in deep mode)
   */
  void add (const geo::Layout &layout, const geo::Instance &inst, unsigned int mode);

  /**
   *  @brief Add an transformed instance to the clipboard data
   *
   *  Depending on the mode, not only the instance but the cell which is instantiated 
   *  is added to the clipboard data as well.
   *  It is assumed that all add operations are made from the same layout object for 
   *  one ClipboardData object.
   *
   *  @param mode 0, if to copy just the instance, 1 to copy the cell as well (in deep mode)
   */
  void add (const geo::Layout &layout, const geo::Instance &inst, unsigned int mode, const geo::ICplxTrans &trans);

  /**
   *  @brief Add a cell to the clipboard data
   *
   *  Depending on the mode, not only the cell but all subcells are added to the clipboard 
   *  data as well. In "toplevel only" mode, just the instances are copied, not the subcells.
   *  It is assumed that all add operations are made from the same layout object for 
   *  one ClipboardData object.
   *
   *  @param mode 0, if to copy just the cell, 1 to copy the subcells as well, 2 to copy the first level of the hierarchy.
   *  @return The index of the created cell
   */
  geo::cell_index_type add (const geo::Layout &layout, const geo::Cell &cell, unsigned int mode);

  /**
   *  @brief Insert the data into the given layout
   *
   *  Cells that are stored in this object are either looked for (if the cell is copied without
   *  content, i.e as target for an instance) or created newly if stored with content.
   *  Layers are mapped where required. 
   *  Layer mapping involves looking up the target layer. The target layer is looked up by layer/datatype
   *  first, then name. If a layer is not found, it will be created newly.
   *
   *  @param into The layout into which to insert the new items
   *  @param cell If non-null, the items will be created in this cell 
   *  @param new_top If non-null, this vector will be filled with the indices of the newly
   *                 created top cells in "into" layout
   *  @param insert_receiver A notification object that receives insert events, i.e. for providing a selection
   *  @return An array containing a vector of newly created layers in the "into" layout
   */
  std::vector<unsigned int> insert (geo::Layout &into, geo::Cell *cell = 0, std::vector<geo::cell_index_type> *new_tops = 0, ClipboardDataInsertReceiver *insert_receiver = 0) const
  {
    return do_insert (into, 0, cell, new_tops, insert_receiver);
  }

  /**
   *  @brief Insert the data into the given layout with a transformation
   *
   *  Cells that are stored in this object are either looked for (if the cell is copied without
   *  content, i.e as target for an instance) or created newly if stored with content.
   *  Layers are mapped where required. 
   *  Layer mapping involves looking up the target layer. The target layer is looked up by layer/datatype
   *  first, then name. If a layer is not found, it will be created newly.
   *
   *  @param into The layout into which to insert the new items
   *  @param trans The transformation to apply
   *  @param cell If non-null, the items will be created in this cell 
   *  @param new_tops If non-null, this vector will be filled with the indices of the newly
   *                  created top cells in "into" layout
   *  @param insert_receiver A notification object that receives insert events, i.e. for providing a selection
   *  @return An array containing a vector of newly created layers in the "into" layout
   */
  std::vector<unsigned int> insert (geo::Layout &into, const geo::ICplxTrans &trans, geo::Cell *cell = 0, std::vector<geo::cell_index_type> *new_tops = 0, ClipboardDataInsertReceiver *insert_receiver = 0) const
  {
    return do_insert (into, &trans, cell, new_tops, insert_receiver);
  }

private:
  geo::Layout m_layout;  //  this is where we store data into.
  std::set <geo::cell_index_type> m_incomplete_cells;
  std::map <geo::cell_index_type, std::vector<std::string> > m_context_info;
  std::map <geo::cell_index_type, geo::cell_index_type> m_cell_index_map;
  geo::cell_index_type m_container_cell_index;
  
  PropertyMapper m_prop_id_map;

  geo::cell_index_type cell_for_cell (const geo::Layout &layout, geo::cell_index_type cell_index, bool incomplete);
  std::vector<unsigned int> do_insert (geo::Layout &into, const geo::ICplxTrans *trans, geo::Cell *cell, std::vector<geo::cell_index_type> *new_tops, ClipboardDataInsertReceiver *insert_receiver) const;

  ClipboardData (const ClipboardData &);
  ClipboardData &operator= (const ClipboardData &);
};

}

#endif


