#include "geoEmptyRegion.h"
#include "geoEmptyEdges.h"
#include "geoEmptyEdgePairs.h"
#include "geoRegion.h"

namespace geo
{

// -------------------------------------------------------------------------------------------------------------
//  EmptyRegion implementation

EmptyRegion::EmptyRegion ()
{
  //  .. nothing yet ..
}

EmptyRegion::~EmptyRegion ()
{
  //  .. nothing yet ..
}

EmptyRegion::EmptyRegion (const EmptyRegion &other)
  : RegionDelegate (other)
{
  // .. nothing yet ..
}

RegionDelegate *
EmptyRegion::clone () const
{
  return new EmptyRegion (*this);
}

RegionDelegate *
EmptyRegion::add_in_place (const Region &other)
{
  return add (other);
}

RegionDelegate *
EmptyRegion::add (const Region &other) const
{
  return other.delegate ()->clone ();
}

RegionDelegate *
EmptyRegion::xor_with (const Region &other) const
{
  return or_with (other);
}

RegionDelegate *
EmptyRegion::or_with (const Region &other) const
{
  if (other.empty ()) {
    return new EmptyRegion ();
  } else if (! other.strict_handling ()) {
    return other.delegate ()->clone ();
  } else {
    return other.delegate ()->merged ();
  }
}

EdgesDelegate *
EmptyRegion::processed_to_edges (const PolygonToEdgeProcessorBase &) const
{
  return new EmptyEdges ();
}

EdgePairsDelegate *
EmptyRegion::processed_to_edge_pairs (const PolygonToEdgePairProcessorBase &) const
{
  return new EmptyEdgePairs ();
}

EdgePairsDelegate *
EmptyRegion::cop_to_edge_pairs (geo::CompoundRegionOperationNode &)
{
  return new EmptyEdgePairs ();
}

RegionDelegate *
EmptyRegion::cop_to_region (geo::CompoundRegionOperationNode &)
{
  return new EmptyRegion ();
}

EdgesDelegate *
EmptyRegion::cop_to_edges (geo::CompoundRegionOperationNode &)
{
  return new EmptyEdges ();
}

EdgePairsDelegate *
EmptyRegion::width_check (geo::Coord, const RegionCheckOptions &) const
{
  return new EmptyEdgePairs ();
}

EdgePairsDelegate *
EmptyRegion::space_check (geo::Coord, const RegionCheckOptions &) const
{
  return new EmptyEdgePairs ();
}

EdgePairsDelegate *
EmptyRegion::isolated_check (geo::Coord, const RegionCheckOptions &) const
{
  return new EmptyEdgePairs ();
}

EdgePairsDelegate *
EmptyRegion::notch_check (geo::Coord, const RegionCheckOptions &) const
{
  return new EmptyEdgePairs ();
}

EdgePairsDelegate *
EmptyRegion::enclosing_check (const Region &, geo::Coord, const RegionCheckOptions &) const
{
  return new EmptyEdgePairs ();
}

EdgePairsDelegate *
EmptyRegion::overlap_check (const Region &, geo::Coord, const RegionCheckOptions &) const
{
  return new EmptyEdgePairs ();
}

EdgePairsDelegate *
EmptyRegion::separation_check (const Region &, geo::Coord, const RegionCheckOptions &) const
{
  return new EmptyEdgePairs ();
}

EdgePairsDelegate *
EmptyRegion::inside_check (const Region &, geo::Coord, const RegionCheckOptions &) const
{
  return new EmptyEdgePairs ();
}

EdgePairsDelegate *
EmptyRegion::grid_check (geo::Coord, geo::Coord) const
{
  return new EmptyEdgePairs ();
}

EdgePairsDelegate *
EmptyRegion::angle_check (double, double, bool) const
{
  return new EmptyEdgePairs ();
}

EdgesDelegate *
EmptyRegion::edges (const EdgeFilterBase *) const
{
  return new EmptyEdges ();
}

bool
EmptyRegion::equals (const Region &other) const
{
  return other.empty ();
}

bool
EmptyRegion::less (const Region &other) const
{
  return other.empty () ? false : true;
}

}

