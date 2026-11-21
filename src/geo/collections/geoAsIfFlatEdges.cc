#include "geoAsIfFlatEdges.h"
#include "geoFlatEdges.h"
#include "geoFlatEdgePairs.h"
#include "geoFlatRegion.h"
#include "geoEmptyEdges.h"
#include "geoEdges.h"
#include "geoEdgesUtils.h"
#include "geoEdgeBoolean.h"
#include "geoBoxConvert.h"
#include "geoRegion.h"
#include "geoFlatRegion.h"
#include "geoEmptyRegion.h"
#include "geoPolygonTools.h"
#include "geoShapeProcessor.h"
#include "geoEdgeProcessor.h"
#include "geoPolygonGenerators.h"
#include "geoPolygon.h"
#include "geoPath.h"

#include <sstream>

namespace geo
{

// -------------------------------------------------------------------------------------------------------------
//  AsIfFlagEdges implementation

AsIfFlatEdges::AsIfFlatEdges ()
  : EdgesDelegate (), m_bbox_valid (false)
{
  //  .. nothing yet ..
}

AsIfFlatEdges::~AsIfFlatEdges ()
{
  //  .. nothing yet ..
}

AsIfFlatEdges::AsIfFlatEdges (const AsIfFlatEdges &other)
  : EdgesDelegate (other), m_bbox_valid (false)
{
  operator= (other);
}

AsIfFlatEdges &
AsIfFlatEdges::operator= (const AsIfFlatEdges &other)
{
  if (this != &other) {
    m_bbox_valid = other.m_bbox_valid;
    m_bbox = other.m_bbox;
  }
  return *this;
}

std::string
AsIfFlatEdges::to_string (size_t nmax) const
{
  std::ostringstream os;
  EdgesIterator p (begin ());
  bool first = true;
  for ( ; ! p.at_end () && nmax != 0; ++p, --nmax) {
    if (! first) {
      os << ";";
    }
    first = false;
    os << p->to_string ();
  }
  if (! p.at_end ()) {
    os << "...";
  }
  return os.str ();
}

EdgesDelegate *
AsIfFlatEdges::selected_interacting_generic (const Region &other, bool inverse) const
{
  //  shortcuts
  if (other.empty () || empty ()) {
    return new EmptyEdges ();
  }

  geo::box_scanner2<geo::Edge, size_t, geo::Polygon, size_t> scanner (report_progress (), progress_desc ());

  AddressableEdgeDelivery e (begin_merged (), has_valid_merged_edges ());

  for ( ; ! e.at_end (); ++e) {
    scanner.insert1 (e.operator-> (), 0);
  }

  AddressablePolygonDelivery p = other.addressable_polygons ();

  for ( ; ! p.at_end (); ++p) {
    scanner.insert2 (p.operator-> (), 1);
  }

  std::unique_ptr<FlatEdges> output (new FlatEdges (true));

  if (! inverse) {

    edge_to_region_interaction_filter<FlatEdges> filter (*output);
    scanner.process (filter, 1, geo::box_convert<geo::Edge> (), geo::box_convert<geo::Polygon> ());

  } else {

    std::set<geo::Edge> interacting;
    edge_to_region_interaction_filter<std::set<geo::Edge> > filter (interacting);
    scanner.process (filter, 1, geo::box_convert<geo::Edge> (), geo::box_convert<geo::Polygon> ());

    for (EdgesIterator o (begin_merged ()); ! o.at_end (); ++o) {
      if (interacting.find (*o) == interacting.end ()) {
        output->insert (*o);
      }
    }

  }

  return output.release ();
}

EdgesDelegate *
AsIfFlatEdges::selected_interacting_generic (const Edges &edges, bool inverse) const
{
  geo::box_scanner<geo::Edge, size_t> scanner (report_progress (), progress_desc ());

  AddressableEdgeDelivery e (begin_merged (), has_valid_merged_edges ());

  for ( ; ! e.at_end (); ++e) {
    scanner.insert (e.operator-> (), 0);
  }

  AddressableEdgeDelivery ee = edges.addressable_edges ();

  for ( ; ! ee.at_end (); ++ee) {
    scanner.insert (ee.operator-> (), 1);
  }

  std::unique_ptr<FlatEdges> output (new FlatEdges (true));

  if (! inverse) {

    edge_interaction_filter<FlatEdges> filter (*output);
    scanner.process (filter, 1, geo::box_convert<geo::Edge> ());

  } else {

    std::set<geo::Edge> interacting;
    edge_interaction_filter<std::set<geo::Edge> > filter (interacting);
    scanner.process (filter, 1, geo::box_convert<geo::Edge> ());

    for (EdgesIterator o (begin_merged ()); ! o.at_end (); ++o) {
      if (interacting.find (*o) == interacting.end ()) {
        output->insert (*o);
      }
    }

  }

  return output.release ();
}

EdgesDelegate *
AsIfFlatEdges::pull_generic (const Edges &edges) const
{
  geo::box_scanner<geo::Edge, size_t> scanner (report_progress (), progress_desc ());

  AddressableEdgeDelivery e (begin (), has_valid_edges ());

  for ( ; ! e.at_end (); ++e) {
    scanner.insert (e.operator-> (), 1);
  }

  AddressableEdgeDelivery ee = edges.addressable_merged_edges ();

  for ( ; ! ee.at_end (); ++ee) {
    scanner.insert (ee.operator-> (), 0);
  }

  std::unique_ptr<FlatEdges> output (new FlatEdges (true));
  edge_interaction_filter<FlatEdges> filter (*output);
  scanner.process (filter, 1, geo::box_convert<geo::Edge> ());

  return output.release ();
}

RegionDelegate *
AsIfFlatEdges::pull_generic (const Region &other) const
{
  //  shortcuts
  if (other.empty () || empty ()) {
    return new EmptyRegion ();
  }

  geo::box_scanner2<geo::Edge, size_t, geo::Polygon, size_t> scanner (report_progress (), progress_desc ());

  AddressableEdgeDelivery e (begin (), true);

  for ( ; ! e.at_end (); ++e) {
    scanner.insert1 (e.operator-> (), 0);
  }

  AddressablePolygonDelivery p = other.addressable_merged_polygons ();

  for ( ; ! p.at_end (); ++p) {
    scanner.insert2 (p.operator-> (), 1);
  }

  std::unique_ptr<FlatRegion> output (new FlatRegion (true));

  edge_to_region_interaction_filter<FlatRegion> filter (*output);
  scanner.process (filter, 1, geo::box_convert<geo::Edge> (), geo::box_convert<geo::Polygon> ());

  return output.release ();
}

EdgesDelegate *
AsIfFlatEdges::pull_interacting (const Edges &other) const
{
  return pull_generic (other);
}

RegionDelegate *
AsIfFlatEdges::pull_interacting (const Region &other) const
{
  return pull_generic (other);
}

EdgesDelegate *
AsIfFlatEdges::selected_interacting (const Edges &other) const
{
  return selected_interacting_generic (other, false);
}

EdgesDelegate *
AsIfFlatEdges::selected_not_interacting (const Edges &other) const
{
  return selected_interacting_generic (other, true);
}

EdgesDelegate *
AsIfFlatEdges::selected_interacting (const Region &other) const
{
  return selected_interacting_generic (other, false);
}

EdgesDelegate *
AsIfFlatEdges::selected_not_interacting (const Region &other) const
{
  return selected_interacting_generic (other, true);
}

namespace
{

struct JoinEdgesClusterCollector
  : public geo::cluster_collector<geo::Edge, size_t, JoinEdgesCluster>
{
  typedef geo::Edge::coord_type coord_type;

  JoinEdgesClusterCollector (geo::PolygonSink *output, coord_type ext_b, coord_type ext_e, coord_type ext_o, coord_type ext_i)
    : geo::cluster_collector<geo::Edge, size_t, JoinEdgesCluster> (JoinEdgesCluster (output, ext_b, ext_e, ext_o, ext_i), true)
  {
    //  .. nothing yet ..
  }

  void add (const geo::Edge *o1, size_t p1, const geo::Edge *o2, size_t p2)
  {
    if (o1->p2 () == o2->p1 () || o1->p1 () == o2->p2 ()) {
      geo::cluster_collector<geo::Edge, size_t, JoinEdgesCluster>::add (o1, p1, o2, p2);
    }
  }
};

}

RegionDelegate *
AsIfFlatEdges::extended (coord_type ext_b, coord_type ext_e, coord_type ext_o, coord_type ext_i, bool join) const
{
  if (join) {

    std::unique_ptr<FlatRegion> output (new FlatRegion ());
    geo::ShapeGenerator sg (output->raw_polygons (), false);
    JoinEdgesClusterCollector cluster_collector (&sg, ext_b, ext_e, ext_o, ext_i);

    geo::box_scanner<geo::Edge, size_t> scanner (report_progress (), progress_desc ());
    scanner.reserve (count ());

    AddressableEdgeDelivery e (begin (), has_valid_edges ());

    size_t n = 0;
    for ( ; ! e.at_end (); ++e) {
      scanner.insert (e.operator-> (), n);
      ++n;
    }

    scanner.process (cluster_collector, 1, geo::box_convert<geo::Edge> ());

    return output.release ();

  } else {

    std::unique_ptr<FlatRegion> output (new FlatRegion ());
    for (EdgesIterator e (begin_merged ()); ! e.at_end (); ++e) {
      output->insert (extended_edge (*e, ext_b, ext_e, ext_o, ext_i));
    }

    return output.release ();

  }
}

EdgesDelegate *
AsIfFlatEdges::in (const Edges &other, bool invert) const
{
  std::set <geo::Edge> op;
  for (EdgesIterator o (other.begin_merged ()); ! o.at_end (); ++o) {
    op.insert (*o);
  }

  std::unique_ptr<FlatEdges> new_region (new FlatEdges (false));

  for (EdgesIterator o (begin_merged ()); ! o.at_end (); ++o) {
    if ((op.find (*o) == op.end ()) == invert) {
      new_region->insert (*o);
    }
  }

  return new_region.release ();
}

size_t
AsIfFlatEdges::count () const
{
  size_t n = 0;
  for (EdgesIterator p (begin ()); ! p.at_end (); ++p) {
    ++n;
  }
  return n;
}

size_t
AsIfFlatEdges::hier_count () const
{
  return count ();
}

AsIfFlatEdges::length_type
AsIfFlatEdges::length (const geo::Box &box) const
{
  distance_type l = 0;

  for (EdgesIterator e (begin_merged ()); ! e.at_end (); ++e) {

    if (box.empty () || (box.contains (e->p1 ()) && box.contains (e->p2 ()))) {
      l += e->length ();
    } else {

      std::pair<bool, geo::Edge> ce = e->clipped (box);
      if (ce.first) {

        geo::Coord dx = ce.second.dx ();
        geo::Coord dy = ce.second.dy ();
        geo::Coord x = ce.second.p1 ().x ();
        geo::Coord y = ce.second.p1 ().y ();
        if ((dx == 0 && x == box.left ()   && dy < 0) ||
            (dx == 0 && x == box.right ()  && dy > 0) ||
            (dy == 0 && y == box.top ()    && dx < 0) ||
            (dy == 0 && y == box.bottom () && dx > 0)) {
          //  not counted -> box is at outside side of the edge
        } else {
          l += ce.second.length ();
        }

      }

    }

  }

  return l;
}

Box AsIfFlatEdges::bbox () const
{
  if (! m_bbox_valid) {
    m_bbox = compute_bbox ();
    m_bbox_valid = true;
  }
  return m_bbox;
}

Box AsIfFlatEdges::compute_bbox () const
{
  geo::Box b;
  for (EdgesIterator e (begin ()); ! e.at_end (); ++e) {
    b += e->bbox ();
  }
  return b;
}

void AsIfFlatEdges::update_bbox (const geo::Box &b)
{
  m_bbox = b;
  m_bbox_valid = true;
}

void AsIfFlatEdges::invalidate_bbox ()
{
  m_bbox_valid = false;
}

EdgesDelegate *
AsIfFlatEdges::processed (const EdgeProcessorBase &filter) const
{
  std::unique_ptr<FlatEdges> edges (new FlatEdges ());

  if (filter.result_must_not_be_merged ()) {
    edges->set_merged_semantics (false);
  }

  std::vector<geo::Edge> res_edges;

  for (EdgesIterator e (filter.requires_raw_input () ? begin () : begin_merged ()); ! e.at_end (); ++e) {
    res_edges.clear ();
    filter.process (*e, res_edges);
    for (std::vector<geo::Edge>::const_iterator er = res_edges.begin (); er != res_edges.end (); ++er) {
      edges->insert (*er);
    }
  }

  return edges.release ();
}

EdgePairsDelegate *
AsIfFlatEdges::processed_to_edge_pairs (const EdgeToEdgePairProcessorBase &filter) const
{
  std::unique_ptr<FlatEdgePairs> edge_pairs (new FlatEdgePairs ());

  if (filter.result_must_not_be_merged ()) {
    edge_pairs->set_merged_semantics (false);
  }

  std::vector<geo::EdgePair> res_edge_pairs;

  for (EdgesIterator e (filter.requires_raw_input () ? begin () : begin_merged ()); ! e.at_end (); ++e) {
    res_edge_pairs.clear ();
    filter.process (*e, res_edge_pairs);
    for (std::vector<geo::EdgePair>::const_iterator epr = res_edge_pairs.begin (); epr != res_edge_pairs.end (); ++epr) {
      edge_pairs->insert (*epr);
    }
  }

  return edge_pairs.release ();
}

RegionDelegate *
AsIfFlatEdges::processed_to_polygons (const EdgeToPolygonProcessorBase &filter) const
{
  std::unique_ptr<FlatRegion> region (new FlatRegion ());

  if (filter.result_must_not_be_merged ()) {
    region->set_merged_semantics (false);
  }

  std::vector<geo::Polygon> res_polygons;

  for (EdgesIterator e (filter.requires_raw_input () ? begin () : begin_merged ()); ! e.at_end (); ++e) {
    res_polygons.clear ();
    filter.process (*e, res_polygons);
    for (std::vector<geo::Polygon>::const_iterator pr = res_polygons.begin (); pr != res_polygons.end (); ++pr) {
      region->insert (*pr);
    }
  }

  return region.release ();
}

EdgesDelegate *
AsIfFlatEdges::filtered (const EdgeFilterBase &filter) const
{
  std::unique_ptr<FlatEdges> new_region (new FlatEdges ());

  for (EdgesIterator p (begin_merged ()); ! p.at_end (); ++p) {
    if (filter.selected (*p)) {
      new_region->insert (*p);
    }
  }

  return new_region.release ();
}

EdgePairsDelegate *
AsIfFlatEdges::run_check (geo::edge_relation_type rel, const Edges *other, geo::Coord d, const geo::EdgesCheckOptions &options) const
{
  std::unique_ptr<FlatEdgePairs> result (new FlatEdgePairs ());

  geo::box_scanner<geo::Edge, size_t> scanner (report_progress (), progress_desc ());
  scanner.reserve (count () + (other ? other->count () : 0));

  AddressableEdgeDelivery e (begin_merged (), has_valid_merged_edges ());

  size_t n = 0;
  for ( ; ! e.at_end (); ++e) {
    scanner.insert (e.operator-> (), n);
    n += 2;
  }

  AddressableEdgeDelivery ee;

  if (other) {
    ee = other->addressable_merged_edges ();
    n = 1;
    for ( ; ! ee.at_end (); ++ee) {
      scanner.insert (ee.operator-> (), n);
      n += 2;
    }
  }

  EdgeRelationFilter check (rel, d, options.metrics);
  check.set_include_zero (false);
  check.set_whole_edges (options.whole_edges);
  check.set_ignore_angle (options.ignore_angle);
  check.set_min_projection (options.min_projection);
  check.set_max_projection (options.max_projection);

  edge2edge_check_for_edges<geo::FlatEdgePairs> edge_check (check, *result, other != 0);
  scanner.process (edge_check, d, geo::box_convert<geo::Edge> ());

  return result.release ();
}

EdgesDelegate * 
AsIfFlatEdges::boolean (const Edges *other, EdgeBoolOp op) const
{
  std::unique_ptr<FlatEdges> output (new FlatEdges (true));
  EdgeBooleanClusterCollectorToShapes cluster_collector (&output->raw_edges (), op);

  geo::box_scanner<geo::Edge, size_t> scanner (report_progress (), progress_desc ());
  scanner.reserve (count () + (other ? other->count () : 0));

  AddressableEdgeDelivery e (begin (), has_valid_edges ());

  for ( ; ! e.at_end (); ++e) {
    if (! e->is_degenerate ()) {
      scanner.insert (e.operator-> (), 0);
    }
  }

  AddressableEdgeDelivery ee;

  if (other) {
    ee = other->addressable_edges ();
    for ( ; ! ee.at_end (); ++ee) {
      if (! ee->is_degenerate ()) {
        scanner.insert (ee.operator-> (), 1);
      }
    }
  }

  scanner.process (cluster_collector, 1, geo::box_convert<geo::Edge> ());

  return output.release ();
}

EdgesDelegate * 
AsIfFlatEdges::edge_region_op (const Region &other, bool outside, bool include_borders) const
{
  //  shortcuts
  if (other.empty ()) {
    if (! outside) {
      return new EmptyEdges ();
    } else {
      return clone ();
    }
  } else if (empty ()) {
    return new EmptyEdges ();
  }

  geo::EdgeProcessor ep (report_progress (), progress_desc ());

  for (geo::Region::const_iterator p = other.begin (); ! p.at_end (); ++p) {
    if (p->box ().touches (bbox ())) {
      ep.insert (*p, 0);
    }
  }

  for (EdgesIterator e (begin ()); ! e.at_end (); ++e) {
    ep.insert (*e, 1);
  }

  std::unique_ptr<FlatEdges> output (new FlatEdges (false));
  geo::EdgeShapeGenerator cc (output->raw_edges (), true /*clear*/);
  geo::EdgePolygonOp op (outside, include_borders);
  ep.process (cc, op);

  return output.release ();
}

EdgesDelegate *
AsIfFlatEdges::add (const Edges &other) const
{
  FlatEdges *other_flat = dynamic_cast<FlatEdges *> (other.delegate ());
  if (other_flat) {

    std::unique_ptr<FlatEdges> new_edges (new FlatEdges (*other_flat));
    new_edges->set_is_merged (false);
    new_edges->invalidate_cache ();

    size_t n = new_edges->raw_edges ().size () + count ();

    new_edges->reserve (n);

    for (EdgesIterator p (begin ()); ! p.at_end (); ++p) {
      new_edges->raw_edges ().insert (*p);
    }

    return new_edges.release ();

  } else {

    std::unique_ptr<FlatEdges> new_edges (new FlatEdges (false /*not merged*/));

    size_t n = count () + other.count ();

    new_edges->reserve (n);

    for (EdgesIterator p (begin ()); ! p.at_end (); ++p) {
      new_edges->raw_edges ().insert (*p);
    }
    for (EdgesIterator p (other.begin ()); ! p.at_end (); ++p) {
      new_edges->raw_edges ().insert (*p);
    }

    return new_edges.release ();

  }
}

bool
AsIfFlatEdges::equals (const Edges &other) const
{
  if (empty () != other.empty ()) {
    return false;
  }
  if (count () != other.count ()) {
    return false;
  }
  EdgesIterator o1 (begin ());
  EdgesIterator o2 (other.begin ());
  while (! o1.at_end () && ! o2.at_end ()) {
    if (*o1 != *o2) {
      return false;
    }
    ++o1;
    ++o2;
  }
  return true;
}

bool
AsIfFlatEdges::less (const Edges &other) const
{
  if (empty () != other.empty ()) {
    return empty () < other.empty ();
  }
  if (count () != other.count ()) {
    return (count () < other.count ());
  }
  EdgesIterator o1 (begin ());
  EdgesIterator o2 (other.begin ());
  while (! o1.at_end () && ! o2.at_end ()) {
    if (*o1 != *o2) {
      return *o1 < *o2;
    }
    ++o1;
    ++o2;
  }
  return false;
}

void
AsIfFlatEdges::insert_into (Layout *layout, geo::cell_index_type into_cell, unsigned int into_layer) const
{
  //  improves performance when inserting an original layout into the same layout
  geo::LayoutLocker locker (layout);

  geo::Shapes &shapes = layout->cell (into_cell).shapes (into_layer);
  for (EdgesIterator e (begin ()); ! e.at_end (); ++e) {
    shapes.insert (*e);
  }
}

}

