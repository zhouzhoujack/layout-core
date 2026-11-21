#include "geoEmptyEdgePairs.h"
#include "geoEmptyRegion.h"
#include "geoEmptyEdges.h"

namespace geo
{

// -------------------------------------------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//  EmptyEdgePairs implementation

EmptyEdgePairs::EmptyEdgePairs ()
{
  //  .. nothing yet ..
}

EmptyEdgePairs::EmptyEdgePairs (const EmptyEdgePairs &other)
  : EdgePairsDelegate (other)
{
  // .. nothing yet ..
}

EdgePairsDelegate *
EmptyEdgePairs::clone () const
{
  return new EmptyEdgePairs (*this);
}

RegionDelegate *
EmptyEdgePairs::polygons (geo::Coord) const
{
  return new EmptyRegion ();
}

RegionDelegate *
EmptyEdgePairs::processed_to_polygons (const EdgePairToPolygonProcessorBase &) const
{
  return new EmptyRegion ();
}

EdgesDelegate *
EmptyEdgePairs::processed_to_edges (const EdgePairToEdgeProcessorBase &) const
{
  return new EmptyEdges ();
}

EdgesDelegate *
EmptyEdgePairs::edges () const
{
  return new EmptyEdges ();
}

EdgesDelegate *
EmptyEdgePairs::first_edges () const
{
  return new EmptyEdges ();
}

EdgesDelegate *
EmptyEdgePairs::second_edges () const
{
  return new EmptyEdges ();
}

EdgePairsDelegate *
EmptyEdgePairs::add_in_place (const EdgePairs &other)
{
  return add (other);
}

EdgePairsDelegate *
EmptyEdgePairs::add (const EdgePairs &other) const
{
  return other.delegate ()->clone ();
}

bool 
EmptyEdgePairs::equals (const EdgePairs &other) const
{
  return other.empty ();
}

bool 
EmptyEdgePairs::less (const EdgePairs &other) const
{
  return other.empty () ? false : true;
}

}

