#include "geoEmptyEdges.h"
#include "geoEmptyEdgePairs.h"
#include "geoEmptyRegion.h"
#include "geoEdges.h"

namespace geo
{

// -------------------------------------------------------------------------------------------------------------
//  EmptyEdges implementation

EmptyEdges::EmptyEdges ()
{
  //  .. nothing yet ..
}

EmptyEdges::~EmptyEdges ()
{
  //  .. nothing yet ..
}

EmptyEdges::EmptyEdges (const EmptyEdges &other)
  : EdgesDelegate (other)
{
  // .. nothing yet ..
}

EdgesDelegate *
EmptyEdges::clone () const
{
  return new EmptyEdges (*this);
}

RegionDelegate *
EmptyEdges::extended (coord_type, coord_type, coord_type, coord_type, bool) const
{
  return new EmptyRegion ();
}

RegionDelegate *
EmptyEdges::pull_interacting (const Region &) const
{
  return new EmptyRegion ();
}

EdgePairsDelegate *
EmptyEdges::processed_to_edge_pairs (const EdgeToEdgePairProcessorBase &) const
{
  return new EmptyEdgePairs ();
}

RegionDelegate *
EmptyEdges::processed_to_polygons (const EdgeToPolygonProcessorBase &) const
{
  return new EmptyRegion ();
}

EdgePairsDelegate *
EmptyEdges::width_check (geo::Coord, const geo::EdgesCheckOptions &) const
{
  return new EmptyEdgePairs ();
}

EdgePairsDelegate *
EmptyEdges::space_check (geo::Coord, const geo::EdgesCheckOptions &) const
{
  return new EmptyEdgePairs ();
}

EdgePairsDelegate *
EmptyEdges::enclosing_check (const Edges &, geo::Coord, const geo::EdgesCheckOptions &) const
{
  return new EmptyEdgePairs ();
}

EdgePairsDelegate *
EmptyEdges::overlap_check (const Edges &, geo::Coord, const geo::EdgesCheckOptions &) const
{
  return new EmptyEdgePairs ();
}

EdgePairsDelegate *
EmptyEdges::separation_check (const Edges &, geo::Coord, const geo::EdgesCheckOptions &) const
{
  return new EmptyEdgePairs ();
}

EdgePairsDelegate *
EmptyEdges::inside_check (const Edges &, geo::Coord, const geo::EdgesCheckOptions &) const
{
  return new EmptyEdgePairs ();
}

EdgesDelegate *
EmptyEdges::add_in_place (const Edges &other)
{
  return add (other);
}

EdgesDelegate *
EmptyEdges::add (const Edges &other) const
{
  return other.delegate ()->clone ();
}

EdgesDelegate *
EmptyEdges::xor_with (const Edges &other) const
{
  return or_with (other);
}

EdgesDelegate *
EmptyEdges::or_with (const Edges &other) const
{
  if (other.empty ()) {
    return new EmptyEdges ();
  } else if (! other.strict_handling ()) {
    return other.delegate ()->clone ();
  } else {
    return other.delegate ()->merged ();
  }
}

bool
EmptyEdges::equals (const Edges &other) const
{
  return other.empty ();
}

bool
EmptyEdges::less (const Edges &other) const
{
  return other.empty () ? false : true;
}

}

