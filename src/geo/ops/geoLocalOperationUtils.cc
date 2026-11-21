#include "geoLocalOperationUtils.h"
#include "geoPolygonTools.h"

namespace geo
{

// -----------------------------------------------------------------------------------------------
//  class EdgeToEdgeSetGenerator

// -----------------------------------------------------------------------------
//  LocalOperationUtils implementation

EdgeToEdgeSetGenerator::EdgeToEdgeSetGenerator (std::unordered_set<geo::Edge> &edges)
  : mp_edges (&edges)
{
  //  .. nothing yet ..
}

void EdgeToEdgeSetGenerator::put (const geo::Edge &edge)
{
  mp_edges->insert (edge);
}

// -----------------------------------------------------------------------------------------------
//  class PolygonRefGenerator

PolygonRefToShapesGenerator::PolygonRefToShapesGenerator (geo::Layout *layout, geo::Shapes *shapes)
  : PolygonSink (), mp_layout (layout), mp_shapes (shapes)
{
  //  .. nothing yet ..
}

void PolygonRefToShapesGenerator::put (const geo::Polygon &polygon)
{
  geo::MutexLocker locker (&mp_layout->lock ());
  mp_shapes->insert (geo::PolygonRef (polygon, mp_layout->shape_repository ()));
}

// -----------------------------------------------------------------------------------------------
//  class PolygonSplitter

PolygonSplitter::PolygonSplitter (PolygonSink &sink, double max_area_ratio, size_t max_vertex_count)
  : mp_sink (&sink), m_max_area_ratio (max_area_ratio), m_max_vertex_count (max_vertex_count)
{
  //  .. nothing yet ..
}

void
PolygonSplitter::put (const geo::Polygon &poly)
{
  if ((m_max_vertex_count > 0 && poly.vertices () > m_max_vertex_count) || (m_max_area_ratio > 0.0 && poly.area_ratio () > m_max_area_ratio)) {

    std::vector <geo::Polygon> split_polygons;
    geo::split_polygon (poly, split_polygons);
    for (std::vector <geo::Polygon>::const_iterator sp = split_polygons.begin (); sp != split_polygons.end (); ++sp) {
      put (*sp);
    }

  } else {
    mp_sink->put (poly);
  }
}

}
