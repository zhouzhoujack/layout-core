#ifndef HDR_geoEmptyEdgePairs
#define HDR_geoEmptyEdgePairs

#include "geoCommon.h"

#include "geoEdgePairsDelegate.h"
#include "geoRecursiveShapeIterator.h"

namespace geo {

/**
 *  @brief The delegate for the actual edge set implementation
 */
class DB_PUBLIC EmptyEdgePairs
  : public EdgePairsDelegate
{
public:
  EmptyEdgePairs ();
  EmptyEdgePairs (const EmptyEdgePairs &other);

  virtual EdgePairsDelegate *clone () const;

  virtual std::string to_string (size_t) const { return std::string (); }

  virtual EdgePairsIteratorDelegate *begin () const { return 0; }
  virtual std::pair<geo::RecursiveShapeIterator, geo::ICplxTrans> begin_iter () const { return std::make_pair (geo::RecursiveShapeIterator (), geo::ICplxTrans ()); }

  virtual bool empty () const { return true; }
  virtual size_t count () const { return 0; }
  virtual size_t hier_count () const { return 0; }

  virtual Box bbox () const { return Box (); }

  virtual EdgePairsDelegate *filter_in_place (const EdgePairFilterBase &) { return this; }
  virtual EdgePairsDelegate *filtered (const EdgePairFilterBase &) const { return new EmptyEdgePairs (); }
  virtual RegionDelegate *processed_to_polygons (const EdgePairToPolygonProcessorBase &filter) const;
  virtual EdgesDelegate *processed_to_edges (const EdgePairToEdgeProcessorBase &filter) const;

  virtual RegionDelegate *polygons (geo::Coord e) const;
  virtual EdgesDelegate *edges () const;
  virtual EdgesDelegate *first_edges () const;
  virtual EdgesDelegate *second_edges () const;

  virtual EdgePairsDelegate *add_in_place (const EdgePairs &other);
  virtual EdgePairsDelegate *add (const EdgePairs &other) const;

  virtual EdgePairsDelegate *in (const EdgePairs &, bool) const { return new EmptyEdgePairs (); }

  virtual const geo::EdgePair *nth (size_t) const { tl_assert (false); }
  virtual bool has_valid_edge_pairs () const { return true; }

  virtual const geo::RecursiveShapeIterator *iter () const { return 0; }

  virtual bool equals (const EdgePairs &other) const;
  virtual bool less (const EdgePairs &other) const;

  virtual void insert_into (Layout *, geo::cell_index_type, unsigned int) const { }
  virtual void insert_into_as_polygons (Layout *, geo::cell_index_type, unsigned int, geo::Coord) const { }

private:
  EmptyEdgePairs &operator= (const EmptyEdgePairs &other);
};

}

#endif

