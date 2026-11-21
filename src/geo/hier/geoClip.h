#ifndef HDR_geoClip
#define HDR_geoClip

#include "geoCommon.h"

#include "geoTypes.h"
#include "geoBox.h"
#include "geoPolygon.h"

#include <vector>

namespace geo {

class Layout;

/**
 *  @brief A helper method: collect clip boxes from a layer
 *
 *  This is basically not a function specific for the clipper - the layer's content below
 *  the given cell is flattened and the resulting boxes are returned in the "boxes" vector.
 *
 *  @param layout The layout from which to take the boxes
 *  @param cell_index The cell from which to start
 *  @param layer The layer index from which to take the boxes
 *  @param boxes Where the boxes are stored
 */
DB_PUBLIC void collect_clip_boxes (const geo::Layout &layout, geo::cell_index_type cell_index, unsigned int layer, std::vector <geo::Box> &boxes);

/**
 *  @brief Clip a given simple polygon with the given box
 *
 *  In the generic case, multiple polygons may be created.
 *
 *  @param poly The input polygon to clip
 *  @param box The box at which to clip
 *  @param clipped_poly Where the clip results are stored. The clip results are appended to this vector
 */
DB_PUBLIC void clip_poly (const geo::SimplePolygon &poly, const geo::Box &box, std::vector <geo::SimplePolygon> &clipped_poly, bool resolve_holes = true);

/**
 *  @brief Clip a given polygon with the given box
 *
 *  In the generic case, multiple polygons may be created.
 *
 *  @param poly The input polygon to clip
 *  @param box The box at which to clip
 *  @param clipped_poly Where the clip results are stored. The clip results are appended to this vector
 */
DB_PUBLIC void clip_poly (const geo::Polygon &poly, const geo::Box &box, std::vector <geo::Polygon> &clipped_poly, bool resolve_holes = true);

/**
 *  @brief Clip a layout
 *
 *  Clips a given cell at a set of given rectangles and produces a new set of cells and clip variants
 *  which is instantated in the target layout. Source and target layout may be identical. 
 *
 *  @param layout The input layout
 *  @param target_layout The target layout where to produce the clip cell
 *  @param cell_index Which cell to clip
 *  @param clip_boxes Which boxes to clip at
 *  @param stable If true, the function will return corresponding clip cells for each clip box. The clip cells may be empty.
 *  @return A set of cells which contain the clips. If the layout and target layout is identical, these cells may be identical with original cells.
 */
DB_PUBLIC std::vector <geo::cell_index_type> clip_layout (const Layout &layout, Layout &target_layout, geo::cell_index_type cell_index, const std::vector <geo::Box> &clip_boxes, bool stable);

} // namespace geo

#endif


