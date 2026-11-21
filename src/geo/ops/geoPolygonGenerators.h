#ifndef HDR_geoPolygonGenerators
#define HDR_geoPolygonGenerators


#include "geoTypes.h"
#include "geoEdge.h"
#include "geoEdgeProcessor.h"
#include "geoPolygon.h"

#include <vector>

namespace geo
{

class PGPolyContour;
class PGContourList;
struct PGPoint;
class PolygonSink;
class SimplePolygonSink;

/**
 *  @brief Forms polygons from a edge set
 *
 *  This class implements EdgeSink. It builds polygons from the edges delivered to it 
 *  and outputs the polygons to another receiver (PolygonSink). 
 *  The way how touching corners are resolved can be specified (minimum and maximum coherence).
 *  In addition, it can be specified if the resulting polygons contain holes are whether the 
 *  holes are attached to the hull contour by stich lines.
 */
class PolygonGenerator
  : public EdgeSink
{
public:
  typedef std::list <PGPoint> open_map_type;
  typedef open_map_type::iterator open_map_iterator_type;

  /**
   *  @brief Constructor
   *
   *  This constructor takes the polygon receiver (of which is keeps a reference).
   *  It allows one to specify how holes are resolved and how touching corners are resolved.
   *
   *  @param psink The polygon receiver
   *  @param resolve_holes true, if holes should be resolved into the hull using stich lines
   *  @param min_coherence true, if the resulting polygons should be minimized (less holes, more polygons)
   */
  PolygonGenerator (PolygonSink &psink, bool resolve_holes = true, bool min_coherence = true); 

  /**
   *  @brief Constructor
   *
   *  This constructor takes the simple polygon receiver (of which is keeps a reference).
   *  It allows one to specify how touching corners are resolved. Holes are always resolved since this is
   *  the only way to create a simple polygon.
   *
   *  @param spsink The simple polygon receiver
   *  @param resolve_holes true, if holes should be resolved into the hull using stich lines
   *  @param min_coherence true, if the resulting polygons should be minimized (less holes, more polygons)
   */
  PolygonGenerator (SimplePolygonSink &spsink, bool min_coherence = true); 

  /**
   *  @brief Destructor
   */
  ~PolygonGenerator ();

  /**
   *  @brief Implementation of the EdgeSink interface
   */
  virtual void start ();

  /**
   *  @brief Implementation of the EdgeSink interface
   */
  virtual void flush ();

  /**
   *  @brief Implementation of the EdgeSink interface
   */
  virtual void begin_scanline (geo::Coord y);

  /**
   *  @brief Implementation of the EdgeSink interface
   */
  virtual void end_scanline (geo::Coord y);

  /**
   *  @brief Implementation of the EdgeSink interface
   */
  virtual void crossing_edge (const geo::Edge &e);

  /**
   *  @brief Implementation of the EdgeSink interface
   */
  virtual void skip_n (size_t n);

  /**
   *  @brief Implementation of the EdgeSink interface
   */
  virtual void put (const geo::Edge &e);

  /**
   *  @brief Sets the way how holes are resolved dynamically
   *
   *  This property should not be changed why polygons are created (between start and flush)
   */
  void resolve_holes (bool f) { m_resolve_holes = f; }

  /**
   *  @brief Enables open contours for hole resolution
   *
   *  With this property set to false (the default), holes are resolved with a single stitch
   *  line. This will create self-touching polygons finally. By setting this property to true,
   *  a different hole resolution strategy is chosen which resolves holes by inserting a new
   *  contour at the left of the hole.
   *
   *  Using is feature will result in a larger number but less complex polygons on output.
   */
  void open_contours (bool f) { m_open_contours = f; }

  /**
   *  @brief Sets the way how touching corners are resolved dynamically
   *
   *  This property should not be changed while polygons are created (between start and flush)
   */
  void min_coherence (bool f) { m_min_coherence = f; }

  /**
   *  @brief Disables or enable compression for polygon contours
   *
   *  If compression is disabled, no vertices will be dropped.
   */
  void enable_compression (bool enable) { m_compress = enable; }

  /**
   *  @brief Disables or enable compression for polygon contours
   *
   *  This method switches the global flag and is intended for regression test purposes only!
   */
  static void enable_compression_global (bool enable) { ms_compress = enable; }

private:
  PGContourList *mp_contours;
  open_map_type m_open;
  geo::Coord m_y;
  open_map_iterator_type m_open_pos;
  PolygonSink *mp_psink;
  SimplePolygonSink *mp_spsink;
  bool m_resolve_holes;
  bool m_open_contours;
  bool m_min_coherence;
  geo::Polygon m_poly;
  geo::SimplePolygon m_spoly;
  static bool ms_compress;
  bool m_compress;

  void join_contours (geo::Coord x);
  void produce_poly (const PGPolyContour &c);
  void eliminate_hole ();

  PolygonGenerator &operator= (const PolygonGenerator &);
  PolygonGenerator (const PolygonGenerator &);
};

/**
 *  @brief Forms trapezoids from an edge set
 *
 *  This class implements EdgeSink. It builds simple polygons from the edges delivered to it
 *  and outputs the polygons to another receiver (PolygonSink or SimplePolygonSink). The
 *  polygons created form a horizontal trapezoid decomposition of the full polygon.
 */
class TrapezoidGenerator
  : public EdgeSink
{
public:
  typedef std::vector <std::pair<geo::Edge, geo::Edge> > edge_map_type;
  typedef edge_map_type::iterator edge_map_type_iterator;

  /**
   *  @brief Constructor
   *
   *  This constructor takes the polygon receiver (of which is keeps a reference).
   *  The trapezoids will be delivered to this sink.
   *
   *  @param psink The polygon receiver
   */
  TrapezoidGenerator (PolygonSink &psink);

  /**
   *  @brief Constructor
   *
   *  This constructor takes the polygon receiver (of which is keeps a reference).
   *  The trapezoids will be delivered to this sink.
   *
   *  @param spsink The simple polygon receiver
   */
  TrapezoidGenerator (SimplePolygonSink &spsink);

  /**
   *  @brief Destructor
   */
  ~TrapezoidGenerator ();

  /**
   *  @brief Implementation of the EdgeSink interface
   */
  virtual void start ();

  /**
   *  @brief Implementation of the EdgeSink interface
   */
  virtual void flush ();

  /**
   *  @brief Implementation of the EdgeSink interface
   */
  virtual void begin_scanline (geo::Coord y);

  /**
   *  @brief Implementation of the EdgeSink interface
   */
  virtual void end_scanline (geo::Coord y);

  /**
   *  @brief Implementation of the EdgeSink interface
   */
  virtual void crossing_edge (const geo::Edge &e);

  /**
   *  @brief Implementation of the EdgeSink interface
   */
  virtual void skip_n (size_t n);

  /**
   *  @brief Implementation of the EdgeSink interface
   */
  virtual void put (const geo::Edge &e);

private:
  geo::Coord m_y;
  PolygonSink *mp_psink;
  SimplePolygonSink *mp_spsink;
  geo::Polygon m_poly;
  geo::SimplePolygon m_spoly;
  edge_map_type m_edges, m_new_edges;
  edge_map_type_iterator m_current_edge;
  std::vector<size_t> m_new_edge_refs;

  TrapezoidGenerator &operator= (const TrapezoidGenerator &);
  TrapezoidGenerator (const TrapezoidGenerator &);

  void make_trap (const geo::Point (&pts)[4]);
};

/**
 *  @brief Declaration of the simple polygon sink interface
 */
class SimplePolygonSink
{
public:
  /**
   *  @brief Constructor
   */
  SimplePolygonSink () { }

  /**
   *  @brief Destructor
   */
  virtual ~SimplePolygonSink () { }

  /**
   *  @brief Deliver a simple polygon
   *
   *  This method is called to deliver a new polygon
   */
  virtual void put (const geo::SimplePolygon & /*polygon*/) { }

  /**
   *  @brief Start event
   *
   *  This method is called before the first polygon is delivered. 
   *  The PolygonGenerator will simply forward the EdgeSink's start method to the
   *  polygon sink.
   */
  virtual void start () { }

  /**
   *  @brief End event
   *
   *  This method is called after the last polygon was delivered. 
   *  The PolygonGenerator will deliver all remaining polygons and call flush then.
   */
  virtual void flush () { }
};

/**
 *  @brief A simple polygon receiver collecting the simple polygons in a vector
 *
 *  This class implements the SimplePolygonSink interface.
 *  Like EdgeContainer, this receiver collects the objects either in an external
 *  or an internal vector of polygons.
 */
class SimplePolygonContainer
  : public SimplePolygonSink
{
public:
  /**
   *  @brief Constructor specifying an external vector for storing the polygons
   */
  SimplePolygonContainer (std::vector<geo::SimplePolygon> &polygons, bool clear = false) 
    : SimplePolygonSink (), mp_polygons (&polygons), m_clear (clear) 
  { }

  /**
   *  @brief Constructor which tells the container to use the internal vector for storing the polygons
   */
  SimplePolygonContainer () 
    : SimplePolygonSink (), mp_polygons (&m_polygons), m_clear (false) 
  { }

  /**
   *  @brief Start the sequence
   */
  virtual void start ()
  {
    if (m_clear) {
      mp_polygons->clear ();
      //  The single-shot scheme is a easy way to overcome problems with multiple start/flush brackets (i.e. on size filter)
      m_clear = false;
    }
  }

  /**
   *  @brief The polygons collected so far (const version)
   */
  const std::vector<geo::SimplePolygon> &polygons () const
  { 
    return *mp_polygons; 
  }

  /**
   *  @brief The polygons collected so far (non-const version)
   */
  std::vector<geo::SimplePolygon> &polygons () 
  { 
    return *mp_polygons; 
  }

  /**
   *  @brief Implementation of the PolygonSink interface
   */
  virtual void put (const geo::SimplePolygon &polygon) 
  {
    mp_polygons->push_back (polygon);
  }

private:
  std::vector<geo::SimplePolygon> m_polygons;
  std::vector<geo::SimplePolygon> *mp_polygons;
  bool m_clear;
};

/**
 *  @brief Declaration of the polygon sink interface
 */
class PolygonSink
{
public:
  /**
   *  @brief Constructor
   */
  PolygonSink () { }

  /**
   *  @brief Destructor
   */
  virtual ~PolygonSink () { }

  /**
   *  @brief Deliver a polygons
   *
   *  This method is called to deliver a new polygon
   */
  virtual void put (const geo::Polygon &) { }

  /**
   *  @brief Start event
   *
   *  This method is called before the first polygon is delivered. 
   *  The PolygonGenerator will simply forward the EdgeSink's start method to the
   *  polygon sink.
   */
  virtual void start () { }

  /**
   *  @brief End event
   *
   *  This method is called after the last polygon was delivered. 
   *  The PolygonGenerator will deliver all remaining polygons and call flush then.
   */
  virtual void flush () { }
};

/**
 *  @brief A polygon receiver collecting the polygons in a vector
 *
 *  This class implements the PolygonSink interface.
 *  Like EdgeContainer, this receiver collects the objects either in an external
 *  or an internal vector of polygons.
 */
class PolygonContainer
  : public PolygonSink
{
public:
  /**
   *  @brief Constructor specifying an external vector for storing the polygons
   */
  PolygonContainer (std::vector<geo::Polygon> &polygons, bool clear = false) 
    : PolygonSink (), mp_polygons (&polygons), m_clear (clear)
  { }

  /**
   *  @brief Constructor which tells the container to use the internal vector for storing the polygons
   */
  PolygonContainer () 
    : PolygonSink (), mp_polygons (&m_polygons), m_clear (false) 
  { }

  /**
   *  @brief The polygons collected so far (const version)
   */
  const std::vector<geo::Polygon> &polygons () const
  { 
    return *mp_polygons; 
  }

  /**
   *  @brief The polygons collected so far (non-const version)
   */
  std::vector<geo::Polygon> &polygons () 
  { 
    return *mp_polygons; 
  }

  /**
   *  @brief Start the sequence
   */
  virtual void start ()
  {
    if (m_clear) {
      mp_polygons->clear ();
      //  The single-shot scheme is a easy way to overcome problems with multiple start/flush brackets (i.e. on size filter)
      m_clear = false;
    }
  }

  /**
   *  @brief Implementation of the PolygonSink interface
   */
  virtual void put (const geo::Polygon &polygon) 
  {
    mp_polygons->push_back (polygon);
  }

private:
  std::vector<geo::Polygon> m_polygons;
  std::vector<geo::Polygon> *mp_polygons;
  bool m_clear;
};

/**
 *  @brief A polygon filter that sizes the polygons 
 *
 *  This class implements the PolygonSink interface and delivers the sized polygons to an EdgeSink.
 */
class SizingPolygonFilter
  : public PolygonSink
{
public:
  /**
   *  @brief Constructor 
   */
  SizingPolygonFilter (EdgeSink &output, Coord dx, Coord dy, unsigned int mode)
    : PolygonSink (), mp_output (&output), m_dx (dx), m_dy (dy), m_mode (mode)
  { }

  /**
   *  @brief Implementation of the PolygonSink interface
   */
  virtual void put (const geo::Polygon &polygon);

private:
  EdgeProcessor m_sizing_processor;
  EdgeSink *mp_output;
  Coord m_dx, m_dy;
  unsigned int m_mode;
};

}

#endif


