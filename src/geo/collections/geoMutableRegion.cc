#include "geoMutableRegion.h"

namespace geo
{

// -------------------------------------------------------------------------------------------------------------
//  FlatRegion implementation

MutableRegion::MutableRegion ()
  : AsIfFlatRegion ()
{
  //  .. nothing yet ..
}

MutableRegion::MutableRegion (const MutableRegion &other)
  : AsIfFlatRegion (other)
{
  //  .. nothing yet ..
}

MutableRegion::~MutableRegion ()
{
  //  .. nothing yet ..
}

void
MutableRegion::insert (const geo::Box &box)
{
  if (! box.empty () && box.width () > 0 && box.height () > 0) {
    do_insert (geo::Polygon (box));
  }
}

void
MutableRegion::insert (const geo::Path &path)
{
  if (path.points () > 0) {
    do_insert (path.polygon ());
  }
}

void
MutableRegion::insert (const geo::SimplePolygon &polygon)
{
  if (polygon.vertices () > 0) {
    geo::Polygon poly;
    poly.assign_hull (polygon.begin_hull (), polygon.end_hull ());
    do_insert (poly);
  }
}

void
MutableRegion::insert (const geo::Shape &shape)
{
  if (shape.is_polygon () || shape.is_path () || shape.is_box ()) {
    geo::Polygon poly;
    shape.polygon (poly);
    insert (poly);
  } else if (shape.is_path ()) {
    insert (shape.path ());
  } else if (shape.is_box ()) {
    insert (shape.box ());
  }
}

}

