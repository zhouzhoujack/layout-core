#include "geoHierProcessor.h"
#include "geoBoxScanner.h"
#include "geoRecursiveShapeIterator.h"
#include "geoBoxConvert.h"
#include "geoEdgeProcessor.h"
#include "geoPolygonGenerators.h"
#include "geoPolygonTools.h"
#include "geoLocalOperationUtils.h"
#include "geoEdgeBoolean.h"
#include "utilsLog.h"
#include "utilsTimer.h"
#include "utilsInternational.h"
#include "utilsProgress.h"

namespace geo
{

// ---------------------------------------------------------------------------------------------
//  local_operations implementation

template <class TS, class TI, class TR>
void local_operation<TS, TI, TR>::compute_local (geo::Layout *layout, const shape_interactions<TS, TI> &interactions, std::vector<std::unordered_set<TR> > &results, size_t max_vertex_count, double area_ratio, bool report_progress, const std::string &progress_desc) const
{
  if (interactions.num_subjects () <= 1 || ! requests_single_subjects ()) {

    do_compute_local (layout, interactions, results, max_vertex_count, area_ratio);

  } else {

    std::unique_ptr<geo::RelativeProgress> progress;
    if (report_progress) {
      progress.reset (new geo::RelativeProgress (progress_desc, interactions.size ()));
    }

    for (typename shape_interactions<TS, TI>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
      const TS &subject_shape = interactions.subject_shape (i->first);

      shape_interactions<TS, TI> single_interactions;

      if (on_empty_intruder_hint () == OnEmptyIntruderHint::Drop) {
        single_interactions.add_subject_shape (i->first, subject_shape);
      } else {
        //  this includes the subject-without-intruder "interaction"
        single_interactions.add_subject (i->first, subject_shape);
      }

      const std::vector<unsigned int> &intruders = interactions.intruders_for (i->first);
      for (typename std::vector<unsigned int>::const_iterator ii = intruders.begin (); ii != intruders.end (); ++ii) {
        const std::pair<unsigned int, TI> &is = interactions.intruder_shape (*ii);
        single_interactions.add_intruder_shape (*ii, is.first, is.second);
        single_interactions.add_interaction (i->first, *ii);
      }

      do_compute_local (layout, single_interactions, results, max_vertex_count, area_ratio);

      if (progress.get ()) {
        ++*progress;
      }

    }

  }
}


//  explicit instantiations
template class local_operation<geo::Polygon, geo::Polygon, geo::Polygon>;
template class local_operation<geo::Polygon, geo::Polygon, geo::Edge>;
template class local_operation<geo::Polygon, geo::Text, geo::Polygon>;
template class local_operation<geo::Polygon, geo::Text, geo::Text>;
template class local_operation<geo::Polygon, geo::Edge, geo::Polygon>;
template class local_operation<geo::Polygon, geo::Edge, geo::Edge>;
template class local_operation<geo::PolygonRef, geo::PolygonRef, geo::PolygonRef>;
template class local_operation<geo::PolygonRef, geo::Text, geo::PolygonRef>;
template class local_operation<geo::PolygonRef, geo::TextRef, geo::PolygonRef>;
template class local_operation<geo::PolygonRef, geo::TextRef, geo::TextRef>;
template class local_operation<geo::PolygonRef, geo::Edge, geo::PolygonRef>;
template class local_operation<geo::PolygonRef, geo::Edge, geo::Edge>;
template class local_operation<geo::PolygonRef, geo::PolygonRef, geo::EdgePair>;
template class local_operation<geo::PolygonRef, geo::PolygonRef, geo::Edge>;
template class local_operation<geo::Polygon, geo::Polygon, geo::EdgePair>;
template class local_operation<geo::Polygon, geo::TextRef, geo::TextRef>;
template class local_operation<geo::Edge, geo::Edge, geo::Edge>;
template class local_operation<geo::Edge, geo::PolygonRef, geo::Edge>;
template class local_operation<geo::Edge, geo::PolygonRef, geo::PolygonRef>;
template class local_operation<geo::Edge, geo::Edge, geo::EdgePair>;
template class local_operation<geo::TextRef, geo::PolygonRef, geo::PolygonRef>;
template class local_operation<geo::TextRef, geo::PolygonRef, geo::TextRef>;

// ---------------------------------------------------------------------------------------------
//  BoolAndOrNotLocalOperation implementation

BoolAndOrNotLocalOperation::BoolAndOrNotLocalOperation (bool is_and)
  : m_is_and (is_and)
{
  //  .. nothing yet ..
}

OnEmptyIntruderHint
BoolAndOrNotLocalOperation::on_empty_intruder_hint () const
{
  return m_is_and ? Drop : Copy;
}

std::string
BoolAndOrNotLocalOperation::description () const
{
  return m_is_and ? geo::to_string (tr ("AND operation")) : geo::to_string (tr ("NOT operation"));
}

void
BoolAndOrNotLocalOperation::do_compute_local (geo::Layout *layout, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &interactions, std::vector<std::unordered_set<geo::PolygonRef> > &results, size_t max_vertex_count, double area_ratio) const
{
  tl_assert (results.size () == 1);
  std::unordered_set<geo::PolygonRef> &result = results.front ();

  geo::EdgeProcessor ep;

  size_t p1 = 0, p2 = 1;

  std::set<geo::PolygonRef> others;
  for (shape_interactions<geo::PolygonRef, geo::PolygonRef>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
    for (shape_interactions<geo::PolygonRef, geo::PolygonRef>::iterator2 j = i->second.begin (); j != i->second.end (); ++j) {
      others.insert (interactions.intruder_shape (*j).second);
    }
  }

  for (shape_interactions<geo::PolygonRef, geo::PolygonRef>::iterator i = interactions.begin (); i != interactions.end (); ++i) {

    const geo::PolygonRef &subject = interactions.subject_shape (i->first);
    if (others.find (subject) != others.end ()) {
      if (m_is_and) {
        result.insert (subject);
      }
    } else if (i->second.empty ()) {
      //  shortcut (not: keep, and: drop)
      if (! m_is_and) {
        result.insert (subject);
      }
    } else {
      for (geo::PolygonRef::polygon_edge_iterator e = subject.begin_edge (); ! e.at_end(); ++e) {
        ep.insert (*e, p1);
      }
      p1 += 2;
    }

  }

  if (! others.empty () || p1 > 0) {

    for (std::set<geo::PolygonRef>::const_iterator o = others.begin (); o != others.end (); ++o) {
      for (geo::PolygonRef::polygon_edge_iterator e = o->begin_edge (); ! e.at_end(); ++e) {
        ep.insert (*e, p2);
      }
      p2 += 2;
    }

    geo::BooleanOp op (m_is_and ? geo::BooleanOp::And : geo::BooleanOp::ANotB);
    geo::PolygonRefGenerator pr (layout, result);
    geo::PolygonSplitter splitter (pr, area_ratio, max_vertex_count);
    geo::PolygonGenerator pg (splitter, true, true);
    ep.set_base_verbosity (50);
    ep.process (pg, op);

  }
}

// ---------------------------------------------------------------------------------------------
//  TwoBoolAndNotLocalOperation implementation

TwoBoolAndNotLocalOperation::TwoBoolAndNotLocalOperation ()
  : geo::local_operation<geo::PolygonRef, geo::PolygonRef, geo::PolygonRef> ()
{
  //  .. nothing yet ..
}

void
TwoBoolAndNotLocalOperation::do_compute_local (geo::Layout *layout, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &interactions, std::vector<std::unordered_set<geo::PolygonRef> > &results, size_t max_vertex_count, double area_ratio) const
{
  tl_assert (results.size () == 2);

  geo::EdgeProcessor ep;

  std::unordered_set<geo::PolygonRef> &result0 = results [0];
  std::unordered_set<geo::PolygonRef> &result1 = results [1];

  size_t p1 = 0, p2 = 1;

  std::set<geo::PolygonRef> others;
  for (geo::shape_interactions<geo::PolygonRef, geo::PolygonRef>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
    for (geo::shape_interactions<geo::PolygonRef, geo::PolygonRef>::iterator2 j = i->second.begin (); j != i->second.end (); ++j) {
      others.insert (interactions.intruder_shape (*j).second);
    }
  }

  for (geo::shape_interactions<geo::PolygonRef, geo::PolygonRef>::iterator i = interactions.begin (); i != interactions.end (); ++i) {

    const geo::PolygonRef &subject = interactions.subject_shape (i->first);
    if (others.find (subject) != others.end ()) {
      result0.insert (subject);
    } else if (i->second.empty ()) {
      //  shortcut (not: keep, and: drop)
      result1.insert (subject);
    } else {
      for (geo::PolygonRef::polygon_edge_iterator e = subject.begin_edge (); ! e.at_end(); ++e) {
        ep.insert (*e, p1);
      }
      p1 += 2;
    }

  }

  if (! others.empty () || p1 > 0) {

    for (std::set<geo::PolygonRef>::const_iterator o = others.begin (); o != others.end (); ++o) {
      for (geo::PolygonRef::polygon_edge_iterator e = o->begin_edge (); ! e.at_end(); ++e) {
        ep.insert (*e, p2);
      }
      p2 += 2;
    }

    geo::BooleanOp op0 (geo::BooleanOp::And);
    geo::PolygonRefGenerator pr0 (layout, result0);
    geo::PolygonSplitter splitter0 (pr0, area_ratio, max_vertex_count);
    geo::PolygonGenerator pg0 (splitter0, true, true);

    geo::BooleanOp op1 (geo::BooleanOp::ANotB);
    geo::PolygonRefGenerator pr1 (layout, result1);
    geo::PolygonSplitter splitter1 (pr1, area_ratio, max_vertex_count);
    geo::PolygonGenerator pg1 (splitter1, true, true);

    ep.set_base_verbosity (50);

    std::vector<std::pair<geo::EdgeSink *, geo::EdgeEvaluatorBase *> > procs;
    procs.push_back (std::make_pair (&pg0, &op0));
    procs.push_back (std::make_pair (&pg1, &op1));
    ep.process (procs);

  }

}

std::string TwoBoolAndNotLocalOperation::description () const
{
  return geo::to_string (tr ("ANDNOT operation"));
}

// ---------------------------------------------------------------------------------------------

SelfOverlapMergeLocalOperation::SelfOverlapMergeLocalOperation (unsigned int wrap_count)
  : m_wrap_count (wrap_count)
{
  //  .. nothing yet ..
}

void
SelfOverlapMergeLocalOperation::do_compute_local (geo::Layout *layout, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &interactions, std::vector<std::unordered_set<geo::PolygonRef> > &results, size_t /*max_vertex_count*/, double /*area_ratio*/) const
{
  tl_assert (results.size () == 1);
  std::unordered_set<geo::PolygonRef> &result = results.front ();

  if (m_wrap_count == 0) {
    return;
  }

  geo::EdgeProcessor ep;

  size_t p1 = 0, p2 = 1;
  std::set<unsigned int> seen;

  for (shape_interactions<geo::PolygonRef, geo::PolygonRef>::iterator i = interactions.begin (); i != interactions.end (); ++i) {

    if (seen.find (i->first) == seen.end ()) {
      seen.insert (i->first);
      const geo::PolygonRef &subject = interactions.subject_shape (i->first);
      for (geo::PolygonRef::polygon_edge_iterator e = subject.begin_edge (); ! e.at_end(); ++e) {
        ep.insert (*e, p1);
      }
      p1 += 2;
    }

    for (geo::shape_interactions<geo::PolygonRef, geo::PolygonRef>::iterator2 o = i->second.begin (); o != i->second.end (); ++o) {
      //  don't take the same (really the same, not an identical one) shape twice - the interaction
      //  set does not take care to list just one copy of the same item on the intruder side.
      if (seen.find (*o) == seen.end ()) {
        seen.insert (*o);
        const geo::PolygonRef &intruder = interactions.intruder_shape (*o).second;
        for (geo::PolygonRef::polygon_edge_iterator e = intruder.begin_edge (); ! e.at_end(); ++e) {
          ep.insert (*e, p2);
        }
        p2 += 2;
      }
    }

  }

  geo::MergeOp op (m_wrap_count - 1);
  geo::PolygonRefGenerator pr (layout, result);
  geo::PolygonGenerator pg (pr, true, true);
  ep.set_base_verbosity (50);
  ep.process (pg, op);
}

OnEmptyIntruderHint SelfOverlapMergeLocalOperation::on_empty_intruder_hint () const
{
  return m_wrap_count > 1 ? Drop : Copy;
}

std::string SelfOverlapMergeLocalOperation::description () const
{
  return geo::sprintf (geo::to_string (tr ("Self-overlap (wrap count %d)")), int (m_wrap_count));
}

// ---------------------------------------------------------------------------------------------
//  EdgeBoolAndOrNotLocalOperation implementation

EdgeBoolAndOrNotLocalOperation::EdgeBoolAndOrNotLocalOperation (EdgeBoolOp op)
  : m_op (op)
{
  //  .. nothing yet ..
}

OnEmptyIntruderHint
EdgeBoolAndOrNotLocalOperation::on_empty_intruder_hint () const
{
  return (m_op == EdgeAnd || m_op == EdgeIntersections) ? Drop : Copy;
}

std::string
EdgeBoolAndOrNotLocalOperation::description () const
{
  if (m_op == EdgeIntersections) {
    return geo::to_string (tr ("Edge INTERSECTION operation"));
  } else if (m_op == EdgeAnd) {
    return geo::to_string (tr ("Edge AND operation"));
  } else if (m_op == EdgeNot) {
    return geo::to_string (tr ("Edge NOT operation"));
  } else {
    return std::string ();
  }
}

void
EdgeBoolAndOrNotLocalOperation::do_compute_local (geo::Layout * /*layout*/, const shape_interactions<geo::Edge, geo::Edge> &interactions, std::vector<std::unordered_set<geo::Edge> > &results, size_t /*max_vertex_count*/, double /*area_ratio*/) const
{
  tl_assert (results.size () == 1);
  std::unordered_set<geo::Edge> &result = results.front ();

  EdgeBooleanClusterCollector<std::unordered_set<geo::Edge> > cluster_collector (&result, m_op);

  geo::box_scanner<geo::Edge, size_t> scanner;

  std::set<geo::Edge> others;
  for (shape_interactions<geo::Edge, geo::Edge>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
    for (shape_interactions<geo::Edge, geo::Edge>::iterator2 j = i->second.begin (); j != i->second.end (); ++j) {
      others.insert (interactions.intruder_shape (*j).second);
    }
  }

  bool any_subject = false;
  bool is_and = (m_op == EdgeAnd || m_op == EdgeIntersections);

  for (shape_interactions<geo::Edge, geo::Edge>::iterator i = interactions.begin (); i != interactions.end (); ++i) {

    const geo::Edge &subject = interactions.subject_shape (i->first);
    if (others.find (subject) != others.end ()) {
      if (is_and) {
        result.insert (subject);
      }
    } else if (i->second.empty ()) {
      //  shortcut (not: keep, and: drop)
      if (! is_and) {
        result.insert (subject);
      }
    } else {
      scanner.insert (&subject, 0);
      any_subject = true;
    }

  }

  if (! others.empty () || any_subject) {

    for (std::set<geo::Edge>::const_iterator o = others.begin (); o != others.end (); ++o) {
      scanner.insert (o.operator-> (), 1);
    }

    scanner.process (cluster_collector, 1, geo::box_convert<geo::Edge> ());

  }
}

// ---------------------------------------------------------------------------------------------
//  EdgeToPolygonLocalOperation implementation

EdgeToPolygonLocalOperation::EdgeToPolygonLocalOperation (bool outside, bool include_borders)
  : m_outside (outside), m_include_borders (include_borders)
{
  //  .. nothing yet ..
}

OnEmptyIntruderHint
EdgeToPolygonLocalOperation::on_empty_intruder_hint () const
{
  return m_outside ? Copy : Drop;
}

std::string
EdgeToPolygonLocalOperation::description () const
{
  return geo::to_string (m_outside ? tr ("Edge to polygon AND/INSIDE") : tr ("Edge to polygons NOT/OUTSIDE"));
}

void
EdgeToPolygonLocalOperation::do_compute_local (geo::Layout * /*layout*/, const shape_interactions<geo::Edge, geo::PolygonRef> &interactions, std::vector<std::unordered_set<geo::Edge> > &results, size_t /*max_vertex_count*/, double /*area_ratio*/) const
{
  tl_assert (results.size () == 1);
  std::unordered_set<geo::Edge> &result = results.front ();

  geo::EdgeProcessor ep;

  std::set<geo::PolygonRef> others;
  for (shape_interactions<geo::Edge, geo::PolygonRef>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
    for (shape_interactions<geo::Edge, geo::PolygonRef>::iterator2 j = i->second.begin (); j != i->second.end (); ++j) {
      others.insert (interactions.intruder_shape (*j).second);
    }
  }

  bool any_subject = false;

  for (shape_interactions<geo::Edge, geo::PolygonRef>::iterator i = interactions.begin (); i != interactions.end (); ++i) {

    const geo::Edge &subject = interactions.subject_shape (i->first);
    if (i->second.empty ()) {
      //  shortcut (outside: keep, otherwise: drop)
      if (m_outside) {
        result.insert (subject);
      }
    } else {
      ep.insert (subject, 1);
      any_subject = true;
    }

  }

  if (! others.empty () || any_subject) {

    for (std::set<geo::PolygonRef>::const_iterator o = others.begin (); o != others.end (); ++o) {
      for (geo::PolygonRef::polygon_edge_iterator e = o->begin_edge (); ! e.at_end (); ++e) {
        ep.insert (*e, 0);
      }
    }

    geo::EdgeToEdgeSetGenerator cc (result);
    geo::EdgePolygonOp op (m_outside, m_include_borders);
    ep.process (cc, op);

  }
}

}

