#include "geoMutableEdges.h"

namespace geo
{

// -------------------------------------------------------------------------------------------------------------
//  MutableEdges implementation

MutableEdges::MutableEdges ()
  : AsIfFlatEdges ()
{
  //  .. nothing yet ..
}

MutableEdges::MutableEdges (const MutableEdges &other)
  : AsIfFlatEdges (other)
{
  //  .. nothing yet ..
}

MutableEdges::~MutableEdges ()
{
  //  .. nothing yet ..
}

void
MutableEdges::insert (const geo::Box &box)
{
  if (! box.empty () && box.width () > 0 && box.height () > 0) {
    do_insert (geo::Edge (box.lower_left (), box.upper_left ()));
    do_insert (geo::Edge (box.upper_left (), box.upper_right ()));
    do_insert (geo::Edge (box.upper_right (), box.lower_right ()));
    do_insert (geo::Edge (box.lower_right (), box.lower_left ()));
  }
}

void
MutableEdges::insert (const geo::Path &path)
{
  if (path.points () > 0) {
    insert (path.polygon ());
  }
}

void
MutableEdges::insert (const geo::Polygon &polygon)
{
  if (polygon.holes () > 0 || polygon.vertices () > 0) {
    for (geo::Polygon::polygon_edge_iterator e = polygon.begin_edge (); ! e.at_end (); ++e) {
      do_insert (*e);
    }
  }
}

void
MutableEdges::insert (const geo::SimplePolygon &polygon)
{
  if (polygon.vertices () > 0) {
    for (geo::SimplePolygon::polygon_edge_iterator e = polygon.begin_edge (); ! e.at_end (); ++e) {
      do_insert (*e);
    }
  }
}

void
MutableEdges::insert (const geo::Shape &shape)
{
  if (shape.is_polygon () || shape.is_path () || shape.is_box ()) {

    geo::Polygon poly;
    shape.polygon (poly);
    insert (poly);

  } else if (shape.is_edge ()) {

    geo::Edge edge;
    shape.edge (edge);
    do_insert (edge);

  }
}

}

