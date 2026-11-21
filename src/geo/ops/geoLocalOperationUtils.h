#ifndef HDR_geoLocalOperationUtils
#define HDR_geoLocalOperationUtils

#include "geoCommon.h"

#include "geoLayout.h"
#include "geoPolygonGenerators.h"
#include "geoHash.h"
#include "utilsThreads.h"

#include <unordered_set>

namespace geo
{

template <class Trans>
class polygon_transformation_filter
  : public PolygonSink
{
public:
  /**
   *  @brief Constructor specifying an external vector for storing the polygons
   */
  polygon_transformation_filter (PolygonSink *output, const Trans &tr)
    : mp_output (output), m_trans (tr)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Implementation of the PolygonSink interface
   */
  virtual void put (const geo::Polygon &polygon)
  {
    mp_output->put (polygon.transformed (m_trans));
  }

private:
  geo::PolygonSink *mp_output;
  const Trans m_trans;
};

template <class T>
class DB_PUBLIC polygon_ref_generator;

template <>
class DB_PUBLIC polygon_ref_generator<geo::PolygonRef>
  : public PolygonSink
{
public:
  /**
   *  @brief Constructor
   */
  polygon_ref_generator (geo::Layout *layout, std::unordered_set<geo::PolygonRef> &polyrefs)
    : PolygonSink (), mp_layout (layout), mp_polyrefs (&polyrefs)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Implementation of the PolygonSink interface
   */
  void put (const geo::Polygon &polygon)
  {
    geo::MutexLocker locker (&mp_layout->lock ());
    mp_polyrefs->insert (geo::PolygonRef (polygon, mp_layout->shape_repository ()));
  }

private:
  geo::Layout *mp_layout;
  std::unordered_set<geo::PolygonRef> *mp_polyrefs;
};

template <>
class DB_PUBLIC polygon_ref_generator<geo::Polygon>
  : public PolygonSink
{
public:
  /**
   *  @brief Constructor
   */
  polygon_ref_generator (geo::Layout *, std::unordered_set<geo::Polygon> &polygons)
    : mp_polygons (&polygons)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Implementation of the PolygonSink interface
   */
  virtual void put (const geo::Polygon &polygon)
  {
    mp_polygons->insert (polygon);
  }

private:
  std::unordered_set<geo::Polygon> *mp_polygons;
};

typedef polygon_ref_generator<geo::PolygonRef> PolygonRefGenerator;

class DB_PUBLIC EdgeToEdgeSetGenerator
  : public EdgeSink
{
public:
  /**
   *  @brief Constructor
   */
  EdgeToEdgeSetGenerator (std::unordered_set<geo::Edge> &edges);

  /**
   *  @brief Implementation of the PolygonSink interface
   */
  virtual void put (const geo::Edge &edge);

private:
  std::unordered_set<geo::Edge> *mp_edges;
};

class DB_PUBLIC PolygonRefToShapesGenerator
  : public PolygonSink
{
public:
  /**
   *  @brief Constructor specifying an external vector for storing the polygons
   */
  PolygonRefToShapesGenerator (geo::Layout *layout, geo::Shapes *shapes);

  /**
   *  @brief Implementation of the PolygonSink interface
   */
  virtual void put (const geo::Polygon &polygon);

private:
  geo::Layout *mp_layout;
  geo::Shapes *mp_shapes;
};

class DB_PUBLIC PolygonSplitter
  : public PolygonSink
{
public:
  PolygonSplitter (PolygonSink &sink, double max_area_ratio, size_t max_vertex_count);

  virtual void put (const geo::Polygon &poly);

  virtual void start () { mp_sink->start (); }
  virtual void flush () { mp_sink->flush (); }

private:
  PolygonSink *mp_sink;
  double m_max_area_ratio;
  size_t m_max_vertex_count;
};

}

#endif

