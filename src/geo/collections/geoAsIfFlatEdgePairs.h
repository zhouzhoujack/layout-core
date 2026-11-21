#ifndef HDR_geoAsIfFlatEdgePairs
#define HDR_geoAsIfFlatEdgePairs

#include "geoCommon.h"

#include "geoEdgePairsDelegate.h"

namespace geo {

/**
 *  @brief Provides default flat implementations
 */
class DB_PUBLIC AsIfFlatEdgePairs
  : public EdgePairsDelegate
{
public:
  AsIfFlatEdgePairs ();
  AsIfFlatEdgePairs (const AsIfFlatEdgePairs &other);
  virtual ~AsIfFlatEdgePairs ();

  virtual size_t count () const;
  virtual size_t hier_count () const;
  virtual std::string to_string (size_t) const;
  virtual Box bbox () const;

  virtual EdgePairsDelegate *filter_in_place (const EdgePairFilterBase &filter)
  {
    return filtered (filter);
  }

  virtual EdgePairsDelegate *filtered (const EdgePairFilterBase &) const;

  virtual RegionDelegate *processed_to_polygons (const EdgePairToPolygonProcessorBase &filter) const;
  virtual EdgesDelegate *processed_to_edges (const EdgePairToEdgeProcessorBase &filter) const;

  virtual EdgePairsDelegate *add_in_place (const EdgePairs &other)
  {
    return add (other);
  }

  virtual EdgePairsDelegate *add (const EdgePairs &other) const;

  virtual RegionDelegate *polygons (geo::Coord e) const;
  virtual EdgesDelegate *edges () const;
  virtual EdgesDelegate *first_edges () const;
  virtual EdgesDelegate *second_edges () const;

  virtual EdgePairsDelegate *in (const EdgePairs &, bool) const;

  virtual bool equals (const EdgePairs &other) const;
  virtual bool less (const EdgePairs &other) const;

  virtual void insert_into (Layout *layout, geo::cell_index_type into_cell, unsigned int into_layer) const;
  virtual void insert_into_as_polygons (Layout *layout, geo::cell_index_type into_cell, unsigned int into_layer, geo::Coord enl) const;

protected:
  void update_bbox (const geo::Box &box);
  void invalidate_bbox ();

private:
  friend class DeepEdgePairs;

  AsIfFlatEdgePairs &operator= (const AsIfFlatEdgePairs &other);

  mutable bool m_bbox_valid;
  mutable geo::Box m_bbox;

  virtual geo::Box compute_bbox () const;
};

}

#endif

