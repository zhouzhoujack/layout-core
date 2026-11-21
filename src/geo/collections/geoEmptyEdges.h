#ifndef HDR_geoEmptyEdges
#define HDR_geoEmptyEdges

#include "geoCommon.h"
#include "geoEdgesDelegate.h"
#include "geoRecursiveShapeIterator.h"

namespace geo {

/**
 *  @brief An empty Edges
 */
class DB_PUBLIC EmptyEdges
  : public EdgesDelegate
{
public:
  EmptyEdges ();
  virtual ~EmptyEdges ();

  EmptyEdges (const EmptyEdges &other);
  EdgesDelegate *clone () const;

  virtual EdgesIteratorDelegate *begin () const { return 0; }
  virtual EdgesIteratorDelegate *begin_merged () const { return 0; }

  virtual std::pair<geo::RecursiveShapeIterator, geo::ICplxTrans> begin_iter () const { return std::make_pair (geo::RecursiveShapeIterator (), geo::ICplxTrans ()); }
  virtual std::pair<geo::RecursiveShapeIterator, geo::ICplxTrans> begin_merged_iter () const { return std::make_pair (geo::RecursiveShapeIterator (), geo::ICplxTrans ()); }

  virtual bool empty () const { return true; }
  virtual size_t count () const { return 0; }
  virtual size_t hier_count () const { return 0; }
  virtual std::string to_string (size_t) const { return std::string (); }
  virtual bool is_merged () const { return true; }
  virtual distance_type length (const geo::Box &) const { return 0; }
  virtual Box bbox () const { return geo::Box (); }

  virtual EdgePairsDelegate *width_check (geo::Coord, const geo::EdgesCheckOptions &) const;
  virtual EdgePairsDelegate *space_check (geo::Coord, const geo::EdgesCheckOptions &) const;
  virtual EdgePairsDelegate *enclosing_check (const Edges &, geo::Coord, const geo::EdgesCheckOptions &) const;
  virtual EdgePairsDelegate *overlap_check (const Edges &, geo::Coord, const geo::EdgesCheckOptions &) const;
  virtual EdgePairsDelegate *separation_check (const Edges &, geo::Coord, const geo::EdgesCheckOptions &) const;
  virtual EdgePairsDelegate *inside_check (const Edges &, geo::Coord, const geo::EdgesCheckOptions &) const;

  virtual EdgesDelegate *filter_in_place (const EdgeFilterBase &) { return this; }
  virtual EdgesDelegate *filtered (const EdgeFilterBase &) const { return new EmptyEdges (); }
  virtual EdgesDelegate *process_in_place (const EdgeProcessorBase &) { return this; }
  virtual EdgesDelegate *processed (const EdgeProcessorBase &) const { return new EmptyEdges (); }
  virtual EdgePairsDelegate *processed_to_edge_pairs (const EdgeToEdgePairProcessorBase &) const;
  virtual RegionDelegate *processed_to_polygons (const EdgeToPolygonProcessorBase &) const;

  virtual EdgesDelegate *merged_in_place () { return this; }
  virtual EdgesDelegate *merged () const { return new EmptyEdges (); }

  virtual EdgesDelegate *and_with (const Edges &) const { return new EmptyEdges (); }
  virtual EdgesDelegate *and_with (const Region &) const { return new EmptyEdges (); }
  virtual EdgesDelegate *not_with (const Edges &) const { return new EmptyEdges (); }
  virtual EdgesDelegate *not_with (const Region &) const { return new EmptyEdges (); }
  virtual EdgesDelegate *xor_with (const Edges &other) const;
  virtual EdgesDelegate *or_with (const Edges &other) const;
  virtual EdgesDelegate *add_in_place (const Edges &other);
  virtual EdgesDelegate *add (const Edges &other) const;
  virtual EdgesDelegate *intersections (const Edges &) const { return new EmptyEdges (); }

  virtual RegionDelegate *extended (coord_type, coord_type, coord_type, coord_type, bool) const;

  virtual EdgesDelegate *inside_part (const Region &) const { return new EmptyEdges (); }
  virtual EdgesDelegate *outside_part (const Region &) const { return new EmptyEdges (); }
  virtual RegionDelegate *pull_interacting (const Region &) const;
  virtual EdgesDelegate *pull_interacting (const Edges &) const { return new EmptyEdges (); }
  virtual EdgesDelegate *selected_interacting (const Edges &) const { return new EmptyEdges (); }
  virtual EdgesDelegate *selected_not_interacting (const Edges &) const { return new EmptyEdges (); }
  virtual EdgesDelegate *selected_interacting (const Region &) const { return new EmptyEdges (); }
  virtual EdgesDelegate *selected_not_interacting (const Region &) const { return new EmptyEdges (); }

  virtual EdgesDelegate *in (const Edges &, bool) const { return new EmptyEdges (); }

  virtual const geo::Edge *nth (size_t) const { tl_assert (false); }
  virtual bool has_valid_edges () const { return true; }
  virtual bool has_valid_merged_edges () const { return true; }

  virtual const geo::RecursiveShapeIterator *iter () const { return 0; }

  virtual bool equals (const Edges &other) const;
  virtual bool less (const Edges &other) const;

  virtual void insert_into (Layout *, geo::cell_index_type, unsigned int) const { }

private:
  EmptyEdges &operator= (const EmptyEdges &other);
};

}  // namespace geo

#endif

