#ifndef HDR_geoAsIfFlatEdges
#define HDR_geoAsIfFlatEdges

#include "geoBoxScanner.h"
#include "geoEdgesDelegate.h"
#include "geoEdgeBoolean.h"
#include "geoBoxScanner.h"
#include "geoPolygonTools.h"

#include <map>
#include <vector>

namespace geo {

class PolygonSink;

/**
 *  @brief Provides default flat implementations
 */
class AsIfFlatEdges
  : public EdgesDelegate
{
public:
  AsIfFlatEdges ();
  virtual ~AsIfFlatEdges ();

  virtual size_t count () const;
  virtual size_t hier_count () const;
  virtual std::string to_string (size_t) const;
  virtual distance_type length (const geo::Box &) const;
  virtual Box bbox () const;

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

  virtual EdgesDelegate *process_in_place (const EdgeProcessorBase &filter)
  {
    return processed (filter);
  }

  virtual EdgesDelegate *processed (const EdgeProcessorBase &filter) const;
  virtual EdgePairsDelegate *processed_to_edge_pairs (const EdgeToEdgePairProcessorBase &) const;
  virtual RegionDelegate *processed_to_polygons (const EdgeToPolygonProcessorBase &) const;

  virtual EdgesDelegate *filter_in_place (const EdgeFilterBase &filter)
  {
    return filtered (filter);
  }

  virtual EdgesDelegate *filtered (const EdgeFilterBase &) const;

  virtual EdgesDelegate *merged_in_place ()
  {
    return merged ();
  }

  virtual EdgesDelegate *merged () const
  {
    return boolean (0, EdgeOr);
  }

  virtual EdgesDelegate *and_with (const Edges &other) const
  {
    return boolean (&other, EdgeAnd);
  }

  virtual EdgesDelegate *and_with (const Region &other) const
  {
    return edge_region_op (other, false /*inside*/, true /*include borders*/);
  }

  virtual EdgesDelegate *not_with (const Edges &other) const
  {
    return boolean (&other, EdgeNot);
  }

  virtual EdgesDelegate *not_with (const Region &other) const
  {
    return edge_region_op (other, true /*outside*/, true /*include borders*/);
  }

  virtual EdgesDelegate *xor_with (const Edges &other) const
  {
    return boolean (&other, EdgeXor);
  }

  virtual EdgesDelegate *or_with (const Edges &other) const
  {
    return boolean (&other, EdgeOr);
  }

  virtual EdgesDelegate *intersections (const Edges &other) const
  {
    return boolean (&other, EdgeIntersections);
  }

  virtual EdgesDelegate *add_in_place (const Edges &other)
  {
    return add (other);
  }

  virtual EdgesDelegate *add (const Edges &other) const;

  virtual EdgesDelegate *inside_part (const Region &other) const
  {
    return edge_region_op (other, false /*inside*/, false /*don't include borders*/);
  }

  virtual EdgesDelegate *outside_part (const Region &other) const
  {
    return edge_region_op (other, true /*outside*/, false /*don't include borders*/);
  }

  virtual RegionDelegate *extended (coord_type ext_b, coord_type ext_e, coord_type ext_o, coord_type ext_i, bool join) const;

  virtual EdgesDelegate *pull_interacting (const Edges &) const;
  virtual RegionDelegate *pull_interacting (const Region &) const;
  virtual EdgesDelegate *selected_interacting (const Edges &) const;
  virtual EdgesDelegate *selected_not_interacting (const Edges &) const;
  virtual EdgesDelegate *selected_interacting (const Region &) const;
  virtual EdgesDelegate *selected_not_interacting (const Region &) const;

  virtual EdgesDelegate *in (const Edges &, bool) const;

  virtual bool equals (const Edges &other) const;
  virtual bool less (const Edges &other) const;

  virtual void insert_into (Layout *layout, geo::cell_index_type into_cell, unsigned int into_layer) const;

protected:
  void update_bbox (const geo::Box &box);
  void invalidate_bbox ();
  EdgePairsDelegate *run_check (geo::edge_relation_type rel, const Edges *other, geo::Coord d, const EdgesCheckOptions &options) const;
  virtual EdgesDelegate *pull_generic (const Edges &edges) const;
  virtual RegionDelegate *pull_generic (const Region &region) const;
  virtual EdgesDelegate *selected_interacting_generic (const Edges &edges, bool inverse) const;
  virtual EdgesDelegate *selected_interacting_generic (const Region &region, bool inverse) const;
  AsIfFlatEdges &operator= (const AsIfFlatEdges &other);
  AsIfFlatEdges (const AsIfFlatEdges &other);

private:
  mutable bool m_bbox_valid;
  mutable geo::Box m_bbox;

  virtual geo::Box compute_bbox () const;
  EdgesDelegate *boolean (const Edges *other, EdgeBoolOp op) const;
  EdgesDelegate *edge_region_op (const Region &other, bool outside, bool include_borders) const;
};

}

#endif

