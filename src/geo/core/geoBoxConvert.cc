#include "geoCommon.h"

#include "geoBoxConvert.h"
#include "geoCell.h"

namespace geo
{
// -----------------------------------------------------------------------------
//  BoxConvert implementation


DB_PUBLIC geo::Box cellinst_box_convert_impl (const geo::CellInst &inst, const geo::Layout *layout, int layer, bool allow_empty)
{
  if (layer >= 0) {
    return inst.bbox (*layout, layer);
  } else if (allow_empty) {
    return inst.bbox (*layout);
  } else {
    geo::Box box = inst.bbox (*layout);
    if (box.empty ()) {
      return geo::Box (geo::Point (0, 0), geo::Point (0, 0));
    } else {
      return box;
    }
  }
}

DB_PUBLIC geo::Box cell_box_convert_impl (const geo::Cell &c, int layer, bool allow_empty)
{
  if (layer >= 0) {
    return c.bbox (layer);
  } else if (allow_empty) {
    return c.bbox ();
  } else {
    if (c.bbox ().empty ()) {
      return geo::Box (geo::Point (0, 0), geo::Point (0, 0));
    } else {
      return c.bbox ();
    }
  }
}

template struct box_convert <geo::CellInst, true>;
template struct box_convert <geo::CellInst, false>;
template struct box_convert <geo::Cell, true>;
template struct box_convert <geo::Cell, false>;

}

