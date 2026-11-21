#include "geoCompoundOperation.h"
#include "geoRegion.h"
#include "geoRegionUtils.h"

namespace geo
{

// ---------------------------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//  CompoundOperation implementation

CompoundRegionOperationNode::CompoundRegionOperationNode ()
  : m_dist (0)
{
  // .. nothing yet ..
}

CompoundRegionOperationNode::~CompoundRegionOperationNode ()
{
  // .. nothing yet ..
}

std::string CompoundRegionOperationNode::description() const
{
  if (m_description.empty ()) {
    return generated_description ();
  } else {
    return m_description;
  }
}

void
CompoundRegionOperationNode::set_description (const std::string &d)
{
  m_description = d;
}

std::string
CompoundRegionOperationNode::generated_description () const
{
  return std::string ();
}

bool
CompoundRegionOperationNode::is_merged () const
{
  std::vector<geo::Region *> iv = inputs ();
  //  NOTE: the primary is supposed to be merged always (except in raw mode)
  return iv.size () == 1 && (is_subject_regionptr (iv.front ()) || iv.front ()->is_merged ());
}

bool
CompoundRegionOperationNode::has_external_inputs () const
{
  std::vector<geo::Region *> iv = inputs ();
  return iv.size () == 1 && ! is_subject_regionptr (iv.front ());
}

// ---------------------------------------------------------------------------------------------

static void translate (geo::Layout *layout, const std::vector<std::unordered_set<geo::Polygon> > &in, std::vector<std::unordered_set<geo::PolygonRef> > &out)
{
  tl_assert (layout != 0);
  if (out.size () <= in.size ()) {
    out.resize (in.size ());
  }
  for (std::vector<std::unordered_set<geo::Polygon> >::const_iterator r = in.begin (); r != in.end (); ++r) {
    std::unordered_set<geo::PolygonRef> &o = out[r - in.begin ()];
    for (std::unordered_set<geo::Polygon>::const_iterator p = r->begin (); p != r->end (); ++p) {
      o.insert (geo::PolygonRef (*p, layout->shape_repository ()));
    }
  }
}

static void translate (geo::Layout *, const std::vector<std::unordered_set<geo::PolygonRef> > &in, std::vector<std::unordered_set<geo::Polygon> > &out)
{
  if (out.size () <= in.size ()) {
    out.resize (in.size ());
  }
  for (std::vector<std::unordered_set<geo::PolygonRef> >::const_iterator r = in.begin (); r != in.end (); ++r) {
    std::unordered_set<geo::Polygon> &o = out[r - in.begin ()];
    for (std::unordered_set<geo::PolygonRef>::const_iterator p = r->begin (); p != r->end (); ++p) {
      o.insert (p->obj ().transformed (p->trans ()));
    }
  }
}

void
CompoundRegionOperationNode::compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::Polygon, geo::Polygon> &interactions, std::vector<std::unordered_set<geo::PolygonRef> > &results, size_t max_vertex_count, double area_ratio) const
{
  std::vector<std::unordered_set<geo::Polygon> > intermediate;
  intermediate.push_back (std::unordered_set<geo::Polygon> ());
  implement_compute_local (cache, layout, interactions, intermediate, max_vertex_count, area_ratio);
  translate (layout, intermediate, results);
}

void
CompoundRegionOperationNode::compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &interactions, std::vector<std::unordered_set<geo::Polygon> > &results, size_t max_vertex_count, double area_ratio) const
{
  std::vector<std::unordered_set<geo::PolygonRef> > intermediate;
  intermediate.push_back (std::unordered_set<geo::PolygonRef> ());
  implement_compute_local (cache, layout, interactions, intermediate, max_vertex_count, area_ratio);
  translate (layout, intermediate, results);
}

// ---------------------------------------------------------------------------------------------

CompoundRegionOperationPrimaryNode::CompoundRegionOperationPrimaryNode ()
{
  set_description (std::string ("this"));
}

CompoundRegionOperationPrimaryNode::~CompoundRegionOperationPrimaryNode ()
{
  //  .. nothing yet ..
}

std::vector<geo::Region *> CompoundRegionOperationPrimaryNode::inputs () const
{
  std::vector<geo::Region *> is;
  is.push_back (subject_regionptr ());
  return is;
}

void CompoundRegionOperationPrimaryNode::do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout *, const shape_interactions<geo::Polygon, geo::Polygon> &interactions, std::vector<std::unordered_set<geo::Polygon> > &results, size_t, double) const
{
  for (shape_interactions<geo::Polygon, geo::Polygon>::subject_iterator i = interactions.begin_subjects (); i != interactions.end_subjects (); ++i) {
    results.front ().insert (i->second);
  }
}

void CompoundRegionOperationPrimaryNode::do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout *, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &interactions, std::vector<std::unordered_set<geo::PolygonRef> > &results, size_t, double) const
{
  for (shape_interactions<geo::PolygonRef, geo::PolygonRef>::subject_iterator i = interactions.begin_subjects (); i != interactions.end_subjects (); ++i) {
    results.front ().insert (i->second);
  }
}

// ---------------------------------------------------------------------------------------------

CompoundRegionOperationSecondaryNode::CompoundRegionOperationSecondaryNode (geo::Region *input)
  : mp_input (input)
{
  set_description ("other");
}

CompoundRegionOperationSecondaryNode::~CompoundRegionOperationSecondaryNode ()
{
  //  .. nothing yet ..
}


std::vector<geo::Region *> CompoundRegionOperationSecondaryNode::inputs () const
{
  std::vector<geo::Region *> iv;
  iv.push_back (mp_input);
  return iv;
}

void CompoundRegionOperationSecondaryNode::do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout *, const shape_interactions<geo::Polygon, geo::Polygon> &interactions, std::vector<std::unordered_set<geo::Polygon> > &results, size_t, double) const
{
  for (shape_interactions<geo::Polygon, geo::Polygon>::intruder_iterator i = interactions.begin_intruders (); i != interactions.end_intruders (); ++i) {
    results.front ().insert (i->second.second);
  }
}

void CompoundRegionOperationSecondaryNode::do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout *, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &interactions, std::vector<std::unordered_set<geo::PolygonRef> > &results, size_t, double) const
{
  for (shape_interactions<geo::PolygonRef, geo::PolygonRef>::intruder_iterator i = interactions.begin_intruders (); i != interactions.end_intruders (); ++i) {
    results.front ().insert (i->second.second);
  }
}

// ---------------------------------------------------------------------------------------------

CompoundRegionOperationForeignNode::CompoundRegionOperationForeignNode ()
{
  set_description ("foreign");
}

CompoundRegionOperationForeignNode::~CompoundRegionOperationForeignNode ()
{
  //  .. nothing yet ..
}


std::vector<geo::Region *> CompoundRegionOperationForeignNode::inputs () const
{
  std::vector<geo::Region *> iv;
  iv.push_back (foreign_regionptr ());
  return iv;
}

void CompoundRegionOperationForeignNode::do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout *, const shape_interactions<geo::Polygon, geo::Polygon> &interactions, std::vector<std::unordered_set<geo::Polygon> > &results, size_t, double) const
{
  for (shape_interactions<geo::Polygon, geo::Polygon>::intruder_iterator i = interactions.begin_intruders (); i != interactions.end_intruders (); ++i) {
    results.front ().insert (i->second.second);
  }
}

void CompoundRegionOperationForeignNode::do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout *, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &interactions, std::vector<std::unordered_set<geo::PolygonRef> > &results, size_t, double) const
{
  for (shape_interactions<geo::PolygonRef, geo::PolygonRef>::intruder_iterator i = interactions.begin_intruders (); i != interactions.end_intruders (); ++i) {
    results.front ().insert (i->second.second);
  }
}

// ---------------------------------------------------------------------------------------------

CompoundTransformationReducer::CompoundTransformationReducer ()
{
  //  .. nothing yet ..
}

void
CompoundTransformationReducer::add (const geo::TransformationReducer *reducer)
{
  if (reducer) {
    m_vars.push_back (reducer);
  }
}

geo::Trans
CompoundTransformationReducer::reduce_trans (const geo::Trans &trans) const
{
  geo::Trans t = trans;
  for (std::vector<const geo::TransformationReducer *>::const_iterator v = m_vars.begin (); v != m_vars.end (); ++v) {
    (*v)->reduce_trans (t);
  }
  return t;
}

geo::ICplxTrans
CompoundTransformationReducer::reduce_trans (const geo::ICplxTrans &trans) const
{
  geo::ICplxTrans t = trans;
  for (std::vector<const geo::TransformationReducer *>::const_iterator v = m_vars.begin (); v != m_vars.end (); ++v) {
    (*v)->reduce_trans (t);
  }
  return t;
}

geo::Trans
CompoundTransformationReducer::reduce (const geo::Trans &trans) const
{
  geo::Trans t = trans;
  for (std::vector<const geo::TransformationReducer *>::const_iterator v = m_vars.begin (); v != m_vars.end (); ++v) {
    (*v)->reduce (t);
  }
  return t;
}

geo::ICplxTrans
CompoundTransformationReducer::reduce (const geo::ICplxTrans &trans) const
{
  geo::ICplxTrans t = trans;
  for (std::vector<const geo::TransformationReducer *>::const_iterator v = m_vars.begin (); v != m_vars.end (); ++v) {
    (*v)->reduce (t);
  }
  return t;
}

bool
CompoundTransformationReducer::is_translation_invariant () const
{
  for (std::vector<const geo::TransformationReducer *>::const_iterator v = m_vars.begin (); v != m_vars.end (); ++v) {
    if (! (*v)->is_translation_invariant ()) {
      return false;
    }
  }
  return true;
}

// ---------------------------------------------------------------------------------------------

CompoundRegionMultiInputOperationNode::CompoundRegionMultiInputOperationNode (const std::vector<CompoundRegionOperationNode *> &children)
{
  for (std::vector<CompoundRegionOperationNode *>::const_iterator c = children.begin (); c != children.end (); ++c) {
    (*c)->keep ();
    m_children.push_back (*c);
  }
  init ();
}

CompoundRegionMultiInputOperationNode::CompoundRegionMultiInputOperationNode ()
{
  init ();
}

CompoundRegionMultiInputOperationNode::CompoundRegionMultiInputOperationNode (CompoundRegionOperationNode *child)
{
  child->keep ();
  m_children.push_back (child);
  init ();
}

CompoundRegionMultiInputOperationNode::CompoundRegionMultiInputOperationNode (CompoundRegionOperationNode *a, CompoundRegionOperationNode *b)
{
  a->keep ();
  m_children.push_back (a);
  b->keep ();
  m_children.push_back (b);
  init ();
}

void
CompoundRegionMultiInputOperationNode::init ()
{
  std::map<geo::Region *, unsigned int> input_index;

  unsigned int child_index = 0;
  for (geo::shared_collection<CompoundRegionOperationNode>::iterator i = m_children.begin (); i != m_children.end (); ++i, ++child_index) {

    std::vector<geo::Region *> child_inputs = i->inputs ();
    for (std::vector<geo::Region *>::const_iterator ii = child_inputs.begin (); ii != child_inputs.end (); ++ii) {

      std::map<geo::Region *, unsigned int>::const_iterator im = input_index.find (*ii);
      unsigned int li = (unsigned int) m_inputs.size ();
      if (im != input_index.end ()) {
        li = im->second;
      } else {
        m_inputs.push_back (*ii);
        input_index.insert (std::make_pair (*ii, li));
      }

      m_map_layer_to_child [std::make_pair (child_index, li)] = (unsigned int) (ii - child_inputs.begin ());

    }

  }

  //  build the reducer
  for (geo::shared_collection<CompoundRegionOperationNode>::iterator i = m_children.begin (); i != m_children.end (); ++i) {
    m_vars.add (i->vars ());
  }
}

CompoundRegionMultiInputOperationNode::~CompoundRegionMultiInputOperationNode ()
{
  //  .. nothing yet ..
}

geo::Coord
CompoundRegionMultiInputOperationNode::computed_dist () const
{
  geo::Coord d = 0;
  for (geo::shared_collection<CompoundRegionOperationNode>::const_iterator i = m_children.begin (); i != m_children.end (); ++i) {
    d = std::max (d, i->dist ());
  }
  return d;
}

std::string
CompoundRegionMultiInputOperationNode::generated_description () const
{
  std::string r = "(";
  for (geo::shared_collection<CompoundRegionOperationNode>::const_iterator i = m_children.begin (); i != m_children.end (); ++i) {
    if (i != m_children.begin ()) {
      r += ",";
    }
    r += i->description ();
  }
  return r;
}

CompoundRegionOperationNode *
CompoundRegionMultiInputOperationNode::child (unsigned int index)
{
  geo::shared_collection<CompoundRegionOperationNode>::iterator c = m_children.begin ();
  while (c != m_children.end () && index > 0) {
    ++c;
    --index;
  }
  return c == m_children.end () ? 0 : c.operator-> ();
}

const CompoundRegionOperationNode *
CompoundRegionMultiInputOperationNode::child (unsigned int index) const
{
  return const_cast<CompoundRegionMultiInputOperationNode *> (this)->child (index);
}

const TransformationReducer *
CompoundRegionMultiInputOperationNode::vars () const
{
  return (m_vars.is_empty () ? &m_vars : 0);
}

bool
CompoundRegionMultiInputOperationNode::wants_variants () const
{
  for (geo::shared_collection<CompoundRegionOperationNode>::const_iterator i = m_children.begin (); i != m_children.end (); ++i) {
    if (i->wants_variants ()) {
      return true;
    }
  }
  return false;
}

// ---------------------------------------------------------------------------------------------

CompoundRegionLogicalBoolOperationNode::CompoundRegionLogicalBoolOperationNode (LogicalOp op, bool invert, const std::vector<CompoundRegionOperationNode *> &inputs)
  : CompoundRegionMultiInputOperationNode (inputs), m_op (op), m_invert (invert)
{
  //  .. nothing yet ..
}

CompoundRegionLogicalBoolOperationNode::ResultType
CompoundRegionLogicalBoolOperationNode::result_type () const
{
  return Region;
}

std::string CompoundRegionLogicalBoolOperationNode::generated_description () const
{
  std::string r;
  if (m_invert) {
    r = "!";
  }
  if (m_op == And) {
    r += "and";
  } else if (m_op == Or) {
    r += "or";
  }
  return r + CompoundRegionMultiInputOperationNode::generated_description ();
}

template <class T>
void CompoundRegionLogicalBoolOperationNode::implement_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<T, T> &interactions, std::vector<std::unordered_set<T> > &results, size_t max_vertex_count, double area_ratio) const
{
  bool ok = (m_op == And ? true : false);

  const T &subject_shape = interactions.subject_shape (interactions.begin ()->first);

  for (unsigned int ci = 0; ci < children (); ++ci) {

    shape_interactions<T, T> child_interactions_computed;
    const shape_interactions<T, T> &child_interactions = interactions_for_child<T, T> (interactions, ci, child_interactions_computed);

    const CompoundRegionOperationNode *node = child (ci);

    bool any = node->compute_local_bool<T> (cache, layout, child_interactions, max_vertex_count, area_ratio);

    if (m_op == And) {
      if (! any) {
        ok = false;
        break;
      }
    } else if (m_op == Or) {
      if (any) {
        ok = true;
        break;
      }
    }

  }

  if (m_invert) {
    ok = ! ok;
  }

  if (ok) {
    tl_assert (! results.empty ());
    results.front ().insert (subject_shape);
  }
}

// ---------------------------------------------------------------------------------------------

CompoundRegionCountFilterNode::CompoundRegionCountFilterNode (CompoundRegionOperationNode *input, bool invert, size_t min_count, size_t max_count)
  : CompoundRegionMultiInputOperationNode (input), m_min_count (min_count), m_max_count (max_count), m_invert (invert)
{
  //  .. nothing yet ..
}

CompoundRegionCountFilterNode::~CompoundRegionCountFilterNode ()
{
  //  .. nothing yet ..
}

std::string CompoundRegionCountFilterNode::generated_description () const
{
  return std::string ("count_filter") + CompoundRegionMultiInputOperationNode::generated_description ();
}

// ---------------------------------------------------------------------------------------------

CompoundRegionGeometricalBoolOperationNode::CompoundRegionGeometricalBoolOperationNode (GeometricalOp op, CompoundRegionOperationNode *a, CompoundRegionOperationNode *b)
  : CompoundRegionMultiInputOperationNode (a, b), m_op (op)
{
  //  .. nothing yet ..
}

std::string
CompoundRegionGeometricalBoolOperationNode::generated_description () const
{
  std::string r;
  if (m_op == And) {
    r = "and";
  } else if (m_op == Or) {
    r = "or";
  } else if (m_op == Xor) {
    r = "xor";
  } else if (m_op == Not) {
    r = "not";
  }
  r += CompoundRegionMultiInputOperationNode::generated_description ();
  return r;
}

CompoundRegionGeometricalBoolOperationNode::ResultType
CompoundRegionGeometricalBoolOperationNode::result_type () const
{
  ResultType res_a = child (0)->result_type ();
  ResultType res_b = child (1)->result_type ();

  if (res_a == Edges || (res_a == Region && res_b == Edges && m_op == And)) {
    return Edges;
  } else {
    return res_a;
  }
}

geo::Coord
CompoundRegionGeometricalBoolOperationNode::computed_dist () const
{
  geo::Coord d = CompoundRegionMultiInputOperationNode::computed_dist ();

  ResultType res_a = child (0)->result_type ();
  ResultType res_b = child (1)->result_type ();

  if (res_a == Region && res_b == Region) {
    return d;  //  overlapping is sufficient
  } else {
    return d + 1;  //  we need "touching" if edges are involved
  }
}

template <class T1, class T2, class TR>
static
void run_bool (CompoundRegionGeometricalBoolOperationNode::GeometricalOp, geo::Layout *, const std::unordered_set<T1> &, const std::unordered_set<T2> &, const std::unordered_set<TR> &)
{
  tl_assert (false);
}

static void
init_region (geo::Region &r, const std::unordered_set<geo::PolygonRef> &p)
{
  for (std::unordered_set<geo::PolygonRef>::const_iterator i = p.begin (); i != p.end (); ++i) {
    r.insert (i->obj ().transformed (i->trans ()));
  }
}

static void
init_region (geo::Region &r, const std::unordered_set<geo::Polygon> &p)
{
  for (std::unordered_set<geo::Polygon>::const_iterator i = p.begin (); i != p.end (); ++i) {
    r.insert (*i);
  }
}

static void
init_edges (geo::Edges &ee, const std::unordered_set<geo::Edge> &e)
{
  for (std::unordered_set<geo::Edge>::const_iterator i = e.begin (); i != e.end (); ++i) {
    ee.insert (*i);
  }
}

static void
write_result (geo::Layout *layout, std::unordered_set<geo::PolygonRef> &results, const geo::Region &r)
{
  for (geo::Region::const_iterator p = r.begin (); ! p.at_end (); ++p) {
    results.insert (geo::PolygonRef (*p, layout->shape_repository ()));
  }
}

static void
write_result (geo::Layout *, std::unordered_set<geo::Polygon> &results, const geo::Region &r)
{
  for (geo::Region::const_iterator p = r.begin (); ! p.at_end (); ++p) {
    results.insert (*p);
  }
}

static void
write_result (geo::Layout *, std::unordered_set<geo::Edge> &results, const geo::Edges &e)
{
  for (geo::Edges::const_iterator i = e.begin (); ! i.at_end (); ++i) {
    results.insert (*i);
  }
}

template <class T>
static void
run_poly_bool (CompoundRegionGeometricalBoolOperationNode::GeometricalOp op, geo::Layout *layout, const std::unordered_set<T> &a, const std::unordered_set<T> &b, std::unordered_set<T> &res)
{
  //  TODO: it's more efficient to feed the EP directly for polygon-to-polygon bools
  geo::Region ra, rb;
  init_region (ra, a);
  init_region (rb, b);

  if (op == CompoundRegionGeometricalBoolOperationNode::And) {
    write_result (layout, res, ra & rb);
  } else if (op == CompoundRegionGeometricalBoolOperationNode::Or) {
    write_result (layout, res, ra + rb);
  } else if (op == CompoundRegionGeometricalBoolOperationNode::Xor) {
    write_result (layout, res, ra ^ rb);
  } else if (op == CompoundRegionGeometricalBoolOperationNode::Not) {
    write_result (layout, res, ra - rb);
  }
}

static void
run_bool (CompoundRegionGeometricalBoolOperationNode::GeometricalOp op, geo::Layout *layout, const std::unordered_set<geo::PolygonRef> &a, const std::unordered_set<geo::PolygonRef> &b, std::unordered_set<geo::PolygonRef> &res)
{
  run_poly_bool (op, layout, a, b, res);
}

static void
run_bool (CompoundRegionGeometricalBoolOperationNode::GeometricalOp op, geo::Layout *layout, const std::unordered_set<geo::Polygon> &a, const std::unordered_set<geo::Polygon> &b, std::unordered_set<geo::Polygon> &res)
{
  run_poly_bool (op, layout, a, b, res);
}

template <class T>
static void
run_poly_vs_edge_bool (CompoundRegionGeometricalBoolOperationNode::GeometricalOp op, geo::Layout *layout, const std::unordered_set<T> &a, const std::unordered_set<geo::Edge> &b, std::unordered_set<geo::Edge> &res)
{
  if (op != CompoundRegionGeometricalBoolOperationNode::And) {
    return;
  }

  //  TODO: it's more efficient to feed the EP directly for polygon-to-polygon bools
  geo::Region ra;
  init_region (ra, a);

  geo::Edges eb;
  init_edges (eb, b);

  if (eb.empty ()) {
    write_result (layout, res, eb);
  } else {
    write_result (layout, res, eb & ra);
  }
}

static void
run_bool (CompoundRegionGeometricalBoolOperationNode::GeometricalOp op, geo::Layout *layout, const std::unordered_set<geo::Polygon> &a, const std::unordered_set<geo::Edge> &b, std::unordered_set<geo::Edge> &res)
{
  run_poly_vs_edge_bool (op, layout, a, b, res);
}

static void
run_bool (CompoundRegionGeometricalBoolOperationNode::GeometricalOp op, geo::Layout *layout, const std::unordered_set<geo::PolygonRef> &a, const std::unordered_set<geo::Edge> &b, std::unordered_set<geo::Edge> &res)
{
  run_poly_vs_edge_bool (op, layout, a, b, res);
}

template <class T>
static void
run_edge_vs_poly_bool (CompoundRegionGeometricalBoolOperationNode::GeometricalOp op, geo::Layout *layout, const std::unordered_set<geo::Edge> &a, const std::unordered_set<T> &b, std::unordered_set<geo::Edge> &res)
{
  if (op != CompoundRegionGeometricalBoolOperationNode::And && op != CompoundRegionGeometricalBoolOperationNode::Not) {
    return;
  }

  geo::Edges ea;
  init_edges (ea, a);

  //  TODO: it's more efficient to feed the EP directly for polygon-to-polygon bools
  geo::Region rb;
  init_region (rb, b);

  if (op == CompoundRegionGeometricalBoolOperationNode::And) {
    write_result (layout, res, ea & rb);
  } else if (op == CompoundRegionGeometricalBoolOperationNode::Not) {
    write_result (layout, res, ea - rb);
  }
}

static void
run_bool (CompoundRegionGeometricalBoolOperationNode::GeometricalOp op, geo::Layout *layout, const std::unordered_set<geo::Edge> &a, const std::unordered_set<geo::PolygonRef> &b, std::unordered_set<geo::Edge> &res)
{
  run_edge_vs_poly_bool (op, layout, a, b, res);
}

static void
run_bool (CompoundRegionGeometricalBoolOperationNode::GeometricalOp op, geo::Layout *layout, const std::unordered_set<geo::Edge> &a, const std::unordered_set<geo::Polygon> &b, std::unordered_set<geo::Edge> &res)
{
  run_edge_vs_poly_bool (op, layout, a, b, res);
}

static void
run_bool (CompoundRegionGeometricalBoolOperationNode::GeometricalOp op, geo::Layout *layout, const std::unordered_set<geo::Edge> &a, const std::unordered_set<geo::Edge> &b, std::unordered_set<geo::Edge> &res)
{
  geo::Edges ea, eb;
  init_edges (ea, a);
  init_edges (eb, b);

  if (op == CompoundRegionGeometricalBoolOperationNode::And) {
    write_result (layout, res, ea & eb);
  } else if (op == CompoundRegionGeometricalBoolOperationNode::Or) {
    write_result (layout, res, ea | eb);
  } else if (op == CompoundRegionGeometricalBoolOperationNode::Xor) {
    write_result (layout, res, ea ^ eb);
  } else if (op == CompoundRegionGeometricalBoolOperationNode::Not) {
    write_result (layout, res, ea - eb);
  }
}

template <class T>
static void copy_results (std::vector<std::unordered_set<T> > &res, const std::vector<std::unordered_set<T> > &in)
{
  res.front ().insert (in.front ().begin (), in.front ().end ());
}

template <class T, class R>
static void copy_results (std::vector<std::unordered_set<R> > &, const std::vector<std::unordered_set<T> > &)
{
  // don't copy
}

template <class T, class T1, class T2, class TR>
void
CompoundRegionGeometricalBoolOperationNode::implement_bool (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<T, T> &interactions, std::vector<std::unordered_set<TR> > &results, size_t max_vertex_count, double area_ratio) const
{
  std::vector<std::unordered_set<T1> > one_a;
  one_a.push_back (std::unordered_set<T1> ());

  shape_interactions<T, T> computed_a;
  child (0)->compute_local (cache, layout, interactions_for_child (interactions, 0, computed_a), one_a, max_vertex_count, area_ratio);

  if (one_a.front ().empty ()) {

    if (m_op == GeometricalOp::And || m_op == GeometricalOp::Not) {

      //  .. no results ..

    } else {

      std::vector<std::unordered_set<T2> > one_b;
      one_b.push_back (std::unordered_set<T2> ());

      shape_interactions<T, T> computed_b;
      child (1)->compute_local (cache, layout, interactions_for_child (interactions, 1, computed_b), one_b, max_vertex_count, area_ratio);

      copy_results (results, one_b);

    }

  } else {

    std::vector<std::unordered_set<T2> > one_b;
    one_b.push_back (std::unordered_set<T2> ());

    shape_interactions<T, T> computed_b;
    child (1)->compute_local (cache, layout, interactions_for_child (interactions, 1, computed_b), one_b, max_vertex_count, area_ratio);

    if (one_b.front ().empty ()) {

      if (m_op != GeometricalOp::And) {
        copy_results (results, one_a);
      }

    } else {

      run_bool (m_op, layout, one_a.front (), one_b.front (), results.front ());

    }

  }
}

template <class T, class TR>
void
CompoundRegionGeometricalBoolOperationNode::implement_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<T, T> &interactions, std::vector<std::unordered_set<TR> > &results, size_t max_vertex_count, double area_ratio) const
{
  ResultType res_a = child (0)->result_type ();
  ResultType res_b = child (1)->result_type ();

  if (res_a == Region && res_b == Region) {
    implement_bool<T, T, T, TR> (cache, layout, interactions, results, max_vertex_count, area_ratio);
  } else if (res_a == Region && res_b == Edges) {
    implement_bool<T, T, geo::Edge, TR> (cache, layout, interactions, results, max_vertex_count, area_ratio);
  } else if (res_a == Edges && res_b == Region) {
    implement_bool<T, geo::Edge, T, TR> (cache, layout, interactions, results, max_vertex_count, area_ratio);
  } else if (res_a == Edges && res_b == Edges) {
    implement_bool<T, geo::Edge, geo::Edge, TR> (cache, layout, interactions, results, max_vertex_count, area_ratio);
  }
}

void
CompoundRegionGeometricalBoolOperationNode::do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::Polygon, geo::Polygon> &interactions, std::vector<std::unordered_set<geo::Polygon> > &results, size_t max_vertex_count, double area_ratio) const
{
  implement_compute_local (cache, layout, interactions, results, max_vertex_count, area_ratio);
}

void
CompoundRegionGeometricalBoolOperationNode::do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::Polygon, geo::Polygon> &interactions, std::vector<std::unordered_set<geo::Edge> > &results, size_t max_vertex_count, double area_ratio) const
{
  implement_compute_local (cache, layout, interactions, results, max_vertex_count, area_ratio);
}

void
CompoundRegionGeometricalBoolOperationNode::do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &interactions, std::vector<std::unordered_set<geo::PolygonRef> > &results, size_t max_vertex_count, double area_ratio) const
{
  implement_compute_local (cache, layout, interactions, results, max_vertex_count, area_ratio);
}

void
CompoundRegionGeometricalBoolOperationNode::do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &interactions, std::vector<std::unordered_set<geo::Edge> > &results, size_t max_vertex_count, double area_ratio) const
{
  implement_compute_local (cache, layout, interactions, results, max_vertex_count, area_ratio);
}

// ---------------------------------------------------------------------------------------------

namespace
{
  template <class T>
  struct generic_result_adaptor
  {
  public:
    generic_result_adaptor<T> (std::vector<std::unordered_set<T> > *results)
      : mp_results (results)
    {
      m_intermediate.reserve (results->size ());
      for (size_t i = 0; i < results->size (); ++i) {
        m_shapes.push_back (geo::Shapes ());
        m_intermediate.push_back (&m_shapes.back ());
      }
    }

    static void insert (geo::Layout *, const geo::Shape &shape, std::unordered_set<geo::Edge> &result)
    {
      result.insert (shape.edge ());
    }

    static void insert (geo::Layout *, const geo::Shape &shape, std::unordered_set<geo::EdgePair> &result)
    {
      result.insert (shape.edge_pair ());
    }

    static void insert (geo::Layout *, const geo::Shape &shape, std::unordered_set<geo::Polygon> &result)
    {
      geo::Polygon p;
      shape.polygon (p);
      result.insert (p);
    }

    static void insert (geo::Layout *layout, const geo::Shape &shape, std::unordered_set<geo::PolygonRef> &result)
    {
      geo::Polygon p;
      shape.polygon (p);
      result.insert (geo::PolygonRef (p, layout->shape_repository ()));
    }

    const std::vector<geo::Shapes *> &results ()
    {
      return m_intermediate;
    }

    void finish (geo::Layout *layout)
    {
      for (size_t i = 0; i < m_intermediate.size (); ++i) {
        for (geo::Shapes::shape_iterator s = m_intermediate [i]->begin (geo::ShapeIterator::All); ! s.at_end (); ++s) {
          insert (layout, *s, (*mp_results)[i]);
        }
      }
    }

  private:
    std::vector<std::unordered_set<T> > *mp_results;
    std::vector<geo::Shapes *> m_intermediate;
    std::list<geo::Shapes> m_shapes;
  };

}

template <class TS, class TI, class TR>
template <class TTS, class TTI, class TTR>
void compound_region_generic_operation_node<TS, TI, TR>::implement_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<TTS, TTI> &interactions, std::vector<std::unordered_set<TTR> > &results, size_t max_vertex_count, double area_ratio) const
{
  generic_result_adaptor <TTR> adaptor (&results);

  if (! layout) {
    layout = const_cast<geo::Layout *> (&m_aux_layout);
  }

  shape_interactions<TS, TI> internal;

  const CompoundRegionOperationNode *self = child (0);
  std::vector<std::unordered_set<TS> > self_result;
  self_result.push_back (std::unordered_set<TS> ());

  shape_interactions<TTS, TTI> self_interactions_heap;
  const shape_interactions<TTS, TTI> &self_interactions = interactions_for_child (interactions, 0, self_interactions_heap);

  self->compute_local (cache, layout, self_interactions, self_result, max_vertex_count, area_ratio);

  geo::generic_shape_iterator <TS> is (self_result.front ().begin (), self_result.front ().end ());

  std::vector<geo::generic_shape_iterator<TI> > iiv;
  std::vector<std::unordered_set<TI> > intruder_results;
  intruder_results.reserve (children () - 1);  //  important, so that the memory layout will not change while we generate them

  for (unsigned int ci = 1; ci < children (); ++ci) {

    const CompoundRegionOperationNode *intruder = child (ci);
    std::vector<std::unordered_set<TI> > intruder_result;
    intruder_result.push_back (std::unordered_set<TI> ());

    shape_interactions<TTS, TTI> intruder_interactions_heap;
    const shape_interactions<TTS, TTI> &intruder_interactions = interactions_for_child (interactions, ci, intruder_interactions_heap);

    intruder->compute_local (cache, layout, intruder_interactions, intruder_result, max_vertex_count, area_ratio);

    intruder_results.push_back (std::unordered_set<TI> ());
    intruder_results.back ().swap (intruder_result.front ());

    iiv.push_back (geo::generic_shape_iterator<TI> (intruder_results.back ().begin (), intruder_results.back ().end ()));

  }

  geo::local_processor <TS, TI, TR> proc (layout);
  proc.run_flat (is, iiv, std::vector<bool> (), m_op, adaptor.results ());

  adaptor.finish (layout);
}

//  explicit instantiations
template void compound_region_generic_operation_node<geo::Polygon, geo::Polygon, geo::Polygon>::implement_compute_local (geo::CompoundRegionOperationCache *, geo::Layout *, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &, std::vector<std::unordered_set<geo::PolygonRef> > &, size_t, double) const;
template void compound_region_generic_operation_node<geo::Polygon, geo::Polygon, geo::Polygon>::implement_compute_local (geo::CompoundRegionOperationCache *, geo::Layout *, const shape_interactions<geo::Polygon, geo::Polygon> &, std::vector<std::unordered_set<geo::Polygon> > &, size_t, double) const;
template void compound_region_generic_operation_node<geo::Polygon, geo::Polygon, geo::Polygon>::implement_compute_local (geo::CompoundRegionOperationCache *, geo::Layout *, const shape_interactions<geo::Polygon, geo::Polygon> &, std::vector<std::unordered_set<geo::Edge> > &, size_t, double) const;
template void compound_region_generic_operation_node<geo::Polygon, geo::Polygon, geo::Polygon>::implement_compute_local (geo::CompoundRegionOperationCache *, geo::Layout *, const shape_interactions<geo::Polygon, geo::Polygon> &, std::vector<std::unordered_set<geo::EdgePair> > &, size_t, double) const;
template void compound_region_generic_operation_node<geo::Polygon, geo::Edge, geo::Polygon>::implement_compute_local (geo::CompoundRegionOperationCache *, geo::Layout *, const shape_interactions<geo::Polygon, geo::Polygon> &, std::vector<std::unordered_set<geo::Polygon> > &, size_t, double) const;
template void compound_region_generic_operation_node<geo::Polygon, geo::Edge, geo::Polygon>::implement_compute_local (geo::CompoundRegionOperationCache *, geo::Layout *, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &, std::vector<std::unordered_set<geo::PolygonRef> > &, size_t, double) const;
template void compound_region_generic_operation_node<geo::Polygon, geo::Edge, geo::Edge>::implement_compute_local (geo::CompoundRegionOperationCache *, geo::Layout *, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &, std::vector<std::unordered_set<geo::Edge> > &, size_t, double) const;
template void compound_region_generic_operation_node<geo::Polygon, geo::Edge, geo::Edge>::implement_compute_local (geo::CompoundRegionOperationCache *, geo::Layout *, const shape_interactions<geo::Polygon, geo::Polygon> &, std::vector<std::unordered_set<geo::Edge> > &, size_t, double) const;

// ---------------------------------------------------------------------------------------------

CompoundRegionLogicalCaseSelectOperationNode::CompoundRegionLogicalCaseSelectOperationNode (const std::vector<CompoundRegionOperationNode *> &inputs)
  : CompoundRegionMultiInputOperationNode (inputs), m_multi_layer (false)   //  TODO: multi-output mode not supported so far.
{
  //  .. nothing yet ..
}

std::string CompoundRegionLogicalCaseSelectOperationNode::generated_description () const
{
  //  TODO: could be nicer ...
  std::string r;
  r = "if-then";
  return r + CompoundRegionMultiInputOperationNode::generated_description ();
}

CompoundRegionLogicalCaseSelectOperationNode::ResultType
CompoundRegionLogicalCaseSelectOperationNode::result_type () const
{
  ResultType result = Region;
  for (size_t i = 1; i < children (); i += 2) {
    if (i == 1) {
      result = child ((unsigned int) i)->result_type ();
    } else {
      tl_assert (result == child ((unsigned int) i)->result_type ());
    }
  }
  return result;
}

template <class T, class TR>
void CompoundRegionLogicalCaseSelectOperationNode::implement_compute_local (geo::CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<T, T> &interactions, std::vector<std::unordered_set<TR> > &results, size_t max_vertex_count, double area_ratio) const
{
  bool ok = false;

  for (unsigned int ci = 0; ci < children (); ++ci) {

    shape_interactions<T, T> computed;
    const shape_interactions<T, T> &child_interactions = interactions_for_child<T, T> (interactions, ci, computed);

    const CompoundRegionOperationNode *node = child (ci);

    if (ci % 2 == 0) {

      if (ci + 1 < children ()) {

        ok = node->compute_local_bool<T> (cache, layout, child_interactions, max_vertex_count, area_ratio);
        continue;

      } else {
        //  executes the following statement as default branch
        ok = true;
      }

    }

    if (ok) {

      if (m_multi_layer && results.size () > size_t (ci / 2)) {

        std::vector<std::unordered_set<TR> > one;
        one.push_back (std::unordered_set<TR> ());
        node->compute_local (cache, layout, child_interactions, one, max_vertex_count, area_ratio);
        results[ci / 2].swap (one.front ());

      } else {

        node->compute_local (cache, layout, child_interactions, results, max_vertex_count, area_ratio);

      }

      break;

    }

  }

}

template void CompoundRegionLogicalCaseSelectOperationNode::implement_compute_local<geo::Polygon, geo::Polygon> (CompoundRegionOperationCache *, geo::Layout *, const shape_interactions<geo::Polygon, geo::Polygon> &, std::vector<std::unordered_set<geo::Polygon> > &, size_t, double) const;
template void CompoundRegionLogicalCaseSelectOperationNode::implement_compute_local<geo::PolygonRef, geo::PolygonRef> (CompoundRegionOperationCache *, geo::Layout *, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &, std::vector<std::unordered_set<geo::PolygonRef> > &, size_t, double) const;
template void CompoundRegionLogicalCaseSelectOperationNode::implement_compute_local<geo::Polygon, geo::Edge> (CompoundRegionOperationCache *, geo::Layout *, const shape_interactions<geo::Polygon, geo::Polygon> &, std::vector<std::unordered_set<geo::Edge> > &, size_t, double) const;
template void CompoundRegionLogicalCaseSelectOperationNode::implement_compute_local<geo::PolygonRef, geo::Edge> (CompoundRegionOperationCache *, geo::Layout *, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &, std::vector<std::unordered_set<geo::Edge> > &, size_t, double) const;
template void CompoundRegionLogicalCaseSelectOperationNode::implement_compute_local<geo::Polygon, geo::EdgePair> (CompoundRegionOperationCache *, geo::Layout *, const shape_interactions<geo::Polygon, geo::Polygon> &, std::vector<std::unordered_set<geo::EdgePair> > &, size_t, double) const;
template void CompoundRegionLogicalCaseSelectOperationNode::implement_compute_local<geo::PolygonRef, geo::EdgePair> (CompoundRegionOperationCache *, geo::Layout *, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &, std::vector<std::unordered_set<geo::EdgePair> > &, size_t, double) const;

// ---------------------------------------------------------------------------------------------

CompoundRegionInteractOperationNode::CompoundRegionInteractOperationNode (CompoundRegionOperationNode *a, CompoundRegionOperationNode *b, int mode, bool touching, bool inverse, size_t min_count, size_t max_count)
  : compound_region_generic_operation_node<geo::Polygon, geo::Polygon, geo::Polygon> (&m_op, a, b), m_op (mode, touching, inverse ? Negative : Positive, min_count, max_count, b->is_merged ())
{
  //  .. nothing yet ..
}

CompoundRegionInteractOperationNode::CompoundRegionInteractOperationNode (geo::Region *a, geo::Region *b, int mode, bool touching, bool inverse, size_t min_count, size_t max_count)
  : compound_region_generic_operation_node<geo::Polygon, geo::Polygon, geo::Polygon> (&m_op, a, b), m_op (mode, touching, inverse ? Negative : Positive, min_count, max_count, b->is_merged ())
{
  //  .. nothing yet ..
}

std::string
CompoundRegionInteractOperationNode::generated_description () const
{
  return std::string ("interact") + compound_region_generic_operation_node<geo::Polygon, geo::Polygon, geo::Polygon>::generated_description ();
}

// ---------------------------------------------------------------------------------------------

CompoundRegionInteractWithEdgeOperationNode::CompoundRegionInteractWithEdgeOperationNode (CompoundRegionOperationNode *a, CompoundRegionOperationNode *b, bool inverse, size_t min_count, size_t max_count)
  : compound_region_generic_operation_node<geo::Polygon, geo::Edge, geo::Polygon> (&m_op, a, b), m_op (inverse ? Negative : Positive, min_count, max_count, b->is_merged ())
{
  //  .. nothing yet ..
}

std::string
CompoundRegionInteractWithEdgeOperationNode::generated_description () const
{
  return std::string ("interact") + compound_region_generic_operation_node<geo::Polygon, geo::Edge, geo::Polygon>::generated_description ();
}

// ---------------------------------------------------------------------------------------------

CompoundRegionPullOperationNode::CompoundRegionPullOperationNode (CompoundRegionOperationNode *a, CompoundRegionOperationNode *b, int mode, bool touching)
  : compound_region_generic_operation_node<geo::Polygon, geo::Polygon, geo::Polygon> (&m_op, a, b), m_op (mode, touching)
{
  //  .. nothing yet ..
}

CompoundRegionPullOperationNode::CompoundRegionPullOperationNode (geo::Region *a, geo::Region *b, int mode, bool touching)
  : compound_region_generic_operation_node<geo::Polygon, geo::Polygon, geo::Polygon> (&m_op, a, b), m_op (mode, touching)
{
  //  .. nothing yet ..
}

std::string
CompoundRegionPullOperationNode::generated_description () const
{
  return std::string ("pull") + compound_region_generic_operation_node<geo::Polygon, geo::Polygon, geo::Polygon>::generated_description ();
}

// ---------------------------------------------------------------------------------------------

CompoundRegionPullWithEdgeOperationNode::CompoundRegionPullWithEdgeOperationNode (CompoundRegionOperationNode *a, CompoundRegionOperationNode *b)
  : compound_region_generic_operation_node<geo::Polygon, geo::Edge, geo::Edge> (&m_op, a, b), m_op ()
{
  //  .. nothing yet ..
}

std::string
CompoundRegionPullWithEdgeOperationNode::generated_description () const
{
  return std::string ("pull") + compound_region_generic_operation_node<geo::Polygon, geo::Edge, geo::Edge>::generated_description ();
}


// ---------------------------------------------------------------------------------------------

CompoundRegionJoinOperationNode::CompoundRegionJoinOperationNode (const std::vector<CompoundRegionOperationNode *> &inputs)
  : CompoundRegionMultiInputOperationNode (inputs)
{
  //  .. nothing yet ..
}

std::string CompoundRegionJoinOperationNode::generated_description () const
{
  //  TODO: could be nicer ...
  std::string r;
  r = "if-then";
  return r + CompoundRegionMultiInputOperationNode::generated_description ();
}

CompoundRegionJoinOperationNode::ResultType
CompoundRegionJoinOperationNode::result_type () const
{
  ResultType result = Region;
  for (size_t i = 0; i < children (); ++i) {
    if (i == 0) {
      result = child ((unsigned int) i)->result_type ();
    } else {
      tl_assert (result == child ((unsigned int) i)->result_type ());
    }
  }
  return result;
}

template <class T, class TR>
void CompoundRegionJoinOperationNode::implement_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<T, T> &interactions, std::vector<std::unordered_set<TR> > &results, size_t max_vertex_count, double area_ratio) const
{
  for (unsigned int ci = 0; ci < children (); ++ci) {

    shape_interactions<T, T> computed;
    const shape_interactions<T, T> &child_interactions = interactions_for_child<T, T> (interactions, ci, computed);

    child (ci)->compute_local (cache, layout, child_interactions, results, max_vertex_count, area_ratio);

  }
}

template void CompoundRegionJoinOperationNode::implement_compute_local<geo::Polygon, geo::Polygon> (CompoundRegionOperationCache *, geo::Layout *, const shape_interactions<geo::Polygon, geo::Polygon> &, std::vector<std::unordered_set<geo::Polygon> > &, size_t, double) const;
template void CompoundRegionJoinOperationNode::implement_compute_local<geo::PolygonRef, geo::PolygonRef> (CompoundRegionOperationCache *, geo::Layout *, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &, std::vector<std::unordered_set<geo::PolygonRef> > &, size_t, double) const;
template void CompoundRegionJoinOperationNode::implement_compute_local<geo::Polygon, geo::Edge> (CompoundRegionOperationCache *, geo::Layout *, const shape_interactions<geo::Polygon, geo::Polygon> &, std::vector<std::unordered_set<geo::Edge> > &, size_t, double) const;
template void CompoundRegionJoinOperationNode::implement_compute_local<geo::PolygonRef, geo::Edge> (CompoundRegionOperationCache *, geo::Layout *, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &, std::vector<std::unordered_set<geo::Edge> > &, size_t, double) const;
template void CompoundRegionJoinOperationNode::implement_compute_local<geo::Polygon, geo::EdgePair> (CompoundRegionOperationCache *, geo::Layout *, const shape_interactions<geo::Polygon, geo::Polygon> &, std::vector<std::unordered_set<geo::EdgePair> > &, size_t, double) const;
template void CompoundRegionJoinOperationNode::implement_compute_local<geo::PolygonRef, geo::EdgePair> (CompoundRegionOperationCache *, geo::Layout *, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &, std::vector<std::unordered_set<geo::EdgePair> > &, size_t, double) const;

// ---------------------------------------------------------------------------------------------

CompoundRegionFilterOperationNode::CompoundRegionFilterOperationNode (PolygonFilterBase *filter, CompoundRegionOperationNode *input, bool owns_filter, bool sum_of_set)
  : CompoundRegionMultiInputOperationNode (input), mp_filter (filter), m_owns_filter (owns_filter), m_sum_of_set (sum_of_set)
{
  set_description ("filter");
}

CompoundRegionFilterOperationNode::~CompoundRegionFilterOperationNode ()
{
  if (m_owns_filter) {
    delete mp_filter;
  }
  mp_filter = 0;
}

void
CompoundRegionFilterOperationNode::do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::Polygon, geo::Polygon> &interactions, std::vector<std::unordered_set<geo::Polygon> > &results, size_t max_vertex_count, double area_ratio) const
{
  implement_compute_local (cache, layout, interactions, results, max_vertex_count, area_ratio);
}

void
CompoundRegionFilterOperationNode::do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &interactions, std::vector<std::unordered_set<geo::PolygonRef> > &results, size_t max_vertex_count, double area_ratio) const
{
  implement_compute_local (cache, layout, interactions, results, max_vertex_count, area_ratio);
}

// ---------------------------------------------------------------------------------------------

CompoundRegionEdgeFilterOperationNode::CompoundRegionEdgeFilterOperationNode (EdgeFilterBase *filter, CompoundRegionOperationNode *input, bool owns_filter, bool sum_of)
  : CompoundRegionMultiInputOperationNode (input), mp_filter (filter), m_owns_filter (owns_filter), m_sum_of (sum_of)
{
  set_description ("filter");
}

CompoundRegionEdgeFilterOperationNode::~CompoundRegionEdgeFilterOperationNode ()
{
  if (m_owns_filter) {
    delete mp_filter;
  }
  mp_filter = 0;
}

void
CompoundRegionEdgeFilterOperationNode::do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::Polygon, geo::Polygon> &interactions, std::vector<std::unordered_set<geo::Edge> > &results, size_t max_vertex_count, double area_ratio) const
{
  implement_compute_local (cache, layout, interactions, results, max_vertex_count, area_ratio);
}

void
CompoundRegionEdgeFilterOperationNode::do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &interactions, std::vector<std::unordered_set<geo::Edge> > &results, size_t max_vertex_count, double area_ratio) const
{
  implement_compute_local (cache, layout, interactions, results, max_vertex_count, area_ratio);
}

// ---------------------------------------------------------------------------------------------

CompoundRegionEdgePairFilterOperationNode::CompoundRegionEdgePairFilterOperationNode (EdgePairFilterBase *filter, CompoundRegionOperationNode *input, bool owns_filter)
  : CompoundRegionMultiInputOperationNode (input), mp_filter (filter), m_owns_filter (owns_filter)
{
  set_description ("filter");
}

CompoundRegionEdgePairFilterOperationNode::~CompoundRegionEdgePairFilterOperationNode ()
{
  if (m_owns_filter) {
    delete mp_filter;
  }
  mp_filter = 0;
}

void
CompoundRegionEdgePairFilterOperationNode::do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::Polygon, geo::Polygon> &interactions, std::vector<std::unordered_set<geo::EdgePair> > &results, size_t max_vertex_count, double area_ratio) const
{
  implement_compute_local (cache, layout, interactions, results, max_vertex_count, area_ratio);
}

void
CompoundRegionEdgePairFilterOperationNode::do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &interactions, std::vector<std::unordered_set<geo::EdgePair> > &results, size_t max_vertex_count, double area_ratio) const
{
  implement_compute_local (cache, layout, interactions, results, max_vertex_count, area_ratio);
}

bool
CompoundRegionEdgePairFilterOperationNode::is_selected (const geo::EdgePair &p) const
{
  return mp_filter->selected (p);
}

// ---------------------------------------------------------------------------------------------

CompoundRegionProcessingOperationNode::CompoundRegionProcessingOperationNode (PolygonProcessorBase *proc, CompoundRegionOperationNode *input, bool owns_proc, geo::Coord dist_adder)
  : CompoundRegionMultiInputOperationNode (input), mp_proc (proc), m_owns_proc (owns_proc), m_dist_adder (dist_adder)
{
  set_description ("processor");
}

CompoundRegionProcessingOperationNode::~CompoundRegionProcessingOperationNode ()
{
  if (m_owns_proc) {
    delete mp_proc;
    mp_proc = 0;
  }
}

void
CompoundRegionProcessingOperationNode::do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::Polygon, geo::Polygon> &interactions, std::vector<std::unordered_set<geo::Polygon> > &results, size_t max_vertex_count, double area_ratio) const
{
  implement_compute_local (cache, layout, interactions, results, max_vertex_count, area_ratio);
}

void
CompoundRegionProcessingOperationNode::do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &interactions, std::vector<std::unordered_set<geo::PolygonRef> > &results, size_t max_vertex_count, double area_ratio) const
{
  implement_compute_local (cache, layout, interactions, results, max_vertex_count, area_ratio);
}

void
CompoundRegionProcessingOperationNode::processed (geo::Layout *, const geo::Polygon &p, std::vector<geo::Polygon> &res) const
{
  mp_proc->process (p, res);
}

void
CompoundRegionProcessingOperationNode::processed (geo::Layout *layout, const geo::PolygonRef &p, std::vector<geo::PolygonRef> &res) const
{
  std::vector<geo::Polygon> poly;
  mp_proc->process (p.obj ().transformed (p.trans ()), poly);
  for (std::vector<geo::Polygon>::const_iterator p = poly.begin (); p != poly.end (); ++p) {
    res.push_back (geo::PolygonRef (*p, layout->shape_repository ()));
  }
}

// ---------------------------------------------------------------------------------------------

CompoundRegionToEdgeProcessingOperationNode::CompoundRegionToEdgeProcessingOperationNode (PolygonToEdgeProcessorBase *proc, CompoundRegionOperationNode *input, bool owns_proc)
  : CompoundRegionMultiInputOperationNode (input), mp_proc (proc), m_owns_proc (owns_proc)
{
  set_description ("processor");
}

CompoundRegionToEdgeProcessingOperationNode::~CompoundRegionToEdgeProcessingOperationNode ()
{
  if (m_owns_proc) {
    delete mp_proc;
    mp_proc = 0;
  }
}

void
CompoundRegionToEdgeProcessingOperationNode::do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::Polygon, geo::Polygon> &interactions, std::vector<std::unordered_set<geo::Edge> > &results, size_t max_vertex_count, double area_ratio) const
{
  implement_compute_local (cache, layout, interactions, results, max_vertex_count, area_ratio);
}

void
CompoundRegionToEdgeProcessingOperationNode::do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &interactions, std::vector<std::unordered_set<geo::Edge> > &results, size_t max_vertex_count, double area_ratio) const
{
  implement_compute_local (cache, layout, interactions, results, max_vertex_count, area_ratio);
}

void
CompoundRegionToEdgeProcessingOperationNode::processed (geo::Layout *, const geo::Polygon &p, std::vector<geo::Edge> &res) const
{
  mp_proc->process (p, res);
}

void
CompoundRegionToEdgeProcessingOperationNode::processed (geo::Layout *, const geo::PolygonRef &p, std::vector<geo::Edge> &res) const
{
  mp_proc->process (p.obj ().transformed (p.trans ()), res);
}

// ---------------------------------------------------------------------------------------------

CompoundRegionEdgeProcessingOperationNode::CompoundRegionEdgeProcessingOperationNode (EdgeProcessorBase *proc, CompoundRegionOperationNode *input, bool owns_proc)
  : CompoundRegionMultiInputOperationNode (input), mp_proc (proc), m_owns_proc (owns_proc)
{
  set_description ("processor");
}

CompoundRegionEdgeProcessingOperationNode::~CompoundRegionEdgeProcessingOperationNode ()
{
  if (m_owns_proc) {
    delete mp_proc;
    mp_proc = 0;
  }
}

void
CompoundRegionEdgeProcessingOperationNode::do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::Polygon, geo::Polygon> &interactions, std::vector<std::unordered_set<geo::Edge> > &results, size_t max_vertex_count, double area_ratio) const
{
  implement_compute_local (cache, layout, interactions, results, max_vertex_count, area_ratio);
}

void
CompoundRegionEdgeProcessingOperationNode::do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &interactions, std::vector<std::unordered_set<geo::Edge> > &results, size_t max_vertex_count, double area_ratio) const
{
  implement_compute_local (cache, layout, interactions, results, max_vertex_count, area_ratio);
}

void CompoundRegionEdgeProcessingOperationNode::processed (geo::Layout *, const geo::Edge &p, std::vector<geo::Edge> &res) const
{
  mp_proc->process (p, res);
}

// ---------------------------------------------------------------------------------------------

CompoundRegionEdgeToPolygonProcessingOperationNode::CompoundRegionEdgeToPolygonProcessingOperationNode (EdgeToPolygonProcessorBase *proc, CompoundRegionOperationNode *input, bool owns_proc)
  : CompoundRegionMultiInputOperationNode (input), mp_proc (proc), m_owns_proc (owns_proc)
{
  set_description ("processor");
}

CompoundRegionEdgeToPolygonProcessingOperationNode::~CompoundRegionEdgeToPolygonProcessingOperationNode ()
{
  if (m_owns_proc) {
    delete mp_proc;
    mp_proc = 0;
  }
}

void
CompoundRegionEdgeToPolygonProcessingOperationNode::do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::Polygon, geo::Polygon> &interactions, std::vector<std::unordered_set<geo::Polygon> > &results, size_t max_vertex_count, double area_ratio) const
{
  implement_compute_local (cache, layout, interactions, results, max_vertex_count, area_ratio);
}

void
CompoundRegionEdgeToPolygonProcessingOperationNode::do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &interactions, std::vector<std::unordered_set<geo::PolygonRef> > &results, size_t max_vertex_count, double area_ratio) const
{
  implement_compute_local (cache, layout, interactions, results, max_vertex_count, area_ratio);
}

void
CompoundRegionEdgeToPolygonProcessingOperationNode::processed (geo::Layout *, const geo::Edge &e, std::vector<geo::Polygon> &res) const
{
  mp_proc->process (e, res);
}

void
CompoundRegionEdgeToPolygonProcessingOperationNode::processed (geo::Layout *layout, const geo::Edge &e, std::vector<geo::PolygonRef> &res) const
{
  std::vector<geo::Polygon> polygons;
  mp_proc->process (e, polygons);

  for (std::vector<geo::Polygon>::const_iterator p = polygons.begin (); p != polygons.end (); ++p) {
    res.push_back (geo::PolygonRef (*p, layout->shape_repository ()));
  }
}

// ---------------------------------------------------------------------------------------------

CompoundRegionToEdgePairProcessingOperationNode::CompoundRegionToEdgePairProcessingOperationNode (PolygonToEdgePairProcessorBase *proc, CompoundRegionOperationNode *input, bool owns_proc)
  : CompoundRegionMultiInputOperationNode (input), mp_proc (proc), m_owns_proc (owns_proc)
{
  set_description ("processor");
}

CompoundRegionToEdgePairProcessingOperationNode::~CompoundRegionToEdgePairProcessingOperationNode ()
{
  if (m_owns_proc) {
    delete mp_proc;
    mp_proc = 0;
  }
}

void
CompoundRegionToEdgePairProcessingOperationNode::do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::Polygon, geo::Polygon> &interactions, std::vector<std::unordered_set<geo::EdgePair> > &results, size_t max_vertex_count, double area_ratio) const
{
  implement_compute_local (cache, layout, interactions, results, max_vertex_count, area_ratio);
}

void
CompoundRegionToEdgePairProcessingOperationNode::do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &interactions, std::vector<std::unordered_set<geo::EdgePair> > &results, size_t max_vertex_count, double area_ratio) const
{
  implement_compute_local (cache, layout, interactions, results, max_vertex_count, area_ratio);
}

void
CompoundRegionToEdgePairProcessingOperationNode::processed (geo::Layout *, const geo::Polygon &p, std::vector<geo::EdgePair> &res) const
{
  mp_proc->process (p, res);
}

void
CompoundRegionToEdgePairProcessingOperationNode::processed (geo::Layout *, const geo::PolygonRef &p, std::vector<geo::EdgePair> &res) const
{
  mp_proc->process (p.obj ().transformed (p.trans ()), res);
}

// ---------------------------------------------------------------------------------------------

CompoundRegionEdgePairToPolygonProcessingOperationNode::CompoundRegionEdgePairToPolygonProcessingOperationNode (EdgePairToPolygonProcessorBase *proc, CompoundRegionOperationNode *input, bool owns_proc)
  : CompoundRegionMultiInputOperationNode (input), mp_proc (proc), m_owns_proc (owns_proc)
{
  set_description ("processor");
}

CompoundRegionEdgePairToPolygonProcessingOperationNode::~CompoundRegionEdgePairToPolygonProcessingOperationNode ()
{
  if (m_owns_proc) {
    delete mp_proc;
    mp_proc = 0;
  }
}

void
CompoundRegionEdgePairToPolygonProcessingOperationNode::do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::Polygon, geo::Polygon> &interactions, std::vector<std::unordered_set<geo::Polygon> > &results, size_t max_vertex_count, double area_ratio) const
{
  implement_compute_local (cache, layout, interactions, results, max_vertex_count, area_ratio);
}

void
CompoundRegionEdgePairToPolygonProcessingOperationNode::do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &interactions, std::vector<std::unordered_set<geo::PolygonRef> > &results, size_t max_vertex_count, double area_ratio) const
{
  implement_compute_local (cache, layout, interactions, results, max_vertex_count, area_ratio);
}

void
CompoundRegionEdgePairToPolygonProcessingOperationNode::processed (geo::Layout *, const geo::EdgePair &e, std::vector<geo::Polygon> &res) const
{
  mp_proc->process (e, res);
}

void
CompoundRegionEdgePairToPolygonProcessingOperationNode::processed (geo::Layout *layout, const geo::EdgePair &e, std::vector<geo::PolygonRef> &res) const
{
  std::vector<geo::Polygon> polygons;
  mp_proc->process (e, polygons);

  for (std::vector<geo::Polygon>::const_iterator p = polygons.begin (); p != polygons.end (); ++p) {
    res.push_back (geo::PolygonRef (*p, layout->shape_repository ()));
  }
}

// ---------------------------------------------------------------------------------------------

CompoundRegionEdgePairToEdgeProcessingOperationNode::CompoundRegionEdgePairToEdgeProcessingOperationNode (EdgePairToEdgeProcessorBase *proc, CompoundRegionOperationNode *input, bool owns_proc)
  : CompoundRegionMultiInputOperationNode (input), mp_proc (proc), m_owns_proc (owns_proc)
{
  set_description ("processor");
}

CompoundRegionEdgePairToEdgeProcessingOperationNode::~CompoundRegionEdgePairToEdgeProcessingOperationNode ()
{
  if (m_owns_proc) {
    delete mp_proc;
    mp_proc = 0;
  }
}

void
CompoundRegionEdgePairToEdgeProcessingOperationNode::do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::Polygon, geo::Polygon> &interactions, std::vector<std::unordered_set<geo::Edge> > &results, size_t max_vertex_count, double area_ratio) const
{
  implement_compute_local (cache, layout, interactions, results, max_vertex_count, area_ratio);
}

void
CompoundRegionEdgePairToEdgeProcessingOperationNode::do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &interactions, std::vector<std::unordered_set<geo::Edge> > &results, size_t max_vertex_count, double area_ratio) const
{
  implement_compute_local (cache, layout, interactions, results, max_vertex_count, area_ratio);
}

// ---------------------------------------------------------------------------------------------

CompoundRegionCheckOperationNode::CompoundRegionCheckOperationNode (geo::edge_relation_type rel, bool different_polygons, geo::Coord d, const geo::RegionCheckOptions &options)
  : CompoundRegionMultiInputOperationNode (), m_check (rel, d, options.metrics), m_different_polygons (different_polygons), m_options (options), m_has_other (false), m_is_other_merged (false)
{
  set_description ("check");

  m_check.set_include_zero (false);
  m_check.set_whole_edges (options.whole_edges);
  m_check.set_ignore_angle (options.ignore_angle);
  m_check.set_min_projection (options.min_projection);
  m_check.set_max_projection (options.max_projection);
}

CompoundRegionCheckOperationNode::CompoundRegionCheckOperationNode (CompoundRegionOperationNode *input, geo::edge_relation_type rel, bool different_polygons, geo::Coord d, const geo::RegionCheckOptions &options)
  : CompoundRegionMultiInputOperationNode (input), m_check (rel, d, options.metrics), m_different_polygons (different_polygons), m_options (options), m_has_other (false), m_is_other_merged (false)
{
  set_description ("check");

  m_check.set_include_zero (false);
  m_check.set_whole_edges (options.whole_edges);
  m_check.set_ignore_angle (options.ignore_angle);
  m_check.set_min_projection (options.min_projection);
  m_check.set_max_projection (options.max_projection);
}

CompoundRegionCheckOperationNode::CompoundRegionCheckOperationNode (CompoundRegionOperationNode * /*input*/, CompoundRegionOperationNode *other, geo::edge_relation_type rel, bool different_polygons, geo::Coord d, const geo::RegionCheckOptions &options)
  : CompoundRegionMultiInputOperationNode (other), m_check (rel, d, options.metrics), m_different_polygons (different_polygons), m_options (options)
{
  m_has_other = other->has_external_inputs ();
  m_is_other_merged = other->is_merged ();

  set_description ("check");

  m_check.set_include_zero (false);
  m_check.set_whole_edges (options.whole_edges);
  m_check.set_ignore_angle (options.ignore_angle);
  m_check.set_min_projection (options.min_projection);
  m_check.set_max_projection (options.max_projection);
}

geo::OnEmptyIntruderHint
CompoundRegionCheckOperationNode::on_empty_intruder_hint () const
{
  return (m_different_polygons || m_has_other) ? OnEmptyIntruderHint::Drop : OnEmptyIntruderHint::Ignore;
}

geo::Coord
CompoundRegionCheckOperationNode::computed_dist () const
{
  return m_check.distance ();
}

void
CompoundRegionCheckOperationNode::do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout *layout, const shape_interactions<geo::Polygon, geo::Polygon> &interactions, std::vector<std::unordered_set<geo::EdgePair> > &results, size_t max_vertex_count, double area_ratio) const
{
  geo::check_local_operation<geo::Polygon, geo::Polygon> op (m_check, m_different_polygons, m_has_other, m_is_other_merged, m_options);

  tl_assert (results.size () == 1);
  if (results.front ().empty ()) {
    op.do_compute_local (layout, interactions, results, max_vertex_count, area_ratio);
  } else {
    std::vector<std::unordered_set<geo::EdgePair> > r;
    r.resize (1);
    op.do_compute_local (layout, interactions, r, max_vertex_count, area_ratio);
    results.front ().insert (r.front ().begin (), r.front ().end ());
  }
}

void
CompoundRegionCheckOperationNode::do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout *layout, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &interactions, std::vector<std::unordered_set<geo::EdgePair> > &results, size_t max_vertex_count, double area_ratio) const
{
  geo::check_local_operation<geo::PolygonRef, geo::PolygonRef> op (m_check, m_different_polygons, m_has_other, m_is_other_merged, m_options);

  tl_assert (results.size () == 1);
  if (results.front ().empty ()) {
    op.do_compute_local (layout, interactions, results, max_vertex_count, area_ratio);
  } else {
    std::vector<std::unordered_set<geo::EdgePair> > r;
    r.resize (1);
    op.do_compute_local (layout, interactions, r, max_vertex_count, area_ratio);
    results.front ().insert (r.front ().begin (), r.front ().end ());
  }
}

}

