#include "geoAsIfFlatRegion.h"
#include "geoFlatRegion.h"
#include "geoFlatEdgePairs.h"
#include "geoFlatEdges.h"
#include "geoFlatTexts.h"
#include "geoEmptyRegion.h"
#include "geoEmptyEdgePairs.h"
#include "geoEmptyEdges.h"
#include "geoRegion.h"
#include "geoRegionUtils.h"
#include "geoShapeProcessor.h"
#include "geoBoxConvert.h"
#include "geoBoxScanner.h"
#include "geoClip.h"
#include "geoPolygonTools.h"
#include "geoHash.h"
#include "geoRegionLocalOperations.h"
#include "geoHierProcessor.h"
#include "geoCompoundOperation.h"

#include <sstream>

#define USE_LOCAL_PROCESSOR   // comment out for original implementation based on a single scan

namespace geo
{

namespace {

struct ResultCountingInserter
{
  typedef geo::Polygon value_type;

  ResultCountingInserter (std::unordered_map<const geo::Polygon *, size_t, std::ptr_hash_from_value<geo::Polygon> > &result)
    : mp_result (&result)
  {
    //  .. nothing yet ..
  }

  void insert (const geo::Polygon &p)
  {
    (*mp_result)[&p] += 1;
  }

  void init (const geo::Polygon *p)
  {
    (*mp_result)[p] = 0;
  }

private:
  std::unordered_map<const geo::Polygon *, size_t, std::ptr_hash_from_value<geo::Polygon> > *mp_result;
};

}

// -----------------------------------------------------------------------------
//  AsIfFlatRegion implementation

AsIfFlatRegion::AsIfFlatRegion ()
  : RegionDelegate (), m_bbox_valid (false)
{
  //  .. nothing yet ..
}

AsIfFlatRegion::~AsIfFlatRegion ()
{
  //  .. nothing yet ..
}

AsIfFlatRegion::AsIfFlatRegion (const AsIfFlatRegion &other)
  : RegionDelegate (other), m_bbox_valid (false)
{
  operator= (other);
}

AsIfFlatRegion &
AsIfFlatRegion::operator= (const AsIfFlatRegion &other)
{
  if (this != &other) {
    m_bbox_valid = other.m_bbox_valid;
    m_bbox = other.m_bbox;
  }

  return *this;
}

std::string
AsIfFlatRegion::to_string (size_t nmax) const
{
  std::ostringstream os;
  RegionIterator p (begin ());
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
AsIfFlatRegion::edges (const EdgeFilterBase *filter) const
{
  std::unique_ptr<FlatEdges> result (new FlatEdges ());

  size_t n = 0;
  for (RegionIterator p (begin_merged ()); ! p.at_end (); ++p) {
    n += p->vertices ();
  }
  result->reserve (n);

  for (RegionIterator p (begin_merged ()); ! p.at_end (); ++p) {
    for (geo::Polygon::polygon_edge_iterator e = p->begin_edge (); ! e.at_end (); ++e) {
      if (! filter || filter->selected (*e)) {
        result->insert (*e);
      }
    }
  }

  return result.release ();
}

RegionDelegate *
AsIfFlatRegion::in (const Region &other, bool invert) const
{
  std::set <geo::Polygon> op;
  for (RegionIterator o (other.begin_merged ()); ! o.at_end (); ++o) {
    op.insert (*o);
  }

  std::unique_ptr<FlatRegion> new_region (new FlatRegion (false));

  for (RegionIterator o (begin_merged ()); ! o.at_end (); ++o) {
    if ((op.find (*o) == op.end ()) == invert) {
      new_region->insert (*o);
    }
  }

  return new_region.release ();
}

bool
AsIfFlatRegion::is_box () const
{
  RegionIterator p (begin ());
  if (p.at_end ()) {
    return false;
  } else {
    const geo::Polygon &poly = *p;
    ++p;
    if (! p.at_end ()) {
      return false;
    } else {
      return poly.is_box ();
    }
  }
}

AsIfFlatRegion::area_type
AsIfFlatRegion::area (const geo::Box &box) const
{
  area_type a = 0;

  for (RegionIterator p (begin_merged ()); ! p.at_end (); ++p) {
    if (box.empty () || p->box ().inside (box)) {
      a += p->area ();
    } else {
      std::vector<geo::Polygon> clipped;
      clip_poly (*p, box, clipped);
      for (std::vector<geo::Polygon>::const_iterator c = clipped.begin (); c != clipped.end (); ++c) {
        a += c->area ();
      }
    }
  }

  return a;
}

AsIfFlatRegion::perimeter_type
AsIfFlatRegion::perimeter (const geo::Box &box) const
{
  perimeter_type d = 0;

  for (RegionIterator p (begin_merged ()); ! p.at_end (); ++p) {

    if (box.empty () || p->box ().inside (box)) {
      d += p->perimeter ();
    } else {

      for (geo::Polygon::polygon_edge_iterator e = p->begin_edge (); ! e.at_end (); ++e) {

        if (box.empty ()) {
          d += (*e).length ();
        } else {

          std::pair<bool, geo::Edge> ce = (*e).clipped (box);
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
              d += ce.second.length ();
            }

          }

        }

      }

    }

  }

  return d;
}

Box AsIfFlatRegion::bbox () const
{
  if (! m_bbox_valid) {
    m_bbox = compute_bbox ();
    m_bbox_valid = true;
  }
  return m_bbox;
}

Box AsIfFlatRegion::compute_bbox () const
{
  geo::Box b;
  for (RegionIterator p (begin ()); ! p.at_end (); ++p) {
    b += p->box ();
  }
  return b;
}

void AsIfFlatRegion::update_bbox (const geo::Box &b)
{
  m_bbox = b;
  m_bbox_valid = true;
}

void AsIfFlatRegion::invalidate_bbox ()
{
  m_bbox_valid = false;
}

RegionDelegate *
AsIfFlatRegion::filtered (const PolygonFilterBase &filter) const
{
  std::unique_ptr<FlatRegion> new_region (new FlatRegion ());

  for (RegionIterator p (begin_merged ()); ! p.at_end (); ++p) {
    if (filter.selected (*p)) {
      new_region->insert (*p);
    }
  }

  return new_region.release ();
}

RegionDelegate *
AsIfFlatRegion::processed (const PolygonProcessorBase &filter) const
{
  std::unique_ptr<FlatRegion> new_region (new FlatRegion ());
  if (filter.result_must_not_be_merged ()) {
    new_region->set_merged_semantics (false);
  }

  std::vector<geo::Polygon> poly_res;

  for (RegionIterator p (filter.requires_raw_input () ? begin () : begin_merged ()); ! p.at_end (); ++p) {

    poly_res.clear ();
    filter.process (*p, poly_res);
    for (std::vector<geo::Polygon>::const_iterator pr = poly_res.begin (); pr != poly_res.end (); ++pr) {
      new_region->insert (*pr);
    }

  }

  return new_region.release ();
}

EdgesDelegate *
AsIfFlatRegion::processed_to_edges (const PolygonToEdgeProcessorBase &filter) const
{
  std::unique_ptr<FlatEdges> new_edges (new FlatEdges ());
  if (filter.result_must_not_be_merged ()) {
    new_edges->set_merged_semantics (false);
  }

  std::vector<geo::Edge> edge_res;

  for (RegionIterator p (filter.requires_raw_input () ? begin () : begin_merged ()); ! p.at_end (); ++p) {

    edge_res.clear ();
    filter.process (*p, edge_res);
    for (std::vector<geo::Edge>::const_iterator er = edge_res.begin (); er != edge_res.end (); ++er) {
      new_edges->insert (*er);
    }

  }

  return new_edges.release ();
}

EdgePairsDelegate *
AsIfFlatRegion::processed_to_edge_pairs (const PolygonToEdgePairProcessorBase &filter) const
{
  std::unique_ptr<FlatEdgePairs> new_edge_pairs (new FlatEdgePairs ());
  if (filter.result_must_not_be_merged ()) {
    new_edge_pairs->set_merged_semantics (false);
  }

  std::vector<geo::EdgePair> edge_pair_res;

  for (RegionIterator p (filter.requires_raw_input () ? begin () : begin_merged ()); ! p.at_end (); ++p) {

    edge_pair_res.clear ();
    filter.process (*p, edge_pair_res);
    for (std::vector<geo::EdgePair>::const_iterator epr = edge_pair_res.begin (); epr != edge_pair_res.end (); ++epr) {
      new_edge_pairs->insert (*epr);
    }

  }

  return new_edge_pairs.release ();
}

namespace {

class OutputPairHolder
{
public:
  OutputPairHolder (InteractingOutputMode output_mode, bool merged_semantics)
  {
    if (output_mode == None) {
      return;
    }

    if (output_mode == Positive || output_mode == Negative || output_mode == PositiveAndNegative) {
      m_positive.reset (new FlatRegion (merged_semantics));
      m_results.push_back (& m_positive->raw_polygons ());
    } else {
      m_results.push_back ((geo::Shapes *) 0);
    }

    if (output_mode == PositiveAndNegative) {
      m_negative.reset (new FlatRegion (merged_semantics));
      m_results.push_back (& m_negative->raw_polygons ());
    }
  }

  std::pair<RegionDelegate *, RegionDelegate *> region_pair ()
  {
    return std::make_pair (m_positive.release (), m_negative.release ());
  }

  const std::vector<geo::Shapes *> &results () { return m_results; }

private:
  std::unique_ptr<FlatRegion> m_positive, m_negative;
  std::vector<geo::Shapes *> m_results;
};

}

std::pair<RegionDelegate *, RegionDelegate *>
AsIfFlatRegion::selected_interacting_generic (const Edges &other, InteractingOutputMode output_mode, size_t min_count, size_t max_count) const
{
  OutputPairHolder oph (output_mode, merged_semantics ());

  if (output_mode == None) {
    return oph.region_pair ();
  }

  min_count = std::max (size_t (1), min_count);

  //  shortcut
  if (empty ()) {
    if (output_mode == Positive || output_mode == Negative) {
      return std::make_pair (clone (), (RegionDelegate *) 0);
    } else {
      return std::make_pair (clone (), clone ());
    }
  } else if (max_count < min_count || other.empty ()) {
    if (output_mode == Positive) {
      return std::make_pair (new EmptyRegion (), (RegionDelegate *) 0);
    } else if (output_mode == Negative) {
      return std::make_pair (clone (), (RegionDelegate *) 0);
    } else {
      return std::make_pair (new EmptyRegion (), clone ());
    }
  }

  bool counting = !(min_count == 1 && max_count == std::numeric_limits<size_t>::max ());

  geo::RegionIterator polygons (begin_merged ());

  geo::interacting_with_edge_local_operation<geo::Polygon, geo::Edge, geo::Polygon> op (output_mode, min_count, max_count, true);

  geo::local_processor<geo::Polygon, geo::Edge, geo::Polygon> proc;
  proc.set_base_verbosity (base_verbosity ());
  proc.set_description (progress_desc ());
  proc.set_report_progress (report_progress ());

  std::vector<generic_shape_iterator<geo::Edge> > others;
  others.push_back (counting ? other.begin_merged () : other.begin ());

  std::unique_ptr<FlatRegion> output (new FlatRegion (merged_semantics ()));
  std::vector<geo::Shapes *> results;
  results.push_back (&output->raw_polygons ());

  proc.run_flat (polygons, others, std::vector<bool> (), &op, oph.results ());

  return oph.region_pair ();
}

std::pair<RegionDelegate *, RegionDelegate *>
AsIfFlatRegion::selected_interacting_generic (const Texts &other, InteractingOutputMode output_mode, size_t min_count, size_t max_count) const
{
  OutputPairHolder oph (output_mode, merged_semantics ());

  if (output_mode == None) {
    return oph.region_pair ();
  }

  min_count = std::max (size_t (1), min_count);

  //  shortcut
  if (empty ()) {
    if (output_mode == Positive || output_mode == Negative) {
      return std::make_pair (clone (), (RegionDelegate *) 0);
    } else {
      return std::make_pair (clone (), clone ());
    }
  } else if (max_count < min_count || other.empty ()) {
    if (output_mode == Positive) {
      return std::make_pair (new EmptyRegion (), (RegionDelegate *) 0);
    } else if (output_mode == Negative) {
      return std::make_pair (clone (), (RegionDelegate *) 0);
    } else {
      return std::make_pair (new EmptyRegion (), clone ());
    }
  }

  geo::RegionIterator polygons (begin_merged ());

  geo::interacting_with_text_local_operation<geo::Polygon, geo::Text, geo::Polygon> op (output_mode, min_count, max_count);

  geo::local_processor<geo::Polygon, geo::Text, geo::Polygon> proc;
  proc.set_base_verbosity (base_verbosity ());
  proc.set_description (progress_desc ());
  proc.set_report_progress (report_progress ());

  std::vector<generic_shape_iterator<geo::Text> > others;
  others.push_back (other.begin ());

  proc.run_flat (polygons, others, std::vector<bool> (), &op, oph.results ());

  return oph.region_pair ();
}


std::pair<RegionDelegate *, RegionDelegate *>
AsIfFlatRegion::selected_interacting_generic (const Region &other, int mode, bool touching, InteractingOutputMode output_mode, size_t min_count, size_t max_count) const
{
  OutputPairHolder oph (output_mode, merged_semantics ());

  if (output_mode == None) {
    return oph.region_pair ();
  }

  min_count = std::max (size_t (1), min_count);

  //  shortcut
  if (empty ()) {
    if (output_mode == Positive || output_mode == Negative) {
      return std::make_pair (clone (), (RegionDelegate *) 0);
    } else {
      return std::make_pair (clone (), clone ());
    }
  } else if (max_count < min_count || other.empty ()) {
    //  clear, if b is empty and
    //   * mode is inside, enclosing or interacting and inverse is false ("inside" or "interacting")
    //   * mode is outside and inverse is true ("not outside")
    if ((mode <= 0)) {
      if (output_mode == Positive) {
        return std::make_pair (new EmptyRegion (), (RegionDelegate *) 0);
      } else if (output_mode == Negative) {
        return std::make_pair (clone (), (RegionDelegate *) 0);
      } else {
        return std::make_pair (new EmptyRegion (), clone ());
      }
    } else {
      if (output_mode == Positive) {
        return std::make_pair (clone(), (RegionDelegate *) 0);
      } else if (output_mode == Negative) {
        return std::make_pair (new EmptyRegion (), (RegionDelegate *) 0);
      } else {
        return std::make_pair (clone (), new EmptyRegion ());
      }
    }
  }

  bool counting = !(min_count == 1 && max_count == std::numeric_limits<size_t>::max ());

  geo::RegionIterator polygons (begin_merged ());

  geo::interacting_local_operation<geo::Polygon, geo::Polygon, geo::Polygon> op (mode, touching, output_mode, min_count, max_count, true);

  geo::local_processor<geo::Polygon, geo::Polygon, geo::Polygon> proc;
  proc.set_base_verbosity (base_verbosity ());
  proc.set_description (progress_desc ());
  proc.set_report_progress (report_progress ());

  std::vector<generic_shape_iterator<geo::Polygon> > others;
  others.push_back ((mode < 0 || counting) ? other.begin_merged () : other.begin ());

  proc.run_flat (polygons, others, std::vector<bool> (), &op, oph.results ());

  return oph.region_pair ();
}

EdgesDelegate *
AsIfFlatRegion::pull_generic (const Edges &other) const
{
  if (other.empty ()) {
    return other.delegate ()->clone ();
  } else if (empty ()) {
    return new EmptyEdges ();
  }

#if defined(USE_LOCAL_PROCESSOR)

  geo::RegionIterator polygons (begin ());

  geo::pull_with_edge_local_operation <geo::Polygon, geo::Edge, geo::Edge> op;

  geo::local_processor<geo::Polygon, geo::Edge, geo::Edge> proc;
  proc.set_base_verbosity (base_verbosity ());
  proc.set_description (progress_desc ());
  proc.set_report_progress (report_progress ());

  std::vector<generic_shape_iterator<geo::Edge> > others;
  others.push_back (other.begin_merged ());

  std::unique_ptr<FlatEdges> output (new FlatEdges (merged_semantics ()));
  std::vector<geo::Shapes *> results;
  results.push_back (&output->raw_edges ());

  proc.run_flat (polygons, others, std::vector<bool> (), &op, results);

  return output.release ();

#else

  geo::box_scanner2<geo::Polygon, size_t, geo::Edge, size_t> scanner (report_progress (), progress_desc ());
  scanner.reserve1 (count ());
  scanner.reserve2 (other.count ());

  std::unique_ptr<FlatEdges> output (new FlatEdges (false));
  region_to_edge_interaction_filter<geo::Polygon, geo::Edge, geo::Shapes, geo::Edge> filter (output->raw_edges (), false);

  AddressablePolygonDelivery p (begin ());

  for ( ; ! p.at_end (); ++p) {
    scanner.insert1 (p.operator-> (), 0);
  }

  AddressableEdgeDelivery e (other.addressable_merged_edges ());

  for ( ; ! e.at_end (); ++e) {
    scanner.insert2 (e.operator-> (), 0);
  }

  scanner.process (filter, 1, geo::box_convert<geo::Polygon> (), geo::box_convert<geo::Edge> ());

  return output.release ();
#endif
}

TextsDelegate *
AsIfFlatRegion::pull_generic (const Texts &other) const
{
#if defined(USE_LOCAL_PROCESSOR)

  geo::RegionIterator polygons (begin ());

  geo::pull_with_text_local_operation <geo::Polygon, geo::Text, geo::Text> op;

  geo::local_processor<geo::Polygon, geo::Text, geo::Text> proc;
  proc.set_base_verbosity (base_verbosity ());
  proc.set_description (progress_desc ());
  proc.set_report_progress (report_progress ());

  std::vector<generic_shape_iterator<geo::Text> > others;
  others.push_back (other.begin ());

  std::unique_ptr<FlatTexts> output (new FlatTexts ());
  std::vector<geo::Shapes *> results;
  results.push_back (&output->raw_texts ());

  proc.run_flat (polygons, others, std::vector<bool> (), &op, results);

  return output.release ();

#else
  if (other.empty ()) {
    return other.delegate ()->clone ();
  } else if (empty ()) {
    return new EmptyTexts ();
  }

  geo::box_scanner2<geo::Polygon, size_t, geo::Text, size_t> scanner (report_progress (), progress_desc ());
  scanner.reserve1 (count ());
  scanner.reserve2 (other.count ());

  std::unique_ptr<FlatTexts> output (new FlatTexts (false));
  region_to_text_interaction_filter<geo::Polygon, geo::Text, geo::Shapes, geo::Text> filter (output->raw_texts (), false);

  AddressablePolygonDelivery p (begin ());

  for ( ; ! p.at_end (); ++p) {
    scanner.insert1 (p.operator-> (), 0);
  }

  AddressableTextDelivery e (other.addressable_texts ());

  for ( ; ! e.at_end (); ++e) {
    scanner.insert2 (e.operator-> (), 0);
  }

  scanner.process (filter, 1, geo::box_convert<geo::Polygon> (), geo::box_convert<geo::Text> ());

  return output.release ();
#endif
}

RegionDelegate *
AsIfFlatRegion::pull_generic (const Region &other, int mode, bool touching) const
{
#if defined(USE_LOCAL_PROCESSOR)

  geo::RegionIterator polygons (begin ());

  geo::pull_local_operation <geo::Polygon, geo::Polygon, geo::Polygon> op (mode, touching);

  geo::local_processor<geo::Polygon, geo::Polygon, geo::Polygon> proc;
  proc.set_base_verbosity (base_verbosity ());
  proc.set_description (progress_desc ());
  proc.set_report_progress (report_progress ());

  std::vector<generic_shape_iterator<geo::Polygon> > others;
  others.push_back (other.begin_merged ());

  std::unique_ptr<FlatRegion> output (new FlatRegion (merged_semantics ()));
  std::vector<geo::Shapes *> results;
  results.push_back (&output->raw_polygons ());

  proc.run_flat (polygons, others, std::vector<bool> (), &op, results);

  return output.release ();

#else
  geo::EdgeProcessor ep (report_progress (), progress_desc ());
  ep.set_base_verbosity (base_verbosity ());

  //  shortcut
  if (empty ()) {
    return clone ();
  } else if (other.empty ()) {
    return new EmptyRegion ();
  }

  size_t n = 1;
  for (RegionIterator p = other.begin_merged (); ! p.at_end (); ++p, ++n) {
    if (p->box ().touches (bbox ())) {
      ep.insert (*p, n);
    }
  }

  for (RegionIterator p (begin ()); ! p.at_end (); ++p) {
    if (mode > 0 || p->box ().touches (other.bbox ())) {
      ep.insert (*p, 0);
    }
  }

  geo::InteractionDetector id (mode, 0);
  id.set_include_touching (touching);
  geo::EdgeSink es;
  ep.process (es, id);
  id.finish ();

  std::unique_ptr<FlatRegion> output (new FlatRegion (false));

  n = 0;
  std::set <size_t> selected;
  for (geo::InteractionDetector::iterator i = id.begin (); i != id.end () && i->first == 0; ++i) {
    ++n;
    selected.insert (i->second);
  }

  output->reserve (n);

  n = 1;
  for (RegionIterator p = other.begin_merged (); ! p.at_end (); ++p, ++n) {
    if (selected.find (n) != selected.end ()) {
      output->raw_polygons ().insert (*p);
    }
  }

  return output.release ();
#endif
}

template <class Trans>
void
AsIfFlatRegion::produce_markers_for_grid_check (const geo::Polygon &poly, const Trans &tr, geo::Coord gx, geo::Coord gy, geo::Shapes &shapes)
{
  gx = std::max (geo::Coord (1), gx);
  gy = std::max (geo::Coord (1), gy);

  for (size_t i = 0; i < poly.holes () + 1; ++i) {

    geo::Polygon::polygon_contour_iterator b, e;

    if (i == 0) {
      b = poly.begin_hull ();
      e = poly.end_hull ();
    } else {
      b = poly.begin_hole ((unsigned int) (i - 1));
      e = poly.end_hole ((unsigned int)  (i - 1));
    }

    for (geo::Polygon::polygon_contour_iterator pt = b; pt != e; ++pt) {
      geo::Point p = tr * *pt;
      if ((p.x () % gx) != 0 || (p.y () % gy) != 0) {
        shapes.insert (EdgePair (geo::Edge (p, p), geo::Edge (p, p)));
      }
    }

  }
}

template void AsIfFlatRegion::produce_markers_for_grid_check<geo::ICplxTrans> (const geo::Polygon &poly, const geo::ICplxTrans &tr, geo::Coord gx, geo::Coord gy, geo::Shapes &shapes);
template void AsIfFlatRegion::produce_markers_for_grid_check<geo::UnitTrans> (const geo::Polygon &poly, const geo::UnitTrans &tr, geo::Coord gx, geo::Coord gy, geo::Shapes &shapes);

EdgePairsDelegate *
AsIfFlatRegion::grid_check (geo::Coord gx, geo::Coord gy) const
{
  if (gx < 0 || gy < 0) {
    throw geo::Exception (geo::to_string (tr ("Grid check requires a positive grid value")));
  }

  if (gx == 0 && gy == 0) {
    return new EmptyEdgePairs ();
  }

  std::unique_ptr<geo::FlatEdgePairs> res (new geo::FlatEdgePairs ());

  for (RegionIterator p (begin_merged ()); ! p.at_end (); ++p) {
    produce_markers_for_grid_check (*p, geo::UnitTrans (), gx, gy, res->raw_edge_pairs ());
  }

  return res.release ();
}

static bool ac_less (double cos_a, bool gt180_a, double cos_b, bool gt180_b)
{
  if (gt180_a != gt180_b) {
    return gt180_a < gt180_b;
  } else {
    if (gt180_a) {
      return cos_a < cos_b - 1e-10;
    } else {
      return cos_a > cos_b + 1e-10;
    }
  }
}

template <class Trans>
void
AsIfFlatRegion::produce_markers_for_angle_check (const geo::Polygon &poly, const Trans &tr, double min, double max, bool inverse, geo::Shapes &shapes)
{
  double cos_min = cos (std::max (0.0, std::min (360.0, min)) / 180.0 * M_PI);
  double cos_max = cos (std::max (0.0, std::min (360.0, max)) / 180.0 * M_PI);
  bool gt180_min = min > 180.0;
  bool gt180_max = max > 180.0;

  for (size_t i = 0; i < poly.holes () + 1; ++i) {

    const geo::Polygon::contour_type *h = 0;
    if (i == 0) {
      h = &poly.hull ();
    } else {
      h = &poly.hole ((unsigned int) (i - 1));
    }

    size_t np = h->size ();

    for (size_t j = 0; j < np; ++j) {

      geo::Edge e ((*h) [j], (*h) [(j + 1) % np]);
      e.transform (tr);
      geo::Edge ee (e.p2 (), (*h) [(j + 2) % np]);
      ee.transform (tr);

      double le = e.double_length ();
      double lee = ee.double_length ();

      double cos_a = -geo::sprod (e, ee) / (le * lee);
      bool gt180_a = geo::vprod_sign (e, ee) > 0;

      if ((ac_less (cos_a, gt180_a, cos_max, gt180_max) && !ac_less (cos_a, gt180_a, cos_min, gt180_min)) == !inverse) {
        shapes.insert (EdgePair (e, ee));
      }

    }

  }
}

template void AsIfFlatRegion::produce_markers_for_angle_check<geo::ICplxTrans> (const geo::Polygon &poly, const geo::ICplxTrans &tr, double min, double max, bool inverse, geo::Shapes &shapes);
template void AsIfFlatRegion::produce_markers_for_angle_check<geo::UnitTrans> (const geo::Polygon &poly, const geo::UnitTrans &tr, double min, double max, bool inverse, geo::Shapes &shapes);

EdgePairsDelegate *
AsIfFlatRegion::angle_check (double min, double max, bool inverse) const
{
  std::unique_ptr<geo::FlatEdgePairs> res (new geo::FlatEdgePairs ());

  for (RegionIterator p (begin_merged ()); ! p.at_end (); ++p) {
    produce_markers_for_angle_check (*p, geo::UnitTrans (), min, max, inverse, res->raw_edge_pairs ());
  }

  return res.release ();
}

RegionDelegate *
AsIfFlatRegion::snapped (geo::Coord gx, geo::Coord gy)
{
  if (gx < 0 || gy < 0) {
    throw geo::Exception (geo::to_string (tr ("Grid snap requires a positive grid value")));
  }

  std::unique_ptr<FlatRegion> new_region (new FlatRegion (merged_semantics ()));

  gx = std::max (geo::Coord (1), gx);
  gy = std::max (geo::Coord (1), gy);

  std::vector<geo::Point> heap;

  for (RegionIterator p (begin_merged ()); ! p.at_end (); ++p) {
    new_region->raw_polygons ().insert (snapped_polygon (*p, gx, gy, heap));
  }

  return new_region.release ();
}

RegionDelegate *
AsIfFlatRegion::scaled_and_snapped (geo::Coord gx, geo::Coord mx, geo::Coord dx, geo::Coord gy, geo::Coord my, geo::Coord dy)
{
  if (gx < 0 || gy < 0) {
    throw geo::Exception (geo::to_string (tr ("Grid snap requires a positive grid value")));
  }

  if (mx <= 0 || dx <= 0 || my <= 0 || dy <= 0) {
    throw geo::Exception (geo::to_string (tr ("Scale and snap requires positive and non-null magnification or divisor values")));
  }

  std::unique_ptr<FlatRegion> new_region (new FlatRegion (merged_semantics ()));

  gx = std::max (geo::Coord (1), gx);
  gy = std::max (geo::Coord (1), gy);

  std::vector<geo::Point> heap;

  for (RegionIterator p (begin_merged ()); ! p.at_end (); ++p) {
    new_region->raw_polygons ().insert (scaled_and_snapped_polygon (*p, gx, mx, dx, 0, gy, my, dy, 0, heap));
  }

  return new_region.release ();
}

template <class TR>
static
void region_cop_impl (AsIfFlatRegion *region, geo::Shapes *output_to, geo::CompoundRegionOperationNode &node)
{
  geo::local_processor<geo::Polygon, geo::Polygon, TR> proc;
  proc.set_base_verbosity (region->base_verbosity ());
  proc.set_description (region->progress_desc ());
  proc.set_report_progress (region->report_progress ());

  geo::RegionIterator polygons (region->begin_merged ());

  std::vector<generic_shape_iterator<geo::Polygon> > others;
  std::vector<bool> foreign;
  std::vector<geo::Region *> inputs = node.inputs ();
  for (std::vector<geo::Region *>::const_iterator i = inputs.begin (); i != inputs.end (); ++i) {
    if (*i == subject_regionptr () || *i == foreign_regionptr ()) {
      others.push_back (region->begin_merged ());
      foreign.push_back (*i == foreign_regionptr ());
    } else {
      others.push_back ((*i)->begin ());
      foreign.push_back (false);
    }
  }

  std::vector<geo::Shapes *> results;
  results.push_back (output_to);

  compound_local_operation<geo::Polygon, geo::Polygon, TR> op (&node);
  proc.run_flat (polygons, others, foreign, &op, results);
}

EdgePairsDelegate *
AsIfFlatRegion::cop_to_edge_pairs (geo::CompoundRegionOperationNode &node)
{
  std::unique_ptr<FlatEdgePairs> output (new FlatEdgePairs ());
  region_cop_impl<geo::EdgePair> (this, &output->raw_edge_pairs (), node);
  return output.release ();
}

RegionDelegate *
AsIfFlatRegion::cop_to_region (geo::CompoundRegionOperationNode &node)
{
  std::unique_ptr<FlatRegion> output (new FlatRegion ());
  region_cop_impl<geo::Polygon> (this, &output->raw_polygons (), node);
  return output.release ();
}

EdgesDelegate *
AsIfFlatRegion::cop_to_edges (geo::CompoundRegionOperationNode &node)
{
  std::unique_ptr<FlatEdges> output (new FlatEdges ());
  region_cop_impl<geo::Edge> (this, &output->raw_edges (), node);
  return output.release ();
}

EdgePairsDelegate *
AsIfFlatRegion::width_check (geo::Coord d, const RegionCheckOptions &options) const
{
  return run_single_polygon_check (geo::WidthRelation, d, options);
}

EdgePairsDelegate *
AsIfFlatRegion::space_or_isolated_check (geo::Coord d, const RegionCheckOptions &options, bool isolated) const
{
  if (options.opposite_filter != NoOppositeFilter || options.rect_filter != NoRectFilter || options.shielded) {
    //  NOTE: we have to use the "foreign" scheme with a filter because only this scheme
    //  guarantees that all subject shapes are visited.
    return run_check (geo::SpaceRelation, isolated, foreign_regionptr (), d, options);
  } else {
    return run_check (geo::SpaceRelation, isolated, subject_regionptr (), d, options);
  }
}

EdgePairsDelegate *
AsIfFlatRegion::space_check (geo::Coord d, const RegionCheckOptions &options) const
{
  return space_or_isolated_check (d, options, false);
}

EdgePairsDelegate *
AsIfFlatRegion::isolated_check (geo::Coord d, const RegionCheckOptions &options) const
{
  return space_or_isolated_check (d, options, true);
}

EdgePairsDelegate *
AsIfFlatRegion::notch_check (geo::Coord d, const RegionCheckOptions &options) const
{
  return run_single_polygon_check (geo::SpaceRelation, d, options);
}

EdgePairsDelegate *
AsIfFlatRegion::enclosing_check (const Region &other, geo::Coord d, const RegionCheckOptions &options) const
{
  return run_check (geo::OverlapRelation, true, &other, d, options);
}

EdgePairsDelegate *
AsIfFlatRegion::overlap_check (const Region &other, geo::Coord d, const RegionCheckOptions &options) const
{
  return run_check (geo::WidthRelation, true, &other, d, options);
}

EdgePairsDelegate *
AsIfFlatRegion::separation_check (const Region &other, geo::Coord d, const RegionCheckOptions &options) const
{
  return run_check (geo::SpaceRelation, true, &other, d, options);
}

EdgePairsDelegate *
AsIfFlatRegion::inside_check (const Region &other, geo::Coord d, const RegionCheckOptions &options) const
{
  return run_check (geo::InsideRelation, true, &other, d, options);
}

EdgePairsDelegate *
AsIfFlatRegion::run_check (geo::edge_relation_type rel, bool different_polygons, const Region *other, geo::Coord d, const RegionCheckOptions &options) const
{
#if defined(USE_LOCAL_PROCESSOR)

  geo::RegionIterator polygons (begin_merged ());

  EdgeRelationFilter check (rel, d, options.metrics);
  check.set_include_zero (false);
  check.set_whole_edges (options.whole_edges);
  check.set_ignore_angle (options.ignore_angle);
  check.set_min_projection (options.min_projection);
  check.set_max_projection (options.max_projection);

  geo::local_processor<geo::Polygon, geo::Polygon, geo::EdgePair> proc;
  proc.set_base_verbosity (base_verbosity ());
  proc.set_description (progress_desc ());
  proc.set_report_progress (report_progress ());

  std::vector<generic_shape_iterator<geo::Polygon> > others;
  std::vector<bool> foreign;
  bool has_other = false;
  bool other_is_merged = true;

  if (other == subject_regionptr () || other == foreign_regionptr ()) {
    foreign.push_back (other == foreign_regionptr ());
    others.push_back (begin_merged ());
  } else {
    foreign.push_back (false);
    if (options.whole_edges) {
      //  NOTE: whole edges needs both inputs merged
      others.push_back (other->begin_merged ());
      other_is_merged = true;
    } else {
      others.push_back (other->begin ());
      other_is_merged = other->is_merged ();
    }
    has_other = true;
  }

  geo::check_local_operation<geo::Polygon, geo::Polygon> op (check, different_polygons, has_other, other_is_merged, options);

  std::unique_ptr<FlatEdgePairs> output (new FlatEdgePairs ());
  std::vector<geo::Shapes *> results;
  results.push_back (&output->raw_edge_pairs ());

  proc.run_flat (polygons, others, foreign, &op, results);

  return output.release ();

#else
  //  not supported in this implementation
  tl_assert (! m_options.no_opposite);

  std::unique_ptr<FlatEdgePairs> result (new FlatEdgePairs ());

  geo::box_scanner<geo::Polygon, size_t> scanner (report_progress (), progress_desc ());
  scanner.reserve (count () + (other ? other->count () : 0));

  AddressablePolygonDelivery p (begin_merged ());

  size_t n = 0;
  for ( ; ! p.at_end (); ++p) {
    scanner.insert (p.operator-> (), n);
    n += 2;
  }

  AddressablePolygonDelivery po;

  if (other) {

    po = other->addressable_merged_polygons ();

    n = 1;
    for ( ; ! po.at_end (); ++po) {
      scanner.insert (po.operator-> (), n);
      n += 2;
    }

  }

  EdgeRelationFilter check (rel, d, options.metrics);
  check.set_include_zero (false);
  check.set_whole_edges (options.whole_edges);
  check.set_ignore_angle (options.ignore_angle);
  check.set_min_projection (options.min_projection);
  check.set_max_projection (options.max_projection);

  edge2edge_check_negative_or_positive<geo::FlatEdgePairs> edge_check (check, *result, options.negative, different_polygons, other != 0 /*requires different layers*/, options.shielded);
  poly2poly_check<geo::Polygon, geo::FlatEdgePairs> poly_check (edge_check);

  do {
    scanner.process (poly_check, d, geo::box_convert<geo::Polygon> ());
  } while (edge_check.prepare_next_pass ());

  return result.release ();
#endif
}

EdgePairsDelegate *
AsIfFlatRegion::run_single_polygon_check (geo::edge_relation_type rel, geo::Coord d, const RegionCheckOptions &options) const
{
  std::unique_ptr<FlatEdgePairs> result (new FlatEdgePairs ());

  EdgeRelationFilter check (rel, d, options.metrics);
  check.set_include_zero (false);
  check.set_whole_edges (options.whole_edges);
  check.set_ignore_angle (options.ignore_angle);
  check.set_min_projection (options.min_projection);
  check.set_max_projection (options.max_projection);

  edge2edge_check_negative_or_positive<geo::FlatEdgePairs> edge_check (check, *result, options.negative, false /*=same polygons*/, false /*=same layers*/, options.shielded, true /*symmetric edge pairs*/);
  poly2poly_check<geo::Polygon> poly_check (edge_check);

  do {

    size_t n = 0;
    for (RegionIterator p (begin_merged ()); ! p.at_end (); ++p) {
      poly_check.enter (*p, n);
      n += 2;
    }

  } while (edge_check.prepare_next_pass ());

  return result.release ();
}

RegionDelegate *
AsIfFlatRegion::merged (bool min_coherence, unsigned int min_wc) const
{
  if (empty ()) {

    return new EmptyRegion ();

  } else if (is_box ()) {

    //  take box only if min_wc == 0, otherwise clear
    if (min_wc > 0) {
      return new EmptyRegion ();
    } else {
      return clone ();
    }

  } else {

    geo::EdgeProcessor ep (report_progress (), progress_desc ());
    ep.set_base_verbosity (base_verbosity ());

    //  count edges and reserve memory
    size_t n = 0;
    for (RegionIterator p (begin ()); ! p.at_end (); ++p, ++n) {
      n += p->vertices ();
    }
    ep.reserve (n);

    //  insert the polygons into the processor
    n = 0;
    for (RegionIterator p (begin ()); ! p.at_end (); ++p, ++n) {
      ep.insert (*p, n);
    }

    std::unique_ptr<FlatRegion> new_region (new FlatRegion (true));

    //  and run the merge step
    geo::MergeOp op (min_wc);
    geo::ShapeGenerator pc (new_region->raw_polygons (), true /*clear*/);
    geo::PolygonGenerator pg (pc, false /*don't resolve holes*/, min_coherence);
    ep.process (pg, op);

    return new_region.release ();

  }
}

RegionDelegate *
AsIfFlatRegion::region_from_box (const geo::Box &b)
{
  if (! b.empty () && b.width () > 0 && b.height () > 0) {
    FlatRegion *new_region = new FlatRegion ();
    new_region->insert (b);
    return new_region;
  } else {
    return new EmptyRegion ();
  }
}

RegionDelegate *
AsIfFlatRegion::sized (coord_type d, unsigned int mode) const
{
  return sized (d, d, mode);
}

RegionDelegate *
AsIfFlatRegion::sized (coord_type dx, coord_type dy, unsigned int mode) const
{
  if (empty ()) {

    //  ignore empty
    return new EmptyRegion ();

  } else if (is_box () && mode >= 2) {

    //  simplified handling for a box
    geo::Box b = bbox ().enlarged (geo::Vector (dx, dy));
    return region_from_box (b);

  } else if (! merged_semantics () || is_merged ()) {

    //  Generic case
    std::unique_ptr<FlatRegion> new_region (new FlatRegion (false /*output isn't merged*/));

    geo::ShapeGenerator pc (new_region->raw_polygons (), false);
    geo::PolygonGenerator pg (pc, false, true);
    geo::SizingPolygonFilter sf (pg, dx, dy, mode);
    for (RegionIterator p (begin ()); ! p.at_end (); ++p) {
      sf.put (*p);
    }

    return new_region.release ();

  } else {

    //  Generic case - the size operation will merge first
    geo::EdgeProcessor ep (report_progress (), progress_desc ());
    ep.set_base_verbosity (base_verbosity ());

    //  count edges and reserve memory
    size_t n = 0;
    for (RegionIterator p (begin ()); ! p.at_end (); ++p) {
      n += p->vertices ();
    }
    ep.reserve (n);

    //  insert the polygons into the processor
    n = 0;
    for (RegionIterator p (begin ()); ! p.at_end (); ++p, ++n) {
      ep.insert (*p, n);
    }

    std::unique_ptr<FlatRegion> new_region (new FlatRegion (false /*output isn't merged*/));
    geo::ShapeGenerator pc (new_region->raw_polygons (), true /*clear*/);
    geo::PolygonGenerator pg2 (pc, false /*don't resolve holes*/, true /*min. coherence*/);
    geo::SizingPolygonFilter siz (pg2, dx, dy, mode);
    geo::PolygonGenerator pg (siz, false /*don't resolve holes*/, min_coherence () /*min. coherence*/);
    geo::BooleanOp op (geo::BooleanOp::Or);
    ep.process (pg, op);

    return new_region.release ();

  }
}

RegionDelegate *
AsIfFlatRegion::and_with (const Region &other) const
{
  if (empty () || other.empty ()) {

    //  Nothing to do
    return new EmptyRegion ();

  } else if (is_box () && other.is_box ()) {

    //  Simplified handling for boxes
    geo::Box b = bbox ();
    b &= other.bbox ();
    return region_from_box (b);

  } else if (is_box () && ! other.strict_handling ()) {

    //  map AND with box to clip ..
    geo::Box b = bbox ();
    std::unique_ptr<FlatRegion> new_region (new FlatRegion (false));

    std::vector<geo::Polygon> clipped;
    for (RegionIterator p (other.begin ()); ! p.at_end (); ++p) {
      clipped.clear ();
      clip_poly (*p, b, clipped);
      new_region->raw_polygons ().insert (clipped.begin (), clipped.end ());
    }

    return new_region.release ();

  } else if (other.is_box () && ! strict_handling ()) {

    //  map AND with box to clip ..
    geo::Box b = other.bbox ();
    std::unique_ptr<FlatRegion> new_region (new FlatRegion (false));

    std::vector<geo::Polygon> clipped;
    for (RegionIterator p (begin ()); ! p.at_end (); ++p) {
      clipped.clear ();
      clip_poly (*p, b, clipped);
      new_region->raw_polygons ().insert (clipped.begin (), clipped.end ());
    }

    return new_region.release ();

  } else if (! bbox ().overlaps (other.bbox ())) {

    //  Result will be nothing
    return new EmptyRegion ();

  } else {

    //  Generic case
    geo::EdgeProcessor ep (report_progress (), progress_desc ());
    ep.set_base_verbosity (base_verbosity ());

    //  count edges and reserve memory
    size_t n = 0;
    for (RegionIterator p (begin ()); ! p.at_end (); ++p) {
      n += p->vertices ();
    }
    for (RegionIterator p (other.begin ()); ! p.at_end (); ++p) {
      n += p->vertices ();
    }
    ep.reserve (n);

    //  insert the polygons into the processor
    n = 0;
    for (RegionIterator p (begin ()); ! p.at_end (); ++p, n += 2) {
      ep.insert (*p, n);
    }
    n = 1;
    for (RegionIterator p (other.begin ()); ! p.at_end (); ++p, n += 2) {
      ep.insert (*p, n);
    }

    std::unique_ptr<FlatRegion> new_region (new FlatRegion (true));
    geo::BooleanOp op (geo::BooleanOp::And);
    geo::ShapeGenerator pc (new_region->raw_polygons (), true /*clear*/);
    geo::PolygonGenerator pg (pc, false /*don't resolve holes*/, min_coherence ());
    ep.process (pg, op);

    return new_region.release ();

  }
}

RegionDelegate *
AsIfFlatRegion::not_with (const Region &other) const
{
  if (empty ()) {

    //  Nothing to do
    return new EmptyRegion ();

  } else if (other.empty () && ! strict_handling ()) {

    //  Nothing to do
    return clone ();

  } else if (! bbox ().overlaps (other.bbox ()) && ! strict_handling ()) {

    //  Nothing to do
    return clone ();

  } else {

    //  Generic case
    geo::EdgeProcessor ep (report_progress (), progress_desc ());
    ep.set_base_verbosity (base_verbosity ());

    //  count edges and reserve memory
    size_t n = 0;
    for (RegionIterator p (begin ()); ! p.at_end (); ++p) {
      n += p->vertices ();
    }
    for (RegionIterator p (other.begin ()); ! p.at_end (); ++p) {
      n += p->vertices ();
    }
    ep.reserve (n);

    //  insert the polygons into the processor
    n = 0;
    for (RegionIterator p (begin ()); ! p.at_end (); ++p, n += 2) {
      ep.insert (*p, n);
    }
    n = 1;
    for (RegionIterator p (other.begin ()); ! p.at_end (); ++p, n += 2) {
      ep.insert (*p, n);
    }

    std::unique_ptr<FlatRegion> new_region (new FlatRegion (true));
    geo::BooleanOp op (geo::BooleanOp::ANotB);
    geo::ShapeGenerator pc (new_region->raw_polygons (), true /*clear*/);
    geo::PolygonGenerator pg (pc, false /*don't resolve holes*/, min_coherence ());
    ep.process (pg, op);

    return new_region.release ();

  }
}


std::pair<RegionDelegate *, RegionDelegate *>
AsIfFlatRegion::andnot_with (const Region &other) const
{
  if (empty ()) {

    //  Nothing to do
    return std::make_pair (new EmptyRegion (), new EmptyRegion ());

  } else if (other.empty () && ! strict_handling ()) {

    //  Nothing to do
    return std::make_pair (new EmptyRegion (), clone ());

  } else if (! bbox ().overlaps (other.bbox ()) && ! strict_handling ()) {

    //  Nothing to do
    return std::make_pair (new EmptyRegion (), clone ());

  } else {

    //  Generic case
    geo::EdgeProcessor ep (report_progress (), progress_desc ());
    ep.set_base_verbosity (base_verbosity ());

    //  count edges and reserve memory
    size_t n = 0;
    for (RegionIterator p (begin ()); ! p.at_end (); ++p) {
      n += p->vertices ();
    }
    for (RegionIterator p (other.begin ()); ! p.at_end (); ++p) {
      n += p->vertices ();
    }
    ep.reserve (n);

    //  insert the polygons into the processor
    n = 0;
    for (RegionIterator p (begin ()); ! p.at_end (); ++p, n += 2) {
      ep.insert (*p, n);
    }
    n = 1;
    for (RegionIterator p (other.begin ()); ! p.at_end (); ++p, n += 2) {
      ep.insert (*p, n);
    }

    std::unique_ptr<FlatRegion> new_region1 (new FlatRegion (true));
    geo::BooleanOp op1 (geo::BooleanOp::And);
    geo::ShapeGenerator pc1 (new_region1->raw_polygons (), true /*clear*/);
    geo::PolygonGenerator pg1 (pc1, false /*don't resolve holes*/, min_coherence ());

    std::unique_ptr<FlatRegion> new_region2 (new FlatRegion (true));
    geo::BooleanOp op2 (geo::BooleanOp::ANotB);
    geo::ShapeGenerator pc2 (new_region2->raw_polygons (), true /*clear*/);
    geo::PolygonGenerator pg2 (pc2, false /*don't resolve holes*/, min_coherence ());

    std::vector<std::pair<geo::EdgeSink *, geo::EdgeEvaluatorBase *> > procs;
    procs.push_back (std::make_pair (&pg1, &op1));
    procs.push_back (std::make_pair (&pg2, &op2));
    ep.process (procs);

    return std::make_pair (new_region1.release (), new_region2.release ());

  }
}

RegionDelegate *
AsIfFlatRegion::xor_with (const Region &other) const
{
  if (empty () && ! other.strict_handling ()) {

    return other.delegate ()->clone ();

  } else if (other.empty () && ! strict_handling ()) {

    return clone ();

  } else if (! bbox ().overlaps (other.bbox ()) && ! strict_handling () && ! other.strict_handling ()) {

    //  Simplified handling for disjunct case
    return or_with (other);

  } else {

    //  Generic case
    geo::EdgeProcessor ep (report_progress (), progress_desc ());
    ep.set_base_verbosity (base_verbosity ());

    //  count edges and reserve memory
    size_t n = 0;
    for (RegionIterator p (begin ()); ! p.at_end (); ++p) {
      n += p->vertices ();
    }
    for (RegionIterator p (other.begin ()); ! p.at_end (); ++p) {
      n += p->vertices ();
    }
    ep.reserve (n);

    //  insert the polygons into the processor
    n = 0;
    for (RegionIterator p (begin ()); ! p.at_end (); ++p, n += 2) {
      ep.insert (*p, n);
    }
    n = 1;
    for (RegionIterator p (other.begin ()); ! p.at_end (); ++p, n += 2) {
      ep.insert (*p, n);
    }

    std::unique_ptr<FlatRegion> new_region (new FlatRegion (true));
    geo::BooleanOp op (geo::BooleanOp::Xor);
    geo::ShapeGenerator pc (new_region->raw_polygons (), true /*clear*/);
    geo::PolygonGenerator pg (pc, false /*don't resolve holes*/, min_coherence ());
    ep.process (pg, op);

    return new_region.release ();

  }
}

RegionDelegate *
AsIfFlatRegion::or_with (const Region &other) const
{
  if (empty () && ! other.strict_handling ()) {

    return other.delegate ()->clone ();

  } else if (other.empty () && ! strict_handling ()) {

    //  Nothing to do
    return clone ();

  } else if (! bbox ().overlaps (other.bbox ()) && ! strict_handling () && ! other.strict_handling ()) {

    //  Simplified handling for disjunct case
    return add (other);

  } else {

    //  Generic case
    geo::EdgeProcessor ep (report_progress (), progress_desc ());
    ep.set_base_verbosity (base_verbosity ());

    //  count edges and reserve memory
    size_t n = 0;
    for (RegionIterator p (begin ()); ! p.at_end (); ++p) {
      n += p->vertices ();
    }
    for (RegionIterator p (other.begin ()); ! p.at_end (); ++p) {
      n += p->vertices ();
    }
    ep.reserve (n);

    //  insert the polygons into the processor
    n = 0;
    for (RegionIterator p (begin ()); ! p.at_end (); ++p, n += 2) {
      ep.insert (*p, n);
    }
    n = 1;
    for (RegionIterator p (other.begin ()); ! p.at_end (); ++p, n += 2) {
      ep.insert (*p, n);
    }

    std::unique_ptr<FlatRegion> new_region (new FlatRegion (true));
    geo::BooleanOp op (geo::BooleanOp::Or);
    geo::ShapeGenerator pc (new_region->raw_polygons (), true /*clear*/);
    geo::PolygonGenerator pg (pc, false /*don't resolve holes*/, min_coherence ());
    ep.process (pg, op);

    return new_region.release ();

  }
}

RegionDelegate *
AsIfFlatRegion::add (const Region &other) const
{
  FlatRegion *other_flat = dynamic_cast<FlatRegion *> (other.delegate ());
  if (other_flat) {

    std::unique_ptr<FlatRegion> new_region (new FlatRegion (*other_flat));
    new_region->set_is_merged (false);
    new_region->invalidate_cache ();

    size_t n = new_region->raw_polygons ().size () + count ();

    new_region->reserve (n);

    for (RegionIterator p (begin ()); ! p.at_end (); ++p) {
      new_region->raw_polygons ().insert (*p);
    }

    return new_region.release ();

  } else {

    std::unique_ptr<FlatRegion> new_region (new FlatRegion (false /*not merged*/));

    size_t n = count () + other.count ();

    new_region->reserve (n);

    for (RegionIterator p (begin ()); ! p.at_end (); ++p) {
      new_region->raw_polygons ().insert (*p);
    }
    for (RegionIterator p (other.begin ()); ! p.at_end (); ++p) {
      new_region->raw_polygons ().insert (*p);
    }

    return new_region.release ();

  }
}

void
AsIfFlatRegion::insert_into (Layout *layout, geo::cell_index_type into_cell, unsigned int into_layer) const
{
  //  improves performance when inserting an original layout into the same layout
  geo::LayoutLocker locker (layout);

  geo::Shapes &shapes = layout->cell (into_cell).shapes (into_layer);
  for (RegionIterator p (begin ()); ! p.at_end (); ++p) {
    shapes.insert (*p);
  }
}

bool
AsIfFlatRegion::equals (const Region &other) const
{
  if (empty () != other.empty ()) {
    return false;
  }
  if (count () != other.count ()) {
    return false;
  }
  RegionIterator o1 (begin ());
  RegionIterator o2 (other.begin ());
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
AsIfFlatRegion::less (const Region &other) const
{
  if (empty () != other.empty ()) {
    return empty () < other.empty ();
  }
  if (count () != other.count ()) {
    return (count () < other.count ());
  }
  RegionIterator o1 (begin ());
  RegionIterator o2 (other.begin ());
  while (! o1.at_end () && ! o2.at_end ()) {
    if (*o1 != *o2) {
      return *o1 < *o2;
    }
    ++o1;
    ++o2;
  }
  return false;
}

}

