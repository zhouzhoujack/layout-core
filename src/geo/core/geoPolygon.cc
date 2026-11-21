#include "geoPolygon.h"

namespace geo
{
// -----------------------------------------------------------------------------
//  Polygon implementation


template <class C>
static
geo::DEdge compute_shifted (const geo::edge<C> &e, C dx, C dy, double ext, int nsign)
{
  tl_assert (! e.is_degenerate ()); // no coincident points allowed

  //  Compute the unit vector of the line and it's normal (times width)
  geo::DVector ec (e.d ());
  ec *= 1.0 / ec.double_length ();
  geo::DVector nc (-ec.y (), ec.x ());

  ec *= sqrt (ec.x () * ec.x () * dx * dx + ec.y () * ec.y () * dy * dy) * ext;
  nc *= sqrt (nc.x () * nc.x () * dx * dx + nc.y () * nc.y () * dy * dy) * nsign;

  //  We create two test lines for the adjacent edges that extend somewhat further (i.e by half
  //  the width). These test lines define the limits of the area where the segments are responsible
  return geo::DEdge (geo::DPoint (e.p1 ()) + nc - ec, geo::DPoint (e.p2 ()) + nc + ec);
}

/**
 *  @brief Smart multiplication of a vector with a distance
 *  This function tries to keep the length of the vector on grid if it's 
 *  a 45 degree or horizontal/vertical one.
 */
template <class C>
inline geo::DVector dpx (const geo::DVector &p, double d);

template <>
inline geo::DVector dpx<Coord> (const geo::DVector &p, double d)
{
  //  Note: "up" mode is used for computing the extension. Some bigger value
  //  is helpful in avoiding the case of too short extensions causing 
  //  a missing intersection even in non-acute angle case
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
static void
compute_normals (const geo::vector<C> &d, C dx, C dy, int nsign, geo::DVector &ed, geo::DVector &nd)
{
  if (geo::coord_traits<C>::equal (dx, dy)) {

    //  Simplified handling for the isotropic case
    double f = d.double_length ();

    if (f < geo::coord_traits<DCoord>::prec_distance ()) {

      //  TODO: this should never happen (we assert before that d is not 0)
      ed = geo::DVector ();
      nd = geo::DVector ();

    } else {

      ed = geo::DVector (d) * (1.0 / f);
      nd = geo::DVector (-ed.y (), ed.x ());

      //  dpx is a smart multiplication trying to preserve 45 degree edges on grid
      nd = dpx<C> (nd, fabs (double (dx)) * nsign);

    }

  } else {

    double f = sqrt(double (dx) * double (dx) * double (d.y()) * double (d.y()) + double (dy) * double (dy) * double (d.x()) * double (d.x()));
    if (f < geo::coord_traits<DCoord>::prec_area ()) {

      if (dx == 0) {
        ed = geo::DVector (0.0, 1.0);
      } else if (dy == 0) {
        ed = geo::DVector (1.0, 0.0);
      } else {
        ed = geo::DVector ();
      }

      nd = geo::DVector();

    } else {

      ed = geo::DVector (d) * (double (dx) * double (dy) / f);

      nd = geo::DVector (double (-d.y ()) * double (dx) * double (dx), double (d.x ()) * double (dy) * double (dy));
      nd *= nsign / f;

    }

  }
}

/**
 *  @brief Provides a special DVector vprod sign for the purpose of representing integer-coordinate vectors
 *  The "zero" criterion is somewhat tighter than that of the normal integer value vectors.
 *  Hence, parallelity is somewhat more strict which makes the size function produce a
 *  better approximation to the desired target contour.
 */
static inline int
vprod_sign_for (const geo::DVector &a, const geo::DVector &b, const geo::Vector &)
{
  double vp = geo::vprod (a, b);
  if (vp <= -1e-2) {
    return -1;
  } else if (vp < 1e-2) {
    return 0;
  } else {
    return 1;
  }
}

/**
 *  @brief Fallback to the default vprod sign in the double-coordinate case
 */
static inline int
vprod_sign_for (const geo::DVector &a, const geo::DVector &b, const geo::DVector &)
{
  return geo::vprod_sign (a, b);
}

/**
 *  @brief Provides a special DVector sprod sign for the purpose of representing integer-coordinate vectors
 *  The "zero" criterion is somewhat tighter than that of the normal integer value vectors.
 *  Hence, orthogonality is somewhat more strict which makes the size function produce a
 *  better approximation to the desired target contour.
 */
static inline int
sprod_sign_for (const geo::DVector &a, const geo::DVector &b, const geo::Vector &)
{
  double sp = geo::sprod (a, b);
  if (sp <= -1e-2) {
    return -1;
  } else if (sp < 1e-2) {
    return 0;
  } else {
    return 1;
  }
}

/**
 *  @brief Fallback to the default sprod sign in the double-coordinate case
 */
static inline int
sprod_sign_for (const geo::DVector &a, const geo::DVector &b, const geo::DVector &)
{
  return geo::sprod_sign (a, b);
}

template <class C>
void polygon_contour<C>::size (C dx, C dy, unsigned int mode)
{
  if (dx == 0 && dy == 0) {
    return;
  }
  if (size () < 2) {
    return;
  }

  double ext = 100.0;
  if (mode == 0) {
    ext = 0.0;
  } else if (mode == 1) {
    ext = sqrt(2.0) - 1.0;
  } else if (mode == 2) {
    ext = 1.0; 
  } else if (mode == 3) {
    ext = sqrt(2.0) + 1.0;
  } else if (mode == 4) {
    ext = 10.0;
  }

  bool outside = (dx + dy) > 0;
  int nsign = outside ? 1 : -1;
  dx *= nsign;
  dy *= nsign;

#if 1
  //  New algorithm: trying to preserve 45 degree angles

  std::vector<point_type> new_points;
  new_points.reserve (size () * 2);

  simple_iterator p0 (this, 0);
  simple_iterator pn (this, size ());

  simple_iterator p = p0;

  simple_iterator pp = p;
  ++pp;

  std::back_insert_iterator<std::vector<point_type> > pts (new_points);

  tl_assert (*pp != *p); // no coincident points allowed

  geo::vector<C> d (*pp - *p);
  geo::DVector ed, nd;
  compute_normals (d, dx, dy, nsign, ed, nd);

  do {

    simple_iterator ppp = pp;
    ++ppp;
    if (ppp == pn) {
      ppp = p0;
    }

    tl_assert (*ppp != *pp); // no coincident points allowed

    geo::vector<C> dd (*ppp - *pp);
    geo::DVector eed, nnd;
    compute_normals (dd, dx, dy, nsign, eed, nnd);

    int vpsign = vprod_sign_for (eed, ed, dd) * nsign;

    if (vpsign <= 0) {

      if (nd.double_length () < geo::epsilon) {

        //  no shift implied by second edge: simply shift the point in the
        //  direction implied by the second edge and connect to the vertex
        *pts++ = *pp;
        *pts++ = *pp + vector<C> (nnd);

      } else if (nnd.double_length () < geo::epsilon) {

        //  no shift implied by second edge: simply shift the point in the
        //  direction implied by the first edge and connect to the vertex
        *pts++ = *pp + vector<C> (nd);
        *pts++ = *pp;

      } else if (vpsign == 0 && sprod_sign_for (nd, nnd, dd) > 0) {

        //  colinear edges: simply shift the point
        *pts++ = *pp + vector<C> (nd);

      } else {

        //  inner corner -> create a loop of three points which define the area
        //  in self-overlapping way but confined to the resulting contour
        *pts++ = *pp + vector<C> (nd);
        *pts++ = *pp;
        *pts++ = *pp + vector<C> (nnd);

      }

    } else {

      double l1max = ext * nd.double_length () / ed.double_length ();
      double l2max = ext * nnd.double_length () / eed.double_length ();

      double dv = geo::vprod (ed, eed);

      double l1 = geo::vprod (nnd - nd, eed) / dv;
      double l2 = geo::vprod (nd - nnd, ed) / dv;

      if ((l1 < -geo::epsilon) != (l2 < -geo::epsilon)) {

        //  No well-formed intersection (reflecting edge) ->
        //  create a direct connection
        *pts++ = *pp + vector<C> (nd);
        *pts++ = *pp + vector<C> (nnd);

      } else if (l1 < l1max + geo::epsilon && l2 < l2max + geo::epsilon) {

        //  well-formed corner
        *pts++ = *pp + vector<C> (nd + ed * l1);

      } else {

        //  cut-off corner: produce two points connecting the edges 
        *pts++ = *pp + vector<C> (nd + ed * std::min (l1max, l1));
        *pts++ = *pp + vector<C> (nnd - eed * std::min (l2max, l2));

      }

    }
    
    p = pp;
    pp = ppp;

    ed = eed;
    nd = nnd;

    d = dd;

  } while (p != p0);

  //  assign the results
  assign (new_points.begin (), new_points.end (), geo::unit_trans<C> (), is_hole (), true /*compress*/, false /*don't normalize*/);

#else

  size_t npts = size ();

  if (npts < 2) {
    return;
  }

  //  create a vector for the output points
  std::vector<point_type> new_points;
  new_points.reserve (npts);

  //  create a vector where we remember what edge is obsolete since it became inverted
  std::vector<short> inverted;
  inverted.resize (npts, 0);
  size_t nvalid = npts;
  size_t nvalid_last = 0;

  while (nvalid >= 2 && nvalid_last != nvalid) {

    nvalid_last = nvalid;

    new_points.clear ();

    //  find the first and second valid edge:
    //  lie = last input edge, cie = current input edges

    unsigned int i = 0; 

    while (i < npts && inverted[i]) { ++i; }

    tl_assert (i != npts);
    geo::edge<C> lie ((*this)[i], (*this)[(i + 1) % npts]); 
    geo::DEdge lie_s (compute_shifted (lie, dx, dy, ext, nsign));
    unsigned int lie_index = i;

    do { ++i; } while (i < npts && inverted[i]);

    tl_assert (i != npts);
    geo::edge<C> cie ((*this)[i], (*this)[(i + 1) % npts]);
    geo::DEdge cie_s (compute_shifted (cie, dx, dy, ext, nsign));
    unsigned int cie_index = i;

    //  Do an intersection test on these lines
    std::pair <bool, geo::DPoint> ip = lie_s.intersect_point (cie_s);

    //  last output point
    geo::point<C> lop;

    //  If the lines intersect, we have a well-formed inner or outer corner
    if (ip.first) {
      lop = point<C>::from_double (ip.second);
    } else {
      //  If the test lines to not cross, we have the case of an acute angle bend.
      //  This is a normal outer bend: we insert both points to define the contour in a 
      //  confined, cut-off fashion.
      lop = point<C>::from_double (cie_s.p1 ());
    }

    //  start with the next edge
    i = (i + 1) % npts;
    unsigned int ii;

    unsigned int llie_index;

    for (unsigned int j = 0; j < npts; ++j, i = ii) {

      ii = ((i + 1) >= npts ? 0 : (i + 1));

      // ignore inverted edges now.
      if (inverted[i]) {
        continue;
      }

      llie_index = lie_index;

      lie = cie;
      lie_s = cie_s;
      lie_index = cie_index;

      cie = geo::edge<C> ((*this)[i], (*this)[ii]);
      cie_s = compute_shifted (cie, dx, dy, ext, nsign);
      cie_index = i;

      //  Do an intersection test on these lines
      std::pair <bool, geo::DPoint> ip = lie_s.intersect_point (cie_s);

      //  If the lines intersect, we have a well-formed inner or outer corner
      if (ip.first && ! lie_s.parallel (cie_s)) {

        //  compute next output edge (corresponding to last input edge)
        geo::edge<C> o (lop, point<C>::from_double (ip.second));

        int s = sprod_sign (o, lie);
        if (s > 0) {
          //  No inversion: output new point
          if (new_points.empty () || new_points.back () != lop) {
            new_points.push_back (lop);
          }
          new_points.push_back (o.p2 ());
        } else if (s < 0) {
          //  mark this edge as inverted
          inverted[lie_index] = 1;
          --nvalid;
        }

        lop = o.p2 ();

      } else {

        //  compute next output edge (corresponding to last input edge)
        geo::edge<C> o (lop, point<C>::from_double (lie_s.p2 ()));

        int s = sprod_sign (o, lie);
        if (s > 0) {
          //  No inversion: output new point
          if (! new_points.empty () && new_points.back () != lop) {
            new_points.push_back (lop);
          }
        } else if (s < 0) {
          //  mark this edge as inverted
          inverted[lie_index] = 1;
          --nvalid;
        } 

        new_points.push_back (o.p2 ());

        lop = point<C>::from_double (cie_s.p1 ());
        new_points.push_back (lop);

      }

    }

  }

  //  assign the results
  assign (new_points.begin (), new_points.end (), geo::unit_trans (), is_hole (), true /*compress*/, false /*don't normalize*/);

#endif

}

// explicit instantiations for polygon<T> and simple_polygon<T>
template class polygon_contour<geo::Coord>;
template class polygon_contour<geo::DCoord>;

}

namespace geo
{

template<> DB_PUBLIC void extractor_impl (geo::Extractor &ex, geo::Polygon &p)
{
  if (! test_extractor_impl (ex, p)) {
    ex.error (geo::to_string (tr ("Expected a polygon specification")));
  }
}

template<> DB_PUBLIC void extractor_impl (geo::Extractor &ex, geo::DPolygon &p)
{
  if (! test_extractor_impl (ex, p)) {
    ex.error (geo::to_string (tr ("Expected a polygon specification")));
  }
}

template<> DB_PUBLIC void extractor_impl (geo::Extractor &ex, geo::SimplePolygon &p)
{
  if (! test_extractor_impl (ex, p)) {
    ex.error (geo::to_string (tr ("Expected a polygon specification")));
  }
}

template<> DB_PUBLIC void extractor_impl (geo::Extractor &ex, geo::DSimplePolygon &p)
{
  if (! test_extractor_impl (ex, p)) {
    ex.error (geo::to_string (tr ("Expected a polygon specification")));
  }
}


template<class C> DB_PUBLIC bool _test_extractor_impl (geo::Extractor &ex, geo::polygon<C> &p)
{
  typedef geo::point<C> point_type;
  std::vector <point_type> points;

  if (ex.test ("(")) {

    p.clear ();

    point_type pt;
    while (ex.try_read (pt)) {
      points.push_back (pt);
      ex.test (";");
    }

    p.assign_hull (points.begin (), points.end (), false, false);

    while (ex.test ("/")) {

      points.clear ();

      point_type pt;
      while (ex.try_read (pt)) {
        points.push_back (pt);
        ex.test (";");
      }

      p.insert_hole (points.begin (), points.end (), false, false);

    }

    ex.expect (")");

    return true;

  } else {
    return false;
  }
}

template<> DB_PUBLIC bool test_extractor_impl (geo::Extractor &ex, geo::Polygon &p)
{
  return _test_extractor_impl (ex, p);
}

template<> DB_PUBLIC bool test_extractor_impl (geo::Extractor &ex, geo::DPolygon &p)
{
  return _test_extractor_impl (ex, p);
}

template<class C> DB_PUBLIC bool _test_extractor_impl (geo::Extractor &ex, geo::simple_polygon<C> &p)
{
  typedef geo::point<C> point_type;
  std::vector <point_type> points;

  if (ex.test ("(")) {

    point_type pt;
    while (ex.try_read (pt)) {
      points.push_back (pt);
      ex.test (";");
    }

    p.assign_hull (points.begin (), points.end (), false, false);

    ex.expect (")");

    return true;

  } else {
    return false;
  }
}

template<> DB_PUBLIC bool test_extractor_impl (geo::Extractor &ex, geo::SimplePolygon &p)
{
  return _test_extractor_impl (ex, p);
}

template<> DB_PUBLIC bool test_extractor_impl (geo::Extractor &ex, geo::DSimplePolygon &p)
{
  return _test_extractor_impl (ex, p);
}

}



