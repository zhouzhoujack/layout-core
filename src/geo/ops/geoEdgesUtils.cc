#include "geoEdgesUtils.h"
#include "geoRegion.h"

namespace geo
{

// -------------------------------------------------------------------------------------------------------------
//  JoinEdgesCluster implementation

JoinEdgesCluster::JoinEdgesCluster (geo::PolygonSink *output, coord_type ext_b, coord_type ext_e, coord_type ext_o, coord_type ext_i)
  : mp_output (output), m_ext_b (ext_b), m_ext_e (ext_e), m_ext_o (ext_o), m_ext_i (ext_i)
{
  //  .. nothing yet ..
}

void
JoinEdgesCluster::finish ()
{
  std::multimap<geo::Point, iterator> objects_by_p1;
  std::multimap<geo::Point, iterator> objects_by_p2;
  for (iterator o = begin (); o != end (); ++o) {
    if (o->first->p1 () != o->first->p2 ()) {
      objects_by_p1.insert (std::make_pair (o->first->p1 (), o));
      objects_by_p2.insert (std::make_pair (o->first->p2 (), o));
    }
  }

  while (! objects_by_p2.empty ()) {

    tl_assert (! objects_by_p1.empty ());

    //  Find the beginning of a new sequence
    std::multimap<geo::Point, iterator>::iterator j0 = objects_by_p1.begin ();
    std::multimap<geo::Point, iterator>::iterator j = j0;
    do {
      std::multimap<geo::Point, iterator>::iterator jj = objects_by_p2.find (j->first);
      if (jj == objects_by_p2.end ()) {
        break;
      } else {
        j = objects_by_p1.find (jj->second->first->p1 ());
        tl_assert (j != objects_by_p1.end ());
      }
    } while (j != j0);

    iterator i = j->second;

    //  determine a sequence
    //  TODO: this chooses any solution in case of forks. Choose a specific one?
    std::vector<geo::Point> pts;
    pts.push_back (i->first->p1 ());

    do {

      //  record the next point
      pts.push_back (i->first->p2 ());

      //  remove the edge as it's taken
      std::multimap<geo::Point, iterator>::iterator jj;
      for (jj = objects_by_p2.find (i->first->p2 ()); jj != objects_by_p2.end () && jj->first == i->first->p2 (); ++jj) {
        if (jj->second == i) {
          break;
        }
      }
      tl_assert (jj != objects_by_p2.end () && jj->second == i);
      objects_by_p2.erase (jj);
      objects_by_p1.erase (j);

      //  process along the edge to the next one
      //  TODO: this chooses any solution in case of forks. Choose a specific one?
      j = objects_by_p1.find (i->first->p2 ());
      if (j != objects_by_p1.end ()) {
        i = j->second;
      } else {
        break;
      }

    } while (true);

    bool cyclic = (pts.back () == pts.front ());

    if (! cyclic) {

      //  non-cyclic sequence
      geo::Path path (pts.begin (), pts.end (), 0, m_ext_b, m_ext_e, false);
      std::vector<geo::Point> hull;
      path.hull (hull, m_ext_o, m_ext_i);
      geo::Polygon poly;
      poly.assign_hull (hull.begin (), hull.end ());
      mp_output->put (poly);

    } else {

      //  we have a loop: form a contour by using the polygon size functions and a "Not" to form the hole
      geo::Polygon poly;
      poly.assign_hull (pts.begin (), pts.end ());

      geo::EdgeProcessor ep;
      geo::PolygonGenerator pg (*mp_output, false, true);

      int mode_a = -1, mode_b = -1;

      if (m_ext_o == 0) {
        ep.insert (poly, 0);
      } else {
        geo::Polygon sized_poly (poly);
        sized_poly.size (m_ext_o, m_ext_o, 2 /*sizing mode*/);
        ep.insert (sized_poly, 0);
        mode_a = 1;
      }

      if (m_ext_i == 0) {
        ep.insert (poly, 1);
      } else {
        geo::Polygon sized_poly (poly);
        sized_poly.size (-m_ext_i, -m_ext_i, 2 /*sizing mode*/);
        ep.insert (sized_poly, 1);
        mode_b = 1;
      }

      geo::BooleanOp2 op (geo::BooleanOp::ANotB, mode_a, mode_b);
      ep.process (pg, op);

    }

  }
}

// -------------------------------------------------------------------------------------------------------------
//  extended_edge implementation

geo::Polygon
extended_edge (const geo::Edge &edge, geo::Coord ext_b, geo::Coord ext_e, geo::Coord ext_o, geo::Coord ext_i)
{
  geo::DVector d;
  if (edge.is_degenerate ()) {
    d = geo::DVector (1.0, 0.0);
  } else {
    d = geo::DVector (edge.d ()) * (1.0 / edge.double_length ());
  }

  geo::DVector n (-d.y (), d.x ());

  geo::Point pts[4] = {
    geo::Point (geo::DPoint (edge.p1 ()) - d * double (ext_b) + n * double (ext_o)),
    geo::Point (geo::DPoint (edge.p2 ()) + d * double (ext_e) + n * double (ext_o)),
    geo::Point (geo::DPoint (edge.p2 ()) + d * double (ext_e) - n * double (ext_i)),
    geo::Point (geo::DPoint (edge.p1 ()) - d * double (ext_b) - n * double (ext_i)),
  };

  geo::Polygon poly;
  poly.assign_hull (pts + 0, pts + 4);
  return poly;
}

// -------------------------------------------------------------------------------------------------------------
//  EdgeSegmentSelector processor

EdgeSegmentSelector::EdgeSegmentSelector (int mode, geo::Edges::length_type length, double fraction)
  : m_mode (mode), m_length (length), m_fraction (fraction)
{ }

EdgeSegmentSelector::~EdgeSegmentSelector ()
{ }

void
EdgeSegmentSelector::process (const geo::Edge &edge, std::vector<geo::Edge> &res) const
{
  double l = std::max (edge.double_length () * m_fraction, double (m_length));

  if (m_mode < 0) {

    res.push_back (geo::Edge (edge.p1 (), geo::Point (geo::DPoint (edge.p1 ()) + geo::DVector (edge.d ()) * (l / edge.double_length ()))));

  } else if (m_mode > 0) {

    res.push_back (geo::Edge (geo::Point (geo::DPoint (edge.p2 ()) - geo::DVector (edge.d ()) * (l / edge.double_length ())), edge.p2 ()));

  } else {

    geo::DVector dl = geo::DVector (edge.d ()) * (0.5 * l / edge.double_length ());
    geo::DPoint center = geo::DPoint (edge.p1 ()) + geo::DVector (edge.p2 () - edge.p1 ()) * 0.5;

    res.push_back (geo::Edge (geo::Point (center - dl), geo::Point (center + dl)));

  }
}

// -------------------------------------------------------------------------------------------------------------
//  EdgeAngleChecker implementation

EdgeAngleChecker::EdgeAngleChecker (double angle_start, bool include_angle_start, double angle_end, bool include_angle_end)
{
  m_t_start = geo::CplxTrans(1.0, angle_start, false, geo::DVector ());
  m_t_end = geo::CplxTrans(1.0, angle_end, false, geo::DVector ());

  m_include_start = include_angle_start;
  m_include_end = include_angle_end;

  m_big_angle = (angle_end - angle_start + geo::epsilon) > 180.0;
  m_all = (angle_end - angle_start - geo::epsilon) > 360.0;
}

bool
EdgeAngleChecker::check (const geo::Vector &a, const geo::Vector &b) const
{
  geo::DVector vout (b);

  geo::DVector v1 = m_t_start * a;
  geo::DVector v2 = m_t_end * a;

  int vps1 = geo::vprod_sign (v1, vout);
  int vps2 = geo::vprod_sign (v2, vout);
  bool opp1 = vps1 == 0 && (geo::sprod_sign (v1, vout) < 0);
  bool opp2 = vps2 == 0 && (geo::sprod_sign (v2, vout) < 0);

  bool vp1 = !opp1 && (m_include_start ? (geo::vprod_sign (v1, vout) >= 0) : (geo::vprod_sign (v1, vout) > 0));
  bool vp2 = !opp2 && (m_include_end ? (geo::vprod_sign (v2, vout) <= 0) : (geo::vprod_sign (v2, vout) < 0));

  if (m_big_angle && (vp1 || vp2)) {
    return true;
  } else if (! m_big_angle && vp1 && vp2) {
    return true;
  } else {
    return false;
  }
}

// -------------------------------------------------------------------------------------------------------------
//  EdgeOrientationFilter implementation

EdgeOrientationFilter::EdgeOrientationFilter (double amin, bool include_amin, double amax, bool include_amax, bool inverse)
  : m_inverse (inverse), m_checker (amin, include_amin, amax, include_amax)
{
  //  .. nothing yet ..
}

EdgeOrientationFilter::EdgeOrientationFilter (double a, bool inverse)
  : m_inverse (inverse), m_checker (a, true, a, true)
{
  //  .. nothing yet ..
}

bool
EdgeOrientationFilter::selected (const geo::Edge &edge) const
{
  //  NOTE: this edge normalization confines the angle to a range between (-90 .. 90] (-90 excluded).
  //  A horizontal edge has 0 degree, a vertical one has 90 degree.
  if (edge.dx () < 0 || (edge.dx () == 0 && edge.dy () < 0)) {
    return m_checker (geo::Vector (edge.ortho_length (), 0), -edge.d ()) != m_inverse;
  } else {
    return m_checker (geo::Vector (edge.ortho_length (), 0), edge.d ()) != m_inverse;
  }
}

}
