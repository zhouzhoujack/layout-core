#ifndef HDR_geoDeepEdges
#define HDR_geoDeepEdges


#include "geoMutableEdges.h"
#include "geoDeepShapeStore.h"
#include "geoEdgeBoolean.h"
#include "geoEdgePairs.h"

namespace geo {

class Edges;
class DeepRegion;

/**
 *  @brief Provides hierarchical edges implementation
 */
class DeepEdges
  : public geo::MutableEdges, public geo::DeepShapeCollectionDelegateBase
{
public:
  DeepEdges ();
  DeepEdges (const geo::Edges &other, DeepShapeStore &dss);
  DeepEdges (const RecursiveShapeIterator &si, DeepShapeStore &dss, bool as_edges = true);
  DeepEdges (const RecursiveShapeIterator &si, DeepShapeStore &dss, const geo::ICplxTrans &trans, bool as_edges = true, bool merged_semantics = true);

  DeepEdges (const DeepEdges &other);
  DeepEdges (const DeepLayer &dl);

  virtual ~DeepEdges ();

  virtual void do_transform (const geo::Trans &t);
  virtual void do_transform (const geo::ICplxTrans &t);
  virtual void do_transform (const geo::IMatrix2d &t);
  virtual void do_transform (const geo::IMatrix3d &t);

  virtual void flatten ();

  virtual void reserve (size_t n);

  virtual void do_insert (const geo::Edge &edge);

  EdgesDelegate *clone () const;

  virtual EdgesIteratorDelegate *begin () const;
  virtual EdgesIteratorDelegate *begin_merged () const;

  virtual std::pair<geo::RecursiveShapeIterator, geo::ICplxTrans> begin_iter () const;
  virtual std::pair<geo::RecursiveShapeIterator, geo::ICplxTrans> begin_merged_iter () const;

  virtual bool empty () const;
  virtual bool is_merged () const;

  virtual const geo::Edge *nth (size_t n) const;
  virtual bool has_valid_edges () const;
  virtual bool has_valid_merged_edges () const;

  virtual const geo::RecursiveShapeIterator *iter () const;

  virtual bool equals (const Edges &other) const;
  virtual bool less (const Edges &other) const;

  virtual size_t count () const;
  virtual size_t hier_count () const;
  virtual Box bbox () const;

  virtual DeepEdges::length_type length (const geo::Box &) const;

  virtual std::string to_string (size_t nmax) const;

  virtual EdgePairsDelegate *width_check (geo::Coord d, const geo::EdgesCheckOptions &options) const
  {
    return run_check (geo::WidthRelation, 0, d, options);
  }
    
  virtual EdgePairsDelegate *space_check (geo::Coord d, const geo::EdgesCheckOptions &options) const
  {
    return run_check (geo::SpaceRelation, 0, d, options);
  }

  virtual EdgePairsDelegate *enclosing_check (const Edges &other, geo::Coord d, const geo::EdgesCheckOptions &options) const
  {
    return run_check (geo::OverlapRelation, &other, d, options);
  }

  virtual EdgePairsDelegate *overlap_check (const Edges &other, geo::Coord d, const geo::EdgesCheckOptions &options) const
  {
    return run_check (geo::WidthRelation, &other, d, options);
  }

  virtual EdgePairsDelegate *separation_check (const Edges &other, geo::Coord d, const geo::EdgesCheckOptions &options) const
  {
    return run_check (geo::SpaceRelation, &other, d, options);
  }

  virtual EdgePairsDelegate *inside_check (const Edges &other, geo::Coord d, const geo::EdgesCheckOptions &options) const
  {
    return run_check (geo::InsideRelation, &other, d, options);
  }

  virtual EdgesDelegate *filter_in_place (const EdgeFilterBase &filter);
  virtual EdgesDelegate *filtered (const EdgeFilterBase &) const;
  virtual EdgesDelegate *process_in_place (const EdgeProcessorBase &);
  virtual EdgesDelegate *processed (const EdgeProcessorBase &) const;
  virtual EdgePairsDelegate *processed_to_edge_pairs (const EdgeToEdgePairProcessorBase &filter) const;
  virtual RegionDelegate *processed_to_polygons (const EdgeToPolygonProcessorBase &filter) const;

  virtual EdgesDelegate *merged_in_place ();
  virtual EdgesDelegate *merged () const;

  virtual EdgesDelegate *and_with (const Edges &other) const;
  virtual EdgesDelegate *and_with (const Region &other) const;

  virtual EdgesDelegate *not_with (const Edges &other) const;
  virtual EdgesDelegate *not_with (const Region &other) const;

  virtual EdgesDelegate *xor_with (const Edges &other) const;

  virtual EdgesDelegate *or_with (const Edges &other) const;

  virtual EdgesDelegate *add_in_place (const Edges &other);
  virtual EdgesDelegate *add (const Edges &other) const;

  virtual EdgesDelegate *intersections (const Edges &other) const;

  virtual EdgesDelegate *inside_part (const Region &other) const;
  virtual EdgesDelegate *outside_part (const Region &other) const;

  virtual RegionDelegate *extended (coord_type ext_b, coord_type ext_e, coord_type ext_o, coord_type ext_i, bool join) const;

  virtual EdgesDelegate *in (const Edges &, bool) const;

  virtual void insert_into (Layout *layout, geo::cell_index_type into_cell, unsigned int into_layer) const;

  virtual DeepShapeCollectionDelegateBase *deep ()
  {
    return this;
  }

  void set_is_merged (bool f);

protected:
  virtual void merged_semantics_changed ();

private:
  friend class DeepRegion;

  DeepEdges &operator= (const DeepEdges &other);

  mutable DeepLayer m_merged_edges;
  mutable bool m_merged_edges_valid;
  bool m_is_merged;

  void init ();
  void ensure_merged_edges_valid () const;
  const DeepLayer &merged_deep_layer () const;
  DeepLayer and_or_not_with(const DeepEdges *other, EdgeBoolOp op) const;
  DeepLayer edge_region_op (const DeepRegion *other, bool outside, bool include_borders) const;
  EdgePairsDelegate *run_check (geo::edge_relation_type rel, const Edges *other, geo::Coord d, const geo::EdgesCheckOptions &options) const;
  virtual EdgesDelegate *pull_generic (const Edges &edges) const;
  virtual RegionDelegate *pull_generic (const Region &region) const;
  virtual EdgesDelegate *selected_interacting_generic (const Edges &edges, bool invert) const;
  virtual EdgesDelegate *selected_interacting_generic (const Region &region, bool invert) const;
  DeepEdges *apply_filter (const EdgeFilterBase &filter) const;

  template <class Result, class OutputContainer> OutputContainer *processed_impl (const edge_processor<Result> &filter) const;
};

}

#endif

