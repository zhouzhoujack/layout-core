#include "geoPath.h"

namespace geo
{
// -----------------------------------------------------------------------------
//  Path implementation


template <class C> 
typename path<C>::distance_type 
path<C>::length () const 
{
  double l = m_bgn_ext + m_end_ext;

  typename pointlist_type::const_iterator p = m_points.begin (); 
  if (p != m_points.end ()) {
    typename pointlist_type::const_iterator pp = p;
    ++p;
    while (p != m_points.end ()) {
      l += pp->double_distance (*p);
      ++p;
      ++pp;
    }
  }

  return coord_traits::rounded (l);
}

template <class C> 
typename path<C>::perimeter_type 
path<C>::perimeter () const 
{
  double l;
  if (m_width < 0) {
    //  correction for round path ends. Uses the average radius approximation for the ellipse ends.
    l = M_PI * 0.5 * (sqrt (m_bgn_ext * m_bgn_ext / 2.0 + m_width * m_width / 8.0) + sqrt (m_end_ext * m_end_ext / 2.0 + m_width * m_width / 8.0));
  } else {
    l = m_bgn_ext + m_end_ext + m_width;
  }

  typename pointlist_type::const_iterator p = m_points.begin (); 
  if (p != m_points.end ()) {
    typename pointlist_type::const_iterator pp = p;
    ++p;
    while (p != m_points.end ()) {
      l += pp->double_distance (*p);
      ++p;
      ++pp;
    }
  }

  return coord_traits::rounded_perimeter (2 * l);
}

template <class C> 
typename path<C>::area_type 
path<C>::area () const 
{
  area_type l = m_bgn_ext + m_end_ext;
  if (m_width < 0) {
    //  correction for round path ends
    l = area_type (l * (4.0 - M_PI));
  }

  typename pointlist_type::const_iterator p = m_points.begin (); 
  if (p != m_points.end ()) {
    typename pointlist_type::const_iterator pp = p;
    ++p;
    while (p != m_points.end ()) {
      l += area_type (pp->double_distance (*p));
      ++p;
      ++pp;
    }
  }

  return l * std::abs (m_width);
}

template <class C> 
std::string
path<C>::to_string () const
{
  std::string s = "(";

  //  the point list 
  for (iterator p = begin (); p != end (); ++p) {
    if (p != begin ()) {
      s += ";";
    }
    s += (*p).to_string ();
  }

  s += ") w=";
  s += geo::to_string (width ());
  s += " bx=";
  s += geo::to_string (m_bgn_ext);
  s += " ex=";
  s += geo::to_string (m_end_ext);
  s += " r=";
  s += geo::to_string (round ());
  return s;
}

template <class C> 
void path<C>::update_bbox () const
{
  if (m_bbox.empty () && ! m_points.empty ()) {
#if 0
    //  simple version
    for (typename pointlist_type::const_iterator p = m_points.begin (); p != m_points.end (); ++p) {
      m_bbox += *p;
    }
    coord_type frame = std::max (m_width, m_width / 2 + std::max (m_bgn_ext, m_end_ext));
    m_bbox = m_bbox.enlarged (point_type (frame, frame));
#else
    //  complex version
    pointlist_type tmp_points;
    real_points (tmp_points);
    create_shifted_points (m_bgn_ext, m_end_ext, width (), true, tmp_points.begin (), tmp_points.end (), 2, geo::box_inserter<box_type> (m_bbox));
    create_shifted_points (m_end_ext, m_bgn_ext, width (), false, tmp_points.rbegin (), tmp_points.rend (), 2, geo::box_inserter<box_type> (m_bbox));
#endif
  }
}

template <class C> 
void path<C>::real_points (typename path<C>::pointlist_type &real_pts) const
{
  real_pts.reserve (m_points.size ());
  for (typename pointlist_type::const_iterator p = m_points.begin (); p != m_points.end (); ) {

    real_pts.push_back (*p);
    ++p;

    //  search for the next point not coincident with the first one
    while (p != m_points.end () && *p == real_pts.back ()) {
      ++p;
    }

    //  if there are further points, test if there is not a edge
    //  following that is collinear with the current one.
    if (p != m_points.end ()) {

      typename pointlist_type::const_iterator pn = p;
      while (++p != m_points.end ()) {
        while (p != m_points.end () && *p == *pn) {
          ++p;
        }
        if (p == m_points.end () || ! edge<C> (real_pts.back (), *p).contains (*pn)) {
          break;
        }
        pn = p;
      }

      --p;

    }

  }
}

template <class C>
inline geo::DVector dpx (const geo::DVector &p, double d);

template <>
inline geo::DVector dpx<Coord> (const geo::DVector &p, double d)
{
  if (fabs (p.x ()) < geo::epsilon || fabs (p.y ()) < geo::epsilon) {
    return p * double (coord_traits<Coord>::rounded (d));
  } else if (fabs (fabs (p.x ()) - fabs (p.y ())) < geo::epsilon) {
    //  45 degree case: try to round d such that if p is on the grid it will be later
    return p * (M_SQRT2 * coord_traits<Coord>::rounded (d * M_SQRT1_2));
  } else {
    return p * d;
  }
}

template <>
inline geo::DVector dpx<DCoord> (const geo::DVector &p, double d)
{
  //  No need to round in the double case
  return p * d;
}

template <class C>
template <class Iter, class Inserter>
void path<C>::create_shifted_points (C start, C end, C width, bool forward, Iter from, Iter to, int ncircle, Inserter pts) const
{
  //  for safety reasons
  if (from == to) {
    return;
  }

  double disp = double (width) * 0.5;

  Iter p = from;
  Iter pp = p;
  ++pp;

  if (pp == to) {

    //  Special case of degenerated path with one point: treat as infinitely small segment with direction (1,0)
    geo::DVector ed (forward ? 1.0 : -1.0, 0.0);
    geo::DVector nd (-ed.y (), ed.x ());
    geo::DVector edd, ndd;

    //  The first point is taken as being simply shifted normally and pulled back by start_ext
    //  or, in round mode, approximated by a set of segments 
    if (ncircle > 2) {

      double a0 = M_PI / (2.0 * ncircle);

      double cd = cos(a0);
      double sd = sin(a0);
      double c2d = cd * cd - sd * sd;
      double s2d = 2 * cd * sd;
      double c, s;

      edd = ed * (double(-start) / cd);
      ndd = nd * (disp / cd);

      c = cd;
      s = sd;

      for (int i = 0; i < ncircle / 2; ++i) {
        *pts++ = *p + vector<C> (edd * c + ndd * s);
        double cc = c * c2d - s * s2d;
        double ss = s * c2d + c * s2d;
        c = cc;
        s = ss;
      }

      edd = ed * (double(end) / cd);
      ndd = nd * (disp / cd);

      c = cos(a0 * (ncircle - 1));
      s = sin(a0 * (ncircle - 1));

      for (int i = 0; i < ncircle / 2; ++i) {
        *pts++ = *p + vector<C> (edd * c + ndd * s);
        double cc = c * c2d + s * s2d;
        double ss = s * c2d - c * s2d;
        c = cc;
        s = ss;
      }

    } else {
      *pts++ = (*p + vector<C> (ed * double(-start) + nd * disp));
      *pts++ = (*p + vector<C> (ed * double(end) + nd * disp));
    }

  }

  bool first = true;

  while (pp != to) {

    Iter ppp = pp;
    ++ppp;

    //  Compute the unit vector of the line and it's normal (times width)

    geo::DVector ed (*pp - *p);
    ed *= 1.0 / ed.double_length ();
    geo::DVector nd (-ed.y (), ed.x ());
    nd = dpx<C> (nd, disp);

    if (first) {

      first = false;

      //  The first point is taken as being simply shifted normally and pulled back by start_ext
      //  or, in round mode, approximated by a set of segments 
      if (ncircle > 2) {

        double a0 = M_PI / (2.0 * ncircle);

        double cd = cos(a0);
        double sd = sin(a0);
        double c2d = cd * cd - sd * sd;
        double s2d = 2 * cd * sd;
        double c = cd, s = sd;

        geo::DVector edd = ed * (double(-start) / cd);
        geo::DVector ndd = nd * (1.0 / cd);

        for (int i = 0; i < ncircle / 2; ++i) {
          *pts++ = *p + vector<C> (edd * c + ndd * s);
          double cc = c * c2d - s * s2d;
          double ss = s * c2d + c * s2d;
          c = cc;
          s = ss;
        }

      } else {
        *pts++ = (*p + vector<C> (dpx<C> (ed, -start) + nd));
      }

    }

    if (ppp == to) {

      //  The last point is taken as being simply shifted normally and pulled forward by end_ext
      //  or, in round mode, approximated by a set of segments 
      if (ncircle > 2) {

        double a0 = M_PI / (2.0 * ncircle);

        double cd = cos(a0);
        double sd = sin(a0);
        double c2d = cd * cd - sd * sd;
        double s2d = 2 * cd * sd;
        double c = cos(a0 * (ncircle - 1));
        double s = sin(a0 * (ncircle - 1));

        geo::DVector edd = ed * (double(end) / cd);
        geo::DVector ndd = nd * (1.0 / cd);

        for (int i = 0; i < ncircle / 2; ++i) {
          *pts++ = *pp + vector<C> (edd * c + ndd * s);
          double cc = c * c2d + s * s2d;
          double ss = s * c2d - c * s2d;
          c = cc;
          s = ss;
        }

      } else {
        *pts++ = (*pp + vector<C> (dpx<C> (ed, end) + nd));
      }

    } else {

      //  Points in between are determined from taking two 
      //  edges being shifted perpendicular from the original
      //  and being slightly extended. The intersection point
      //  of both gives the new vertex. If there is no intersection,
      //  the edges are simply connected.

      geo::DVector eed (*ppp - *pp);
      eed *= 1.0 / eed.double_length ();
      geo::DVector nnd (-eed.y (), eed.x ());
      nnd = dpx<C> (nnd, disp);

      double l1max = nd.double_length ();
      double l2max = nnd.double_length ();

      double l1min = -p->double_distance (*pp) - nd.double_length ();
      double l2min = -ppp->double_distance (*pp) - nnd.double_length ();

      double dv = geo::vprod (ed, eed);
      if (fabs (dv) > geo::epsilon) {

        double l1 = geo::vprod (nnd - nd, eed) / dv;
        double l2 = geo::vprod (nd - nnd, ed) / dv;

        if ((l1 < -geo::epsilon) != (l2 < -geo::epsilon)) {

          //  No well-formed intersection (reflecting edge) ->
          //  create a direct connection
          *pts++ = *pp + vector<C> (nd);
          *pts++ = *pp + vector<C> (nnd);

        } else if (l1 < l1min - geo::epsilon || l2 < l2min - geo::epsilon) {

          //  Segments are too short - the won't intersect: In this case we create a loop of three 
          //  points which define the area in self-overlapping way but confined to the path within
          //  the limits of it's width. 
          //  HINT: the execution of this code is a pretty strong evidence for the existence to loops 
          //  in the contour delivered. A proof however is missing ..
          *pts++ = *pp + vector<C> (nd);
          *pts++ = *pp;
          *pts++ = *pp + vector<C> (nnd);

        } else if (l1 < l1max + geo::epsilon && l2 < l2max + geo::epsilon) {

          //  well-formed corner
          *pts++ = *pp + vector<C> (nd + ed * l1);

        } else {

          //  cut-off corner: produce two points connecting the edges 
          *pts++ = *pp + vector<C> (nd + ed * std::min (l1max, l1));
          *pts++ = *pp + vector<C> (nnd - eed * std::min (l2max, l2));

        }

        p = pp;

      } else if (geo::sprod (ed, eed) < -geo::epsilon) {

        //  reflecting segment
        *pts++ = *pp + vector<C> (nd + dpx<C> (ed, disp));
        *pts++ = *pp + vector<C> (nnd - dpx<C> (eed, disp));

        p = pp;

      }

    }

    pp = ppp;

  }
  
}

DPath
round_path_corners (const geo::DPath &input, double r, int n, double accuracy)
{
  std::vector<geo::DPoint> path_points;

  //  collect the path's points and remove collinear points
  {
    geo::DPath::iterator p = input.begin ();
    if (p != input.end ()) {

      path_points.push_back (*p);

      geo::DPath::iterator pp = p;
      ++pp;
      if (pp != input.end ()) {

        geo::DPath::iterator ppp = pp;
        ++ppp;
        while (ppp != input.end ()) {

          if (geo::vprod_sign (*p - *ppp, *pp - *ppp) != 0) {
            path_points.push_back (*pp);
          }

          ++p;
          ++pp;
          ++ppp;

        }

        if (path_points.back ().distance (*pp) > accuracy) {
          path_points.push_back (*pp);
        }

      }

    }

  }

  std::vector<geo::DPoint> new_points;

  std::vector<geo::DPoint>::const_iterator p = path_points.begin ();
  if (p != path_points.end ()) {

    new_points.push_back (*p);
    std::vector<geo::DPoint>::const_iterator pp = p;
    ++pp;

    if (pp != path_points.end ()) {

      std::vector<geo::DPoint>::const_iterator ppp = pp;
      ++ppp;
      while (ppp != path_points.end ()) {

        geo::DVector s1 = *p - *pp;
        geo::DVector s2 = *ppp - *pp;
        s1 *= 1.0 / s1.length ();
        s2 *= 1.0 / s2.length ();

        double a = atan2 (geo::vprod (s1, s2), geo::sprod (s1, s2));

        double cota2 = sin (a * 0.5) / cos (a * 0.5);
        double d1 = (p == path_points.begin () ? 1.0 : 0.5) * (*p).distance (*pp);
        double d2 = (ppp + 1 == path_points.end () ? 1.0 : 0.5) * (*ppp).distance (*pp);
        double rmin = std::min (d1, d2) * cota2;
        double ract = rmin;
        if (fabs (r) < fabs (ract)) {
          ract = (ract < 0) ? -fabs (r) : fabs (r);
        }

        if (fabs (ract) < accuracy) {

          if (new_points.back ().distance (*pp) > accuracy) {
            new_points.push_back (*pp);
          }

        } else {

          double rs = ract / cota2;

          double xc = (*pp).x () + rs * s1.x () - ract * s1.y ();
          double yc = (*pp).y () + rs * s1.y () + ract * s1.x ();

          double ac = M_PI - fabs (a);
          int npts = std::max (2, (int) floor (0.5 + n * ac / (2.0 * M_PI)));

          double x, y;
          geo::DPoint dp;

          x = xc + ract * s1.y ();
          y = yc - ract * s1.x ();

          dp = geo::DPoint (x, y);
          if (new_points.back ().distance (dp) > accuracy) {
            new_points.push_back (dp);
          }

          //  Note: the choice of the actual radius and the angle steps
          //  is supposed to create a centerline with roughly the
          //  same length that the ideal line and end segments which are
          //  in the direction of the tangent, so they merge smoothly with
          //  adjacent line segments.
          double f0 = 1.0 / 3.0;
          double nn = npts - 1.0 + 2.0 * f0;
          double ract_outer = ract / cos (f0 * ac / nn);

          for (int i = 0; i < npts; ++i) {

            double aa = (ac * (i + f0)) / nn;

            x = xc + ract_outer * s1.y () * cos (aa) - fabs (ract_outer) * s1.x () * sin (aa);
            y = yc - ract_outer * s1.x () * cos (aa) - fabs (ract_outer) * s1.y () * sin (aa);

            dp = geo::DPoint (x, y);
            if (new_points.back ().distance (dp) > accuracy) {
              new_points.push_back (dp);
            }

          }

          x = xc + ract * s1.y () * cos (ac) - fabs (ract) * s1.x () * sin (ac);
          y = yc - ract * s1.x () * cos (ac) - fabs (ract) * s1.y () * sin (ac);

          dp = geo::DPoint (x, y);
          if (new_points.back ().distance (dp) > accuracy) {
            new_points.push_back (dp);
          }

        }

        ++p;
        ++pp;
        ++ppp;

      }

      if (new_points.back ().distance (*pp) > accuracy) {
        new_points.push_back (*pp);
      }

    }

  }

  //  Create a new path (use double for accuracy)
  geo::DPath new_path (input);
  new_path.assign (new_points.begin (), new_points.end ());
  return new_path;
}

Path round_path_corners (const Path &path, int rad, int n)
{
  return Path (round_path_corners (geo::DPath (path), double (rad), n, 0.5));
}

template class path<Coord>;
template class path<DCoord>;

//  explicit instantiations
template void path<Coord>::create_shifted_points (Coord, Coord, Coord, bool, path<Coord>::pointlist_type::iterator, path<Coord>::pointlist_type::iterator, int, box_inserter<path<Coord>::box_type>) const;
template void path<Coord>::create_shifted_points (Coord, Coord, Coord, bool, path<Coord>::pointlist_type::iterator, path<Coord>::pointlist_type::iterator, int, std::back_insert_iterator<geo::uvector<path<Coord>::point_type> >) const;
template void path<Coord>::create_shifted_points (Coord, Coord, Coord, bool, path<Coord>::pointlist_type::iterator, path<Coord>::pointlist_type::iterator, int, std::back_insert_iterator<std::vector<path<Coord>::point_type> >) const;
template void path<Coord>::create_shifted_points (Coord, Coord, Coord, bool, path<Coord>::pointlist_type::reverse_iterator, path<Coord>::pointlist_type::reverse_iterator, int, std::back_insert_iterator<geo::uvector<path<Coord>::point_type> >) const;
template void path<Coord>::create_shifted_points (Coord, Coord, Coord, bool, path<Coord>::pointlist_type::reverse_iterator, path<Coord>::pointlist_type::reverse_iterator, int, std::back_insert_iterator<std::vector<path<Coord>::point_type> >) const;

template void path<DCoord>::create_shifted_points (DCoord, DCoord, DCoord, bool, path<DCoord>::pointlist_type::iterator, path<DCoord>::pointlist_type::iterator, int, box_inserter<path<DCoord>::box_type>) const;
template void path<DCoord>::create_shifted_points (DCoord, DCoord, DCoord, bool, path<DCoord>::pointlist_type::iterator, path<DCoord>::pointlist_type::iterator, int, std::back_insert_iterator<geo::uvector<path<DCoord>::point_type> >) const;
template void path<DCoord>::create_shifted_points (DCoord, DCoord, DCoord, bool, path<DCoord>::pointlist_type::iterator, path<DCoord>::pointlist_type::iterator, int, std::back_insert_iterator<std::vector<path<DCoord>::point_type> >) const;
template void path<DCoord>::create_shifted_points (DCoord, DCoord, DCoord, bool, path<DCoord>::pointlist_type::reverse_iterator, path<DCoord>::pointlist_type::reverse_iterator, int, std::back_insert_iterator<geo::uvector<path<DCoord>::point_type> >) const;
template void path<DCoord>::create_shifted_points (DCoord, DCoord, DCoord, bool, path<DCoord>::pointlist_type::reverse_iterator, path<DCoord>::pointlist_type::reverse_iterator, int, std::back_insert_iterator<std::vector<path<DCoord>::point_type> >) const;

}

namespace geo
{

template<> void extractor_impl (geo::Extractor &ex, geo::Path &p)
{
  if (! test_extractor_impl (ex, p)) {
    ex.error (geo::to_string (tr ("Expected a path specification")));
  }
}

template<> void extractor_impl (geo::Extractor &ex, geo::DPath &p)
{
  if (! test_extractor_impl (ex, p)) {
    ex.error (geo::to_string (tr ("Expected a path specification")));
  }
}


template<class C> bool _test_extractor_impl (geo::Extractor &ex, geo::path<C> &p)
{
  typedef geo::point<C> point_type;
  std::vector <point_type> points;

  if (ex.test ("(")) {

    point_type pt;
    while (ex.try_read (pt)) {
      points.push_back (pt);
      ex.test (";");
    }

    p.assign (points.begin (), points.end ());

    ex.expect (")");

    if (ex.test ("w=")) {
      C w = 0;
      ex.read (w);
      p.width (w);
    } 

    if (ex.test ("bx=")) {
      C e = 0;
      ex.read (e);
      p.extensions (e, p.extensions ().second);
    } 

    if (ex.test ("ex=")) {
      C e = 0;
      ex.read (e);
      p.extensions (p.extensions ().first, e);
    } 

    if (ex.test ("r=")) {
      bool r = false;
      ex.read (r);
      p.round (r);
    } 

    return true;

  } else {
    return false;
  }
}

template<> bool test_extractor_impl (geo::Extractor &ex, geo::Path &p)
{
  return _test_extractor_impl (ex, p);
}

template<> bool test_extractor_impl (geo::Extractor &ex, geo::DPath &p)
{
  return _test_extractor_impl (ex, p);
}

}

