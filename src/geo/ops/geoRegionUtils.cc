#include "geoRegionUtils.h"
#include "geoPolygonTools.h"
#include "geoEdgeBoolean.h"
#include "utilsSelect.h"

namespace geo
{

// -------------------------------------------------------------------------------------
//  Edge2EdgeCheckBase implementation

Edge2EdgeCheckBase::Edge2EdgeCheckBase (const EdgeRelationFilter &check, bool different_polygons, bool requires_different_layers, bool with_shielding, bool symmetric_edges)
  : mp_check (&check), m_requires_different_layers (requires_different_layers), m_different_polygons (different_polygons),
    m_first_pseudo (std::numeric_limits<size_t>::max ()),
    m_with_shielding (with_shielding),
    m_symmetric_edges (symmetric_edges),
    m_has_edge_pair_output (true),
    m_has_negative_edge_output (false),
    m_pass (0)
{
  m_distance = check.distance ();
}

bool
Edge2EdgeCheckBase::prepare_next_pass ()
{
  ++m_pass;

  if (m_pass == 1) {

    m_first_pseudo = m_ep.size ();

    if (m_with_shielding && ! m_ep.empty ()) {

      m_ep_discarded.resize (m_ep.size (), false);

      //  second pass:
      return true;

    } else if (m_has_negative_edge_output) {

      //  second pass:
      return true;

    }

  }

  if (! m_ep.empty () && m_has_edge_pair_output) {

    std::vector<bool>::const_iterator d = m_ep_discarded.begin ();
    std::vector<bool>::const_iterator i = m_ep_intra_polygon.begin ();
    std::vector<geo::EdgePair>::const_iterator ep = m_ep.begin ();
    while (ep != m_ep.end () && size_t (ep - m_ep.begin ()) < m_first_pseudo) {
      bool use_result = true;
      if (d != m_ep_discarded.end ()) {
        use_result = ! *d;
        ++d;
      }
      if (use_result) {
        put (*ep, *i);
      }
      ++ep;
      ++i;
    }

  }

  return false;
}

static inline bool shields (const geo::EdgePair &ep, const geo::Edge &q)
{
  geo::Edge pe1 (ep.first ().p1 (), ep.second ().p2 ());
  geo::Edge pe2 (ep.second ().p1 (), ep.first ().p2 ());

  std::pair<bool, geo::Point> ip1 = pe1.intersect_point (q);
  std::pair<bool, geo::Point> ip2 = pe2.intersect_point (q);

  if (ip1.first && ip2.first) {
    return ip1.second != ip2.second || (pe1.side_of (q.p1 ()) != 0 && pe2.side_of (q.p2 ()) != 0);
  } else {
    return false;
  }
}

void
Edge2EdgeCheckBase::finish (const Edge *o, size_t p)
{
  if (m_has_negative_edge_output && m_pass == 1 && m_pseudo_edges.find (std::make_pair (*o, p)) == m_pseudo_edges.end ()) {

    std::pair<geo::Edge, size_t> k (*o, p);
    std::multimap<std::pair<geo::Edge, size_t>, size_t>::const_iterator i0 = m_e2ep.find (k);

    bool fully_removed = false;
    bool any = false;
    for (std::multimap<std::pair<geo::Edge, size_t>, size_t>::const_iterator i = i0; ! fully_removed && i != m_e2ep.end () && i->first == k; ++i) {
      size_t n = i->second / 2;
      if (n >= m_ep_discarded.size () || !m_ep_discarded [n]) {
        any = true;
        fully_removed = (((i->second & 1) == 0 ? m_ep [n].first () : m_ep [n].second ()) == *o);
      }
    }

    if (! any) {

      put_negative (*o, (int) p);

    } else if (! fully_removed) {

      std::set<geo::Edge> partial_edges;

      geo::EdgeBooleanCluster<std::set<geo::Edge> > ec (&partial_edges, geo::EdgeNot);
      ec.add (o, 0);

      for (std::multimap<std::pair<geo::Edge, size_t>, size_t>::const_iterator i = i0; i != m_e2ep.end () && i->first == k; ++i) {
        size_t n = i->second / 2;
        if (n >= m_ep_discarded.size () || !m_ep_discarded [n]) {
          ec.add (((i->second & 1) == 0 ? &m_ep [n].first () : &m_ep [n].second ()), 1);
        }
      }

      ec.finish ();

      for (std::set<geo::Edge>::const_iterator e = partial_edges.begin (); e != partial_edges.end (); ++e) {
        put_negative (*e, (int) p);
      }

    }

  }
}

bool
Edge2EdgeCheckBase::feed_pseudo_edges (geo::box_scanner<geo::Edge, size_t> &scanner)
{
  if (m_pass == 1) {
    for (std::set<std::pair<geo::Edge, size_t> >::const_iterator e = m_pseudo_edges.begin (); e != m_pseudo_edges.end (); ++e) {
      scanner.insert (&e->first, e->second);
    }
    return ! m_pseudo_edges.empty ();
  } else {
    return false;
  }
}

inline bool edges_considered (bool requires_different_polygons, bool requires_different_layers, size_t p1, size_t p2)
{
  if (p1 == p2) {
    if (requires_different_polygons) {
      return false;
    } else if ((p1 & size_t (1)) != 0) {
      //  edges from the same polygon are only considered on first layer.
      //  Reasoning: this case happens when "intruder" polygons are put on layer 1
      //  while "subject" polygons are put on layer 0. We don't want "intruders"
      //  to generate intra-polygon markers.
      return false;
    }
  }

  if (((p1 ^ p2) & size_t (1)) == 0) {
    if (requires_different_layers) {
      return false;
    } else if ((p1 & size_t (1)) != 0) {
      //  edges on the same layer are only considered on first layer.
      //  Reasoning: this case happens when "intruder" polygons are put on layer 1
      //  while "subject" polygons are put on layer 0. We don't want "intruders"
      //  to generate inter-polygon markers between them.
      return false;
    }
  }

  return true;
}

void
Edge2EdgeCheckBase::add (const geo::Edge *o1, size_t p1, const geo::Edge *o2, size_t p2)
{
  if (m_pass == 0) {

    //  Overlap or inside checks require input from different layers
    if (edges_considered (m_different_polygons, m_requires_different_layers, p1, p2)) {

      //  ensure that the first check argument is of layer 1 and the second of
      //  layer 2 (unless both are of the same layer)
      int l1 = int (p1 & size_t (1));
      int l2 = int (p2 & size_t (1));

      if (l1 > l2) {
        std::swap (o1, o2);
        std::swap (p1, p2);
      }

      geo::EdgePair ep;
      if (mp_check->check (*o1, *o2, &ep)) {

        ep.set_symmetric (m_symmetric_edges);

        //  found a violation: store inside the local buffer for now. In the second
        //  pass we will eliminate those which are shielded completely (with shielding)
        //  and/or compute the negative edges.
        size_t n = m_ep.size ();

        m_ep.push_back (ep);
        m_ep_intra_polygon.push_back (p1 == p2);

        m_e2ep.insert (std::make_pair (std::make_pair (*o1, p1), n * 2));
        m_e2ep.insert (std::make_pair (std::make_pair (*o2, p2), n * 2 + 1));

        if (m_has_negative_edge_output) {
          m_pseudo_edges.insert (std::make_pair (geo::Edge (ep.first ().p1 (), ep.second ().p2 ()), p1));
          m_pseudo_edges.insert (std::make_pair (geo::Edge (ep.second ().p1 (), ep.first ().p2 ()), p1));
          if (p1 != p2) {
            m_pseudo_edges.insert (std::make_pair (geo::Edge (ep.first ().p1 (), ep.second ().p2 ()), p2));
            m_pseudo_edges.insert (std::make_pair (geo::Edge (ep.second ().p1 (), ep.first ().p2 ()), p2));
          }
        }

      }

    }

  } else {

    //  set the discarded flags for shielded output
    if (m_with_shielding) {

      //  a simple (complete) shielding implementation which is based on the
      //  assumption that shielding is relevant as soon as a foreign edge cuts through
      //  both of the edge pair's connecting edges.

      //  TODO: this implementation does not take into account the nature of the
      //  EdgePair - because of "whole_edge" it may not reflect the part actually
      //  violating the distance.

      std::vector<size_t> n1, n2;

      for (unsigned int p = 0; p < 2; ++p) {

        std::pair<geo::Edge, size_t> k (*o1, p1);
        for (std::multimap<std::pair<geo::Edge, size_t>, size_t>::const_iterator i = m_e2ep.find (k); i != m_e2ep.end () && i->first == k; ++i) {
          size_t n = i->second / 2;
          if (n < m_first_pseudo && ! m_ep_discarded [n]) {
            n1.push_back (n);
          }
        }

        std::sort (n1.begin (), n1.end ());

        std::swap (o1, o2);
        std::swap (p1, p2);
        n1.swap (n2);

      }

      for (unsigned int p = 0; p < 2; ++p) {

        std::vector<size_t> nn;
        std::set_difference (n1.begin (), n1.end (), n2.begin (), n2.end (), std::back_inserter (nn));

        for (std::vector<size_t>::const_iterator i = nn.begin (); i != nn.end (); ++i) {
          geo::EdgePair ep = m_ep [*i].normalized ();
          if (shields (ep, *o2)) {
            m_ep_discarded [*i] = true;
          }
        }

        std::swap (o1, o2);
        std::swap (p1, p2);
        n1.swap (n2);

      }

    }

    //  for negative output edges are cancelled by short interactions perpendicular to them
    //  For this we have generated "pseudo edges" running along the sides of the original violation. We now check a real
    //  edge vs. a pseudo edge with the same conditions as the normal interaction and add them to the results. In the
    //  negative case this means we cancel a real edge.

    if (m_has_negative_edge_output &&
      (m_pseudo_edges.find (std::make_pair (*o1, p1)) != m_pseudo_edges.end ()) != (m_pseudo_edges.find (std::make_pair (*o2, p2)) != m_pseudo_edges.end ())) {

      //  Overlap or inside checks require input from different layers
      if (edges_considered (m_different_polygons, m_requires_different_layers, p1, p2)) {

        //  ensure that the first check argument is of layer 1 and the second of
        //  layer 2 (unless both are of the same layer)
        int l1 = int (p1 & size_t (1));
        int l2 = int (p2 & size_t (1));

        if (l1 > l2) {
          std::swap (o1, o2);
          std::swap (p1, p2);
        }

        geo::EdgePair ep;
        if (mp_check->check (*o1, *o2, &ep)) {

          size_t n = m_ep.size ();

          m_ep.push_back (ep);
          m_ep_intra_polygon.push_back (p1 == p2);  //  not really required, but there for consistency

          m_e2ep.insert (std::make_pair (std::make_pair (*o1, p1), n * 2));
          m_e2ep.insert (std::make_pair (std::make_pair (*o2, p2), n * 2 + 1));

        }

      }

    }

  }

}

/**
 *  @brief Gets a value indicating whether the check requires different layers
 */
bool
Edge2EdgeCheckBase::requires_different_layers () const
{
  return m_requires_different_layers;
}

/**
 *  @brief Sets a value indicating whether the check requires different layers
 */
void
Edge2EdgeCheckBase::set_requires_different_layers (bool f)
{
  m_requires_different_layers = f;
}

/**
 *  @brief Gets a value indicating whether the check requires different layers
 */
bool
Edge2EdgeCheckBase::different_polygons () const
{
  return m_different_polygons;
}

/**
 *  @brief Sets a value indicating whether the check requires different layers
 */
void
Edge2EdgeCheckBase::set_different_polygons (bool f)
{
  m_different_polygons = f;
}

/**
 *  @brief Gets the distance value
 */
EdgeRelationFilter::distance_type
Edge2EdgeCheckBase::distance () const
{
  return m_distance;
}

// -------------------------------------------------------------------------------------
//  Poly2PolyCheckBase implementation

template <class PolygonType>
poly2poly_check<PolygonType>::poly2poly_check (Edge2EdgeCheckBase &output)
  : mp_output (& output)
{
  //  .. nothing yet ..
}

template <class PolygonType>
void
poly2poly_check<PolygonType>::finish (const PolygonType *o, size_t p)
{
  enter (*o, p);
}

static size_t vertices (const geo::Polygon &p)
{
  return p.vertices ();
}

static size_t vertices (const geo::PolygonRef &p)
{
  return p.obj ().vertices ();
}

template <class PolygonType>
void
poly2poly_check<PolygonType>::enter (const PolygonType &o, size_t p)
{
  if (! mp_output->requires_different_layers () && ! mp_output->different_polygons ()) {

    //  finally we check the polygons vs. itself for checks involving intra-polygon interactions

    m_scanner.clear ();
    m_scanner.reserve (vertices (o));

    m_edges.clear ();
    m_edges.reserve (vertices (o));

    for (typename PolygonType::polygon_edge_iterator e = o.begin_edge (); ! e.at_end (); ++e) {
      m_edges.push_back (*e);
      m_scanner.insert (& m_edges.back (), p);
    }

    mp_output->feed_pseudo_edges (m_scanner);

    tl_assert (m_edges.size () == vertices (o));

    m_scanner.process (*mp_output, mp_output->distance (), geo::box_convert<geo::Edge> ());

  }
}

template <class PolygonType>
void
poly2poly_check<PolygonType>::add (const PolygonType *o1, size_t p1, const PolygonType *o2, size_t p2)
{
  enter (*o1, p1, *o2, p2);
}

static bool interact (const geo::Box &box, const geo::Edge &e)
{
  if (! e.bbox ().touches (box)) {
    return false;
  } else if (e.is_ortho ()) {
    return true;
  } else {
    return e.clipped (box).first;
  }
}

template <class PolygonType>
void
poly2poly_check<PolygonType>::enter (const PolygonType &o1, size_t p1, const PolygonType &o2, size_t p2)
{
  if (p1 != p2 && (! mp_output->requires_different_layers () || ((p1 ^ p2) & 1) != 0)) {

    bool take_all = mp_output->has_negative_edge_output ();

    geo::Box common_box;
    if (! take_all) {
      geo::Vector e (mp_output->distance (), mp_output->distance ());
      common_box = o1.box ().enlarged (e) & o2.box ().enlarged (e);
      if (common_box.empty ()) {
        return;
      }
    }

    m_scanner.clear ();
    m_scanner.reserve (vertices (o1) + vertices (o2));

    m_edges.clear ();
    m_edges.reserve (vertices (o1) + vertices (o2));

    bool any_o1 = false, any_o2 = false;

    for (typename PolygonType::polygon_edge_iterator e = o1.begin_edge (); ! e.at_end (); ++e) {
      if (take_all || interact (common_box, *e)) {
        m_edges.push_back (*e);
        m_scanner.insert (& m_edges.back (), p1);
        any_o1 = true;
      }
    }

    for (typename PolygonType::polygon_edge_iterator e = o2.begin_edge (); ! e.at_end (); ++e) {
      if (take_all || interact (common_box, *e)) {
        m_edges.push_back (*e);
        m_scanner.insert (& m_edges.back (), p2);
        any_o2 = true;
      }
    }

    if (! take_all && (! any_o1 || ! any_o2)) {
      return;
    }

    mp_output->feed_pseudo_edges (m_scanner);

    //  temporarily disable intra-polygon check in that step .. we do that later in finish()
    //  if required (#650).
    bool no_intra = mp_output->different_polygons ();
    mp_output->set_different_polygons (true);

    m_scanner.process (*mp_output, mp_output->distance (), geo::box_convert<geo::Edge> ());

    mp_output->set_different_polygons (no_intra);

  }
}

//  explicit instantiations
template class poly2poly_check<geo::Polygon>;
template class poly2poly_check<geo::PolygonRef>;

// -------------------------------------------------------------------------------------
//  RegionPerimeterFilter implementation

RegionPerimeterFilter::RegionPerimeterFilter (perimeter_type pmin, perimeter_type pmax, bool inverse)
  : m_pmin (pmin), m_pmax (pmax), m_inverse (inverse)
{
  //  .. nothing yet ..
}

bool RegionPerimeterFilter::check (perimeter_type p) const
{
  if (! m_inverse) {
    return p >= m_pmin && p < m_pmax;
  } else {
    return ! (p >= m_pmin && p < m_pmax);
  }
}

bool RegionPerimeterFilter::selected (const geo::Polygon &poly) const
{
  return check (poly.perimeter ());
}

bool RegionPerimeterFilter::selected (const geo::PolygonRef &poly) const
{
  return check (poly.perimeter ());
}

bool RegionPerimeterFilter::selected_set (const std::unordered_set<geo::Polygon> &poly) const
{
  perimeter_type ps = 0;
  for (std::unordered_set<geo::Polygon>::const_iterator p = poly.begin (); p != poly.end (); ++p) {
    ps += p->perimeter ();
  }
  return check (ps);
}

bool RegionPerimeterFilter::selected_set (const std::unordered_set<geo::PolygonRef> &poly) const
{
  perimeter_type ps = 0;
  for (std::unordered_set<geo::PolygonRef>::const_iterator p = poly.begin (); p != poly.end (); ++p) {
    ps += p->perimeter ();
  }
  return check (ps);
}

const TransformationReducer *RegionPerimeterFilter::vars () const
{
  return &m_vars;
}

// -------------------------------------------------------------------------------------
//  RegionAreaFilter implementation

RegionAreaFilter::RegionAreaFilter (area_type amin, area_type amax, bool inverse)
  : m_amin (amin), m_amax (amax), m_inverse (inverse)
{
  //  .. nothing yet ..
}

bool RegionAreaFilter::check (area_type a) const
{
  if (! m_inverse) {
    return a >= m_amin && a < m_amax;
  } else {
    return ! (a >= m_amin && a < m_amax);
  }
}

bool RegionAreaFilter::selected (const geo::Polygon &poly) const
{
  return check (poly.area ());
}

bool RegionAreaFilter::selected (const geo::PolygonRef &poly) const
{
  return check (poly.area ());
}

bool RegionAreaFilter::selected_set (const std::unordered_set<geo::Polygon> &poly) const
{
  area_type as = 0;
  for (std::unordered_set<geo::Polygon>::const_iterator p = poly.begin (); p != poly.end (); ++p) {
    as += p->area ();
  }
  return check (as);
}

bool RegionAreaFilter::selected_set (const std::unordered_set<geo::PolygonRef> &poly) const
{
  area_type as = 0;
  for (std::unordered_set<geo::PolygonRef>::const_iterator p = poly.begin (); p != poly.end (); ++p) {
    as += p->area ();
  }
  return check (as);
}

const TransformationReducer *
RegionAreaFilter::vars () const
{
  return &m_vars;
}

// -------------------------------------------------------------------------------------
//  RectilinearFilter implementation

RectilinearFilter::RectilinearFilter (bool inverse)
  : m_inverse (inverse)
{
  //  .. nothing yet ..
}

bool
RectilinearFilter::selected (const geo::Polygon &poly) const
{
  return poly.is_rectilinear () != m_inverse;
}

bool
RectilinearFilter::selected (const geo::PolygonRef &poly) const
{
  return poly.is_rectilinear () != m_inverse;
}

const TransformationReducer *
RectilinearFilter::vars () const
{
  return 0;
}

// -------------------------------------------------------------------------------------
//  HoleCountFilter implementation

HoleCountFilter::HoleCountFilter (size_t min_count, size_t max_count, bool inverse)
  : m_min_count (min_count), m_max_count (max_count), m_inverse (inverse)
{
  //  .. nothing yet ..
}

bool
HoleCountFilter::selected (const geo::Polygon &poly) const
{
  bool ok = poly.holes () < m_max_count && poly.holes () >= m_min_count;
  return ok != m_inverse;
}

bool
HoleCountFilter::selected (const geo::PolygonRef &poly) const
{
  bool ok = poly.obj ().holes () < m_max_count && poly.obj ().holes () >= m_min_count;
  return ok != m_inverse;
}

const TransformationReducer *HoleCountFilter::vars () const
{
  return 0;
}

// -------------------------------------------------------------------------------------
//  RectilinearFilter implementation

RectangleFilter::RectangleFilter (bool is_square, bool inverse)
  : m_is_square (is_square), m_inverse (inverse)
{
  //  .. nothing yet ..
}

bool
RectangleFilter::selected (const geo::Polygon &poly) const
{
  bool ok = poly.is_box ();
  if (ok && m_is_square) {
    geo::Box box = poly.box ();
    ok = box.width () == box.height ();
  }
  return ok != m_inverse;
}

bool
RectangleFilter::selected (const geo::PolygonRef &poly) const
{
  bool ok = poly.is_box ();
  if (ok && m_is_square) {
    geo::Box box = poly.box ();
    ok = box.width () == box.height ();
  }
  return ok != m_inverse;
}

const TransformationReducer *RectangleFilter::vars () const
{
  return 0;
}

// -------------------------------------------------------------------------------------
//  RectilinearFilter implementation

RegionBBoxFilter::RegionBBoxFilter (value_type vmin, value_type vmax, bool inverse, parameter_type parameter)
  : m_vmin (vmin), m_vmax (vmax), m_inverse (inverse), m_parameter (parameter)
{
  //  .. nothing yet ..
}

bool
RegionBBoxFilter::check (const geo::Box &box) const
{
  value_type v = 0;
  if (m_parameter == BoxWidth) {
    v = box.width ();
  } else if (m_parameter == BoxHeight) {
    v = box.height ();
  } else if (m_parameter == BoxMinDim) {
    v = std::min (box.width (), box.height ());
  } else if (m_parameter == BoxMaxDim) {
    v = std::max (box.width (), box.height ());
  } else if (m_parameter == BoxAverageDim) {
    v = (box.width () + box.height ()) / 2;
  }
  if (! m_inverse) {
    return v >= m_vmin && v < m_vmax;
  } else {
    return ! (v >= m_vmin && v < m_vmax);
  }
}

bool
RegionBBoxFilter::selected (const geo::Polygon &poly) const
{
  return check (poly.box ());
}

bool
RegionBBoxFilter::selected (const geo::PolygonRef &poly) const
{
  return check (poly.box ());
}

const TransformationReducer *
RegionBBoxFilter::vars () const
{
  if (m_parameter != BoxWidth && m_parameter != BoxHeight) {
    return &m_isotropic_vars;
  } else {
    return &m_anisotropic_vars;
  }
}

// -------------------------------------------------------------------------------------
//  RectilinearFilter implementation

RegionRatioFilter::RegionRatioFilter (double vmin, bool min_included, double vmax, bool max_included, bool inverse, parameter_type parameter)
  : m_vmin (vmin), m_vmax (vmax), m_vmin_included (min_included), m_vmax_included (max_included), m_inverse (inverse), m_parameter (parameter)
{
  //  .. nothing yet ..
}

template <class P>
static double compute_ratio_parameter (const P &poly, RegionRatioFilter::parameter_type parameter)
{
  double v = 0.0;

  if (parameter == RegionRatioFilter::AreaRatio) {

    v = poly.area_ratio ();

  } else if (parameter == RegionRatioFilter::AspectRatio) {

    geo::Box box = poly.box ();
    double f = std::max (box.height (), box.width ());
    double d = std::min (box.height (), box.width ());
    if (d < 1) {
      return false;
    }

    v = f / d;

  } else if (parameter == RegionRatioFilter::RelativeHeight) {

    geo::Box box = poly.box ();
    double f = box.height ();
    double d = box.width ();
    if (d < 1) {
      return false;
    }

    v = f / d;

  }

  return v;
}

bool RegionRatioFilter::selected (const geo::Polygon &poly) const
{
  double v = compute_ratio_parameter (poly, m_parameter);

  bool ok = (v - (m_vmin_included ? -geo::epsilon : geo::epsilon) > m_vmin  && v - (m_vmax_included ? geo::epsilon : -geo::epsilon) < m_vmax);
  return ok != m_inverse;
}

bool RegionRatioFilter::selected (const geo::PolygonRef &poly) const
{
  double v = compute_ratio_parameter (poly, m_parameter);

  bool ok = (v - (m_vmin_included ? -geo::epsilon : geo::epsilon) > m_vmin  && v - (m_vmax_included ? geo::epsilon : -geo::epsilon) < m_vmax);
  return ok != m_inverse;
}

const TransformationReducer *RegionRatioFilter::vars () const
{
  if (m_parameter != RelativeHeight) {
    return &m_isotropic_vars;
  } else {
    return &m_anisotropic_vars;
  }
}

// -------------------------------------------------------------------------------------
//  SinglePolygonCheck implementation

SinglePolygonCheck::SinglePolygonCheck (geo::edge_relation_type rel, geo::Coord d, const RegionCheckOptions &options)
  : m_relation (rel), m_d (d), m_options (options)
{ }

void
SinglePolygonCheck::process (const geo::Polygon &polygon, std::vector<geo::EdgePair> &res) const
{
  std::unordered_set<geo::EdgePair> result;

  EdgeRelationFilter check (m_relation, m_d, m_options.metrics);
  check.set_include_zero (false);
  check.set_whole_edges (m_options.whole_edges);
  check.set_ignore_angle (m_options.ignore_angle);
  check.set_min_projection (m_options.min_projection);
  check.set_max_projection (m_options.max_projection);

  edge2edge_check_negative_or_positive <std::unordered_set<geo::EdgePair> > edge_check (check, result, m_options.negative, false /*=same polygons*/, false /*=same layers*/, m_options.shielded, true /*=symmetric*/);
  poly2poly_check<geo::Polygon> poly_check (edge_check);

  do {
    poly_check.enter (polygon, 0);
  } while (edge_check.prepare_next_pass ());

  res.insert (res.end (), result.begin (), result.end ());
}

// -------------------------------------------------------------------------------------------------------------
//  Strange polygon processor

namespace {

/**
 *  @brief A helper class to implement the strange polygon detector
 */
struct StrangePolygonInsideFunc
{
  inline bool operator() (int wc) const
  {
    return wc < 0 || wc > 1;
  }
};

}

StrangePolygonCheckProcessor::StrangePolygonCheckProcessor () { }

StrangePolygonCheckProcessor::~StrangePolygonCheckProcessor () { }

void
StrangePolygonCheckProcessor::process (const geo::Polygon &poly, std::vector<geo::Polygon> &res) const
{
  EdgeProcessor ep;
  ep.insert (poly);

  StrangePolygonInsideFunc inside;
  geo::GenericMerge<StrangePolygonInsideFunc> op (inside);
  geo::PolygonContainer pc (res, false);
  geo::PolygonGenerator pg (pc, false, false);
  ep.process (pg, op);
}

// -------------------------------------------------------------------------------------------------------------
//  Smoothing processor

SmoothingProcessor::SmoothingProcessor (geo::Coord d, bool keep_hv) : m_d (d), m_keep_hv (keep_hv) { }

SmoothingProcessor::~SmoothingProcessor () { }

void
SmoothingProcessor::process (const geo::Polygon &poly, std::vector<geo::Polygon> &res) const
{
  res.push_back (geo::smooth (poly, m_d, m_keep_hv));
}

// -------------------------------------------------------------------------------------------------------------
//  Rounded corners processor

RoundedCornersProcessor::RoundedCornersProcessor (double rinner, double router, unsigned int n)
  : m_rinner (rinner), m_router (router), m_n (n)
{ }

RoundedCornersProcessor::~RoundedCornersProcessor ()
{ }

void
RoundedCornersProcessor::process (const geo::Polygon &poly, std::vector<geo::Polygon> &res) const
{
  res.push_back (geo::compute_rounded (poly, m_rinner, m_router, m_n));
}

// -------------------------------------------------------------------------------------------------------------
//  Holes decomposition processor

HolesExtractionProcessor::HolesExtractionProcessor ()
{
}

HolesExtractionProcessor::~HolesExtractionProcessor ()
{
}

void
HolesExtractionProcessor::process (const geo::Polygon &poly, std::vector<geo::Polygon> &res) const
{
  for (size_t i = 0; i < poly.holes (); ++i) {
    res.push_back (geo::Polygon ());
    res.back ().assign_hull (poly.begin_hole ((unsigned int) i), poly.end_hole ((unsigned int) i));
  }
}

// -------------------------------------------------------------------------------------------------------------
//  Hull decomposition processor

HullExtractionProcessor::HullExtractionProcessor ()
{
}

HullExtractionProcessor::~HullExtractionProcessor ()
{
}

void
HullExtractionProcessor::process (const geo::Polygon &poly, std::vector<geo::Polygon> &res) const
{
  res.push_back (geo::Polygon ());
  res.back ().assign_hull (poly.begin_hull (), poly.end_hull ());
}

// -------------------------------------------------------------------------------------
//  RegionToEdgeInteractionFilterBase implementation

template <class PolygonType, class EdgeType, class OutputType>
region_to_edge_interaction_filter_base<PolygonType, EdgeType, OutputType>::region_to_edge_interaction_filter_base (bool inverse, bool get_all)
  : m_inverse (inverse), m_get_all (get_all)
{
  //  .. nothing yet ..
}

template <class PolygonType, class EdgeType, class OutputType>
void
region_to_edge_interaction_filter_base<PolygonType, EdgeType, OutputType>::preset (const OutputType *s)
{
  m_seen.insert (s);
}

template <class PolygonType, class EdgeType, class OutputType>
void
region_to_edge_interaction_filter_base<PolygonType, EdgeType, OutputType>::add (const PolygonType *p, size_t, const EdgeType *e, size_t)
{
  const OutputType *o = 0;
  geo::select (o, p, e);

  if (m_get_all || (m_seen.find (o) == m_seen.end ()) != m_inverse) {

    //  A polygon and an edge interact if the edge is either inside completely
    //  of at least one edge of the polygon intersects with the edge
    bool interacts = false;
    if (p->box ().contains (e->p1 ()) && geo::inside_poly (p->begin_edge (), e->p1 ()) >= 0) {
      interacts = true;
    } else {
      for (typename PolygonType::polygon_edge_iterator pe = p->begin_edge (); ! pe.at_end () && ! interacts; ++pe) {
        if ((*pe).intersect (*e)) {
          interacts = true;
        }
      }
    }

    if (interacts) {
      if (m_inverse) {
        m_seen.erase (o);
      } else {
        if (! m_get_all) {
          m_seen.insert (o);
        }
        put (*o);
      }
    }

  }
}

template <class PolygonType, class EdgeType, class OutputType>
void
region_to_edge_interaction_filter_base<PolygonType, EdgeType, OutputType>::fill_output ()
{
  for (typename std::set<const OutputType *>::const_iterator s = m_seen.begin (); s != m_seen.end (); ++s) {
    put (**s);
  }
}

//  explicit instantiations
template class region_to_edge_interaction_filter_base<geo::Polygon, geo::Edge, geo::Polygon>;
template class region_to_edge_interaction_filter_base<geo::PolygonRef, geo::Edge, geo::PolygonRef>;
template class region_to_edge_interaction_filter_base<geo::Polygon, geo::Edge, geo::Edge>;
template class region_to_edge_interaction_filter_base<geo::PolygonRef, geo::Edge, geo::Edge>;

// -------------------------------------------------------------------------------------
//  RegionToTextInteractionFilterBase implementation

template <class PolygonType, class TextType, class OutputType>
region_to_text_interaction_filter_base<PolygonType, TextType, OutputType>::region_to_text_interaction_filter_base (bool inverse, bool get_all)
  : m_inverse (inverse), m_get_all (get_all)
{
  //  .. nothing yet ..
}

template <class PolygonType, class TextType, class OutputType>
void
region_to_text_interaction_filter_base<PolygonType, TextType, OutputType>::preset (const OutputType *s)
{
  m_seen.insert (s);
}

template <class PolygonType, class TextType, class OutputType>
void
region_to_text_interaction_filter_base<PolygonType, TextType, OutputType>::add (const PolygonType *p, size_t, const TextType *t, size_t)
{
  const OutputType *o = 0;
  geo::select (o, p, t);

  if (m_get_all || (m_seen.find (o) == m_seen.end ()) != m_inverse) {

    //  A polygon and an text interact if the text is either inside completely
    //  of at least one text of the polygon intersects with the text
    geo::Point pt = geo::box_convert<TextType> () (*t).p1 ();
    if (p->box ().contains (pt) && geo::inside_poly (p->begin_edge (), pt) >= 0) {
      if (m_inverse) {
        m_seen.erase (o);
      } else {
        if (! m_get_all) {
          m_seen.insert (o);
        }
        put (*o);
      }
    }

  }
}

template <class PolygonType, class TextType, class OutputType>
void
region_to_text_interaction_filter_base<PolygonType, TextType, OutputType>::fill_output ()
{
  for (typename std::set<const OutputType *>::const_iterator s = m_seen.begin (); s != m_seen.end (); ++s) {
    put (**s);
  }
}

//  explicit instantiations
template class region_to_text_interaction_filter_base<geo::PolygonRef, geo::TextRef, geo::PolygonRef>;
template class region_to_text_interaction_filter_base<geo::Polygon, geo::Text, geo::Polygon>;
template class region_to_text_interaction_filter_base<geo::Polygon, geo::Text, geo::Text>;
template class region_to_text_interaction_filter_base<geo::Polygon, geo::TextRef, geo::TextRef>;
template class region_to_text_interaction_filter_base<geo::PolygonRef, geo::TextRef, geo::TextRef>;

// -------------------------------------------------------------------------------------
//  Polygon snapping

geo::Polygon
snapped_polygon (const geo::Polygon &poly, geo::Coord gx, geo::Coord gy, std::vector<geo::Point> &heap)
{
  geo::Polygon pnew;

  for (size_t i = 0; i < poly.holes () + 1; ++i) {

    heap.clear ();

    geo::Polygon::polygon_contour_iterator b, e;

    if (i == 0) {
      b = poly.begin_hull ();
      e = poly.end_hull ();
    } else {
      b = poly.begin_hole ((unsigned int)  (i - 1));
      e = poly.end_hole ((unsigned int)  (i - 1));
    }

    for (geo::Polygon::polygon_contour_iterator pt = b; pt != e; ++pt) {
      heap.push_back (geo::Point (snap_to_grid ((*pt).x (), gx), snap_to_grid ((*pt).y (), gy)));
    }

    if (i == 0) {
      pnew.assign_hull (heap.begin (), heap.end ());
    } else {
      pnew.insert_hole (heap.begin (), heap.end ());
    }

  }

  return pnew;
}

geo::Polygon
scaled_and_snapped_polygon (const geo::Polygon &poly, geo::Coord gx, geo::Coord mx, geo::Coord dx, geo::Coord ox, geo::Coord gy, geo::Coord my, geo::Coord dy, geo::Coord oy, std::vector<geo::Point> &heap)
{
  geo::Polygon pnew;

  int64_t dgx = int64_t (gx) * int64_t (dx);
  int64_t dgy = int64_t (gy) * int64_t (dy);

  for (size_t i = 0; i < poly.holes () + 1; ++i) {

    heap.clear ();

    geo::Polygon::polygon_contour_iterator b, e;

    if (i == 0) {
      b = poly.begin_hull ();
      e = poly.end_hull ();
    } else {
      b = poly.begin_hole ((unsigned int)  (i - 1));
      e = poly.end_hole ((unsigned int)  (i - 1));
    }

    for (geo::Polygon::polygon_contour_iterator pt = b; pt != e; ++pt) {
      int64_t x = snap_to_grid (int64_t ((*pt).x ()) * mx + int64_t (ox), dgx) / int64_t (dx);
      int64_t y = snap_to_grid (int64_t ((*pt).y ()) * my + int64_t (oy), dgy) / int64_t (dy);
      heap.push_back (geo::Point (geo::Coord (x), geo::Coord (y)));
    }

    if (i == 0) {
      pnew.assign_hull (heap.begin (), heap.end ());
    } else {
      pnew.insert_hole (heap.begin (), heap.end ());
    }

  }

  return pnew;
}

geo::Vector
scaled_and_snapped_vector (const geo::Vector &v, geo::Coord gx, geo::Coord mx, geo::Coord dx, geo::Coord ox, geo::Coord gy, geo::Coord my, geo::Coord dy, geo::Coord oy)
{
  int64_t dgx = int64_t (gx) * int64_t (dx);
  int64_t dgy = int64_t (gy) * int64_t (dy);

  int64_t x = snap_to_grid (int64_t (v.x ()) * mx + int64_t (ox), dgx) / int64_t (dx);
  int64_t y = snap_to_grid (int64_t (v.y ()) * my + int64_t (oy), dgy) / int64_t (dy);

  return geo::Vector (geo::Coord (x), geo::Coord (y));
}

}
