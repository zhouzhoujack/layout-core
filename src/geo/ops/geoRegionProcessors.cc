#include "geoRegionProcessors.h"
#include "geoPolygon.h"
#include "geoPolygonGenerators.h"

namespace geo
{

// -----------------------------------------------------------------------------------
//  CornerDetectorCore implementation

CornerDetectorCore::CornerDetectorCore (double angle_start, bool include_angle_start, double angle_end, bool include_angle_end)
  : m_checker (angle_start, include_angle_start, angle_end, include_angle_end)
{
  //  .. nothing yet ..
}

void CornerDetectorCore::detect_corners (const geo::Polygon &poly, const CornerPointDelivery &delivery) const
{
  size_t n = poly.holes () + 1;
  for (size_t i = 0; i < n; ++i) {

    const geo::Polygon::contour_type &ctr = poly.contour (int (i));
    size_t nn = ctr.size ();
    if (nn > 2) {

      geo::Point pp = ctr [nn - 2];
      geo::Point pt = ctr [nn - 1];
      for (size_t j = 0; j < nn; ++j) {

        geo::Point pn = ctr [j];

        if (m_checker (pt - pp, pn - pt)) {
          delivery.make_point (pt, geo::Edge (pp, pt), geo::Edge (pt, pn));
        }

        pp = pt;
        pt = pn;

      }

    }

  }
}

// -----------------------------------------------------------------------------------
//  Extents implementation

void Extents::process (const geo::Polygon &poly, std::vector<geo::Polygon> &result) const
{
  geo::Box b = poly.box ();
  if (! b.empty ()) {
    result.push_back (geo::Polygon (b));
  }
}

// -----------------------------------------------------------------------------------
//  RelativeExtents implementation

void RelativeExtents::process (const geo::Polygon &poly, std::vector<geo::Polygon> &result) const
{
  geo::Box b = poly.box ();
  geo::Point p1 (b.left () + geo::coord_traits<geo::Coord>::rounded (m_fx1 * b.width ()),
                b.bottom () + geo::coord_traits<geo::Coord>::rounded (m_fy1 * b.height ()));
  geo::Point p2 (b.left () + geo::coord_traits<geo::Coord>::rounded (m_fx2 * b.width ()),
                b.bottom () + geo::coord_traits<geo::Coord>::rounded (m_fy2 * b.height ()));
  geo::Box box = geo::Box (p1, p2).enlarged (geo::Vector (m_dx, m_dy));
  if (! box.empty ()) {
    result.push_back (geo::Polygon (box));
  }
}

const TransformationReducer *RelativeExtents::vars () const
{
  if (m_dx == 0 && m_dy == 0 && fabs (m_fx1) < geo::epsilon && fabs (m_fy1) < geo::epsilon && fabs (1.0 - m_fx2) < geo::epsilon && fabs (1.0 - m_fy2) < geo::epsilon) {
    return 0;
  } else if (m_dx == m_dy && fabs (m_fx1 - m_fy1) < geo::epsilon && fabs (1.0 - (m_fx1 + m_fx2)) < geo::epsilon  && fabs (m_fx2 - m_fy2) < geo::epsilon && fabs (1.0 - (m_fy1 + m_fy2)) < geo::epsilon) {
    return & m_isotropic_reducer;
  } else {
    return & m_anisotropic_reducer;
  }
}

// -----------------------------------------------------------------------------------
//  RelativeExtentsAsEdges implementation

void RelativeExtentsAsEdges::process (const geo::Polygon &poly, std::vector<geo::Edge> &result) const
{
  geo::Box b = poly.box ();
  geo::Point p1 (b.left () + geo::coord_traits<geo::Coord>::rounded (m_fx1 * b.width ()),
                b.bottom () + geo::coord_traits<geo::Coord>::rounded (m_fy1 * b.height ()));
  geo::Point p2 (b.left () + geo::coord_traits<geo::Coord>::rounded (m_fx2 * b.width ()),
                b.bottom () + geo::coord_traits<geo::Coord>::rounded (m_fy2 * b.height ()));
  result.push_back (geo::Edge (p1, p2));
}

const TransformationReducer *RelativeExtentsAsEdges::vars () const
{
  return & m_anisotropic_reducer;
}

bool RelativeExtentsAsEdges::result_must_not_be_merged () const
{
  //  don't merge if the results will just be points
  return (fabs (m_fx1 - m_fx2) < geo::epsilon && fabs (m_fy1 - m_fy2) < geo::epsilon);
}

// -----------------------------------------------------------------------------------
//  PolygonToEdgeProcessor implementation

void PolygonToEdgeProcessor::process (const geo::Polygon &poly, std::vector<geo::Edge> &result) const
{
  for (geo::Polygon::polygon_edge_iterator e = poly.begin_edge (); ! e.at_end (); ++e) {
    result.push_back (*e);
  }
}

// -----------------------------------------------------------------------------------
//  ConvexDecomposition implementation

void ConvexDecomposition::process (const geo::Polygon &poly, std::vector<geo::Polygon> &result) const
{
  geo::SimplePolygonContainer sp;
  geo::decompose_convex (poly, m_mode, sp);
  for (std::vector <geo::SimplePolygon>::const_iterator i = sp.polygons ().begin (); i != sp.polygons ().end (); ++i) {
    result.push_back (geo::simple_polygon_to_polygon (*i));
  }
}

// -----------------------------------------------------------------------------------
//  TrapezoidDecomposition implementation

void TrapezoidDecomposition::process (const geo::Polygon &poly, std::vector<geo::Polygon> &result) const
{
  geo::SimplePolygonContainer sp;
  geo::decompose_trapezoids (poly, m_mode, sp);
  for (std::vector <geo::SimplePolygon>::const_iterator i = sp.polygons ().begin (); i != sp.polygons ().end (); ++i) {
    result.push_back (geo::simple_polygon_to_polygon (*i));
  }
}

// -----------------------------------------------------------------------------------
//  PolygonBreaker implementation

void PolygonBreaker::process (const geo::Polygon &poly, std::vector<geo::Polygon> &result) const
{
  if ((m_max_vertex_count > 0 && poly.vertices () > m_max_vertex_count) ||
      (m_max_area_ratio > 0 && poly.area_ratio () > m_max_area_ratio)) {

    std::vector<geo::Polygon> split_polygons;
    geo::split_polygon (poly, split_polygons);
    for (std::vector<geo::Polygon>::const_iterator p = split_polygons.begin (); p != split_polygons.end (); ++p) {
      process (*p, result);
    }

  } else {
    result.push_back (poly);
  }
}

// -----------------------------------------------------------------------------------
//  PolygonSizer implementation

PolygonSizer::PolygonSizer (geo::Coord dx, geo::Coord dy, unsigned int mode)
  : m_dx (dx), m_dy (dy), m_mode (mode)
{
  if (dx == dy) {
    m_vars = new geo::MagnificationReducer ();
  } else {
    m_vars = new geo::XYAnisotropyAndMagnificationReducer ();
  }
}

PolygonSizer::~PolygonSizer ()
{
  delete m_vars;
}

void PolygonSizer::process (const geo::Polygon &poly, std::vector<geo::Polygon> &result) const
{
  geo::PolygonContainer pr (result);
  geo::PolygonGenerator pg2 (pr, false /*don't resolve holes*/, true /*min. coherence*/);
  geo::SizingPolygonFilter siz (pg2, m_dx, m_dy, m_mode);
  siz.put (poly);
}

bool PolygonSizer::result_is_merged () const
{
  return (m_dx < 0 && m_dy < 0);
}

}
