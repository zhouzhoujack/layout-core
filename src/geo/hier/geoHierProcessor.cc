#include "geoHierProcessor.h"
#include "geoBoxScanner.h"
#include "geoRecursiveShapeIterator.h"
#include "geoBoxConvert.h"
#include "geoEdgeProcessor.h"
#include "geoPolygonGenerators.h"
#include "geoLocalOperationUtils.h"
#include "geoShapeFlags.h"
#include "utilsLog.h"
#include "utilsTimer.h"
#include "utilsInternational.h"

// ---------------------------------------------------------------------------------------------
//  Cronology debugging support (TODO: experimental)

#if defined(HAVE_CRONOLOGY)

#include <cronology/events.hpp>

CRONOLOGY_MAKE_EVENT(event_compute_contexts, "Compute contexts")
CRONOLOGY_MAKE_EVENT(event_compute_contexts_unlocked, "Compute contexts (unlocked)")

cronology::events::event_collection<event_compute_contexts, event_compute_contexts_unlocked> collect_events;

#define CRONOLOGY_COLLECTION_BRACKET(event_name) cronology::EventBracket __bracket##event_name (collect_events.threadwise ()->event<event_name> ().event ());

CRONOLOGY_MAKE_EVENT(event_compute_results, "Compute results")
CRONOLOGY_MAKE_EVENT(event_compute_local_cell, "Compute local cell results")
CRONOLOGY_MAKE_EVENT(event_propagate, "Propagate local cell results")

cronology::events::event_collection<event_compute_results, event_compute_local_cell, event_propagate> compute_events;

#define CRONOLOGY_COMPUTE_BRACKET(event_name) cronology::EventBracket __bracket##event_name (compute_events.threadwise ()->event<event_name> ().event ());

#else
#define CRONOLOGY_COLLECTION_BRACKET(event_name)
#define CRONOLOGY_COMPUTE_BRACKET(event_name)
#endif

namespace geo
{
// -----------------------------------------------------------------------------
//  HierProcessor implementation


// ---------------------------------------------------------------------------------------------
//  Shape reference translator

template <class Ref> class shape_reference_translator;
template <class Ref, class Trans> class shape_reference_translator_with_trans;

template <class Ref>
class shape_reference_translator
{
public:
  typedef typename Ref::shape_type shape_type;
  typedef typename Ref::trans_type ref_trans_type;

  shape_reference_translator (geo::Layout *target_layout)
    : mp_layout (target_layout)
  {
    //  .. nothing yet ..
  }

  Ref operator() (const Ref &ref) const
  {
    typename std::unordered_map<const shape_type *, const shape_type *>::const_iterator m = m_cache.find (ref.ptr ());
    if (m != m_cache.end ()) {

      return Ref (m->second, ref.trans ());

    } else {

      const shape_type *ptr;
      {
        geo::MutexLocker locker (&mp_layout->lock ());
        ptr = mp_layout->shape_repository ().repository (typename shape_type::tag ()).insert (ref.obj ());
      }

      m_cache[ref.ptr ()] = ptr;
      return Ref (ptr, ref.trans ());

    }
  }

  template <class Trans>
  Ref operator() (const Ref &ref, const Trans &tr) const
  {
    shape_type sh = ref.obj ().transformed (tr * Trans (ref.trans ()));
    ref_trans_type red_trans;
    sh.reduce (red_trans);

    typename std::unordered_map<shape_type, const shape_type *>::const_iterator m = m_cache_by_shape.find (sh);
    if (m != m_cache_by_shape.end ()) {

      return Ref (m->second, red_trans);

    } else {

      const shape_type *ptr;
      {
        geo::MutexLocker locker (&mp_layout->lock ());
        ptr = mp_layout->shape_repository ().repository (typename shape_type::tag ()).insert (sh);
      }

      m_cache_by_shape[sh] = ptr;
      return Ref (ptr, red_trans);

    }
  }

private:
  geo::Layout *mp_layout;
  mutable std::unordered_map<const shape_type *, const shape_type *> m_cache;
  mutable std::unordered_map<shape_type, const shape_type *> m_cache_by_shape;
};

template <class Shape>
class simple_shape_reference_translator
{
public:
  typedef Shape shape_type;

  simple_shape_reference_translator ()
  {
    //  .. nothing yet ..
  }

  const shape_type &operator() (const shape_type &s) const
  {
    return s;
  }

  template <class Trans>
  shape_type operator() (const shape_type &s, const Trans &tr) const
  {
    return s.transformed (tr);
  }
};

template <>
class shape_reference_translator<geo::Edge>
  : public simple_shape_reference_translator<geo::Edge>
{
public:
  shape_reference_translator (geo::Layout * /*target_layout*/) { }
};

template <>
class shape_reference_translator<geo::Polygon>
  : public simple_shape_reference_translator<geo::Polygon>
{
public:
  shape_reference_translator (geo::Layout * /*target_layout*/) { }
};

template <>
class shape_reference_translator<geo::Text>
  : public simple_shape_reference_translator<geo::Text>
{
public:
  shape_reference_translator (geo::Layout * /*target_layout*/) { }
};

template <class Ref, class Trans>
class shape_reference_translator_with_trans_from_shape_ref
{
public:
  typedef typename Ref::shape_type shape_type;
  typedef typename Ref::trans_type ref_trans_type;

  shape_reference_translator_with_trans_from_shape_ref (geo::Layout *target_layout, const Trans &trans)
    : mp_layout (target_layout), m_trans (trans), m_ref_trans (trans), m_bare_trans (Trans (m_ref_trans.inverted ()) * trans)
  {
    //  .. nothing yet ..
  }

  Ref operator() (const Ref &ref) const
  {
    typename std::unordered_map<const shape_type *, std::pair<const shape_type *, ref_trans_type> >::const_iterator m = m_cache.find (ref.ptr ());
    if (m != m_cache.end ()) {

      return Ref (m->second.first, ref_trans_type (m_trans * Trans (ref.trans ())) * m->second.second);

    } else {

      shape_type sh = ref.obj ().transformed (m_bare_trans);
      ref_trans_type red_trans;
      sh.reduce (red_trans);

      const shape_type *ptr;
      {
        geo::MutexLocker locker (&mp_layout->lock ());
        ptr = mp_layout->shape_repository ().repository (typename shape_type::tag ()).insert (sh);
      }

      m_cache[ref.ptr ()] = std::make_pair (ptr, red_trans);

      return Ref (ptr, ref_trans_type (m_trans * Trans (ref.trans ())) * red_trans);

    }
  }

private:
  geo::Layout *mp_layout;
  Trans m_trans;
  ref_trans_type m_ref_trans;
  Trans m_bare_trans;
  mutable std::unordered_map<const shape_type *, std::pair<const shape_type *, ref_trans_type> > m_cache;
};

template <class Trans>
class shape_reference_translator_with_trans<geo::PolygonRef, Trans>
  : public shape_reference_translator_with_trans_from_shape_ref<geo::PolygonRef, Trans>
{
public:
  shape_reference_translator_with_trans (geo::Layout *target_layout, const Trans &trans)
    : shape_reference_translator_with_trans_from_shape_ref<geo::PolygonRef, Trans> (target_layout, trans)
  {
    //  .. nothing yet ..
  }
};

template <class Sh, class Trans>
class shape_reference_translator_with_trans
{
public:
  typedef Sh shape_type;

  shape_reference_translator_with_trans (geo::Layout * /*target_layout*/, const Trans &trans)
    : m_trans (trans)
  {
    //  .. nothing yet ..
  }

  shape_type operator() (const shape_type &s) const
  {
    return s.transformed (m_trans);
  }

private:
  Trans m_trans;
};

// ---------------------------------------------------------------------------------------------

/**
 *  @brief a utility to capture insert attempts of the wrong type into a box scanner
 *  These attempts can happen because the generic nature of the interaction detector code
 */

template <class T, class P>
void safe_insert2_into_box_scanner (geo::box_scanner2 <T, P, T, P> &scanner, const T *t, const P &p)
{
  scanner.insert2 (t, p);
}

template <class T1, class P1, class T2, class P2>
void safe_insert2_into_box_scanner (geo::box_scanner2 <T1, P1, T2, P2> &scanner, const T2 *t, const P2 &p)
{
  scanner.insert2 (t, p);
}

template <class T1, class P1, class T2, class P2>
void safe_insert2_into_box_scanner (geo::box_scanner2 <T1, P1, T2, P2> &, const T1 *, const P1 &)
{
  tl_assert (false);
}

// ---------------------------------------------------------------------------------------------

/**
 *  @brief Safe enlargement of a box
 *  Boxes must not vanish when augmented for overlapping queries. Hence we must not make
 *  the boxes shrinked too much on enlarge.
 */
geo::Box safe_box_enlarged (const geo::Box &box, geo::Coord dx, geo::Coord dy)
{
  if (box.empty ()) {
    return box;
  } else {
    geo::Coord w2 = geo::Coord (box.width () / 2);
    geo::Coord h2 = geo::Coord (box.height () / 2);
    if (dx + w2 < 0) {
      dx = -w2;
    }
    if (dy + h2 < 0) {
      dy = -h2;
    }
    return box.enlarged (geo::Vector (dx, dy));
  }
}

// ---------------------------------------------------------------------------------------------
//  LocalProcessorCellContext implementation

template <class TS, class TI, class TR>
local_processor_cell_context<TS, TI, TR>::local_processor_cell_context ()
{
  //  .. nothing yet ..
}

template <class TS, class TI, class TR>
local_processor_cell_context<TS, TI, TR>::local_processor_cell_context (const local_processor_cell_context &other)
  : m_propagated (other.m_propagated), m_drops (other.m_drops)
{
  //  .. nothing yet ..
}

template <class TS, class TI, class TR>
void
local_processor_cell_context<TS, TI, TR>::add (geo::local_processor_cell_context<TS, TI, TR> *parent_context, geo::Cell *parent, const geo::ICplxTrans &cell_inst)
{
  m_drops.push_back (local_processor_cell_drop<TS, TI, TR> (parent_context, parent, cell_inst));
}

template <class TS, class TI, class TR>
void
local_processor_cell_context<TS, TI, TR>::propagate (unsigned int output_layer, const std::unordered_set<TR> &res)
{
  if (res.empty ()) {
    return;
  }

  for (typename std::vector<local_processor_cell_drop<TS, TI, TR> >::const_iterator d = m_drops.begin (); d != m_drops.end (); ++d) {

    tl_assert (d->parent_context != 0);
    tl_assert (d->parent != 0);

    geo::Layout *subject_layout = d->parent->layout ();
    shape_reference_translator_with_trans<TR, geo::ICplxTrans> rt (subject_layout, d->cell_inst);
    std::vector<TR> new_refs;
    new_refs.reserve (res.size ());
    for (typename std::unordered_set<TR>::const_iterator r = res.begin (); r != res.end (); ++r) {
      new_refs.push_back (rt (*r));
    }

    {
      geo::MutexLocker locker (&d->parent_context->lock ());
      d->parent_context->propagated (output_layer).insert (new_refs.begin (), new_refs.end ());
    }

  }
}

//  explicit instantiations
template class DB_PUBLIC local_processor_cell_context<geo::Polygon, geo::Polygon, geo::Polygon>;
template class DB_PUBLIC local_processor_cell_context<geo::Polygon, geo::Text, geo::Polygon>;
template class DB_PUBLIC local_processor_cell_context<geo::Polygon, geo::Edge, geo::Polygon>;
template class DB_PUBLIC local_processor_cell_context<geo::Polygon, geo::Text, geo::Text>;
template class DB_PUBLIC local_processor_cell_context<geo::Polygon, geo::Edge, geo::Edge>;
template class DB_PUBLIC local_processor_cell_context<geo::PolygonRef, geo::PolygonRef, geo::PolygonRef>;
template class DB_PUBLIC local_processor_cell_context<geo::PolygonRef, geo::Edge, geo::PolygonRef>;
template class DB_PUBLIC local_processor_cell_context<geo::PolygonRef, geo::PolygonRef, geo::EdgePair>;
template class DB_PUBLIC local_processor_cell_context<geo::Polygon, geo::Polygon, geo::EdgePair>;
template class DB_PUBLIC local_processor_cell_context<geo::Edge, geo::Edge, geo::Edge>;
template class DB_PUBLIC local_processor_cell_context<geo::Edge, geo::Polygon, geo::Edge>;
template class DB_PUBLIC local_processor_cell_context<geo::Edge, geo::Edge, geo::EdgePair>;

// ---------------------------------------------------------------------------------------------
//  LocalProcessorCellContexts implementation

template <class TS, class TI, class TR>
local_processor_cell_contexts<TS, TI, TR>::local_processor_cell_contexts ()
  : mp_intruder_cell (0)
{
  //  .. nothing yet ..
}

template <class TS, class TI, class TR>
local_processor_cell_contexts<TS, TI, TR>::local_processor_cell_contexts (const geo::Cell *intruder_cell)
  : mp_intruder_cell (intruder_cell)
{
  //  .. nothing yet ..
}

template <class TS, class TI, class TR>
geo::local_processor_cell_context<TS, TI, TR> *
local_processor_cell_contexts<TS, TI, TR>::find_context (const context_key_type &intruders)
{
  typename std::unordered_map<context_key_type, geo::local_processor_cell_context<TS, TI, TR> >::iterator c = m_contexts.find (intruders);
  return c != m_contexts.end () ? &c->second : 0;
}

template <class TS, class TI, class TR>
geo::local_processor_cell_context<TS, TI, TR> *
local_processor_cell_contexts<TS, TI, TR>::create (const context_key_type &intruders)
{
  return &m_contexts[intruders];
}

template <class TR>
static void
subtract_set (std::unordered_set<TR> &res, const std::unordered_set<TR> &other)
{
  //  for everything else, we don't use a boolean core but just set intersection
  for (typename std::unordered_set<TR>::const_iterator o = other.begin (); o != other.end (); ++o) {
    res.erase (*o);
  }
}

template <class TS, class TI>
static void
subtract (std::unordered_set<geo::PolygonRef> &res, const std::unordered_set<geo::PolygonRef> &other, geo::Layout *layout, const geo::local_processor<TS, TI, geo::PolygonRef> *proc)
{
  if (other.empty ()) {
    return;
  }

  if (! proc->boolean_core ()) {
    subtract_set (res, other);
    return;
  }

  size_t max_vertex_count = proc->max_vertex_count ();
  double area_ratio = proc->area_ratio ();

  geo::EdgeProcessor ep;
  ep.set_base_verbosity (proc->base_verbosity () + 30);

  size_t p1 = 0, p2 = 1;

  for (std::unordered_set<geo::PolygonRef>::const_iterator i = res.begin (); i != res.end (); ++i) {
    const geo::PolygonRef &subject = *i;
    for (geo::PolygonRef::polygon_edge_iterator e = subject.begin_edge (); ! e.at_end(); ++e) {
      ep.insert (*e, p1);
    }
    p1 += 2;
  }

  for (std::unordered_set<geo::PolygonRef>::const_iterator i = other.begin (); i != other.end (); ++i) {
    const geo::PolygonRef &subject = *i;
    for (geo::PolygonRef::polygon_edge_iterator e = subject.begin_edge (); ! e.at_end(); ++e) {
      ep.insert (*e, p2);
    }
    p2 += 2;
  }

  res.clear ();
  geo::BooleanOp op (geo::BooleanOp::ANotB);
  geo::PolygonRefGenerator pr (layout, res);
  geo::PolygonSplitter splitter (pr, area_ratio, max_vertex_count);
  geo::PolygonGenerator pg (splitter, true, true);
  ep.process (pg, op);
}

template <class TS, class TI>
static void
subtract (std::unordered_set<geo::Edge> &res, const std::unordered_set<geo::Edge> &other, geo::Layout * /*layout*/, const geo::local_processor<TS, TI, geo::Edge> *proc)
{
  if (other.empty ()) {
    return;
  }

  if (! proc->boolean_core ()) {
    subtract_set (res, other);
    return;
  }

  geo::box_scanner<geo::Edge, size_t> scanner;
  scanner.reserve (res.size () + other.size ());

  for (std::unordered_set<Edge>::const_iterator i = res.begin (); i != res.end (); ++i) {
    scanner.insert (i.operator-> (), 0);
  }
  for (std::unordered_set<Edge>::const_iterator i = other.begin (); i != other.end (); ++i) {
    scanner.insert (i.operator-> (), 1);
  }

  std::unordered_set<geo::Edge> result;
  EdgeBooleanClusterCollector<std::unordered_set<geo::Edge> > cluster_collector (&result, EdgeNot);
  scanner.process (cluster_collector, 1, geo::box_convert<geo::Edge> ());

  res.swap (result);
}

template <class TS, class TI, class TR>
static void
subtract (std::unordered_set<TR> &res, const std::unordered_set<TR> &other, geo::Layout * /*layout*/, const geo::local_processor<TS, TI, TR> * /*proc*/)
{
  subtract_set (res, other);
}

//  determines the default boolean core flag per result type

namespace
{

template <class TR>
struct default_boolean_core
{
  bool operator() () const { return false; }
};

template <>
struct default_boolean_core<geo::PolygonRef>
{
  bool operator() () const { return true; }
};

}

namespace {

template <class TS, class TI, class TR>
struct context_sorter
{
  bool operator () (const std::pair<const typename local_processor_cell_contexts<TS, TI, TR>::context_key_type *, geo::local_processor_cell_context<TS, TI, TR> *> &a,
                    const std::pair<const typename local_processor_cell_contexts<TS, TI, TR>::context_key_type *, geo::local_processor_cell_context<TS, TI, TR> *> &b)
  {
    return *a.first < *b.first;
  }
};

}

template <class TS, class TI, class TR>
void
local_processor_cell_contexts<TS, TI, TR>::compute_results (const local_processor_contexts<TS, TI, TR> &contexts, geo::Cell *cell, const local_operation<TS, TI, TR> *op, const std::vector<unsigned int> &output_layers, const local_processor<TS, TI, TR> *proc)
{
  CRONOLOGY_COMPUTE_BRACKET(event_compute_results)

  bool first = true;
  std::vector<std::unordered_set<TR> > common;
  common.resize (output_layers.size ());

  int index = 0;
  int total = int (m_contexts.size ());

  //  NOTE: we use the ordering provided by key_type::operator< rather than the unordered map to achieve
  //  reproducibility across different platforms. unordered_map is faster, but for processing them,
  //  strict ordering is a more robust choice.
  std::vector<std::pair<const context_key_type *, geo::local_processor_cell_context<TS, TI, TR> *> > sorted_contexts;
  sorted_contexts.reserve (m_contexts.size ());
  for (typename std::unordered_map<context_key_type, geo::local_processor_cell_context<TS, TI, TR> >::iterator c = m_contexts.begin (); c != m_contexts.end (); ++c) {
    sorted_contexts.push_back (std::make_pair (&c->first, &c->second));
  }

  std::sort (sorted_contexts.begin (), sorted_contexts.end (), context_sorter<TS, TI, TR> ());

  for (typename std::vector<std::pair<const context_key_type *, geo::local_processor_cell_context<TS, TI, TR> *> >::const_iterator c = sorted_contexts.begin (); c != sorted_contexts.end (); ++c) {

    proc->next ();
    ++index;

    if (geo::verbosity () >= proc->base_verbosity () + 20) {
      geo::log << tr ("Computing local results for ") << cell->layout ()->cell_name (cell->cell_index ()) << " (context " << index << "/" << total << ")";
    }

    if (first) {

      {
        geo::MutexLocker locker (&c->second->lock ());
        for (std::vector<unsigned int>::const_iterator o = output_layers.begin (); o != output_layers.end (); ++o) {
          common [o - output_layers.begin ()] = c->second->propagated (*o);
        }
      }

      CRONOLOGY_COMPUTE_BRACKET(event_compute_local_cell)
      proc->compute_local_cell (contexts, cell, mp_intruder_cell, op, *c->first, common);
      first = false;

    } else {

      std::vector<std::unordered_set<TR> > res;
      res.resize (output_layers.size ());

      {
        geo::MutexLocker locker (&c->second->lock ());
        for (std::vector<unsigned int>::const_iterator o = output_layers.begin (); o != output_layers.end (); ++o) {
          res [o - output_layers.begin ()] = c->second->propagated (*o);
        }
      }

      {
        CRONOLOGY_COMPUTE_BRACKET(event_compute_local_cell)
        proc->compute_local_cell (contexts, cell, mp_intruder_cell, op, *c->first, res);
      }

      if (common.empty ()) {

        CRONOLOGY_COMPUTE_BRACKET(event_propagate)
        for (std::vector<unsigned int>::const_iterator o = output_layers.begin (); o != output_layers.end (); ++o) {
          c->second->propagate (*o, res [o - output_layers.begin ()]);
        }

//  gcc 4.4.7 (at least) doesn't have an operator== in std::unordered_set, so we skip this
//  optimization
#if defined(__GNUC__) && __GNUC__ == 4
      } else {
#else
      } else if (res != common) {
#endif

        CRONOLOGY_COMPUTE_BRACKET(event_propagate)

        for (std::vector<unsigned int>::const_iterator o = output_layers.begin (); o != output_layers.end (); ++o) {

          size_t oi = o - output_layers.begin ();

          std::unordered_set<TR> lost;

          for (typename std::unordered_set<TR>::const_iterator i = common[oi].begin (); i != common[oi].end (); ++i) {
            if (res[oi].find (*i) == res[oi].end ()) {
              lost.insert (*i);
            }
          }

          if (! lost.empty ()) {

            subtract (lost, res[oi], cell->layout (), proc);

            if (! lost.empty ()) {
              subtract (common[oi], lost, cell->layout (), proc);
              for (typename std::vector<std::pair<const context_key_type *, geo::local_processor_cell_context<TS, TI, TR> *> >::const_iterator cc = sorted_contexts.begin (); cc != c; ++cc) {
                cc->second->propagate (*o, lost);
              }
            }

          }

        }

        for (std::vector<unsigned int>::const_iterator o = output_layers.begin (); o != output_layers.end (); ++o) {

          std::unordered_set<TR> gained;

          size_t oi = o - output_layers.begin ();
          for (typename std::unordered_set<TR>::const_iterator i = res[oi].begin (); i != res[oi].end (); ++i) {
            if (common[oi].find (*i) == common[oi].end ()) {
              gained.insert (*i);
            }
          }

          if (! gained.empty ()) {

            subtract (gained, common[oi], cell->layout (), proc);

            if (! gained.empty ()) {
              c->second->propagate (*o, gained);
            }

          }

        }

      }

    }

  }

  for (std::vector<unsigned int>::const_iterator o = output_layers.begin (); o != output_layers.end (); ++o) {
    size_t oi = o - output_layers.begin ();
    proc->push_results (cell, *o, common[oi]);
  }
}

//  explicit instantiations
template class DB_PUBLIC local_processor_cell_contexts<geo::Polygon, geo::Polygon, geo::Polygon>;
template class DB_PUBLIC local_processor_cell_contexts<geo::Polygon, geo::Edge, geo::Polygon>;
template class DB_PUBLIC local_processor_cell_contexts<geo::Polygon, geo::Edge, geo::Edge>;
template class DB_PUBLIC local_processor_cell_contexts<geo::Polygon, geo::Text, geo::Polygon>;
template class DB_PUBLIC local_processor_cell_contexts<geo::Polygon, geo::Text, geo::Text>;
template class DB_PUBLIC local_processor_cell_contexts<geo::PolygonRef, geo::PolygonRef, geo::PolygonRef>;
template class DB_PUBLIC local_processor_cell_contexts<geo::PolygonRef, geo::Edge, geo::PolygonRef>;
template class DB_PUBLIC local_processor_cell_contexts<geo::PolygonRef, geo::PolygonRef, geo::EdgePair>;
template class DB_PUBLIC local_processor_cell_contexts<geo::Polygon, geo::Polygon, geo::EdgePair>;
template class DB_PUBLIC local_processor_cell_contexts<geo::Edge, geo::PolygonRef, geo::Edge>;
template class DB_PUBLIC local_processor_cell_contexts<geo::Edge, geo::Edge, geo::Edge>;
template class DB_PUBLIC local_processor_cell_contexts<geo::Edge, geo::Edge, geo::EdgePair>;

// ---------------------------------------------------------------------------------------------

template <class TS, class TI>
shape_interactions<TS, TI>::shape_interactions ()
  : m_id (0)
{
  //  .. nothing yet ..
}

template <class TS, class TI>
bool
shape_interactions<TS, TI>::has_intruder_shape_id (unsigned int id) const
{
  return m_intruder_shapes.find (id) != m_intruder_shapes.end ();
}

template <class TS, class TI>
bool
shape_interactions<TS, TI>::has_subject_shape_id (unsigned int id) const
{
  return m_subject_shapes.find (id) != m_subject_shapes.end ();
}

template <class TS, class TI>
void
shape_interactions<TS, TI>::add_intruder_shape (unsigned int id, unsigned int layer, const TI &shape)
{
  m_intruder_shapes [id] = std::make_pair (layer, shape);
}

template <class TS, class TI>
void
shape_interactions<TS, TI>::add_subject_shape (unsigned int id, const TS &shape)
{
  m_subject_shapes [id] = shape;
}

template <class TS, class TI>
void
shape_interactions<TS, TI>::add_subject (unsigned int id, const TS &shape)
{
  m_subject_shapes [id] = shape;
  m_interactions.insert (std::make_pair (id, container::value_type::second_type ()));
}

template <class TS, class TI>
void
shape_interactions<TS, TI>::add_interaction (unsigned int subject_id, unsigned int intruder_id)
{
  m_interactions [subject_id].push_back (intruder_id);
}

template <class TS, class TI>
const std::vector<unsigned int> &
shape_interactions<TS, TI>::intruders_for (unsigned int subject_id) const
{
  iterator i = m_interactions.find (subject_id);
  if (i == m_interactions.end ()) {
    static std::vector<unsigned int> empty;
    return empty;
  } else {
    return i->second;
  }
}

template <class TS, class TI>
const TS &
shape_interactions<TS, TI>::subject_shape (unsigned int id) const
{
  typename std::unordered_map<unsigned int, TS>::const_iterator i = m_subject_shapes.find (id);
  if (i == m_subject_shapes.end ()) {
    static TS s;
    return s;
  } else {
    return i->second;
  }
}

template <class TS, class TI>
const std::pair<unsigned int, TI> &
shape_interactions<TS, TI>::intruder_shape (unsigned int id) const
{
  typename std::unordered_map<unsigned int, std::pair<unsigned int, TI> >::const_iterator i = m_intruder_shapes.find (id);
  if (i == m_intruder_shapes.end ()) {
    static std::pair<unsigned int, TI> s;
    return s;
  } else {
    return i->second;
  }
}

//  explicit instantiations
template class DB_PUBLIC shape_interactions<geo::Polygon, geo::Polygon>;
template class DB_PUBLIC shape_interactions<geo::Polygon, geo::Text>;
template class DB_PUBLIC shape_interactions<geo::Polygon, geo::TextRef>;
template class DB_PUBLIC shape_interactions<geo::Polygon, geo::Edge>;
template class DB_PUBLIC shape_interactions<geo::PolygonRef, geo::PolygonRef>;
template class DB_PUBLIC shape_interactions<geo::PolygonRef, geo::TextRef>;
template class DB_PUBLIC shape_interactions<geo::PolygonRef, geo::Text>;
template class DB_PUBLIC shape_interactions<geo::PolygonRef, geo::Edge>;
template class DB_PUBLIC shape_interactions<geo::Edge, geo::Edge>;
template class DB_PUBLIC shape_interactions<geo::Edge, geo::PolygonRef>;
template class DB_PUBLIC shape_interactions<geo::TextRef, geo::TextRef>;
template class DB_PUBLIC shape_interactions<geo::TextRef, geo::PolygonRef>;

// ---------------------------------------------------------------------------------------------
//  Helper classes for the LocalProcessor

namespace
{

template <class TS, class TI>
struct interaction_registration_shape2shape
  : geo::box_scanner_receiver2<TS, unsigned int, TI, unsigned int>
{
public:
  interaction_registration_shape2shape (geo::Layout *layout, shape_interactions<TS, TI> *result, unsigned int intruder_layer_index)
    : mp_result (result), mp_layout (layout), m_intruder_layer_index (intruder_layer_index)
  {
    //  nothing yet ..
  }

  void add (const TS *ref1, unsigned int id1, const TI *ref2, unsigned int id2)
  {
    if (!mp_result->has_subject_shape_id (id1)) {
      mp_result->add_subject_shape (id1, *ref1);
    }

    if (!mp_result->has_intruder_shape_id (id2)) {
      if (mp_layout) {
        //  In order to guarantee the refs come from the subject layout, we'd need to
        //  rewrite them
        geo::shape_reference_translator<TI> rt (mp_layout);
        mp_result->add_intruder_shape (id2, m_intruder_layer_index, rt (*ref2));
      } else {
        mp_result->add_intruder_shape (id2, m_intruder_layer_index, *ref2);
      }
    }

    mp_result->add_interaction (id1, id2);
  }

  void same (unsigned int, unsigned int)
  {
    //  ignore. Two shapes of a different kind can't be the same.
  }

private:
  shape_interactions<TS, TI> *mp_result;
  geo::Layout *mp_layout;
  unsigned int m_intruder_layer_index;
};

template <class T>
struct interaction_registration_shape2shape<T, T>
  : geo::box_scanner_receiver2<T, unsigned int, T, unsigned int>
{
public:
  interaction_registration_shape2shape (geo::Layout *layout, shape_interactions<T, T> *result, unsigned int intruder_layer_index)
    : mp_result (result), mp_layout (layout), m_intruder_layer_index (intruder_layer_index)
  {
    //  nothing yet ..
  }

  void add (const T *ref1, unsigned int id1, const T *ref2, unsigned int id2)
  {
    if (! m_same.empty () && (m_same.find (std::make_pair (id1, id2)) != m_same.end () || m_same.find (std::make_pair (id2, id1)) != m_same.end ())) {
      //  ignore self-interactions
      return;
    }

    if (!mp_result->has_subject_shape_id (id1)) {
      mp_result->add_subject_shape (id1, *ref1);
    }

    if (!mp_result->has_intruder_shape_id (id2)) {
      if (mp_layout) {
        //  In order to guarantee the refs come from the subject layout, we'd need to
        //  rewrite them
        geo::shape_reference_translator<T> rt (mp_layout);
        mp_result->add_intruder_shape (id2, m_intruder_layer_index, rt (*ref2));
      } else {
        mp_result->add_intruder_shape (id2, m_intruder_layer_index, *ref2);
      }
    }

    mp_result->add_interaction (id1, id2);
  }

  void same (unsigned int a, unsigned int b)
  {
    m_same.insert (std::make_pair (a, b));
  }

private:
  shape_interactions<T, T> *mp_result;
  std::unordered_set<std::pair<unsigned int, unsigned int> > m_same;
  geo::Layout *mp_layout;
  unsigned int m_intruder_layer_index;
};

template <class TS, class TI>
struct interaction_registration_shape1
  : geo::box_scanner_receiver2<TS, unsigned int, TI, unsigned int>
{
public:
  interaction_registration_shape1 (shape_interactions<TS, TI> *result, unsigned int intruder_layer_index)
    : mp_result (result), m_intruder_layer_index (intruder_layer_index)
  {
    //  .. nothing yet ..
  }

  void add (const TS *ref1, unsigned int id1, const TI *ref2, unsigned int id2)
  {
    if (!mp_result->has_subject_shape_id (id1)) {
      mp_result->add_subject_shape (id1, *ref1);
    }
    if (!mp_result->has_intruder_shape_id (id2)) {
      mp_result->add_intruder_shape (id2, m_intruder_layer_index, *ref2);
    }
    mp_result->add_interaction (id1, id2);
  }

private:
  shape_interactions<TS, TI> *mp_result;
  unsigned int m_intruder_layer_index;
};

template <class T>
struct interaction_registration_shape1<T, T>
  : geo::box_scanner_receiver<T, unsigned int>
{
public:
  interaction_registration_shape1 (shape_interactions<T, T> *result, unsigned int intruder_layer_index)
    : mp_result (result), m_intruder_layer_index (intruder_layer_index)
  {
    //  nothing yet ..
  }

  void add (const T *ref1, unsigned int id1, const T *ref2, unsigned int id2)
  {
    if (!mp_result->has_subject_shape_id (id1)) {
      mp_result->add_subject_shape (id1, *ref1);
    }
    if (!mp_result->has_intruder_shape_id (id2)) {
      mp_result->add_intruder_shape (id2, m_intruder_layer_index, *ref2);
    }
    mp_result->add_interaction (id1, id2);
  }

private:
  shape_interactions<T, T> *mp_result;
  unsigned int m_intruder_layer_index;
};

template <class TS, class TI>
struct interaction_registration_shape2inst
  : geo::box_scanner_receiver2<TS, unsigned int, geo::CellInstArray, unsigned int>
{
public:
  interaction_registration_shape2inst (geo::Layout *subject_layout, const geo::Layout *intruder_layout, unsigned int intruder_layer, unsigned int intruder_layer_index, geo::Coord dist, shape_interactions<TS, TI> *result)
    : mp_subject_layout (subject_layout), mp_intruder_layout (intruder_layout), m_intruder_layer (intruder_layer), m_intruder_layer_index (intruder_layer_index), m_dist (dist), mp_result (result)
  {
    //  nothing yet ..
  }

  void add (const TS *ref, unsigned int id1, const geo::CellInstArray *inst, unsigned int inst_id)
  {
    const geo::Cell &intruder_cell = mp_intruder_layout->cell (inst->object ().cell_index ());
    geo::box_convert <geo::CellInst, true> inst_bc (*mp_intruder_layout, m_intruder_layer);
    mp_result->add_subject_shape (id1, *ref);

    //  Find all instance array members that potentially interact with the shape and use
    //  add_shapes_from_intruder_inst on them
    geo::Box ref_box = geo::box_convert<TS> () (*ref);
    for (geo::CellInstArray::iterator n = inst->begin_touching (safe_box_enlarged (ref_box, m_dist - 1, m_dist - 1), inst_bc); !n.at_end (); ++n) {
      geo::ICplxTrans tn = inst->complex_trans (*n);
      geo::Box region = ref_box.transformed (tn.inverted ()).enlarged (geo::Vector (m_dist, m_dist)) & intruder_cell.bbox (m_intruder_layer).enlarged (geo::Vector (m_dist, m_dist));
      if (! region.empty ()) {
        add_shapes_from_intruder_inst (id1, intruder_cell, tn, inst_id, region);
      }
    }
  }

private:
  geo::Layout *mp_subject_layout;
  const geo::Layout *mp_intruder_layout;
  unsigned int m_intruder_layer, m_intruder_layer_index;
  geo::Coord m_dist;
  shape_interactions<TS, TI> *mp_result;
  std::unordered_map<TI, unsigned int> m_inst_shape_ids;

  void add_shapes_from_intruder_inst (unsigned int id1, const geo::Cell &intruder_cell, const geo::ICplxTrans &tn, unsigned int /*inst_id*/, const geo::Box &region)
  {
    geo::shape_reference_translator<TI> rt (mp_subject_layout);

    //  Look up all shapes from the intruder instance which interact with the subject shape
    //  (given through region)
    //  TODO: should be lighter, cache, handle arrays ..
    geo::RecursiveShapeIterator si (*mp_intruder_layout, intruder_cell, m_intruder_layer, region);
    si.shape_flags (shape_flags<TI> ());
    while (! si.at_end ()) {

      //  NOTE: we intentionally rewrite to the *subject* layout - this way polygon refs in the context come from the
      //  subject, not from the intruder.
      TI ref2 = rt (*si.shape ().basic_ptr (typename TI::tag ()), tn * si.trans ());

      //  reuse the same id for shapes from the same instance -> this avoid duplicates with different IDs on
      //  the intruder side.
      typename std::unordered_map<TI, unsigned int>::const_iterator k = m_inst_shape_ids.find (ref2);
      if (k == m_inst_shape_ids.end ()) {

        k = m_inst_shape_ids.insert (std::make_pair (ref2, mp_result->next_id ())).first;
        mp_result->add_intruder_shape (k->second, m_intruder_layer_index, ref2);

      }

      mp_result->add_interaction (id1, k->second);

      ++si;

    }
  }
};

static bool
instances_interact (const geo::Layout *layout1, const geo::CellInstArray *inst1, unsigned int layer1, const geo::Layout *layout2, const geo::CellInstArray *inst2, unsigned int layer2, geo::Coord dist)
{
  //  TODO: this algorithm is not in particular effective for identical arrays

  const geo::Cell &cell1 = layout1->cell (inst1->object ().cell_index ());
  const geo::Cell &cell2 = layout2->cell (inst2->object ().cell_index ());
  geo::box_convert <geo::CellInst, true> inst2_bc (*layout2, layer2);

  std::unordered_set<geo::ICplxTrans> relative_trans_seen;

  for (geo::CellInstArray::iterator n = inst1->begin (); ! n.at_end (); ++n) {

    geo::ICplxTrans tn1 = inst1->complex_trans (*n);
    geo::ICplxTrans tni1 = tn1.inverted ();
    geo::Box ibox1 = tn1 * cell1.bbox (layer1).enlarged (geo::Vector (dist, dist));

    if (! ibox1.empty ()) {

      //  TODO: in some cases, it may be possible to optimize this for arrays

      for (geo::CellInstArray::iterator k = inst2->begin_touching (safe_box_enlarged (ibox1, -1, -1), inst2_bc); ! k.at_end (); ++k) {

        if (inst1 == inst2 && *n == *k) {
          //  skip self-interactions - this is handled inside the cell
          continue;
        }

        geo::ICplxTrans tn2 = inst2->complex_trans (*k);

        //  NOTE: we need to enlarge both subject *and* intruder boxes - either object comes close to intruder or the other way around
        geo::Box ibox2 = tn2 * cell2.bbox (layer2).enlarged (geo::Vector (dist, dist));

        geo::ICplxTrans tn21 = tni1 * tn2;
        if (! relative_trans_seen.insert (tn21).second) {
          //  this relative transformation was already seen
          continue;
        }

        geo::Box cbox = ibox1 & ibox2;
        if (! cbox.empty ()) {

          geo::ICplxTrans tni2 = tn2.inverted ();

          //  not very strong, but already useful: the cells interact if there is a layer1 in cell1
          //  in the common box and a layer2 in the cell2 in the common box
          //  NOTE: don't use overlap mode for the RecursiveShapeIterator as this would not capture dot-like
          //  objects like texts. Instead safe-shrink the search box and use touching mode ("false" for the last
          //  argument)
          if (! geo::RecursiveShapeIterator (*layout1, cell1, layer1, safe_box_enlarged (tni1 * cbox, -1, -1), false).at_end () &&
              ! geo::RecursiveShapeIterator (*layout2, cell2, layer2, safe_box_enlarged (tni2 * cbox, -1, -1), false).at_end ()) {
            return true;
          }

        }

      }

    }

  }

  return false;
}

template <class T>
struct interaction_registration_inst2inst
  : geo::box_scanner_receiver2<geo::CellInstArray, unsigned int, geo::CellInstArray, unsigned int>
{
public:
  typedef std::pair<std::unordered_set<const geo::CellInstArray *>, std::map<unsigned int, std::unordered_set<T> > > interaction_value_type;

  interaction_registration_inst2inst (const geo::Layout *subject_layout, unsigned int subject_layer, const geo::Layout *intruder_layout, unsigned int intruder_layer, bool foreign, geo::Coord dist, std::unordered_map<const geo::CellInstArray *, interaction_value_type> *result)
    : mp_subject_layout (subject_layout), mp_intruder_layout (intruder_layout), m_subject_layer (subject_layer), m_intruder_layer (intruder_layer), m_dist (dist), mp_result (result), m_foreign (foreign)
  {
    //  nothing yet ..
  }

  void add (const geo::CellInstArray *inst1, unsigned int id1, const geo::CellInstArray *inst2, unsigned int id2)
  {
    //  NOTE: self-interactions are possible for arrays: different elements of the
    //  array may interact which is a cell-external interaction.
    if (mp_subject_layout != mp_intruder_layout || id1 != id2 || inst1->size () > 1) {

      bool ignore = false;
      if (mp_subject_layout == mp_intruder_layout && m_subject_layer == m_intruder_layer && ! m_foreign) {
        if (m_interactions.find (std::make_pair (id2, id1)) != m_interactions.end ()) {
          //  for self interactions ignore the reverse interactions
          ignore = true;
        } else {
          m_interactions.insert (std::make_pair (id1, id2));
        }
      }

      if (! ignore && instances_interact (mp_subject_layout, inst1, m_subject_layer, mp_intruder_layout, inst2, m_intruder_layer, m_dist)) {
        (*mp_result) [inst1].first.insert (inst2);
      }

    }
  }

private:
  const geo::Layout *mp_subject_layout, *mp_intruder_layout;
  unsigned int m_subject_layer, m_intruder_layer;
  geo::Coord m_dist;
  std::unordered_map<const geo::CellInstArray *, std::pair<std::unordered_set<const geo::CellInstArray *>, std::map<unsigned int, std::unordered_set<T> > > > *mp_result;
  std::unordered_set<std::pair<unsigned int, unsigned int> > m_interactions;
  bool m_foreign;
};

template <class T>
static bool
instance_shape_interacts (const geo::Layout *layout, const geo::CellInstArray *inst, unsigned int layer, const T &ref, geo::Coord dist)
{
  const geo::Cell &cell = layout->cell (inst->object ().cell_index ());
  geo::box_convert <geo::CellInst, true> inst_bc (*layout, layer);
  geo::Box rbox = geo::box_convert<T> () (ref);

  for (geo::CellInstArray::iterator n = inst->begin_touching (safe_box_enlarged (rbox, dist - 1, dist - 1), inst_bc); ! n.at_end (); ++n) {

    geo::ICplxTrans tn = inst->complex_trans (*n);
    geo::Box cbox = (tn * cell.bbox (layer)).enlarged (geo::Vector (dist, dist)) & rbox.enlarged (geo::Vector (dist, dist));

    if (! cbox.empty ()) {

      geo::ICplxTrans tni = tn.inverted ();

      //  not very strong, but already useful: the cells interact if there is a layer in cell
      //  in the common box
      //  NOTE: don't use overlapping mode here, because this will not select point-like objects as texts or
      //  dot edges. Instead safe-shrink the search box and use touching mode.
      if (! geo::RecursiveShapeIterator (*layout, cell, layer, safe_box_enlarged (tni * cbox, -1, -1), false).at_end ()) {
        return true;
      }

    }

  }

  return false;
}

template <class T>
struct interaction_registration_inst2shape
  : geo::box_scanner_receiver2<geo::CellInstArray, unsigned int, T, unsigned int>
{
public:
  interaction_registration_inst2shape (const geo::Layout *subject_layout, unsigned int subject_layer, geo::Coord dist, std::unordered_map<const geo::CellInstArray *, std::pair<std::unordered_set<const geo::CellInstArray *>, std::map<unsigned int, std::unordered_set<T> > > > *result)
    : mp_subject_layout (subject_layout), m_subject_layer (subject_layer), m_dist (dist), mp_result (result)
  {
    //  nothing yet ..
  }

  void add (const geo::CellInstArray *inst, unsigned int, const T *ref, unsigned int layer)
  {
    if (instance_shape_interacts (mp_subject_layout, inst, m_subject_layer, *ref, m_dist)) {
      (*mp_result) [inst].second [layer].insert (*ref);
    }
  }

private:
  const geo::Layout *mp_subject_layout;
  unsigned int m_subject_layer;
  geo::Coord m_dist;
  std::unordered_map<const geo::CellInstArray *, std::pair<std::unordered_set<const geo::CellInstArray *>, std::map<unsigned int, std::unordered_set<T> > > > *mp_result;
};

}

// ---------------------------------------------------------------------------------------------
//  LocalProcessorContextComputationTask implementation

template <class TS, class TI, class TR>
local_processor_context_computation_task<TS, TI, TR>::local_processor_context_computation_task (const local_processor<TS, TI, TR> *proc, local_processor_contexts<TS, TI, TR> &contexts, geo::local_processor_cell_context<TS, TI, TR> *parent_context, geo::Cell *subject_parent, geo::Cell *subject_cell, const geo::ICplxTrans &subject_cell_inst, const geo::Cell *intruder_cell, typename local_processor_cell_contexts<TS, TI, TR>::context_key_type &intruders, geo::Coord dist)
  : geo::Task (),
    mp_proc (proc), mp_contexts (&contexts), mp_parent_context (parent_context),
    mp_subject_parent (subject_parent), mp_subject_cell (subject_cell), m_subject_cell_inst (subject_cell_inst),
    mp_intruder_cell (intruder_cell), m_dist (dist)
{
  //  This is quick, but will take away the intruders from the caller
  m_intruders.swap (intruders);
}

template <class TS, class TI, class TR>
void
local_processor_context_computation_task<TS, TI, TR>::perform ()
{
  mp_proc->compute_contexts (*mp_contexts, mp_parent_context, mp_subject_parent, mp_subject_cell, m_subject_cell_inst, mp_intruder_cell, m_intruders, m_dist);
}

//  explicit instantiations
template class DB_PUBLIC local_processor_context_computation_task<geo::Polygon, geo::Polygon, geo::Polygon>;
template class DB_PUBLIC local_processor_context_computation_task<geo::Polygon, geo::Text, geo::Polygon>;
template class DB_PUBLIC local_processor_context_computation_task<geo::Polygon, geo::Text, geo::Text>;
template class DB_PUBLIC local_processor_context_computation_task<geo::Polygon, geo::Edge, geo::Polygon>;
template class DB_PUBLIC local_processor_context_computation_task<geo::Polygon, geo::Edge, geo::Edge>;
template class DB_PUBLIC local_processor_context_computation_task<geo::PolygonRef, geo::PolygonRef, geo::PolygonRef>;
template class DB_PUBLIC local_processor_context_computation_task<geo::PolygonRef, geo::Edge, geo::PolygonRef>;
template class DB_PUBLIC local_processor_context_computation_task<geo::PolygonRef, geo::PolygonRef, geo::EdgePair>;
template class DB_PUBLIC local_processor_context_computation_task<geo::Polygon, geo::Polygon, geo::EdgePair>;
template class DB_PUBLIC local_processor_context_computation_task<geo::Polygon, geo::Polygon, geo::Edge>;
template class DB_PUBLIC local_processor_context_computation_task<geo::Edge, geo::Edge, geo::Edge>;
template class DB_PUBLIC local_processor_context_computation_task<geo::Edge, geo::PolygonRef, geo::Edge>;
template class DB_PUBLIC local_processor_context_computation_task<geo::Edge, geo::Edge, geo::EdgePair>;

// ---------------------------------------------------------------------------------------------
//  LocalProcessorResultComputationTask implementation

template <class TS, class TI, class TR>
local_processor_result_computation_task<TS, TI, TR>::local_processor_result_computation_task (const local_processor<TS, TI, TR> *proc, local_processor_contexts<TS, TI, TR> &contexts, geo::Cell *cell, local_processor_cell_contexts<TS, TI, TR> *cell_contexts, const local_operation<TS, TI, TR> *op, const std::vector<unsigned int> &output_layers)
  : mp_proc (proc), mp_contexts (&contexts), mp_cell (cell), mp_cell_contexts (cell_contexts), mp_op (op), m_output_layers (output_layers)
{
  //  .. nothing yet ..
}

template <class TS, class TI, class TR>
void
local_processor_result_computation_task<TS, TI, TR>::perform ()
{
  mp_cell_contexts->compute_results (*mp_contexts, mp_cell, mp_op, m_output_layers, mp_proc);

  //  erase the contexts we don't need any longer
  {
    geo::MutexLocker locker (& mp_contexts->lock ());

#if defined(ENABLE_DB_HP_SANITY_ASSERTIONS)
    std::set<const geo::local_processor_cell_context<TS, TI, TR> *> td;
    for (typename geo::local_processor_cell_contexts<TS, TI, TR>::iterator i = mp_cell_contexts->begin (); i != mp_cell_contexts->end (); ++i) {
      td.insert (&i->second);
    }
    for (typename geo::local_processor_cell_contexts<TS, TI, TR>::contexts_per_cell_type::iterator pcc = mp_contexts->context_map ().begin (); pcc != mp_contexts->context_map ().end (); ++pcc) {
      for (typename geo::local_processor_cell_contexts<TS, TI, TR>::iterator i = pcc->second.begin (); i != pcc->second.end (); ++i) {
        for (typename geo::local_processor_cell_context<TS, TI, TR>::drop_iterator j = i->second.begin_drops (); j != i->second.end_drops (); ++j) {
          if (td.find (j->parent_context) != td.end ()) {
            tl_assert (false);
          }
        }
      }
    }
#endif

    mp_contexts->context_map ().erase (mp_cell);
  }
}

//  explicit instantiations
template class DB_PUBLIC local_processor_result_computation_task<geo::Polygon, geo::Polygon, geo::Polygon>;
template class DB_PUBLIC local_processor_result_computation_task<geo::Polygon, geo::Text, geo::Polygon>;
template class DB_PUBLIC local_processor_result_computation_task<geo::Polygon, geo::Text, geo::Text>;
template class DB_PUBLIC local_processor_result_computation_task<geo::Polygon, geo::Edge, geo::Polygon>;
template class DB_PUBLIC local_processor_result_computation_task<geo::Polygon, geo::Edge, geo::Edge>;
template class DB_PUBLIC local_processor_result_computation_task<geo::PolygonRef, geo::PolygonRef, geo::PolygonRef>;
template class DB_PUBLIC local_processor_result_computation_task<geo::PolygonRef, geo::Edge, geo::PolygonRef>;
template class DB_PUBLIC local_processor_result_computation_task<geo::PolygonRef, geo::PolygonRef, geo::EdgePair>;
template class DB_PUBLIC local_processor_result_computation_task<geo::Polygon, geo::Polygon, geo::EdgePair>;
template class DB_PUBLIC local_processor_result_computation_task<geo::Edge, geo::Edge, geo::Edge>;
template class DB_PUBLIC local_processor_result_computation_task<geo::Edge, geo::PolygonRef, geo::Edge>;
template class DB_PUBLIC local_processor_result_computation_task<geo::Edge, geo::Edge, geo::EdgePair>;

// ---------------------------------------------------------------------------------------------
//  LocalProcessor implementation

template <class TS, class TI, class TR>
local_processor<TS, TI, TR>::local_processor (geo::Layout *layout, geo::Cell *top, const std::set<geo::cell_index_type> *breakout_cells)
  : mp_subject_layout (layout), mp_intruder_layout (layout),
    mp_subject_top (top), mp_intruder_top (top),
    mp_subject_breakout_cells (breakout_cells), mp_intruder_breakout_cells (breakout_cells),
    m_report_progress (true), m_nthreads (0), m_max_vertex_count (0), m_area_ratio (0.0), m_boolean_core (default_boolean_core<TR> () ()), m_base_verbosity (30), m_progress (0), mp_progress (0)
{
  //  .. nothing yet ..
}

template <class TS, class TI, class TR>
local_processor<TS, TI, TR>::local_processor (geo::Layout *subject_layout, geo::Cell *subject_top, const geo::Layout *intruder_layout, const geo::Cell *intruder_top, const std::set<cell_index_type> *subject_breakout_cells, const std::set<cell_index_type> *intruder_breakout_cells)
  : mp_subject_layout (subject_layout), mp_intruder_layout (intruder_layout),
    mp_subject_top (subject_top), mp_intruder_top (intruder_top),
    mp_subject_breakout_cells (subject_breakout_cells), mp_intruder_breakout_cells (intruder_breakout_cells),
    m_report_progress (true), m_nthreads (0), m_max_vertex_count (0), m_area_ratio (0.0), m_boolean_core (default_boolean_core<TR> () ()), m_base_verbosity (30), m_progress (0), mp_progress (0)
{
  //  .. nothing yet ..
}

template <class TS, class TI, class TR>
std::string local_processor<TS, TI, TR>::description (const local_operation<TS, TI, TR> *op) const
{
  if (op && m_description.empty ()) {
    return op->description ();
  } else {
    return m_description;
  }
}

template <class TS, class TI, class TR>
void local_processor<TS, TI, TR>::next () const
{
  static geo::Mutex s_lock;
  geo::MutexLocker locker (&s_lock);
  ++m_progress;

  geo::RelativeProgress *rp = dynamic_cast<geo::RelativeProgress *> (mp_progress);
  if (rp) {
    rp->set (m_progress);
  }
}

template <class TS, class TI, class TR>
size_t local_processor<TS, TI, TR>::get_progress () const
{
  size_t p = 0;
  {
    static geo::Mutex s_lock;
    geo::MutexLocker locker (&s_lock);
    p = m_progress;
  }
  return p;
}

template <class TS, class TI, class TR>
void local_processor<TS, TI, TR>::run (local_operation<TS, TI, TR> *op, unsigned int subject_layer, unsigned int intruder_layer, unsigned int output_layer)
{
  std::vector<unsigned int> ol, il;
  ol.push_back (output_layer);
  il.push_back (intruder_layer);
  run (op, subject_layer, il, ol);
}

template <class TS, class TI, class TR>
void local_processor<TS, TI, TR>::run (local_operation<TS, TI, TR> *op, unsigned int subject_layer, unsigned int intruder_layer, const std::vector<unsigned int> &output_layers)
{
  std::vector<unsigned int> ol, il;
  il.push_back (intruder_layer);
  run (op, subject_layer, il, output_layers);
}

template <class TS, class TI, class TR>
void local_processor<TS, TI, TR>::run (local_operation<TS, TI, TR> *op, unsigned int subject_layer, const std::vector<unsigned int> &intruder_layers, unsigned int output_layer)
{
  std::vector<unsigned int> ol;
  ol.push_back (output_layer);
  run (op, subject_layer, intruder_layers, ol);
}

template <class TS, class TI, class TR>
void local_processor<TS, TI, TR>::run (local_operation<TS, TI, TR> *op, unsigned int subject_layer, const std::vector<unsigned int> &intruder_layers, const std::vector<unsigned int> &output_layers)
{
  geo::SelfTimer timer (geo::verbosity () > m_base_verbosity, geo::to_string (tr ("Executing ")) + description (op));

  local_processor_contexts<TS, TI, TR> contexts;
  compute_contexts (contexts, op, subject_layer, intruder_layers);
  compute_results (contexts, op, output_layers);
}

template <class TS, class TI, class TR>
void local_processor<TS, TI, TR>::push_results (geo::Cell *cell, unsigned int output_layer, const std::unordered_set<TR> &result) const
{
  if (! result.empty ()) {
    geo::MutexLocker locker (&cell->layout ()->lock ());
    cell->shapes (output_layer).insert (result.begin (), result.end ());
  }
}

template <class TS, class TI, class TR>
void local_processor<TS, TI, TR>::compute_contexts (local_processor_contexts<TS, TI, TR> &contexts, const local_operation<TS, TI, TR> *op, unsigned int subject_layer, const std::vector<unsigned int> &intruder_layers) const
{
  try {

    geo::SelfTimer timer (geo::verbosity () > m_base_verbosity + 10, geo::to_string (tr ("Computing contexts for ")) + description (op));

    if (m_nthreads > 0) {
      mp_cc_job.reset (new geo::Job<local_processor_context_computation_worker<TS, TI, TR> > (m_nthreads));
    } else {
      mp_cc_job.reset (0);
    }

    contexts.clear ();
    contexts.set_intruder_layers (intruder_layers);
    contexts.set_subject_layer (subject_layer);

    typename local_processor_cell_contexts<TS, TI, TR>::context_key_type intruders;
    issue_compute_contexts (contexts, 0, 0, mp_subject_top, geo::ICplxTrans (), mp_intruder_top, intruders, op->dist ());

    if (mp_cc_job.get ()) {
      mp_cc_job->start ();
      mp_cc_job->wait ();
    }

  } catch (...) {
    mp_cc_job.reset (0);
    throw;
  }
}

template <class TS, class TI, class TR>
void local_processor<TS, TI, TR>::issue_compute_contexts (local_processor_contexts<TS, TI, TR> &contexts,
                                                 geo::local_processor_cell_context<TS, TI, TR> *parent_context,
                                                 geo::Cell *subject_parent,
                                                 geo::Cell *subject_cell,
                                                 const geo::ICplxTrans &subject_cell_inst,
                                                 const geo::Cell *intruder_cell,
                                                 typename local_processor_cell_contexts<TS, TI, TR>::context_key_type &intruders,
                                                 geo::Coord dist) const
{
  bool is_small_job = subject_cell->begin ().at_end ();

  if (! is_small_job && mp_cc_job.get ()) {
    mp_cc_job->schedule (new local_processor_context_computation_task<TS, TI, TR> (this, contexts, parent_context, subject_parent, subject_cell, subject_cell_inst, intruder_cell, intruders, dist));
  } else {
    compute_contexts (contexts, parent_context, subject_parent, subject_cell, subject_cell_inst, intruder_cell, intruders, dist);
  }
}

template <class TS, class TI, class TR>
void local_processor<TS, TI, TR>::compute_contexts (local_processor_contexts<TS, TI, TR> &contexts,
                                                    geo::local_processor_cell_context<TS, TI, TR> *parent_context,
                                                    geo::Cell *subject_parent,
                                                    geo::Cell *subject_cell,
                                                    const geo::ICplxTrans &subject_cell_inst,
                                                    const geo::Cell *intruder_cell,
                                                    const typename local_processor_cell_contexts<TS, TI, TR>::context_key_type &intruders,
                                                    geo::Coord dist) const
{
  CRONOLOGY_COLLECTION_BRACKET(event_compute_contexts)

  if (geo::verbosity () >= m_base_verbosity + 20) {
    if (! subject_parent) {
      geo::log << tr ("Computing context for top cell ") << mp_subject_layout->cell_name (subject_cell->cell_index ());
    } else {
      geo::log << tr ("Computing context for ") << mp_subject_layout->cell_name (subject_parent->cell_index ()) << " -> " << mp_subject_layout->cell_name (subject_cell->cell_index ()) << " @" << subject_cell_inst.to_string ();
    }
  }

  geo::local_processor_cell_context<TS, TI, TR> *cell_context = 0;

  //  prepare a new cell context: this has to happen in a thread-safe way as we share the contexts
  //  object between threads

  {
    geo::MutexLocker locker (& contexts.lock ());

    geo::local_processor_cell_contexts<TS, TI, TR> &cell_contexts = contexts.contexts_per_cell (subject_cell, intruder_cell);

#if defined(ENABLE_DB_HP_SANITY_ASSERTIONS)
    if (subject_parent) {
      typename geo::local_processor_cell_contexts<TS, TI, TR>::contexts_per_cell_type::iterator pcc = contexts.context_map ().find (subject_parent);
      if (pcc == contexts.context_map ().end ()) {
        tl_assert (false);
      }
      tl_assert (pcc->first == subject_parent);
      bool any = false;
      for (typename geo::local_processor_cell_contexts<TS, TI, TR>::iterator pcci = pcc->second.begin (); pcci != pcc->second.end () && !any; ++pcci) {
        any = (&pcci->second == parent_context);
      }
      if (!any) {
        tl_assert (false);
      }
    }
 #endif

    cell_context = cell_contexts.find_context (intruders);
    if (cell_context) {
      //  we already have a context for this intruder scheme
      cell_context->add (parent_context, subject_parent, subject_cell_inst);
      return;
    }

    cell_context = cell_contexts.create (intruders);
    cell_context->add (parent_context, subject_parent, subject_cell_inst);
  }

  //  perform the actual task ..

  CRONOLOGY_COLLECTION_BRACKET(event_compute_contexts_unlocked)
  std::map<unsigned int, const geo::Shapes *> intruder_shapes;
  if (intruder_cell) {
    for (std::vector<unsigned int>::const_iterator l = contexts.intruder_layers ().begin (); l != contexts.intruder_layers ().end (); ++l) {
      const geo::Shapes *s = &intruder_cell->shapes (contexts.actual_intruder_layer (*l));
      if (! s->empty ()) {
        intruder_shapes.insert (std::make_pair (*l, s));
      }
    }
  }

  geo::box_convert <geo::CellInstArray, true> inst_bcs (*mp_subject_layout, contexts.subject_layer ());

  //  handle top-down interactions (subject instances interacting with intruder shapes)
  //  and sibling interactions

  if (! subject_cell->begin ().at_end ()) {

    typedef std::pair<std::unordered_set<const geo::CellInstArray *>, std::map<unsigned int, std::unordered_set<TI> > > interaction_value_type;

    std::unordered_map<const geo::CellInstArray *, interaction_value_type> interactions;

    //  insert dummy interactions to handle at least the child cell vs. itself
    //  - this is important so we will always handle the instances unless they are
    //  entirely empty in the subject layer
    for (geo::Cell::const_iterator i = subject_cell->begin (); !i.at_end (); ++i) {
      if (! inst_bcs (i->cell_inst ()).empty ()) {
        interactions.insert (std::make_pair (&i->cell_inst (), interaction_value_type ()));
      }
    }

//  TODO: can we shortcut this if interactions is empty?
    for (std::vector<unsigned int>::const_iterator il = contexts.intruder_layers ().begin (); il != contexts.intruder_layers ().end (); ++il) {

      geo::box_convert <geo::CellInstArray, true> inst_bci (*mp_intruder_layout, contexts.actual_intruder_layer (*il));

      geo::box_scanner2<geo::CellInstArray, int, geo::CellInstArray, int> scanner;
      interaction_registration_inst2inst<TI> rec (mp_subject_layout, contexts.subject_layer (), mp_intruder_layout, contexts.actual_intruder_layer (*il), contexts.is_foreign (*il), dist, &interactions);

      unsigned int id = 0;

      if (subject_cell == intruder_cell) {

        //  Use the same id's for same instances - this way we can easily detect same instances
        //  and don't make them self-interacting

        for (geo::Cell::const_iterator i = subject_cell->begin (); !i.at_end (); ++i) {
          unsigned int iid = ++id;
          if (! inst_bcs (i->cell_inst ()).empty () && ! subject_cell_is_breakout (i->cell_index ())) {
            scanner.insert1 (&i->cell_inst (), iid);
          }
          if (! inst_bci (i->cell_inst ()).empty () && ! intruder_cell_is_breakout (i->cell_index ())) {
            scanner.insert2 (&i->cell_inst (), iid);
          }
        }

      } else {

        for (geo::Cell::const_iterator i = subject_cell->begin (); !i.at_end (); ++i) {
          if (! inst_bcs (i->cell_inst ()).empty () && ! subject_cell_is_breakout (i->cell_index ())) {
            scanner.insert1 (&i->cell_inst (), ++id);
          }
        }

        if (intruder_cell) {
          for (geo::Cell::const_iterator i = intruder_cell->begin (); !i.at_end (); ++i) {
            if (! inst_bci (i->cell_inst ()).empty () && ! intruder_cell_is_breakout (i->cell_index ())) {
              scanner.insert2 (&i->cell_inst (), ++id);
            }
          }
        }

      }

      for (std::set<geo::CellInstArray>::const_iterator i = intruders.first.begin (); i != intruders.first.end (); ++i) {
        if (! inst_bci (*i).empty ()) {
          scanner.insert2 (i.operator-> (), ++id);
        }
      }

      scanner.process (rec, dist, inst_bcs, inst_bci);

    }

//  TODO: can we shortcut this if interactions is empty?
    {
      geo::box_scanner2<geo::CellInstArray, int, TI, int> scanner;
      interaction_registration_inst2shape<TI> rec (mp_subject_layout, contexts.subject_layer (), dist, &interactions);

      for (geo::Cell::const_iterator i = subject_cell->begin (); !i.at_end (); ++i) {
        if (! inst_bcs (i->cell_inst ()).empty () && ! subject_cell_is_breakout (i->cell_index ())) {
          scanner.insert1 (&i->cell_inst (), 0);
        }
      }

      for (typename std::map<unsigned int, std::set<TI> >::const_iterator il = intruders.second.begin (); il != intruders.second.end (); ++il) {
        for (typename std::set<TI>::const_iterator i = il->second.begin (); i != il->second.end (); ++i) {
          scanner.insert2 (i.operator-> (), il->first);
        }
      }

      for (std::map<unsigned int, const geo::Shapes *>::const_iterator im = intruder_shapes.begin (); im != intruder_shapes.end (); ++im) {
        for (geo::Shapes::shape_iterator i = im->second->begin (shape_flags<TI> ()); !i.at_end (); ++i) {
          scanner.insert2 (i->basic_ptr (typename TI::tag ()), im->first);
        }
      }

      scanner.process (rec, dist, inst_bcs, geo::box_convert<TI> ());
    }

    //  this cache should reduce the effort of checking array vs. array
    typedef std::pair<unsigned int, std::pair<geo::cell_index_type, geo::ICplxTrans> > effective_instance_cache_key_type;
    typedef std::map<effective_instance_cache_key_type, std::pair<bool, geo::CellInstArray> > effective_instance_cache_type;
    effective_instance_cache_type effective_instance_cache;

    for (typename std::unordered_map<const geo::CellInstArray *, interaction_value_type>::const_iterator i = interactions.begin (); i != interactions.end (); ++i) {

      geo::Cell &subject_child_cell = mp_subject_layout->cell (i->first->object ().cell_index ());

      for (geo::CellInstArray::iterator n = i->first->begin (); ! n.at_end (); ++n) {

        geo::ICplxTrans tn = i->first->complex_trans (*n);
        geo::ICplxTrans tni = tn.inverted ();
        geo::Box nbox = tn * subject_child_cell.bbox (contexts.subject_layer ()).enlarged (geo::Vector (dist, dist));

        if (! nbox.empty ()) {

          typename local_processor_cell_contexts<TS, TI, TR>::context_key_type intruders_below;

          geo::shape_reference_translator_with_trans<TI, geo::ICplxTrans> rt (mp_subject_layout, tni);

          for (typename std::map<unsigned int, std::unordered_set<TI> >::const_iterator pl = i->second.second.begin (); pl != i->second.second.end (); ++pl) {
            std::set<TI> &out = intruders_below.second [pl->first];
            for (typename std::unordered_set<TI>::const_iterator p = pl->second.begin (); p != pl->second.end (); ++p) {
              if (nbox.overlaps (geo::box_convert<TI> () (*p))) {
                out.insert (rt (*p));
              }
            }
          }

          //  TODO: in some cases, it may be possible to optimize this for arrays

          for (std::vector<unsigned int>::const_iterator il = contexts.intruder_layers ().begin (); il != contexts.intruder_layers ().end (); ++il) {

            unsigned int ail = contexts.actual_intruder_layer (*il);
            geo::box_convert <geo::CellInst, true> inst_bcii (*mp_intruder_layout, ail);

            for (std::unordered_set<const geo::CellInstArray *>::const_iterator j = i->second.first.begin (); j != i->second.first.end (); ++j) {

              for (geo::CellInstArray::iterator k = (*j)->begin_touching (safe_box_enlarged (nbox, -1, -1), inst_bcii); ! k.at_end (); ++k) {

                geo::ICplxTrans tk = (*j)->complex_trans (*k);
                //  NOTE: no self-interactions
                if (i->first != *j || tn != tk) {

                  //  optimize the intruder instance so it will be as low as possible
                  effective_instance_cache_key_type key (ail, std::make_pair ((*j)->object ().cell_index (), tni * tk));
                  effective_instance_cache_type::iterator cached = effective_instance_cache.find (key);
                  if (cached == effective_instance_cache.end ()) {
                    std::pair<bool, geo::CellInstArray> ei = effective_instance (contexts.subject_layer (), i->first->object ().cell_index (), ail, (*j)->object ().cell_index (), tni * tk, dist);
                    cached = effective_instance_cache.insert (std::make_pair (key, ei)).first;
                  }
                  if (cached->second.first) {
                    intruders_below.first.insert (cached->second.second);
                  }

                }

              }

            }

          }

          geo::Cell *intruder_child_cell = (subject_cell == intruder_cell ? &subject_child_cell : 0);
          issue_compute_contexts (contexts, cell_context, subject_cell, &subject_child_cell, tn, intruder_child_cell, intruders_below, dist);

        }

      }

    }

  }

}

/**
 *  @brief Returns a cell instance array suitable for adding as intruder
 *
 *  The given intruder cell with the transformation ti2s - which transforms the intruder instance into
 *  the coordinate system of the subject cell - is analysed and either this instance or a sub-instance
 *  is chosen.
 *  Sub-instances are chosen if the intruder cell does not have shapes which interact with the subject
 *  cell and there is exactly one sub-instance interacting with the subject cell.
 */
template <class TS, class TI, class TR>
std::pair<bool, geo::CellInstArray>
local_processor<TS, TI, TR>::effective_instance (unsigned int subject_layer, geo::cell_index_type subject_cell_index, unsigned int intruder_layer, geo::cell_index_type intruder_cell_index, const geo::ICplxTrans &ti2s, geo::Coord dist) const
{
  geo::Box bbox = safe_box_enlarged (mp_subject_layout->cell (subject_cell_index).bbox (subject_layer), dist - 1, dist - 1);
  if (bbox.empty ()) {
    //  should not happen, but skip if it does
    return std::make_pair (false, geo::CellInstArray ());
  }

  geo::Box ibbox = bbox.transformed (ti2s.inverted ());

  const geo::Cell &intruder_cell = mp_intruder_layout->cell (intruder_cell_index);
  const geo::Shapes &intruder_shapes = intruder_cell.shapes (intruder_layer);
  if (! intruder_shapes.empty () && ! intruder_shapes.begin_touching (ibbox, geo::ShapeIterator::All).at_end ()) {
    return std::make_pair (true, geo::CellInstArray (geo::CellInst (intruder_cell_index), ti2s));
  }

  geo::box_convert <geo::CellInst, true> inst_bcii (*mp_intruder_layout, intruder_layer);

  size_t ni = 0;
  geo::cell_index_type eff_cell_index = 0;
  geo::ICplxTrans eff_trans;

  for (geo::Cell::touching_iterator i = intruder_cell.begin_touching (ibbox); ! i.at_end() && ni < 2; ++i) {
    const geo::CellInstArray &ci = i->cell_inst ();
    geo::Box cbox = mp_intruder_layout->cell (ci.object ().cell_index ()).bbox (intruder_layer);
    for (geo::CellInstArray::iterator k = ci.begin_touching (ibbox, inst_bcii); ! k.at_end () && ni < 2; ++k) {
      geo::ICplxTrans tk = ci.complex_trans (*k);
      if (ibbox.overlaps (cbox.transformed (tk))) {
        eff_trans = tk;
        eff_cell_index = ci.object ().cell_index ();
        ++ni;
      }
    }
  }

  if (ni == 0) {
    //  should not happen, but skip if it does
    return std::make_pair (false, geo::CellInstArray ());
  } else if (ni == 1) {
    //  one instance - dive down
    return effective_instance (subject_layer, subject_cell_index, intruder_layer, eff_cell_index, ti2s * eff_trans, dist);
  } else {
    return std::make_pair (true, geo::CellInstArray (geo::CellInst (intruder_cell_index), ti2s));
  }
}

template <class TS, class TI, class TR>
void
local_processor<TS, TI, TR>::compute_results (local_processor_contexts<TS, TI, TR> &contexts, const local_operation<TS, TI, TR> *op, const std::vector<unsigned int> &output_layers) const
{
  geo::SelfTimer timer (geo::verbosity () > m_base_verbosity + 10, geo::to_string (tr ("Computing results for ")) + description (op));

  //  avoids updates while we work on the layout
  mp_subject_layout->update ();
  geo::LayoutLocker layout_update_locker (mp_subject_layout);

  //  prepare a progress for the computation tasks
  size_t comp_effort = 0;
  if (m_report_progress) {
    for (typename local_processor_contexts<TS, TI, TR>::iterator c = contexts.begin (); c != contexts.end (); ++c) {
      comp_effort += c->second.size ();
    }
  }

  geo::RelativeProgress progress (description (op), comp_effort, 1);
  m_progress = 0;
  mp_progress = 0;

  if (m_nthreads > 0) {

    std::unique_ptr<geo::Job<local_processor_result_computation_worker<TS, TI, TR> > > rc_job (new geo::Job<local_processor_result_computation_worker<TS, TI, TR> > (m_nthreads));

    //  schedule computation jobs in "waves": we need to make sure they are executed
    //  bottom-up. So we identify a new bunch of cells each time we pass through the cell set
    //  and proceed until all cells are removed.

    std::vector<geo::cell_index_type> cells_bu;
    cells_bu.reserve (mp_subject_layout->cells ());
    for (geo::Layout::bottom_up_const_iterator bu = mp_subject_layout->begin_bottom_up (); bu != mp_subject_layout->end_bottom_up (); ++bu) {
      cells_bu.push_back (*bu);
    }

    int iter = 0;
    while (true) {

      ++iter;
      geo::SelfTimer timer (geo::verbosity () > m_base_verbosity + 10, geo::sprintf (geo::to_string (tr ("Computing results iteration #%d")), iter));

      bool any = false;
      std::unordered_set<geo::cell_index_type> later;

      std::vector<geo::cell_index_type> next_cells_bu;
      next_cells_bu.reserve (cells_bu.size ());

      for (std::vector<geo::cell_index_type>::const_iterator bu = cells_bu.begin (); bu != cells_bu.end (); ++bu) {

        typename local_processor_contexts<TS, TI, TR>::iterator cpc = contexts.context_map ().find (&mp_subject_layout->cell (*bu));
        if (cpc != contexts.context_map ().end ()) {

          if (later.find (*bu) == later.end ()) {

            rc_job->schedule (new local_processor_result_computation_task<TS, TI, TR> (this, contexts, cpc->first, &cpc->second, op, output_layers));
            any = true;

          } else {
            next_cells_bu.push_back (*bu);
          }

          for (geo::Cell::parent_cell_iterator pc = cpc->first->begin_parent_cells (); pc != cpc->first->end_parent_cells (); ++pc) {
            later.insert (*pc);
          }

        }

      }

      cells_bu.swap (next_cells_bu);

      if (! any) {
        break;
      }

      if (rc_job.get ()) {

        try {

          rc_job->start ();
          while (! rc_job->wait (10)) {
            progress.set (get_progress ());
          }

        } catch (...) {
          rc_job->terminate ();
          throw;
        }

      }

    }

  } else {

    try {

      mp_progress = m_report_progress ? &progress : 0;

      for (geo::Layout::bottom_up_const_iterator bu = mp_subject_layout->begin_bottom_up (); bu != mp_subject_layout->end_bottom_up (); ++bu) {

        typename local_processor_contexts<TS, TI, TR>::iterator cpc = contexts.context_map ().find (&mp_subject_layout->cell (*bu));
        if (cpc != contexts.context_map ().end ()) {
          cpc->second.compute_results (contexts, cpc->first, op, output_layers, this);
          contexts.context_map ().erase (cpc);
        }

      }

      mp_progress = 0;

    } catch (...) {
      mp_progress = 0;
      throw;
    }

  }
}

namespace {

template <class TS, class TI>
struct scan_shape2shape_same_layer
{
  void
  operator () (const geo::Shapes *subject_shapes, unsigned int subject_id0, const std::set<TI> &intruders, unsigned int intruder_layer_index, shape_interactions<TS, TI> &interactions, geo::Coord dist) const
  {
    geo::box_scanner2<TS, int, TI, int> scanner;
    interaction_registration_shape1<TS, TI> rec (&interactions, intruder_layer_index);

    unsigned int id = subject_id0;
    for (geo::Shapes::shape_iterator i = subject_shapes->begin (shape_flags<TS> ()); !i.at_end (); ++i) {
      const TS *ref = i->basic_ptr (typename TS::tag ());
      scanner.insert1 (ref, id++);
    }

    //  TODO: can we confine this search to the subject's (sized) bounding box?
    for (typename std::set<TI>::const_iterator i = intruders.begin (); i != intruders.end (); ++i) {
      scanner.insert2 (i.operator-> (), interactions.next_id ());
    }

    scanner.process (rec, dist, geo::box_convert<TS> (), geo::box_convert<TI> ());
  }
};

template <class T>
struct scan_shape2shape_same_layer<T, T>
{
  void
  operator () (const geo::Shapes *subject_shapes, unsigned int subject_id0, const std::set<T> &intruders, unsigned int intruder_layer, shape_interactions<T, T> &interactions, geo::Coord dist) const
  {
    geo::box_scanner<T, int> scanner;
    interaction_registration_shape1<T, T> rec (&interactions, intruder_layer);

    unsigned int id = subject_id0;
    for (geo::Shapes::shape_iterator i = subject_shapes->begin (shape_flags<T> ()); !i.at_end (); ++i) {
      const T *ref = i->basic_ptr (typename T::tag ());
      scanner.insert (ref, id++);
    }

    //  TODO: can we confine this search to the subject's (sized) bounding box?
    for (typename std::set<T>::const_iterator i = intruders.begin (); i != intruders.end (); ++i) {
      scanner.insert (i.operator-> (), interactions.next_id ());
    }

    scanner.process (rec, dist, geo::box_convert<T> ());
  }
};

template <class TS, class TI>
struct scan_shape2shape_different_layers
{
  void
  operator () (geo::Layout *layout, const geo::Shapes *subject_shapes, const geo::Shapes *intruder_shapes, unsigned int subject_id0, const std::set<TI> *intruders, unsigned int intruder_layer_index, shape_interactions<TS, TI> &interactions, geo::Coord dist) const
  {
    geo::box_scanner2<TS, int, TI, int> scanner;
    interaction_registration_shape2shape<TS, TI> rec (layout, &interactions, intruder_layer_index);

    unsigned int id = subject_id0;
    for (geo::Shapes::shape_iterator i = subject_shapes->begin (shape_flags<TS> ()); !i.at_end (); ++i, ++id) {
      const TS *ref = i->basic_ptr (typename TS::tag ());
      scanner.insert1 (ref, id);
    }

    //  TODO: can we confine this search to the subject's (sized) bounding box?
    if (intruders) {
      for (typename std::set<TI>::const_iterator i = intruders->begin (); i != intruders->end (); ++i) {
        scanner.insert2 (i.operator-> (), interactions.next_id ());
      }
    }

    if (intruder_shapes == subject_shapes) {

      //  TODO: can we confine this search to the subject's (sized) bounding box?

      //  special case of intra-layer interactions ("foreign"): mark identical shapes as same so that shapes are not reported interacting with
      //  themselves.
      unsigned int id = subject_id0;
      for (geo::Shapes::shape_iterator i = intruder_shapes->begin (shape_flags<TI> ()); !i.at_end (); ++i, ++id) {
        unsigned int iid = interactions.next_id ();
        scanner.insert2 (i->basic_ptr (typename TI::tag ()), iid);
        rec.same (id, iid);
      }

    } else if (intruder_shapes) {

      //  TODO: can we confine this search to the subject's (sized) bounding box?
      for (geo::Shapes::shape_iterator i = intruder_shapes->begin (shape_flags<TI> ()); !i.at_end (); ++i) {
        scanner.insert2 (i->basic_ptr (typename TI::tag ()), interactions.next_id ());
      }

    }

    scanner.process (rec, dist, geo::box_convert<TS> (), geo::box_convert<TI> ());
  }
};

}

template <class TS, class TI, class TR>
void
local_processor<TS, TI, TR>::compute_local_cell (const geo::local_processor_contexts<TS, TI, TR> &contexts, geo::Cell *subject_cell, const geo::Cell *intruder_cell, const local_operation<TS, TI, TR> *op, const typename local_processor_cell_contexts<TS, TI, TR>::context_key_type &intruders, std::vector<std::unordered_set<TR> > &result) const
{
  const geo::Shapes *subject_shapes = &subject_cell->shapes (contexts.subject_layer ());

  shape_interactions<TS, TI> interactions;

  //  insert dummy interactions to accommodate subject vs. nothing and assign an ID
  //  range for the subject shapes.
  unsigned int subject_id0 = 0;
  for (geo::Shapes::shape_iterator i = subject_shapes->begin (shape_flags<TS> ()); !i.at_end (); ++i) {

    unsigned int id = interactions.next_id ();
    if (subject_id0 == 0) {
      subject_id0 = id;
    }

    if (op->on_empty_intruder_hint () != OnEmptyIntruderHint::Drop) {
      const TS *ref = i->basic_ptr (typename TS::tag ());
      interactions.add_subject (id, *ref);
    }

  }

  unsigned int il_index = 0;
  for (std::vector<unsigned int>::const_iterator il = contexts.intruder_layers ().begin (); il != contexts.intruder_layers ().end (); ++il, ++il_index) {

    unsigned int ail = contexts.actual_intruder_layer (*il);
    bool foreign = contexts.is_foreign (*il);

    const geo::Shapes *intruder_shapes = 0;
    if (intruder_cell) {
      intruder_shapes = &intruder_cell->shapes (ail);
      if (intruder_shapes->empty ()) {
        intruder_shapes = 0;
      }
    }

    //  local shapes vs. child cell

    geo::box_convert<geo::CellInstArray, true> inst_bci (*mp_intruder_layout, ail);

    typename std::map<unsigned int, std::set<TI> >::const_iterator ipl = intruders.second.find (*il);
    static std::set<TI> empty_intruders;

    if (! subject_shapes->empty () && (intruder_shapes || ipl != intruders.second.end ())) {

      if (subject_cell == intruder_cell && contexts.subject_layer () == ail && !foreign) {

        scan_shape2shape_same_layer<TS, TI> () (subject_shapes, subject_id0, ipl == intruders.second.end () ? empty_intruders : ipl->second, il_index, interactions, op->dist ());

      } else {

        geo::Layout *target_layout = (mp_subject_layout == mp_intruder_layout ? 0 : mp_subject_layout);
        scan_shape2shape_different_layers<TS, TI> () (target_layout, subject_shapes, intruder_shapes, subject_id0, &(ipl == intruders.second.end () ? empty_intruders : ipl->second), il_index, interactions, op->dist ());

      }

    }

    if (! subject_shapes->empty () && ! ((! intruder_cell || intruder_cell->begin ().at_end ()) && intruders.first.empty ())) {

      geo::box_scanner2<TS, int, geo::CellInstArray, int> scanner;
      interaction_registration_shape2inst<TS, TI> rec (mp_subject_layout, mp_intruder_layout, ail, il_index, op->dist (), &interactions);

      unsigned int id = subject_id0;
      for (geo::Shapes::shape_iterator i = subject_shapes->begin (shape_flags<TS> ()); !i.at_end (); ++i) {
        scanner.insert1 (i->basic_ptr (typename TS::tag ()), id++);
      }

      unsigned int inst_id = 0;

      if (subject_cell == intruder_cell && contexts.subject_layer () == ail && !foreign) {

        //  Same cell, same layer -> no shape to child instance interactions because this will be taken care of
        //  by the instances themselves (and their intruders). This also means, we prefer to deal with
        //  interactions low in the hierarchy.

      } else if (intruder_cell) {
  //  TODO: can we confine this search to the subject's (sized) bounding box?
        for (geo::Cell::const_iterator i = intruder_cell->begin (); !i.at_end (); ++i) {
          if (! inst_bci (i->cell_inst ()).empty () && ! intruder_cell_is_breakout (i->cell_index ())) {
            scanner.insert2 (&i->cell_inst (), ++inst_id);
          }
        }
      }

  //  TODO: can we confine this search to the subject's (sized) bounding box?
      for (std::set<geo::CellInstArray>::const_iterator i = intruders.first.begin (); i != intruders.first.end (); ++i) {
        if (! inst_bci (*i).empty ()) {
          scanner.insert2 (i.operator-> (), ++inst_id);
        }
      }

      scanner.process (rec, op->dist (), geo::box_convert<TS> (), inst_bci);

    }

  }

  if (interactions.begin () != interactions.end ()) {

    if (interactions.begin_intruders () == interactions.end_intruders ()) {

      OnEmptyIntruderHint eh = op->on_empty_intruder_hint ();
      if (eh == OnEmptyIntruderHint::Drop) {
        return;
      }

    }

    op->compute_local (mp_subject_layout, interactions, result, m_max_vertex_count, m_area_ratio);

  }
}

template <class TS, class TI, class TR>
void
local_processor<TS, TI, TR>::run_flat (const geo::Shapes *subject_shapes, const geo::Shapes *intruders, const local_operation<TS, TI, TR> *op, geo::Shapes *result_shapes) const
{
  std::vector<generic_shape_iterator<TI> > is;
  std::vector<bool> foreign;
  if (intruders == subject_idptr () || intruders == foreign_idptr ()) {
    is.push_back (generic_shape_iterator<TI> (subject_shapes));
    foreign.push_back (intruders == foreign_idptr ());
  } else {
    is.push_back (generic_shape_iterator<TI> (intruders));
    foreign.push_back (false);
  }

  std::vector<geo::Shapes *> os;
  os.push_back (result_shapes);

  run_flat (generic_shape_iterator<TS> (subject_shapes), is, foreign, op, os);
}

template <class TS, class TI, class TR>
void
local_processor<TS, TI, TR>::run_flat (const geo::Shapes *subject_shapes, const std::vector<const geo::Shapes *> &intruders, const local_operation<TS, TI, TR> *op, const std::vector<geo::Shapes *> &result_shapes) const
{
  std::vector<generic_shape_iterator<TI> > is;
  is.reserve (intruders.size ());

  std::vector<bool> foreign;
  foreign.reserve (intruders.size ());

  for (std::vector<const geo::Shapes *>::const_iterator i = intruders.begin (); i != intruders.end (); ++i) {
    if (*i == subject_idptr () || *i == foreign_idptr ()) {
      is.push_back (generic_shape_iterator<TI> (subject_shapes));
      foreign.push_back (*i == foreign_idptr ());
    } else {
      is.push_back (generic_shape_iterator<TI> (*i));
      foreign.push_back (false);
    }
  }

  run_flat (generic_shape_iterator<TS> (subject_shapes), is, foreign, op, result_shapes);
}

namespace
{

template <class TS, class TI>
struct interaction_registration_shape1_scanner_combo
{
  interaction_registration_shape1_scanner_combo (shape_interactions<TS, TI> *, unsigned int, bool, const std::string &)
  {
    //  can't have self-interactions with different types
    tl_assert (false);
  }

  void insert (const TS *, unsigned int)
  {
    //  nothing here.
  }

  void process (geo::Coord)
  {
    //  nothing here.
  }
};

template <class T>
struct interaction_registration_shape1_scanner_combo<T, T>
{
  interaction_registration_shape1_scanner_combo (shape_interactions<T, T> *interactions, unsigned int intruder_layer_index, bool report_progress, const std::string &progress_description)
    : m_scanner (report_progress, progress_description), m_rec (interactions, intruder_layer_index)
  {
    //  .. nothing yet ..
  }

  void insert (const T *shape, unsigned int id)
  {
    m_scanner.insert (shape, id);
  }

  void process (geo::Coord dist)
  {
    m_scanner.process (m_rec, dist, geo::box_convert<T> ());
  }

private:
  geo::box_scanner<T, int> m_scanner;
  interaction_registration_shape1<T, T> m_rec;
};

}

template <class TS, class TI, class TR>
void
local_processor<TS, TI, TR>::run_flat (const generic_shape_iterator<TS> &subjects, const std::vector<generic_shape_iterator<TI> > &intruders, const std::vector<bool> &foreign, const local_operation<TS, TI, TR> *op, const std::vector<geo::Shapes *> &result_shapes) const
{
  if (subjects.at_end ()) {
    return;
  }

  tl_assert (mp_subject_top == 0);
  tl_assert (mp_intruder_top == 0);

  std::string process_description, scan_description;

  if (m_report_progress) {

    process_description = description (op);
    if (process_description.empty ()) {
      process_description = geo::to_string (tr ("Processing"));
    } else {
      process_description += geo::to_string (tr (" (processing)"));
    }

    scan_description = description (op);
    if (scan_description.empty ()) {
      scan_description = geo::to_string (tr ("Scanning"));
    } else {
      scan_description += geo::to_string (tr (" (scan)"));
    }

  }

  shape_interactions<TS, TI> interactions;

  bool needs_isolated_subjects = (op->on_empty_intruder_hint () != OnEmptyIntruderHint::Drop);

  //  build the subjects in the intruders list

  geo::Coord dist = op->dist ();

  geo::Box subjects_box = subjects.bbox ();
  if (subjects_box != geo::Box::world ()) {
    subjects_box.enlarge (geo::Vector (dist, dist));
  }

  geo::Box intruders_box;
  for (typename std::vector<generic_shape_iterator<TI> >::const_iterator il = intruders.begin (); il != intruders.end (); ++il) {
    intruders_box += il->bbox ();
  }
  if (intruders_box != geo::Box::world ()) {
    intruders_box.enlarge (geo::Vector (dist, dist));
  }

  geo::Box common_box = intruders_box & subjects_box;
  if (common_box.empty ()) {

    if (needs_isolated_subjects) {
      for (generic_shape_iterator<TS> is = subjects; ! is.at_end (); ++is) {
        //  create subject for subject vs. nothing interactions
        interactions.add_subject (interactions.next_id (), *is);
      }
    }

  } else {

    if (needs_isolated_subjects) {

      addressable_shape_delivery<TS> is (subjects);
      for ( ; !is.at_end (); ++is) {
        const TS *shape = is.operator-> ();
        unsigned int id = interactions.next_id ();
        interactions.add_subject (id, *shape);
      }

      unsigned int il_index = 0;
      for (typename std::vector<generic_shape_iterator<TI> >::const_iterator il = intruders.begin (); il != intruders.end (); ++il, ++il_index) {

        bool ff = foreign.size () > il_index && foreign [il_index];

        if (*il == subjects && ! ff) {

          interaction_registration_shape1_scanner_combo<TS, TI> scanner (&interactions, il_index, m_report_progress, scan_description);

          for (typename shape_interactions<TS, TI>::subject_iterator s = interactions.begin_subjects (); s != interactions.end_subjects (); ++s) {
            scanner.insert (&s->second, s->first);
          }

          scanner.process (dist);

        } else {

          geo::box_scanner2<TS, unsigned int, TI, unsigned int> scanner (m_report_progress, scan_description);
          interaction_registration_shape2shape<TS, TI> rec (0 /*layout*/, &interactions, il_index);

          for (typename shape_interactions<TS, TI>::subject_iterator s = interactions.begin_subjects (); s != interactions.end_subjects (); ++s) {
            scanner.insert1 (&s->second, s->first);
          }

          if (*il == subjects) {

            //  this is the case of intra-layer interactions ("foreign"): we pretend we have two layers and
            //  reject shape self-interactions by registering them as "same"

            for (typename shape_interactions<TS, TI>::subject_iterator s = interactions.begin_subjects (); s != interactions.end_subjects (); ++s) {
              unsigned int iid = interactions.next_id ();
              safe_insert2_into_box_scanner (scanner, &s->second, iid);
              rec.same (s->first, iid);
            }

            scanner.process (rec, dist, geo::box_convert<TS> (), geo::box_convert<TI> ());

          } else {

            addressable_shape_delivery<TI> ii ((*il).confined (common_box, true));
            for (; !ii.at_end (); ++ii) {
              scanner.insert2 (ii.operator-> (), interactions.next_id ());
            }

            scanner.process (rec, dist, geo::box_convert<TS> (), geo::box_convert<TI> ());

          }

        }
      }

    } else {

      unsigned int id_first = 0;

      {
        //  allocate a range of IDs for the subjects
        generic_shape_iterator<TS> is (subjects);
        if (! is.at_end ()) {
          id_first = interactions.next_id ();
          while (! (++is).at_end ()) {
            interactions.next_id ();
          }
        }
      }

      unsigned int il_index = 0;
      for (typename std::vector<generic_shape_iterator<TI> >::const_iterator il = intruders.begin (); il != intruders.end (); ++il, ++il_index) {

        bool ff = foreign.size () > il_index && foreign [il_index];

        if (*il == subjects && ! ff) {

          interaction_registration_shape1_scanner_combo<TS, TI> scanner (&interactions, il_index, m_report_progress, scan_description);

          addressable_shape_delivery<TS> is (subjects.confined (common_box, true));
          unsigned int id = id_first;

          for ( ; ! is.at_end (); ++is, ++id) {
            scanner.insert (is.operator-> (), id);
          }

          scanner.process (dist);

        } else {

          geo::box_scanner2<TS, unsigned int, TI, unsigned int> scanner (m_report_progress, scan_description);
          interaction_registration_shape2shape<TS, TI> rec (0 /*layout*/, &interactions, il_index);

          if (*il == subjects) {

            //  this is the case of intra-layer interactions ("foreign"): we pretend we have two layers and
            //  reject shape self-interactions by registering them as "same"

            addressable_shape_delivery<TS> is (subjects.confined (common_box, true));

            unsigned int id = id_first;
            for ( ; ! is.at_end (); ++is, ++id) {
              unsigned int iid = interactions.next_id ();
              scanner.insert1 (is.operator-> (), id);
              safe_insert2_into_box_scanner (scanner, is.operator-> (), iid);
              rec.same (id, iid);
            }

            scanner.process (rec, dist, geo::box_convert<TS> (), geo::box_convert<TI> ());

          } else {

            addressable_shape_delivery<TS> is (subjects.confined (common_box, true));
            addressable_shape_delivery<TI> ii ((*il).confined (common_box, true));

            unsigned int id = id_first;
            for ( ; ! is.at_end (); ++is, ++id) {
              scanner.insert1 (is.operator-> (), id);
            }
            for (; !ii.at_end (); ++ii) {
              scanner.insert2 (ii.operator-> (), interactions.next_id ());
            }

            scanner.process (rec, dist, geo::box_convert<TS> (), geo::box_convert<TI> ());

          }

        }
      }

    }

  }

  if (interactions.begin () != interactions.end ()) {

    if (interactions.begin_intruders () == interactions.end_intruders ()) {

      OnEmptyIntruderHint eh = op->on_empty_intruder_hint ();
      if (eh == OnEmptyIntruderHint::Drop) {
        return;
      }

    }

    std::vector<std::unordered_set<TR> > result;
    result.resize (result_shapes.size ());
    op->compute_local (mp_subject_layout, interactions, result, m_max_vertex_count, m_area_ratio, m_report_progress, process_description);

    for (std::vector<geo::Shapes *>::const_iterator r = result_shapes.begin (); r != result_shapes.end (); ++r) {
      if (*r) {
        const std::unordered_set<TR> rs = result [r - result_shapes.begin ()];
        (*r)->insert (rs.begin (), rs.end ());
      }
    }

  }
}

//  explicit instantiations
template class DB_PUBLIC local_processor<geo::Polygon, geo::Polygon, geo::Polygon>;
template class DB_PUBLIC local_processor<geo::Polygon, geo::Text, geo::Polygon>;
template class DB_PUBLIC local_processor<geo::Polygon, geo::Text, geo::Text>;
template class DB_PUBLIC local_processor<geo::Polygon, geo::Edge, geo::Polygon>;
template class DB_PUBLIC local_processor<geo::Polygon, geo::Edge, geo::Edge>;
template class DB_PUBLIC local_processor<geo::PolygonRef, geo::PolygonRef, geo::PolygonRef>;
template class DB_PUBLIC local_processor<geo::PolygonRef, geo::Edge, geo::PolygonRef>;
template class DB_PUBLIC local_processor<geo::PolygonRef, geo::Edge, geo::Edge>;
template class DB_PUBLIC local_processor<geo::PolygonRef, geo::TextRef, geo::PolygonRef>;
template class DB_PUBLIC local_processor<geo::PolygonRef, geo::TextRef, geo::TextRef>;
template class DB_PUBLIC local_processor<geo::PolygonRef, geo::PolygonRef, geo::EdgePair>;
template class DB_PUBLIC local_processor<geo::PolygonRef, geo::PolygonRef, geo::Edge>;
template class DB_PUBLIC local_processor<geo::Polygon, geo::Polygon, geo::EdgePair>;
template class DB_PUBLIC local_processor<geo::Polygon, geo::Polygon, geo::Edge>;
template class DB_PUBLIC local_processor<geo::Edge, geo::Edge, geo::Edge>;
template class DB_PUBLIC local_processor<geo::Edge, geo::PolygonRef, geo::Edge>;
template class DB_PUBLIC local_processor<geo::Edge, geo::PolygonRef, geo::PolygonRef>;
template class DB_PUBLIC local_processor<geo::Edge, geo::Edge, geo::EdgePair>;
template class DB_PUBLIC local_processor<geo::TextRef, geo::PolygonRef, geo::TextRef>;
template class DB_PUBLIC local_processor<geo::TextRef, geo::PolygonRef, geo::PolygonRef>;

}

