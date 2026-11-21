#include "geoFlatEdgePairs.h"
#include "geoEmptyEdgePairs.h"
#include "geoEdgePairs.h"

namespace geo
{

// -------------------------------------------------------------------------------------------------------------
//  FlatEdgePairs implementation

FlatEdgePairs::FlatEdgePairs ()
  : MutableEdgePairs (), mp_edge_pairs (new geo::Shapes (false))
{
  //  .. nothing yet ..
}

FlatEdgePairs::~FlatEdgePairs ()
{
  //  .. nothing yet ..
}

FlatEdgePairs::FlatEdgePairs (const FlatEdgePairs &other)
  : MutableEdgePairs (other), mp_edge_pairs (other.mp_edge_pairs)
{
  //  .. nothing yet ..
}

FlatEdgePairs::FlatEdgePairs (const geo::Shapes &edge_pairs)
  : MutableEdgePairs (), mp_edge_pairs (new geo::Shapes (edge_pairs))
{
  //  .. nothing yet ..
}

void FlatEdgePairs::invalidate_cache ()
{
  invalidate_bbox ();
}

void FlatEdgePairs::reserve (size_t n)
{
  mp_edge_pairs->reserve (geo::EdgePair::tag (), n);
}

EdgePairsIteratorDelegate *FlatEdgePairs::begin () const
{
  return new FlatEdgePairsIterator (mp_edge_pairs.get_const ());
}

std::pair<geo::RecursiveShapeIterator, geo::ICplxTrans> FlatEdgePairs::begin_iter () const
{
  return std::make_pair (geo::RecursiveShapeIterator (*mp_edge_pairs), geo::ICplxTrans ());
}

bool FlatEdgePairs::empty () const
{
  return mp_edge_pairs->empty ();
}

size_t FlatEdgePairs::count () const
{
  return mp_edge_pairs->size ();
}

size_t FlatEdgePairs::hier_count () const
{
  return mp_edge_pairs->size ();
}

Box FlatEdgePairs::compute_bbox () const
{
  mp_edge_pairs->update_bbox ();
  return mp_edge_pairs->bbox ();
}

EdgePairsDelegate *
FlatEdgePairs::filter_in_place (const EdgePairFilterBase &filter)
{
  geo::Shapes &ep = *mp_edge_pairs;

  edge_pair_iterator_type pw = ep.get_layer<geo::EdgePair, geo::unstable_layer_tag> ().begin ();
  for (EdgePairsIterator p (begin ()); ! p.at_end (); ++p) {
    if (filter.selected (*p)) {
      if (pw == ep.get_layer<geo::EdgePair, geo::unstable_layer_tag> ().end ()) {
        ep.get_layer<geo::EdgePair, geo::unstable_layer_tag> ().insert (*p);
        pw = ep.get_layer<geo::EdgePair, geo::unstable_layer_tag> ().end ();
      } else {
        ep.get_layer<geo::EdgePair, geo::unstable_layer_tag> ().replace (pw++, *p);
      }
    }
  }

  ep.get_layer<geo::EdgePair, geo::unstable_layer_tag> ().erase (pw, ep.get_layer<geo::EdgePair, geo::unstable_layer_tag> ().end ());

  return this;
}

EdgePairsDelegate *FlatEdgePairs::add (const EdgePairs &other) const
{
  std::unique_ptr<FlatEdgePairs> new_edge_pairs (new FlatEdgePairs (*this));
  new_edge_pairs->invalidate_cache ();

  FlatEdgePairs *other_flat = dynamic_cast<FlatEdgePairs *> (other.delegate ());
  if (other_flat) {

    new_edge_pairs->raw_edge_pairs ().insert (other_flat->raw_edge_pairs ().get_layer<geo::EdgePair, geo::unstable_layer_tag> ().begin (), other_flat->raw_edge_pairs ().get_layer<geo::EdgePair, geo::unstable_layer_tag> ().end ());

  } else {

    size_t n = new_edge_pairs->raw_edge_pairs ().size ();
    for (EdgePairsIterator p (other.begin ()); ! p.at_end (); ++p) {
      ++n;
    }

    new_edge_pairs->raw_edge_pairs ().reserve (geo::EdgePair::tag (), n);

    for (EdgePairsIterator p (other.begin ()); ! p.at_end (); ++p) {
      new_edge_pairs->raw_edge_pairs ().insert (*p);
    }

  }

  return new_edge_pairs.release ();
}

EdgePairsDelegate *FlatEdgePairs::add_in_place (const EdgePairs &other)
{
  invalidate_cache ();

  geo::Shapes &ep = *mp_edge_pairs;

  FlatEdgePairs *other_flat = dynamic_cast<FlatEdgePairs *> (other.delegate ());
  if (other_flat) {

    ep.insert (other_flat->raw_edge_pairs ().get_layer<geo::EdgePair, geo::unstable_layer_tag> ().begin (), other_flat->raw_edge_pairs ().get_layer<geo::EdgePair, geo::unstable_layer_tag> ().end ());

  } else {

    size_t n = ep.size ();
    for (EdgePairsIterator p (other.begin ()); ! p.at_end (); ++p) {
      ++n;
    }

    ep.reserve (geo::EdgePair::tag (), n);

    for (EdgePairsIterator p (other.begin ()); ! p.at_end (); ++p) {
      ep.insert (*p);
    }

  }

  return this;
}

const geo::EdgePair *FlatEdgePairs::nth (size_t n) const
{
  return n < mp_edge_pairs->size () ? &mp_edge_pairs->get_layer<geo::EdgePair, geo::unstable_layer_tag> ().begin () [n] : 0;
}

bool FlatEdgePairs::has_valid_edge_pairs () const
{
  return true;
}

const geo::RecursiveShapeIterator *FlatEdgePairs::iter () const
{
  return 0;
}

void
FlatEdgePairs::insert_into_as_polygons (Layout *layout, geo::cell_index_type into_cell, unsigned int into_layer, geo::Coord enl) const
{
  geo::Shapes &out = layout->cell (into_cell).shapes (into_layer);
  for (EdgePairsIterator p (begin ()); ! p.at_end (); ++p) {
    out.insert (p->normalized ().to_simple_polygon (enl));
  }
}

void
FlatEdgePairs::insert_into (Layout *layout, geo::cell_index_type into_cell, unsigned int into_layer) const
{
  layout->cell (into_cell).shapes (into_layer).insert (*mp_edge_pairs);
}

void
FlatEdgePairs::do_insert (const geo::EdgePair &ep)
{
  mp_edge_pairs->insert (ep);
  invalidate_cache ();
}

}

