#ifndef HDR_geoLayoutUtils
#define HDR_geoLayoutUtils

#include "geoCommon.h"

#include "geoLayout.h"

#include <map>
#include <vector>
#include <limits>

namespace geo
{

/**
 *  @brief A implementation of ImportLayerMapping that does a direct layer mapping
 *
 *  This implementation will create new layers if required.
 */
class DB_PUBLIC DirectLayerMapping
  : public ImportLayerMapping 
{
public:
  /**
   *  @brief Destructor
   */
  DirectLayerMapping (geo::Layout *target_layout);

  /**
   *  @brief Perform the mapping, i.e. deliver a layer index for a given LayerProperties information
   *
   *  This method can return false in the first member of the returned pair to indicate that no mapping shall
   *  be performed. Otherwise it must return the layer index in the second member.
   */
  virtual std::pair <bool, unsigned int> map_layer (const LayerProperties &lprops);

private:
  std::map <geo::LayerProperties, unsigned int, geo::LPLogicalLessFunc> m_lmap;
  geo::Layout *mp_layout;
  bool m_initialized;
};

/**
 *  @brief A property mapper based on a dynamic property id generation
 *
 *  This class can be used as property mapper for certain "insert" flavors of
 *  the Instance and Shapes class. 
 */
class DB_PUBLIC PropertyMapper
{
public:
  /**
   *  @brief Instantiate a property mapper for mapping of property ids from the source to the target layout
   *
   *  @param source The source layout
   *  @param target The target layout
   */
  PropertyMapper (geo::Layout &target, const geo::Layout &source);
  
  /**
   *  @brief Instantiate a property mapper for mapping of property ids from the source to the target layout
   *
   *  This version does not specify a certain source or target layout. These must be set with the
   *  set_source or set_target methods.
   */
  PropertyMapper ();

  /**
   *  @brief Specify the source layout
   */
  void set_source (const geo::Layout &source);
  
  /**
   *  @brief Specify the target layout
   */
  void set_target (geo::Layout &target);
  
  /**
   *  @brief The actual mapping function
   */
  geo::Layout::properties_id_type operator() (geo::Layout::properties_id_type source_id);

private:
  geo::Layout *mp_target;
  const geo::Layout *mp_source;
  std::map <geo::Layout::properties_id_type, geo::Layout::properties_id_type> m_prop_id_map;
};

/**
 *  @brief A constant describing "drop cell" mapping
 *
 *  If used as the target cell index, this constant means "drop the cell".
 *  This cell and it's children will be dropped unless the children are used by other cells.
 */
const geo::cell_index_type DropCell = std::numeric_limits<geo::cell_index_type>::max ();

/**
 *  @brief Merge one layout into another
 *
 *  This method copies a given set of source cells from the source to the target layout.
 *  While doing so, it can apply a transformation. The transformation is applied on the source cell
 *  level while the magnification part of the transformation is applied on all levels of cells below the source 
 *  cell level. This avoids that magnifying instances need to be created.
 *  A cell mapping table can be specified which will tell how cells should be mapped: if a source cell is 
 *  found in the cell mapping table, the respective cell is used as the target cell. If a cell is not listed
 *  in the cell mapping, a new cell is created. If non-null, final_cell_mapping will hold of list of target layout cells
 *  vs. source layout cells.
 *  Instances are only copied for cells which are created new. 
 *  The layer mapping table identifies target layers for source layout layers.
 */
void DB_PUBLIC 
merge_layouts (geo::Layout &target, const geo::Layout &source, const geo::ICplxTrans &trans,
               const std::vector<geo::cell_index_type> &source_cells, 
               const std::map<geo::cell_index_type, geo::cell_index_type> &cell_mapping,
               const std::map<unsigned int, unsigned int> &layer_mapping,
               std::map<geo::cell_index_type, geo::cell_index_type> *final_cell_mapping = 0);

/**
 *  @brief An interface for the shape inserter
 *
 *  This interface is used by copy_shapes and move_shapes to insert
 *  a shape collection into a another one. By reimplementing this interface,
 *  more shape transformations can be provided.
 */
class DB_PUBLIC ShapesTransformer
{
public:
  ShapesTransformer () { }
  virtual ~ShapesTransformer () { }
  virtual void insert_transformed (geo::Shapes &into, const geo::Shapes &from, const geo::ICplxTrans &trans, geo::PropertyMapper &pm) const = 0;
};

/**
 *  @brief Copy shapes from one layout to another
 *
 *  This method copies shapes hierarchically from one layout to another.
 *  A cell mapping can be specified - if no target cell is found for a specific source cell, 
 *  the shapes will be propagated to the next parent.
 *  A transformation can be specified which is applied to the shapes and which can be used
 *  for example to compensate different database units of the layouts.
 */
void DB_PUBLIC
copy_shapes (geo::Layout &target, 
             const geo::Layout &source, 
             const geo::ICplxTrans &trans,
             const std::vector<geo::cell_index_type> &source_cells, 
             const std::map<geo::cell_index_type, geo::cell_index_type> &cell_mapping,
             const std::map<unsigned int, unsigned int> &layer_mapping,
             const ShapesTransformer *transformer = 0);

/**
 *  @brief Move shapes from one layout to another
 *
 *  This method moves shapes hierarchically from one layout to another.
 *  A cell mapping can be specified - if no target cell is found for a specific source cell, 
 *  the shapes will be propagated to the next parent.
 *  A transformation can be specified which is applied to the shapes and which can be used
 *  for example to compensate different database units of the layouts.
 */
void DB_PUBLIC
move_shapes (geo::Layout &target, 
             geo::Layout &source, 
             const geo::ICplxTrans &trans,
             const std::vector<geo::cell_index_type> &source_cells, 
             const std::map<geo::cell_index_type, geo::cell_index_type> &cell_mapping,
             const std::map<unsigned int, unsigned int> &layer_mapping,
             const ShapesTransformer *transformer = 0);

/**
 *  @brief Find an example cell instance from a child to a top cell
 *
 *  Returns true in the first member of the returned cell if such a path exists
 *  and one example instantiation in the second member.
 */
std::pair<bool, geo::ICplxTrans>
DB_PUBLIC find_layout_context (const geo::Layout &layout, geo::cell_index_type from, geo::cell_index_type to);

/**
 *  @brief A cache for contexts
 *
 *  This object is supposed to minimize the effort to compute many contexts.
 */
class DB_PUBLIC ContextCache
{
public:
  /**
   *  @brief Creates an object of ContextCache associated with the given layout
   */
  ContextCache (const geo::Layout *layout);

  /**
   *  @brief Find the context for the given cell combination
   */
  const std::pair<bool, geo::ICplxTrans> &find_layout_context (geo::cell_index_type from, geo::cell_index_type to);

private:
  std::map<std::pair<geo::cell_index_type, geo::cell_index_type>, std::pair<bool, geo::ICplxTrans> > m_cache;
  const geo::Layout *mp_layout;
};

/**
 *  @brief Scales and snaps the layout below the given cell
 *
 *  This method will scale and snap all layers from the given cell and below to the
 *  specified grid. Scaling happens by the rational factor m / d.
 */
DB_PUBLIC void scale_and_snap (geo::Layout &layout, geo::Cell &cell, geo::Coord g, geo::Coord m, geo::Coord d);

}  // namespace geo

#endif

