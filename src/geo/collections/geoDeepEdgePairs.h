#ifndef HDR_geoDeepEdgePairs
#define HDR_geoDeepEdgePairs


#include "geoMutableEdgePairs.h"
#include "geoDeepShapeStore.h"
#include "geoEdgePairs.h"

namespace geo {

/**
 *  @brief Provides hierarchical edges implementation
 */
class DeepEdgePairs
  : public geo::MutableEdgePairs, public geo::DeepShapeCollectionDelegateBase
{
public:
  DeepEdgePairs ();
  DeepEdgePairs (const RecursiveShapeIterator &si, DeepShapeStore &dss);
  DeepEdgePairs (const RecursiveShapeIterator &si, DeepShapeStore &dss, const geo::ICplxTrans &trans);

  DeepEdgePairs (const DeepEdgePairs &other);
  DeepEdgePairs (const DeepLayer &dl);

  virtual ~DeepEdgePairs ();

  EdgePairsDelegate *clone () const;

  virtual void do_insert (const geo::EdgePair &edge_pair);

  virtual void do_transform (const geo::Trans &t);
  virtual void do_transform (const geo::ICplxTrans &t);
  virtual void do_transform (const geo::IMatrix2d &t);
  virtual void do_transform (const geo::IMatrix3d &t);

  virtual void flatten ();

  virtual void reserve (size_t n);

  virtual EdgePairsIteratorDelegate *begin () const;
  virtual std::pair<geo::RecursiveShapeIterator, geo::ICplxTrans> begin_iter () const;

  virtual size_t count () const;
  virtual size_t hier_count () const;
  virtual std::string to_string (size_t) const;
  virtual Box bbox () const;
  virtual bool empty () const;
  virtual const geo::EdgePair *nth (size_t n) const;
  virtual bool has_valid_edge_pairs () const;
  virtual const geo::RecursiveShapeIterator *iter () const;

  virtual EdgePairsDelegate *filter_in_place (const EdgePairFilterBase &filter);
  virtual EdgePairsDelegate *filtered (const EdgePairFilterBase &) const;
  virtual RegionDelegate *processed_to_polygons (const EdgePairToPolygonProcessorBase &filter) const;
  virtual EdgesDelegate *processed_to_edges (const EdgePairToEdgeProcessorBase &filter) const;

  virtual EdgePairsDelegate *add_in_place (const EdgePairs &other);
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

  virtual DeepShapeCollectionDelegateBase *deep ()
  {
    return this;
  }

private:
  DeepEdgePairs &operator= (const DeepEdgePairs &other);

  void init ();
  EdgesDelegate *generic_edges (bool first, bool second) const;
  DeepEdgePairs *apply_filter (const EdgePairFilterBase &filter) const;
};

}

#endif

