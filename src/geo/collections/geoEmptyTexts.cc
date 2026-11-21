#include "geoEmptyTexts.h"
#include "geoEmptyRegion.h"
#include "geoEmptyEdges.h"
#include "geoTexts.h"

namespace geo
{

// -------------------------------------------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//  EmptyTexts implementation

EmptyTexts::EmptyTexts ()
{
  //  .. nothing yet ..
}

EmptyTexts::EmptyTexts (const EmptyTexts &other)
  : TextsDelegate (other)
{
  // .. nothing yet ..
}

TextsDelegate *
EmptyTexts::clone () const
{
  return new EmptyTexts (*this);
}

RegionDelegate *
EmptyTexts::polygons (geo::Coord) const
{
  return new EmptyRegion ();
}

RegionDelegate *
EmptyTexts::processed_to_polygons (const TextToPolygonProcessorBase &) const
{
  return new EmptyRegion ();
}

EdgesDelegate *
EmptyTexts::edges () const
{
  return new EmptyEdges ();
}

TextsDelegate *
EmptyTexts::add_in_place (const Texts &other)
{
  return add (other);
}

TextsDelegate *
EmptyTexts::add (const Texts &other) const
{
  return other.delegate ()->clone ();
}

bool 
EmptyTexts::equals (const Texts &other) const
{
  return other.empty ();
}

bool 
EmptyTexts::less (const Texts &other) const
{
  return other.empty () ? false : true;
}

RegionDelegate *
EmptyTexts::pull_interacting (const Region &) const
{
  return new EmptyRegion ();
}

TextsDelegate *
EmptyTexts::selected_interacting (const Region &) const
{
  return new EmptyTexts ();
}

TextsDelegate *
EmptyTexts::selected_not_interacting (const Region &) const
{
  return new EmptyTexts ();
}

}

