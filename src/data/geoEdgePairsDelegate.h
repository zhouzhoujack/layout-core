#ifndef HDR_geoEdgePairsDelegate
#define HDR_geoEdgePairsDelegate

#include "geoEdgePair.h"
#include "geoShapeCollection.h"
#include "geoShapeCollectionUtils.h"
#include "geoGenericShapeIterator.h"

namespace geo {

class RecursiveShapeIterator;
class EdgePairs;
class EdgePairFilterBase;
class RegionDelegate;
class EdgesDelegate;
class Layout;

typedef shape_collection_processor<geo::EdgePair, geo::Polygon> EdgePairToPolygonProcessorBase;
typedef shape_collection_processor<geo::EdgePair, geo::Edge> EdgePairToEdgeProcessorBase;

class 
EdgePairToPolygonProcessor
  : public EdgePairToPolygonProcessorBase
{
public:
  EdgePairToPolygonProcessor (geo::Coord e)
    : m_e (e)
  { }

  void process(const EdgePair &ep, std::vector<geo::Polygon> &res) const
  {
    geo::Polygon poly = ep.normalized ().to_polygon (m_e);
    if (poly.vertices () >= 3) {
      res.push_back (poly);
    }
  }

private:
  geo::Coord m_e;
};

class 
EdgePairToEdgesProcessor
  : public EdgePairToEdgeProcessorBase
{
public:
  EdgePairToEdgesProcessor ()
  { }

  void process(const EdgePair &ep, std::vector<geo::Edge> &res) const
  {
    res.push_back (ep.first ());
    res.push_back (ep.second ());
  }
};

class 
EdgePairToFirstEdgesProcessor
  : public EdgePairToEdgeProcessorBase
{
public:
  EdgePairToFirstEdgesProcessor ()
  { }

  void process(const EdgePair &ep, std::vector<geo::Edge> &res) const
  {
    res.push_back (ep.first ());
    if (ep.is_symmetric ()) {
      res.push_back (ep.second ());
    }
  }
};

class 
EdgePairToSecondEdgesProcessor
  : public EdgePairToEdgeProcessorBase
{
public:
  EdgePairToSecondEdgesProcessor ()
  { }

  void process(const EdgePair &ep, std::vector<geo::Edge> &res) const
  {
    if (! ep.is_symmetric ()) {
      res.push_back (ep.second ());
    }
  }
};

class 
EdgePairToLesserEdgesProcessor
  : public EdgePairToEdgeProcessorBase
{
public:
  EdgePairToLesserEdgesProcessor ()
  { }

  void process(const EdgePair &ep, std::vector<geo::Edge> &res) const
  {
    res.push_back (ep.lesser ());
  }
};

class 
EdgePairToGreaterEdgesProcessor
  : public EdgePairToEdgeProcessorBase
{
public:
  EdgePairToGreaterEdgesProcessor ()
  { }

  void process(const EdgePair &ep, std::vector<geo::Edge> &res) const
  {
    res.push_back (ep.greater ());
  }
};

/**
 *  @brief The edge pair set iterator delegate
 */
typedef geo::generic_shape_iterator_delegate_base <geo::EdgePair> EdgePairsIteratorDelegate;

/**
 *  @brief The delegate for the actual edge set implementation
 */
class EdgePairsDelegate
  : public ShapeCollectionDelegateBase
{
public:
  typedef geo::Coord coord_type;
  typedef geo::coord_traits<geo::Coord> coord_traits;
  typedef geo::EdgePair edge_pair_type;
  typedef geo::Vector vector_type;
  typedef geo::Point point_type;
  typedef geo::Box box_type;

  EdgePairsDelegate ();
  virtual ~EdgePairsDelegate ();

  EdgePairsDelegate (const EdgePairsDelegate &other);
  EdgePairsDelegate &operator= (const EdgePairsDelegate &other);

  virtual EdgePairsDelegate *clone () const = 0;

  void enable_progress (const std::string &progress_desc);
  void disable_progress ();

  //  dummy features to harmonize the interface of region, edges and edge pair delegates
  void set_merged_semantics (bool) { }
  bool merged_semantics () const { return false; }
  void set_is_merged (bool) { }
  bool is_merged () const { return false; }

  virtual std::string to_string (size_t nmax) const = 0;

  virtual EdgePairsIteratorDelegate *begin () const = 0;
  virtual std::pair<geo::RecursiveShapeIterator, geo::ICplxTrans> begin_iter () const = 0;

  virtual bool empty () const = 0;
  virtual size_t count () const = 0;
  virtual size_t hier_count () const = 0;

  virtual Box bbox () const = 0;

  virtual EdgePairsDelegate *filter_in_place (const EdgePairFilterBase &filter) = 0;
  virtual EdgePairsDelegate *filtered (const EdgePairFilterBase &filter) const = 0;
  virtual RegionDelegate *processed_to_polygons (const EdgePairToPolygonProcessorBase &filter) const = 0;
  virtual EdgesDelegate *processed_to_edges (const EdgePairToEdgeProcessorBase &filter) const = 0;

  virtual RegionDelegate *polygons (geo::Coord e) const = 0;
  virtual EdgesDelegate *edges () const = 0;
  virtual EdgesDelegate *first_edges () const = 0;
  virtual EdgesDelegate *second_edges () const = 0;

  virtual EdgePairsDelegate *add_in_place (const EdgePairs &other) = 0;
  virtual EdgePairsDelegate *add (const EdgePairs &other) const = 0;

  virtual EdgePairsDelegate *in (const EdgePairs &other, bool invert) const = 0;

  virtual const geo::EdgePair *nth (size_t n) const = 0;
  virtual bool has_valid_edge_pairs () const = 0;

  virtual const geo::RecursiveShapeIterator *iter () const = 0;

  virtual bool equals (const EdgePairs &other) const = 0;
  virtual bool less (const EdgePairs &other) const = 0;

  virtual void insert_into (Layout *layout, geo::cell_index_type into_cell, unsigned int into_layer) const = 0;
  virtual void insert_into_as_polygons (Layout *layout, geo::cell_index_type into_cell, unsigned int into_layer, geo::Coord enl) const = 0;

protected:
  const std::string &progress_desc () const
  {
    return m_progress_desc;
  }

  bool report_progress () const
  {
    return m_report_progress;
  }

private:
  bool m_report_progress;
  std::string m_progress_desc;
};

}

#endif

