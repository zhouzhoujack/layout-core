#ifndef HDR_geoRegionDelegate
#define HDR_geoRegionDelegate

#include "geoCommon.h"

#include "geoPolygon.h"
#include "geoEdges.h"
#include "geoTexts.h"
#include "geoEdgePairs.h"
#include "geoEdgePairRelations.h"
#include "geoShapeCollection.h"
#include "geoGenericShapeIterator.h"
#include "geoRegionLocalOperations.h"
#include "geoHash.h"

#include <list>
#include <unordered_set>

namespace geo {

class RecursiveShapeIterator;
class EdgeFilterBase;
class EdgesDelegate;
class EdgePairsDelegate;
class CompoundRegionOperationNode;

/**
 *  @brief A base class for polygon filters
 */
class DB_PUBLIC PolygonFilterBase
{
public:
  /**
   *  @brief Constructor
   */
  PolygonFilterBase () { }

  virtual ~PolygonFilterBase () { }

  /**
   *  @brief Filters the polygon
   *  If this method returns true, the polygon is kept. Otherwise it's discarded.
   */
  virtual bool selected (const geo::Polygon &polygon) const = 0;

  /**
   *  @brief Filters the polygon reference
   *  If this method returns true, the polygon is kept. Otherwise it's discarded.
   */
  virtual bool selected (const geo::PolygonRef &polygon) const = 0;

  /**
   *  @brief Filters the set of polygons (taking the overall properties)
   *  If this method returns true, the polygon is kept. Otherwise it's discarded.
   */
  virtual bool selected_set (const std::unordered_set<geo::Polygon> &polygons) const = 0;

  /**
   *  @brief Filters the set of polygon references (taking the overall properties)
   *  If this method returns true, the polygon is kept. Otherwise it's discarded.
   */
  virtual bool selected_set (const std::unordered_set<geo::PolygonRef> &polygons) const = 0;

  /**
   *  @brief Returns the transformation reducer for building cell variants
   *  This method may return 0. In this case, not cell variants are built.
   */
  virtual const TransformationReducer *vars () const = 0;

  /**
   *  @brief Returns true, if the filter wants raw (not merged) input
   */
  virtual bool requires_raw_input () const = 0;

  /**
   *  @brief Returns true, if the filter wants to build variants
   *  If not true, the filter accepts shape propagation as variant resolution.
   */
  virtual bool wants_variants () const = 0;
};

/**
 *  @brief A template base class for polygon processors
 *
 *  A polygon processor can turn a polygon into something else.
 */
template <class Result>
class DB_PUBLIC polygon_processor
{
public:
  /**
   *  @brief Constructor
   */
  polygon_processor () { }

  /**
   *  @brief Destructor
   */
  virtual ~polygon_processor () { }

  /**
   *  @brief Performs the actual processing
   *  This method will take the input polygon from "polygon" and puts the results into "res".
   *  "res" can be empty - in this case, the polygon will be skipped.
   */
  virtual void process (const geo::Polygon &polygon, std::vector<Result> &res) const = 0;

  /**
   *  @brief Returns the transformation reducer for building cell variants
   *  This method may return 0. In this case, not cell variants are built.
   */
  virtual const TransformationReducer *vars () const = 0;

  /**
   *  @brief Returns true, if the result of this operation can be regarded "merged" always.
   */
  virtual bool result_is_merged () const = 0;

  /**
   *  @brief Returns true, if the result of this operation must not be merged.
   *  This feature can be used, if the result represents "degenerated" objects such
   *  as point-like edges. These must not be merged. Otherwise they disappear.
   */
  virtual bool result_must_not_be_merged () const = 0;

  /**
   *  @brief Returns true, if the processor wants raw (not merged) input
   */
  virtual bool requires_raw_input () const = 0;

  /**
   *  @brief Returns true, if the processor wants to build variants
   *  If not true, the processor accepts shape propagation as variant resolution.
   */
  virtual bool wants_variants () const = 0;
};

typedef shape_collection_processor<geo::Polygon, geo::Polygon> PolygonProcessorBase;
typedef shape_collection_processor<geo::Polygon, geo::Edge> PolygonToEdgeProcessorBase;
typedef shape_collection_processor<geo::Polygon, geo::EdgePair> PolygonToEdgePairProcessorBase;

/**
 *  @brief The region iterator delegate
 */
typedef geo::generic_shape_iterator_delegate_base <geo::Polygon> RegionIteratorDelegate;

/**
 *  @brief The delegate for the actual region implementation
 */
class DB_PUBLIC RegionDelegate
  : public geo::ShapeCollectionDelegateBase
{
public:
  typedef geo::Coord coord_type;
  typedef geo::coord_traits<geo::Coord> coord_traits;
  typedef geo::Polygon polygon_type;
  typedef geo::Vector vector_type;
  typedef geo::Point point_type;
  typedef geo::Box box_type;
  typedef coord_traits::distance_type distance_type;
  typedef coord_traits::perimeter_type perimeter_type;
  typedef coord_traits::area_type area_type;

  RegionDelegate ();
  virtual ~RegionDelegate ();

  RegionDelegate (const RegionDelegate &other);
  RegionDelegate &operator= (const RegionDelegate &other);

  virtual RegionDelegate *clone () const = 0;

  void set_base_verbosity (int vb);
  int base_verbosity () const
  {
    return m_base_verbosity;
  }

  void enable_progress (const std::string &progress_desc);
  void disable_progress ();

  void set_min_coherence (bool f);
  bool min_coherence () const
  {
    return m_merge_min_coherence;
  }

  void set_merged_semantics (bool f);
  bool merged_semantics () const
  {
    return m_merged_semantics;
  }

  void set_strict_handling (bool f);
  bool strict_handling () const
  {
    return m_strict_handling;
  }

  virtual std::string to_string (size_t nmax) const = 0;

  virtual RegionIteratorDelegate *begin () const = 0;
  virtual RegionIteratorDelegate *begin_merged () const = 0;

  virtual std::pair<geo::RecursiveShapeIterator, geo::ICplxTrans> begin_iter () const = 0;
  virtual std::pair<geo::RecursiveShapeIterator, geo::ICplxTrans> begin_merged_iter () const = 0;

  virtual bool empty () const = 0;
  virtual bool is_box () const = 0;
  virtual bool is_merged () const = 0;
  virtual size_t hier_count () const = 0;
  virtual size_t count () const = 0;

  virtual area_type area (const geo::Box &box) const = 0;
  virtual perimeter_type perimeter (const geo::Box &box) const = 0;
  virtual Box bbox () const = 0;

  virtual EdgePairsDelegate *cop_to_edge_pairs (geo::CompoundRegionOperationNode &node) = 0;
  virtual RegionDelegate *cop_to_region (geo::CompoundRegionOperationNode &node) = 0;
  virtual EdgesDelegate *cop_to_edges (geo::CompoundRegionOperationNode &node) = 0;

  virtual EdgePairsDelegate *width_check (geo::Coord d, const RegionCheckOptions &options) const = 0;
  virtual EdgePairsDelegate *space_check (geo::Coord d, const RegionCheckOptions &options) const = 0;
  virtual EdgePairsDelegate *isolated_check (geo::Coord d, const RegionCheckOptions &options) const = 0;
  virtual EdgePairsDelegate *notch_check (geo::Coord d, const RegionCheckOptions &options) const = 0;
  virtual EdgePairsDelegate *enclosing_check (const Region &other, geo::Coord d, const RegionCheckOptions &options) const = 0;
  virtual EdgePairsDelegate *overlap_check (const Region &other, geo::Coord d, const RegionCheckOptions &options) const = 0;
  virtual EdgePairsDelegate *separation_check (const Region &other, geo::Coord d, const RegionCheckOptions &options) const = 0;
  virtual EdgePairsDelegate *inside_check (const Region &other, geo::Coord d, const RegionCheckOptions &options) const = 0;
  virtual EdgePairsDelegate *grid_check (geo::Coord gx, geo::Coord gy) const = 0;
  virtual EdgePairsDelegate *angle_check (double min, double max, bool inverse) const = 0;

  virtual RegionDelegate *snapped_in_place (geo::Coord gx, geo::Coord gy) = 0;
  virtual RegionDelegate *snapped (geo::Coord gx, geo::Coord gy) = 0;
  virtual RegionDelegate *scaled_and_snapped_in_place (geo::Coord gx, geo::Coord mx, geo::Coord dx, geo::Coord gy, geo::Coord my, geo::Coord dy) = 0;
  virtual RegionDelegate *scaled_and_snapped (geo::Coord gx, geo::Coord mx, geo::Coord dx, geo::Coord gy, geo::Coord my, geo::Coord dy) = 0;

  virtual EdgesDelegate *edges (const EdgeFilterBase *filter) const = 0;
  virtual RegionDelegate *filter_in_place (const PolygonFilterBase &filter) = 0;
  virtual RegionDelegate *filtered (const PolygonFilterBase &filter) const = 0;
  virtual RegionDelegate *process_in_place (const PolygonProcessorBase &filter) = 0;
  virtual RegionDelegate *processed (const PolygonProcessorBase &filter) const = 0;
  virtual EdgesDelegate *processed_to_edges (const PolygonToEdgeProcessorBase &filter) const = 0;
  virtual EdgePairsDelegate *processed_to_edge_pairs (const PolygonToEdgePairProcessorBase &filter) const = 0;

  virtual RegionDelegate *merged_in_place () = 0;
  virtual RegionDelegate *merged_in_place (bool min_coherence, unsigned int min_wc) = 0;
  virtual RegionDelegate *merged () const = 0;
  virtual RegionDelegate *merged (bool min_coherence, unsigned int min_wc) const = 0;

  virtual RegionDelegate *sized (coord_type d, unsigned int mode) const = 0;
  virtual RegionDelegate *sized (coord_type dx, coord_type dy, unsigned int mode) const = 0;

  virtual RegionDelegate *and_with (const Region &other) const = 0;
  virtual RegionDelegate *not_with (const Region &other) const = 0;
  virtual RegionDelegate *xor_with (const Region &other) const = 0;
  virtual RegionDelegate *or_with (const Region &other) const = 0;
  virtual RegionDelegate *add_in_place (const Region &other) = 0;
  virtual RegionDelegate *add (const Region &other) const = 0;
  virtual std::pair<RegionDelegate *, RegionDelegate *> andnot_with (const Region &other) const = 0;

  virtual RegionDelegate *selected_outside (const Region &other) const = 0;
  virtual RegionDelegate *selected_not_outside (const Region &other) const = 0;
  virtual std::pair<RegionDelegate *, RegionDelegate *> selected_outside_pair (const Region &other) const = 0;
  virtual RegionDelegate *selected_inside (const Region &other) const = 0;
  virtual RegionDelegate *selected_not_inside (const Region &other) const = 0;
  virtual std::pair<RegionDelegate *, RegionDelegate *> selected_inside_pair (const Region &other) const = 0;
  virtual RegionDelegate *selected_enclosing (const Region &other, size_t min_count, size_t max_count) const = 0;
  virtual RegionDelegate *selected_not_enclosing (const Region &other, size_t min_count, size_t max_count) const = 0;
  virtual std::pair<RegionDelegate *, RegionDelegate *> selected_enclosing_pair (const Region &other, size_t min_count, size_t max_count) const = 0;
  virtual RegionDelegate *selected_interacting (const Region &other, size_t min_count, size_t max_count) const = 0;
  virtual RegionDelegate *selected_not_interacting (const Region &other, size_t min_count, size_t max_count) const = 0;
  virtual std::pair<RegionDelegate *, RegionDelegate *> selected_interacting_pair (const Region &other, size_t min_count, size_t max_count) const = 0;
  virtual RegionDelegate *selected_interacting (const Edges &other, size_t min_count, size_t max_count) const = 0;
  virtual RegionDelegate *selected_not_interacting (const Edges &other, size_t min_count, size_t max_count) const = 0;
  virtual std::pair<RegionDelegate *, RegionDelegate *> selected_interacting_pair (const Edges &other, size_t min_count, size_t max_count) const = 0;
  virtual RegionDelegate *selected_interacting (const Texts &other, size_t min_count, size_t max_count) const = 0;
  virtual RegionDelegate *selected_not_interacting (const Texts &other, size_t min_count, size_t max_count) const = 0;
  virtual std::pair<RegionDelegate *, RegionDelegate *> selected_interacting_pair (const Texts &other, size_t min_count, size_t max_count) const = 0;
  virtual RegionDelegate *selected_overlapping (const Region &other, size_t min_count, size_t max_count) const = 0;
  virtual RegionDelegate *selected_not_overlapping (const Region &other, size_t min_count, size_t max_count) const = 0;
  virtual std::pair<RegionDelegate *, RegionDelegate *> selected_overlapping_pair (const Region &other, size_t min_count, size_t max_count) const = 0;
  virtual RegionDelegate *pull_inside (const Region &other) const = 0;
  virtual RegionDelegate *pull_interacting (const Region &other) const = 0;
  virtual EdgesDelegate *pull_interacting (const Edges &other) const = 0;
  virtual RegionDelegate *pull_overlapping (const Region &other) const = 0;
  virtual TextsDelegate *pull_interacting (const Texts &other) const = 0;
  virtual RegionDelegate *in (const Region &other, bool invert) const = 0;

  virtual const geo::Polygon *nth (size_t n) const = 0;
  virtual bool has_valid_polygons () const = 0;
  virtual bool has_valid_merged_polygons () const = 0;

  virtual const geo::RecursiveShapeIterator *iter () const = 0;

  virtual bool equals (const Region &other) const = 0;
  virtual bool less (const Region &other) const = 0;

  virtual void insert_into (Layout *layout, geo::cell_index_type into_cell, unsigned int into_layer) const = 0;

  const std::string &progress_desc () const
  {
    return m_progress_desc;
  }

  bool report_progress () const
  {
    return m_report_progress;
  }

protected:
  virtual void merged_semantics_changed () { }
  virtual void min_coherence_changed () { }

private:
  bool m_merged_semantics;
  bool m_strict_handling;
  bool m_merge_min_coherence;
  bool m_report_progress;
  std::string m_progress_desc;
  int m_base_verbosity;
};

}

#endif

