#include "geoCommon.h"

#include "geoEdgePairRelations.h"

#include <algorithm>
#include <cmath>

namespace geo
{

/**
 *  @brief Determine the projected length of b on a
 */
geo::Edge::distance_type edge_projection (const geo::Edge &a, const geo::Edge &b)
{
  if (a.is_degenerate () || b.is_degenerate ()) {
    return 0;
  }

  double al = a.double_sq_length ();
  double l1 = (geo::sprod (geo::Vector (b.p1 () - a.p1 ()), a.d ())) / al;
  double l2 = (geo::sprod (geo::Vector (b.p2 () - a.p1 ()), a.d ())) / al;
  l1 = std::min (1.0, std::max (0.0, l1));
  l2 = std::min (1.0, std::max (0.0, l2));
  return geo::coord_traits<geo::Coord>::rounded (a.double_length () * fabs (l2 - l1));
}

/**
 *  @brief Returns the part of the "other" edge which is on the inside side of e and within distance d
 *
 *  This function applies Euclidian metrics.
 *  If no such part is found, this function returns false.
 */
bool euclidian_near_part_of_edge (bool include_zero, geo::coord_traits<geo::Coord>::distance_type d, const geo::Edge &e, const geo::Edge &other, geo::Edge *output)
{
  //  Handle the case of point-like basic edge: cannot determine
  //  orientation

  if (e.is_degenerate ()) {
    return false;
  }

  geo::Edge g (other);
  int s1 = e.side_of (g.p1 ());
  int s2 = e.side_of (g.p2 ());

  //  "kissing corner" issue: force include zero if the edges are collinear and overlap.
  if (! include_zero && s1 == 0 && s2 == 0 && e.intersect (g)) {
    include_zero = true;
  }

  int thr = include_zero ? 0 : -1;

  //  keep only part of other which is on the "inside" side of e
  if (s1 > thr && s2 > thr) {
    return false; 
  } else if (s2 > thr) {
    g = geo::Edge (g.p1 (), g.cut_point (e).second);
  } else if (s1 > thr) {
    g = geo::Edge (g.cut_point (e).second, g.p2 ());
  }

  //  Handle the case of point vs. edge

  if (g.is_degenerate ()) {

    geo::Point o = g.p1 ();

    if (e.side_of (o) >= 0) {
      return false;
    }

    double a = e.double_sq_length ();
    double b = geo::sprod (geo::Vector (e.p1 () - o), e.d ());
    double c = e.p1 ().sq_double_distance (o) - d * d;

    double s = b * b - a * c;
    if (s >= 0) {
      double l1 = std::max (0.0, (-b - sqrt (s)) / a);
      double l2 = std::min (1.0, (-b + sqrt (s)) / a);
      if (l1 <= l2) {
        if (output) {
          *output = g;
        }
        return true;
      }
    }

    return false;

  }

  //  Determine body interactions (projected mode)

  double l1 = std::numeric_limits<double>::max (), l2 = -std::numeric_limits<double>::max ();

  //  handle the parallel case
  if (e.parallel (g)) {
    if (std::abs (double (e.distance (g.p1 ()))) >= double (d)) {
      return false;
    }
  } else {

    double ef = 1.0 / e.double_length ();
    geo::DVector en = geo::DVector (ef * e.dy (), -ef * e.dx ());
    geo::DPoint e1d = geo::DPoint (e.p1 ()) + en * double (d);

    double det = geo::vprod (geo::DVector (g.d ()), geo::DVector (e.d ()));
    double lp1 = geo::vprod (geo::DVector (e1d - geo::DPoint (g.p1 ())), geo::DVector (e.d ())) / det;
    double lp2 = geo::vprod (geo::DVector (e.p1 () - g.p1 ()), geo::DVector (e.d ())) / det;
    if (lp1 > lp2) {
      std::swap (lp1, lp2);
    }

    if (geo::sprod_sign (e, g) == 0) {
      if (g.side_of (e.p1 ()) * g.side_of (e.p2 ()) <= 0) {
        l1 = lp1;
        l2 = lp2;
      }
    } else {

      double det = geo::vprod (geo::DVector (g.d ()), en);
      double lt1 = geo::vprod (geo::DVector (e.p1 () - g.p1 ()), en) / det;
      double lt2 = geo::vprod (geo::DVector (e.p2 () - g.p1 ()), en) / det;
      if (lt1 > lt2) {
        std::swap (lt1, lt2);
      }

      double ll1 = std::max(lp1, lt1);
      double ll2 = std::min(lp2, lt2);
      if (ll1 <= ll2) {
        l1 = ll1;
        l2 = ll2;
      }

    }

  }

  //  Compute a solution for the circles and the ends if there is one

  for (int i = 0; i < 2; ++i) {

    geo::Point o = i ? e.p2 () : e.p1 ();

    double a = g.double_sq_length ();
    double b = geo::sprod (geo::Vector (g.p1 () - o), g.d ());
    double c = g.p1 ().sq_double_distance (o) - double (d) * double (d);

    double s = b * b - a * c;
    if (s >= 0) {
      l1 = std::min (l1, (-b - sqrt (s)) / a);
      l2 = std::max (l2, (-b + sqrt (s)) / a);
    }

  }

  l1 = std::max (0.0, l1);
  l2 = std::min (1.0, l2);

  if (l1 >= l2) {
    return false;
  } else {
    if (output) {
      *output = geo::Edge (g.p1 () + geo::Vector (g.d () * l1), g.p1 () + geo::Vector (g.d () * l2));
    }
    return true;
  }
}

/**
 *  @brief Returns the part of the "other" edge which is on the inside side of e and within distance d
 *
 *  This function applies Square metrics.
 *  If no such part is found, this function returns false.
 */
static bool var_near_part_of_edge (bool include_zero, geo::coord_traits<geo::Coord>::distance_type d, geo::coord_traits<geo::Coord>::distance_type dd, const geo::Edge &e, const geo::Edge &other, geo::Edge *output)
{
  //  Handle the case of point-like basic edge: cannot determine
  //  orientation

  if (e.is_degenerate ()) {
    return false;
  }

  geo::Edge g (other);
  int s1 = e.side_of (g.p1 ());
  int s2 = e.side_of (g.p2 ());

  //  "kissing corner" issue: force include zero if the edges are collinear and overlap
  if (! include_zero && s1 == 0 && s2 == 0 && e.intersect (g)) {
    include_zero = true;
  }

  int thr = include_zero ? 0 : -1;

  //  keep only part of other which is on the "inside" side of e
  if (s1 > thr && s2 > thr) {
    return false; 
  } else if (s2 > thr) {
    g = geo::Edge (g.p1 (), g.cut_point (e).second);
  } else if (s1 > thr) {
    g = geo::Edge (g.cut_point (e).second, g.p2 ());
  }

  //  Handle the case of point vs. edge

  if (g.is_degenerate ()) {
    double gd = double (e.distance (g.p1 ()));
    if (gd <= -double (d) || gd >= 0) {
      return false;
    }
    if (geo::sprod (geo::Vector (g.p1 () - e.p1 ()), e.d ()) < -(dd * e.double_length ())) {
      return false;
    }
    if (geo::sprod (geo::Vector (e.p2 () - g.p1 ()), e.d ()) < -(dd * e.double_length ())) {
      return false;
    }
    if (output) {
      *output = g;
    }
    return true;
  }

  //  Determine body interactions (projected mode)

  double l1 = std::numeric_limits<double>::min (), l2 = std::numeric_limits<double>::max ();

  double ef = 1.0 / e.double_length ();
  geo::DVector ep = geo::DVector (ef * e.dx (), ef * e.dy ());
  geo::DVector en = geo::DVector (ef * e.dy (), -ef * e.dx ());

  //  handle the parallel case
  if (e.parallel (g)) {
    if (std::abs (double (e.distance (g.p1 ()))) >= double (d)) {
      return false;
    }
  } else {

    geo::DPoint e1d = geo::DPoint (e.p1 ()) + en * double (d);

    double det = geo::vprod (geo::DVector (g.d ()), geo::DVector (e.d ()));
    double lp1 = geo::vprod (geo::DVector (e1d - geo::DPoint (g.p1 ())), geo::DVector (e.d ())) / det;
    double lp2 = geo::vprod (geo::DVector (e.p1 () - g.p1 ()), geo::DVector (e.d ())) / det;
    if (lp1 > lp2) {
      std::swap (lp1, lp2);
    }

    l1 = lp1;
    l2 = lp2;

  }

  if (geo::sprod_sign (e, g) == 0) {
    if (geo::sprod (geo::Vector (g.p1 () - e.p1 ()), e.d ()) < -(dd * e.double_length ()) ||
        geo::sprod (geo::Vector (e.p2 () - g.p1 ()), e.d ()) < -(dd * e.double_length ())) {
      return false;
    }
  } else {

    double det = geo::vprod (geo::DVector (g.d ()), en);
    double lt1 = geo::vprod (geo::DVector (e.p1 () - g.p1 ()) - ep * double (dd), en) / det;
    double lt2 = geo::vprod (geo::DVector (e.p2 () - g.p1 ()) + ep * double (dd), en) / det;
    if (lt1 > lt2) {
      std::swap (lt1, lt2);
    }

    l1 = std::max(l1, lt1);
    l2 = std::min(l2, lt2);

  }

  //  Return the solution if one is found

  l1 = std::max (0.0, l1);
  l2 = std::min (1.0, l2);

  if (l1 >= l2) {
    return false;
  } else {
    if (output) {
      *output = geo::Edge (g.p1 () + geo::Vector (g.d () * l1), g.p1 () + geo::Vector (g.d () * l2));
    }
    return true;
  }
}

/**
 *  @brief Returns the part of the "other" edge which is on the inside side of e and within distance d
 *
 *  This function applies Projected metrics.
 *  If no such part is found, this function returns false.
 */
bool projected_near_part_of_edge (bool include_zero, geo::coord_traits<geo::Coord>::distance_type d, const geo::Edge &e, const geo::Edge &other, geo::Edge *output)
{
  return var_near_part_of_edge (include_zero, d, 0, e, other, output);
}

/**
 *  @brief Returns the part of the "other" edge which is on the inside side of e and within distance d
 *
 *  This function applies Square metrics.
 *  If no such part is found, this function returns false.
 */
bool square_near_part_of_edge (bool include_zero, geo::coord_traits<geo::Coord>::distance_type d, const geo::Edge &e, const geo::Edge &other, geo::Edge *output)
{
  return var_near_part_of_edge (include_zero, d, d, e, other, output);
}

// ---------------------------------------------------------------------------------
//  Implementation of EdgeRelationFilter

// -----------------------------------------------------------------------------
//  EdgePairRelations implementation

EdgeRelationFilter::EdgeRelationFilter (edge_relation_type r, distance_type d, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection)
  : m_whole_edges (false), m_include_zero (true), m_r (r), m_d (d), m_metrics (metrics), m_ignore_angle (0), m_min_projection (min_projection), m_max_projection (max_projection)
{
  set_ignore_angle (ignore_angle);
}

void
EdgeRelationFilter::set_ignore_angle (double a)
{
  m_ignore_angle = a;
  m_ignore_angle_cos = cos (m_ignore_angle * M_PI / 180.0);
}

bool 
EdgeRelationFilter::check (const geo::Edge &a, const geo::Edge &b, geo::EdgePair *output) const
{
  //  check projection criterion

  if (m_min_projection > 0 || m_max_projection < std::numeric_limits<distance_type>::max ()) {

    distance_type p = edge_projection (a, b);
    if (! (p >= m_min_projection && p < m_max_projection)) {
      p = edge_projection (b, a);
      if (! (p >= m_min_projection && p < m_max_projection)) {
        return false;
      }
    }

  }

  //  Check angle relation

  geo::Edge aa (a);
  if (m_r == OverlapRelation || m_r == InsideRelation) {
    aa.swap_points ();
  }

  //  Check whether the edges have an angle less than the ignore_angle parameter

  if (m_ignore_angle == 90.0) {
    if (geo::sprod_sign (aa, b) >= 0) {
      return false;
    }
  } else {
    if (-geo::sprod (aa, b) / (aa.double_length () * b.double_length ()) < m_ignore_angle_cos + 1e-10) {
      return false;
    }
  }

  //  Determine part of edges with correct arrangement

  //  normalize edges to "width" interpretation

  geo::Edge an (a), bn (b);
  if (m_r == SpaceRelation || m_r == InsideRelation) {
    an.swap_points ();
  }
  if (m_r == SpaceRelation || m_r == OverlapRelation) {
    bn.swap_points ();
  }

  //  Determine the interacting edge parts

  bool in1, in2;

  if (m_metrics == Euclidian) {
    in2 = euclidian_near_part_of_edge (m_include_zero, m_d, an, bn, ! m_whole_edges && output ? &output->second () : 0);
    in1 = euclidian_near_part_of_edge (m_include_zero, m_d, bn, an, ! m_whole_edges && output ? &output->first () : 0);
  } else if (m_metrics == Square) {
    in2 = square_near_part_of_edge (m_include_zero, m_d, an, bn, ! m_whole_edges && output ? &output->second () : 0);
    in1 = square_near_part_of_edge (m_include_zero, m_d, bn, an, ! m_whole_edges && output ? &output->first () : 0);
  } else {
    in2 = projected_near_part_of_edge (m_include_zero, m_d, an, bn, ! m_whole_edges && output ? &output->second () : 0);
    in1 = projected_near_part_of_edge (m_include_zero, m_d, bn, an, ! m_whole_edges && output ? &output->first () : 0);
  }

  if (in1 && in2) {

    if (output) {

      //  return whole edges if instructed to do so
      if (m_whole_edges) {
        output->set_first (a);
        output->set_second (b);
      } else {
        //  correct the edge orientation back to their initial one
        if (m_r == SpaceRelation || m_r == InsideRelation) {
          output->first ().swap_points ();
        }
        if (m_r == SpaceRelation || m_r == OverlapRelation) {
          output->second ().swap_points ();
        }
      }

    }

    return true;

  } else {
    return false;
  }
}

}

