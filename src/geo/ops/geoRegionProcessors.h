#ifndef HDR_geoRegionProcessors
#define HDR_geoRegionProcessors

#include "geoCommon.h"
#include "geoRegionDelegate.h"
#include "geoPolygonTools.h"
#include "geoEdgesUtils.h"

namespace geo
{

// -----------------------------------------------------------------------------------
//  Corner detection

/**
 *  @brief An interface to accept corners
 */
class DB_PUBLIC CornerPointDelivery
{
public:
  virtual void make_point (const geo::Point &pt, const geo::Edge &e1, const geo::Edge &e2) const = 0;
};

/**
 *  @brief An interface to accept corners and turns them into rectangles with 2*dim x 2*dim
 */
class DB_PUBLIC CornerRectDelivery
  : public CornerPointDelivery
{
public:
  CornerRectDelivery (geo::Coord dim, std::vector<geo::Polygon> &result)
    : m_d (dim, dim), mp_result (&result)
  { }

  virtual void make_point (const geo::Point &pt, const geo::Edge &, const geo::Edge &) const
  {
    mp_result->push_back (geo::Polygon (geo::Box (pt - m_d, pt + m_d)));
  }

private:
  geo::Vector m_d;
  std::vector<geo::Polygon> *mp_result;
};

/**
 *  @brief An interface to accept corners and turns them into degenerated edges (dots)
 */
class DB_PUBLIC CornerDotDelivery
  : public CornerPointDelivery
{
public:
  CornerDotDelivery (std::vector<geo::Edge> &result)
    : mp_result (&result)
  { }

  virtual void make_point (const geo::Point &pt, const geo::Edge &, const geo::Edge &) const
  {
    mp_result->push_back (geo::Edge (pt, pt));
  }

private:
  geo::Vector m_d;
  std::vector<geo::Edge> *mp_result;
};

/**
 *  @brief An interface to accept corners and turns them into edge pairs
 */
class DB_PUBLIC CornerEdgePairDelivery
  : public CornerPointDelivery
{
public:
  CornerEdgePairDelivery (std::vector<geo::EdgePair> &result)
    : mp_result (&result)
  { }

  virtual void make_point (const geo::Point &, const geo::Edge &e1, const geo::Edge &e2) const
  {
    mp_result->push_back (geo::EdgePair (e1, e2));
  }

private:
  std::vector<geo::EdgePair> *mp_result;
};

/**
 *  @brief A helper class implementing the core corner detection algorithm
 */
class DB_PUBLIC CornerDetectorCore
{
public:
  CornerDetectorCore (double angle_start, bool include_angle_start, double angle_end, bool include_angle_end);
  virtual ~CornerDetectorCore () { }

  void detect_corners (const geo::Polygon &poly, const CornerPointDelivery &delivery) const;

private:
  geo::EdgeAngleChecker m_checker;
};

/**
 *  @brief A corner detector delivering small retangles (2*dim x 2*dim) per detected corner
 */
class DB_PUBLIC CornersAsRectangles
  : public geo::PolygonProcessorBase, private CornerDetectorCore
{
public:
  CornersAsRectangles (double angle_start, bool include_angle_start, double angle_end, bool include_angle_end, geo::Coord dim = 1)
    : CornerDetectorCore (angle_start, include_angle_start, angle_end, include_angle_end), m_dim (dim)
  {
    //  .. nothing yet ..
  }

  void process (const geo::Polygon &poly, std::vector<geo::Polygon> &result) const
  {
    detect_corners (poly, CornerRectDelivery (m_dim, result));
  }

  virtual const TransformationReducer *vars () const { return &m_vars; }
  virtual bool result_is_merged () const { return false; } //  overlaps may happen
  virtual bool result_must_not_be_merged () const { return false; }
  virtual bool requires_raw_input () const { return false; }
  virtual bool wants_variants () const { return false; }

private:
  geo::Coord m_dim;
  geo::MagnificationReducer m_vars;
};

/**
 *  @brief A corner detector delivering degenerated edges (dots) for the corners
 */
class DB_PUBLIC CornersAsDots
  : public geo::PolygonToEdgeProcessorBase, private CornerDetectorCore
{
public:
  CornersAsDots (double angle_start, bool include_angle_start, double angle_end, bool include_angle_end)
    : CornerDetectorCore (angle_start, include_angle_start, angle_end, include_angle_end)
  {
    //  .. nothing yet ..
  }

  void process (const geo::Polygon &poly, std::vector<geo::Edge> &result) const
  {
    detect_corners (poly, CornerDotDelivery (result));
  }

  virtual const TransformationReducer *vars () const { return 0; }
  virtual bool result_is_merged () const { return false; }
  virtual bool result_must_not_be_merged () const { return true; }  //  to preserve dots
  virtual bool requires_raw_input () const { return false; }
  virtual bool wants_variants () const { return false; }
};

/**
 *  @brief A corner detector delivering edge pairs for the corners
 */
class DB_PUBLIC CornersAsEdgePairs
  : public geo::PolygonToEdgePairProcessorBase, private CornerDetectorCore
{
public:
  CornersAsEdgePairs (double angle_start, bool include_angle_start, double angle_end, bool include_angle_end)
    : CornerDetectorCore (angle_start, include_angle_start, angle_end, include_angle_end)
  {
    //  .. nothing yet ..
  }

  void process (const geo::Polygon &poly, std::vector<geo::EdgePair> &result) const
  {
    detect_corners (poly, CornerEdgePairDelivery (result));
  }

  virtual const TransformationReducer *vars () const { return 0; }
  virtual bool result_is_merged () const { return false; }
  virtual bool result_must_not_be_merged () const { return true; }  //  to preserve dots
  virtual bool requires_raw_input () const { return false; }
  virtual bool wants_variants () const { return false; }
};

// -----------------------------------------------------------------------------------
//  Extents

/**
 *  @brief A processor delivering the extents (bounding box) of the merged polygons
 */
class DB_PUBLIC Extents
  : public geo::PolygonProcessorBase
{
public:
  Extents ()
  {
    //  .. nothing yet ..
  }

  void process (const geo::Polygon &poly, std::vector<geo::Polygon> &result) const;

  virtual const TransformationReducer *vars () const { return 0; }
  virtual bool result_is_merged () const { return false; }
  virtual bool result_must_not_be_merged () const { return false; }
  virtual bool requires_raw_input () const { return false; }
  virtual bool wants_variants () const { return false; }  //  variants are too common, so don't do this
};

/**
 *  @brief A processor delivering the relative extents (bounding box) of the merged polygons
 *  This processor allows over- or undersizing of the resulting box by a given amount and delivery
 *  of a box relative to the original box.
 */
class DB_PUBLIC RelativeExtents
  : public geo::PolygonProcessorBase
{
public:
  RelativeExtents (double fx1, double fy1, double fx2, double fy2, geo::Coord dx, geo::Coord dy)
    : m_fx1 (fx1), m_fy1 (fy1), m_fx2 (fx2), m_fy2 (fy2), m_dx (dx), m_dy (dy)
  {
    //  .. nothing yet ..
  }

  void process (const geo::Polygon &poly, std::vector<geo::Polygon> &result) const;

  virtual const TransformationReducer *vars () const;
  virtual bool result_is_merged () const { return false; }
  virtual bool result_must_not_be_merged () const { return false; }
  virtual bool requires_raw_input () const { return false; }
  virtual bool wants_variants () const { return false; }  //  variants are too common, so don't do this

private:
  double m_fx1, m_fy1, m_fx2, m_fy2;
  geo::Coord m_dx, m_dy;
  geo::MagnificationAndOrientationReducer m_anisotropic_reducer;
  geo::MagnificationReducer m_isotropic_reducer;
};

/**
 *  @brief A processor delivers one edge per merged polygon
 *  The edge runs from the relative coordinate fx1, fy1 (0: left/bottom, 1: right/top) to
 *  fx2, fy2.
 *  This processor allows over- or undersizing of the resulting box by a given amount and delivery
 *  of a box relative to the original box.
 */
class DB_PUBLIC RelativeExtentsAsEdges
  : public geo::PolygonToEdgeProcessorBase
{
public:
  RelativeExtentsAsEdges (double fx1, double fy1, double fx2, double fy2)
    : m_fx1 (fx1), m_fy1 (fy1), m_fx2 (fx2), m_fy2 (fy2)
  {
    //  .. nothing yet ..
  }

  void process (const geo::Polygon &poly, std::vector<geo::Edge> &result) const;

  virtual const TransformationReducer *vars () const;
  virtual bool result_is_merged () const { return false; }
  virtual bool result_must_not_be_merged () const;
  virtual bool requires_raw_input () const { return false; }
  virtual bool wants_variants () const { return false; }  //  variants are too common, so don't do this

private:
  double m_fx1, m_fy1, m_fx2, m_fy2;
  geo::MagnificationAndOrientationReducer m_anisotropic_reducer;
};

/**
 *  @brief A processor that delivers all edges for a polygon
 */
class DB_PUBLIC PolygonToEdgeProcessor
  : public geo::PolygonToEdgeProcessorBase
{
public:
  PolygonToEdgeProcessor ()
  {
    //  .. nothing yet ..
  }

  void process (const geo::Polygon &poly, std::vector<geo::Edge> &result) const;
};

/**
 *  @brief A decomposition processor to deliver convex-only polygons
 */
class DB_PUBLIC ConvexDecomposition
  : public geo::PolygonProcessorBase
{
public:
  ConvexDecomposition (geo::PreferredOrientation mode)
    : m_mode (mode)
  {
    //  .. nothing yet ..
  }

  void process (const geo::Polygon &poly, std::vector<geo::Polygon> &result) const;

  virtual const TransformationReducer *vars () const { return &m_vars; }
  virtual bool result_is_merged () const { return false; }
  virtual bool result_must_not_be_merged () const { return true; }  //  would spoil the decomposition otherwise
  virtual bool requires_raw_input () const { return false; }
  virtual bool wants_variants () const { return true; }

private:
  geo::PreferredOrientation m_mode;
  geo::OrientationReducer m_vars;
};

/**
 *  @brief A decomposition processor to deliver trapezoids
 */
class DB_PUBLIC TrapezoidDecomposition
  : public geo::PolygonProcessorBase
{
public:
  TrapezoidDecomposition (geo::TrapezoidDecompositionMode mode)
    : m_mode (mode)
  {
    //  .. nothing yet ..
  }

  void process (const geo::Polygon &poly, std::vector<geo::Polygon> &result) const;

  virtual const TransformationReducer *vars () const { return &m_vars; }
  virtual bool result_is_merged () const { return false; }
  virtual bool result_must_not_be_merged () const { return true; }  //  would spoil the decomposition otherwise
  virtual bool requires_raw_input () const { return false; }
  virtual bool wants_variants () const { return true; }

private:
  geo::TrapezoidDecompositionMode m_mode;
  geo::OrientationReducer m_vars;
};

/**
 *  @brief A polygon breaker processor
 *  This processor reduces polygons with more than max_vertex_count vertices and
 *  an bbox-to-polygon area ratio bigger than max_area_ratio.
 *  A zero value for these parameters means "don't care".
 */
class DB_PUBLIC PolygonBreaker
  : public geo::PolygonProcessorBase
{
public:
  PolygonBreaker (size_t max_vertex_count, double max_area_ratio)
    : m_max_vertex_count (max_vertex_count), m_max_area_ratio (max_area_ratio)
  {
    //  .. nothing yet ..
  }

  void process (const geo::Polygon &poly, std::vector<geo::Polygon> &result) const;

  virtual const TransformationReducer *vars () const { return 0; }
  virtual bool result_is_merged () const { return false; }
  virtual bool result_must_not_be_merged () const { return true; }  //  would spoil the decomposition otherwise
  virtual bool requires_raw_input () const { return true; }  //  acts on original shapes
  virtual bool wants_variants () const { return false; }

private:
  size_t m_max_vertex_count;
  double m_max_area_ratio;
};

/**
 *  @brief A sizing processor
 */
class DB_PUBLIC PolygonSizer
  : public geo::PolygonProcessorBase
{
public:
  PolygonSizer (geo::Coord dx, geo::Coord dy, unsigned int mode);
  ~PolygonSizer ();

  virtual const TransformationReducer *vars () const { return m_vars; }

  void process (const geo::Polygon &poly, std::vector<geo::Polygon> &result) const;

  virtual bool result_is_merged () const;
  virtual bool result_must_not_be_merged () const { return false; }
  virtual bool requires_raw_input () const { return false; }
  virtual bool wants_variants () const { return true; }

private:
  TransformationReducer *m_vars;
  geo::Coord m_dx, m_dy;
  unsigned int m_mode;
};

/**
 *  @brief Computes the Minkowski sum between the polygons and the given object
 *  The object can be Edge, Polygon, Box and std::vector<Point>
 */
template <class K>
class DB_PUBLIC_TEMPLATE minkowski_sum_computation
  : public geo::PolygonProcessorBase
{
public:
  minkowski_sum_computation (const K &q)
    : m_q (q)
  {
    //  .. nothing yet ..
  }

  void process (const geo::Polygon &poly, std::vector<geo::Polygon> &result) const
  {
    result.push_back (geo::minkowski_sum (poly, m_q, false));
  }

  //  TODO: could be less if the object is symmetric
  virtual const TransformationReducer *vars () const { return &m_vars; }
  virtual bool result_is_merged () const { return false; }
  virtual bool result_must_not_be_merged () const { return false; }
  virtual bool requires_raw_input () const { return false; }
  virtual bool wants_variants () const { return true; }

private:
  K m_q;
  geo::MagnificationAndOrientationReducer m_vars;
};

}

#endif
