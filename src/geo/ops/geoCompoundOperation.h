#ifndef HDR_geoCompoundOperation
#define HDR_geoCompoundOperation

#include "geoLocalOperation.h"
#include "geoHierProcessor.h"
#include "geoEdgeProcessor.h"
#include "geoRegionDelegate.h"
#include "geoRegion.h"
#include "geoEdges.h"
#include "geoEdgePairs.h"
#include "geoRegionProcessors.h"
#include "geoRegionLocalOperations.h"
#include "geoLocalOperationUtils.h"
#include "geoTypes.h"

#include "utilsObject.h"

#include <memory>

namespace geo
{

inline geo::Region *subject_regionptr () { return (geo::Region *) 0; }
inline geo::Region *foreign_regionptr () { return (geo::Region *) 1; }
inline bool is_subject_regionptr (const geo::Region *ptr) { return ptr == subject_regionptr () || ptr == foreign_regionptr (); }

class CompoundRegionOperationNode;

/**
 *  @brief A per-node cache for results of the compound operations
 *
 *  This cache is important to avoid duplicate evaluation of the same node in
 *  a diamond-graph structure of nodes.
 */
class CompoundRegionOperationCache
{
public:
  template <class TR>
  std::pair<bool, std::vector<std::unordered_set<TR> > *> get (const CompoundRegionOperationNode *node)
  {
    bool valid = false;
    std::vector<std::unordered_set<TR> > *cache = 0;
    get_cache (cache, valid, node);
    return std::make_pair (valid, cache);
  }

private:
  std::map<const CompoundRegionOperationNode *, std::vector<std::unordered_set<geo::PolygonRef> > > m_cache_polyref;
  std::map<const CompoundRegionOperationNode *, std::vector<std::unordered_set<geo::Polygon> > > m_cache_poly;
  std::map<const CompoundRegionOperationNode *, std::vector<std::unordered_set<geo::Edge> > > m_cache_edge;
  std::map<const CompoundRegionOperationNode *, std::vector<std::unordered_set<geo::EdgePair> > > m_cache_edge_pair;

  template <class TR>
  void get_cache_generic (std::map<const CompoundRegionOperationNode *, std::vector<std::unordered_set<TR> > > &caches, std::vector<std::unordered_set<TR> > *&cache_ptr, bool &valid, const CompoundRegionOperationNode *node)
  {
    typename std::map<const CompoundRegionOperationNode *, std::vector<std::unordered_set<TR> > >::iterator c = caches.find (node);
    if (c != caches.end ()) {
      valid = true;
      cache_ptr = &c->second;
    } else {
      cache_ptr = &caches [node];
    }
  }

  void get_cache (std::vector<std::unordered_set<geo::PolygonRef> > *&cache_ptr, bool &valid, const CompoundRegionOperationNode *node) { get_cache_generic (m_cache_polyref, cache_ptr, valid, node); }
  void get_cache (std::vector<std::unordered_set<geo::Polygon> > *&cache_ptr, bool &valid, const CompoundRegionOperationNode *node)    { get_cache_generic (m_cache_poly, cache_ptr, valid, node); }
  void get_cache (std::vector<std::unordered_set<geo::Edge> > *&cache_ptr, bool &valid, const CompoundRegionOperationNode *node)       { get_cache_generic (m_cache_edge, cache_ptr, valid, node); }
  void get_cache (std::vector<std::unordered_set<geo::EdgePair> > *&cache_ptr, bool &valid, const CompoundRegionOperationNode *node)   { get_cache_generic (m_cache_edge_pair, cache_ptr, valid, node); }
};

/**
 *  @brief A node of the compound operation tree
 *
 *  A compound operation if formed of a tree of basic operations. The root node
 *  will act as the main entrance and is fed into a local processor for performing
 *  the complex operation.
 *
 *  The tree nodes will
 */

class CompoundRegionOperationNode
  : public geo::RefCounted
{
public:
  enum  ResultType { Region, Edges, EdgePairs };

  CompoundRegionOperationNode ();
  virtual ~CompoundRegionOperationNode ();

  /**
   *  @brief Returns the description set with "set_description"
   */
  const std::string &raw_description () const
  {
    return m_description;
  }

  /**
   *  @brief Gets the interaction distance of this operation
   *  The returned value will be the minimum distance (set_dist) or the computed distance
   *  (compute_dist), whichever is larger.
   */
  geo::Coord dist () const
  {
    return std::max (m_dist, computed_dist ());
  }

  /**
   *  @brief Sets a minmum distance
   */
  void set_dist (geo::Coord d)
  {
    m_dist = d;
  }

  /**
   *  @brief Gets the description for this node
   */
  std::string description () const;

  /**
   *  @brief Sets the description for this node
   *  If no description is set, the generated description will be used.
   */
  void set_description (const std::string &d);

  /**
   *  @brief Gets a value indicating whether the result is definitely merged
   *  The default implementation is based on a heuristic analysis of the inputs.
   */
  virtual bool is_merged () const;

  /**
   *  @brief Returns a value indicating whether this node provides external inputs
   *  A node has external inputs if it has inputs and does not refer to the primary alone.
   */
  virtual bool has_external_inputs () const;

  /**
   *  @brief Returns a value giving a hint how to handle the case of empty intruders
   */
  //  NOTE: it's probably going to be difficult to compute a different value here:
  virtual OnEmptyIntruderHint on_empty_intruder_hint () const { return OnEmptyIntruderHint::Ignore; }

  /**
   *  @brief Returns the inputs this node depends on
   *  The returned vector may use subject_regionptr() or forein_regionptr() as abstract values
   *  for the subject layer and the subject layer in "foreign" mode (asymmetric: other subjects are treated
   *  as different shapes).
   */
  virtual std::vector<geo::Region *> inputs () const = 0;

  /**
   *  @brief Specifies the type of shapes this node delivers
   */
  virtual ResultType result_type () const = 0;

  /**
   *  @brief Returns the transformation reducer for building cell variants
   *  This method may return 0. In this case, not cell variants are built.
   */
  virtual const TransformationReducer *vars () const  { return 0; }

  /**
   *  @brief Returns true, if the processor wants to build variants
   *  If not true, the processor accepts shape propagation as variant resolution.
   */
  virtual bool wants_variants () const { return false; }

  void compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::Polygon, geo::Polygon> &interactions, std::vector<std::unordered_set<geo::Polygon> > &results, size_t max_vertex_count, double area_ratio) const
  {
    implement_compute_local (cache, layout, interactions, results, max_vertex_count, area_ratio);
  }

  void compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::Polygon, geo::Polygon> &interactions, std::vector<std::unordered_set<geo::PolygonRef> > &results, size_t max_vertex_count, double area_ratio) const;

  void compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::Polygon, geo::Polygon> &interactions, std::vector<std::unordered_set<geo::Edge> > &results, size_t max_vertex_count, double area_ratio) const
  {
    implement_compute_local (cache, layout, interactions, results, max_vertex_count, area_ratio);
  }

  void compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::Polygon, geo::Polygon> &interactions, std::vector<std::unordered_set<geo::EdgePair> > &results, size_t max_vertex_count, double area_ratio) const
  {
    implement_compute_local (cache, layout, interactions, results, max_vertex_count, area_ratio);
  }

  void compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &interactions, std::vector<std::unordered_set<geo::PolygonRef> > &results, size_t max_vertex_count, double area_ratio) const
  {
    implement_compute_local (cache, layout, interactions, results, max_vertex_count, area_ratio);
  }

  void compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &interactions, std::vector<std::unordered_set<geo::Polygon> > &results, size_t max_vertex_count, double area_ratio) const;

  void compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &interactions, std::vector<std::unordered_set<geo::Edge> > &results, size_t max_vertex_count, double area_ratio) const
  {
    implement_compute_local (cache, layout, interactions, results, max_vertex_count, area_ratio);
  }

  void compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &interactions, std::vector<std::unordered_set<geo::EdgePair> > &results, size_t max_vertex_count, double area_ratio) const
  {
    implement_compute_local (cache, layout, interactions, results, max_vertex_count, area_ratio);
  }

  template <class T>
  bool compute_local_bool (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<T, T> &interactions, size_t max_vertex_count, double area_ratio) const
  {
    if (result_type () == Region) {

      std::vector<std::unordered_set<T> > res;
      res.push_back (std::unordered_set<T> ());
      compute_local (cache, layout, interactions, res, max_vertex_count, area_ratio);
      return ! res.front ().empty ();

    } else if (result_type () == Edges) {

      std::vector<std::unordered_set<geo::Edge> > res;
      res.push_back (std::unordered_set<geo::Edge> ());
      compute_local (cache, layout, interactions, res, max_vertex_count, area_ratio);
      return ! res.front ().empty ();

    } else if (result_type () == EdgePairs) {

      std::vector<std::unordered_set<geo::EdgePair> > res;
      res.push_back (std::unordered_set<geo::EdgePair> ());
      compute_local (cache, layout, interactions, res, max_vertex_count, area_ratio);
      return ! res.front ().empty ();

    } else {
      return false;
    }
  }

protected:
  //  the different computation slots
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::Polygon, geo::Polygon> & /*interactions*/,       std::vector<std::unordered_set<geo::Polygon> > & /*results*/,     size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::Polygon, geo::Polygon> & /*interactions*/,       std::vector<std::unordered_set<geo::Edge> > & /*results*/,        size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::Polygon, geo::Polygon> & /*interactions*/,       std::vector<std::unordered_set<geo::EdgePair> > & /*results*/,    size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::PolygonRef, geo::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<geo::PolygonRef> > & /*results*/,  size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::PolygonRef, geo::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<geo::Edge> > & /*results*/,        size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::PolygonRef, geo::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<geo::EdgePair> > & /*results*/,    size_t /*max_vertex_count*/, double /*area_ratio*/) const { }

  virtual geo::Coord computed_dist () const = 0;

  virtual std::string generated_description () const;

private:
  std::string m_description;
  geo::Coord m_dist;

  template <class TS, class TI, class TR>
  void implement_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<TS, TI> &interactions, std::vector<std::unordered_set<TR> > &results, size_t max_vertex_count, double area_ratio) const
  {
    //  TODO: confine caching to those nodes which need it.

    std::pair<bool, std::vector<std::unordered_set<TR> > *> cp = cache->get<TR> (this);

    if (! cp.first) {

      std::vector<std::unordered_set<TR> > uncached_results;
      uncached_results.resize (results.size ());

      do_compute_local (cache, layout, interactions, uncached_results, max_vertex_count, area_ratio);

      cp.second->swap (uncached_results);

    }

    tl_assert (results.size () == cp.second->size ());
    for (size_t r = 0; r < results.size (); ++r) {
      results[r].insert ((*cp.second)[r].begin (), (*cp.second)[r].end ());
    }
  }
};


template <class T>
struct compound_operation_type_traits;

template <>
struct compound_operation_type_traits<geo::PolygonRef>
{
  typedef geo::Region container_type;
  static CompoundRegionOperationNode::ResultType type () { return CompoundRegionOperationNode::Region; }
};

template <>
struct compound_operation_type_traits<geo::Polygon>
{
  typedef geo::Region container_type;
  static CompoundRegionOperationNode::ResultType type () { return CompoundRegionOperationNode::Region; }
};

template <>
struct compound_operation_type_traits<geo::Edge>
{
  typedef geo::Edges container_type;
  static CompoundRegionOperationNode::ResultType type () { return CompoundRegionOperationNode::Edges; }
};

template <>
struct compound_operation_type_traits<geo::EdgePair>
{
  typedef geo::EdgePairs container_type;
  static CompoundRegionOperationNode::ResultType type () { return CompoundRegionOperationNode::EdgePairs; }
};


class CompoundRegionOperationEmptyNode
  : public CompoundRegionOperationNode
{
public:
  CompoundRegionOperationEmptyNode (ResultType type)
    : CompoundRegionOperationNode (), m_type (type)
  { }

  virtual ~CompoundRegionOperationEmptyNode ()
  { }

  virtual std::vector<geo::Region *> inputs () const { return std::vector<geo::Region *> (); }
  virtual geo::Coord computed_dist () const { return 0; }
  virtual ResultType result_type () const { return m_type; }

private:
  ResultType m_type;
};


class CompoundRegionOperationPrimaryNode
  : public CompoundRegionOperationNode
{
public:
  CompoundRegionOperationPrimaryNode ();
  virtual ~CompoundRegionOperationPrimaryNode ();

  virtual std::vector<geo::Region *> inputs () const;
  virtual geo::Coord computed_dist () const { return 0; }
  virtual ResultType result_type () const { return Region; }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::Polygon, geo::Polygon> &interactions, std::vector<std::unordered_set<geo::Polygon> > &results, size_t max_vertex_count, double area_ratio) const;
  virtual void do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &interactions, std::vector<std::unordered_set<geo::PolygonRef> > &results, size_t max_vertex_count, double area_ratio) const;

  //  non-implemented
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::Polygon, geo::Polygon> & /*interactions*/,       std::vector<std::unordered_set<geo::Edge> > & /*results*/,        size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::Polygon, geo::Polygon> & /*interactions*/,       std::vector<std::unordered_set<geo::EdgePair> > & /*results*/,    size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::PolygonRef, geo::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<geo::Edge> > & /*results*/,        size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::PolygonRef, geo::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<geo::EdgePair> > & /*results*/,    size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
};

class CompoundRegionOperationForeignNode
  : public CompoundRegionOperationNode
{
public:
  CompoundRegionOperationForeignNode ();
  virtual ~CompoundRegionOperationForeignNode ();

  virtual std::vector<geo::Region *> inputs () const;
  virtual geo::Coord computed_dist () const { return 0; }
  virtual ResultType result_type () const { return Region; }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::Polygon, geo::Polygon> &interactions, std::vector<std::unordered_set<geo::Polygon> > &results, size_t max_vertex_count, double area_ratio) const;
  virtual void do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &interactions, std::vector<std::unordered_set<geo::PolygonRef> > &results, size_t max_vertex_count, double area_ratio) const;

  //  non-implemented
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::Polygon, geo::Polygon> & /*interactions*/,       std::vector<std::unordered_set<geo::Edge> > & /*results*/,        size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::Polygon, geo::Polygon> & /*interactions*/,       std::vector<std::unordered_set<geo::EdgePair> > & /*results*/,    size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::PolygonRef, geo::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<geo::Edge> > & /*results*/,        size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::PolygonRef, geo::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<geo::EdgePair> > & /*results*/,    size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
};

class CompoundRegionOperationSecondaryNode
  : public CompoundRegionOperationNode
{
public:
  CompoundRegionOperationSecondaryNode (geo::Region *input);
  virtual ~CompoundRegionOperationSecondaryNode ();

  virtual std::vector<geo::Region *> inputs () const;
  virtual geo::Coord computed_dist () const { return 0; }
  virtual ResultType result_type () const { return Region; }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::Polygon, geo::Polygon> &interactions, std::vector<std::unordered_set<geo::Polygon> > &results, size_t max_vertex_count, double area_ratio) const;
  virtual void do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &interactions, std::vector<std::unordered_set<geo::PolygonRef> > &results, size_t max_vertex_count, double area_ratio) const;

  //  non-implemented
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::Polygon, geo::Polygon> & /*interactions*/,       std::vector<std::unordered_set<geo::Edge> > & /*results*/,        size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::Polygon, geo::Polygon> & /*interactions*/,       std::vector<std::unordered_set<geo::EdgePair> > & /*results*/,    size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::PolygonRef, geo::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<geo::Edge> > & /*results*/,        size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::PolygonRef, geo::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<geo::EdgePair> > & /*results*/,    size_t /*max_vertex_count*/, double /*area_ratio*/) const { }

private:
  geo::Region *mp_input;
};


class CompoundTransformationReducer
  : public geo::TransformationReducer
{
public:
  CompoundTransformationReducer ();

  void add (const geo::TransformationReducer *reducer);
  bool is_empty () const { return m_vars.empty (); }

  virtual geo::Trans reduce_trans (const geo::Trans &trans) const;
  virtual geo::ICplxTrans reduce_trans (const geo::ICplxTrans &trans) const;
  virtual geo::Trans reduce (const geo::Trans &trans) const;
  virtual geo::ICplxTrans reduce (const geo::ICplxTrans &trans) const;
  virtual bool is_translation_invariant () const;

private:
  std::vector<const geo::TransformationReducer *> m_vars;
};

class CompoundRegionMultiInputOperationNode
  : public CompoundRegionOperationNode
{
public:
  CompoundRegionMultiInputOperationNode (const std::vector<CompoundRegionOperationNode *> &children);
  CompoundRegionMultiInputOperationNode ();
  CompoundRegionMultiInputOperationNode (CompoundRegionOperationNode *child);
  CompoundRegionMultiInputOperationNode (CompoundRegionOperationNode *a, CompoundRegionOperationNode *b);
  ~CompoundRegionMultiInputOperationNode ();

  virtual geo::Coord computed_dist () const;
  virtual std::string generated_description () const;

  virtual std::vector<geo::Region *> inputs () const
  {
    return m_inputs;
  }

  virtual const TransformationReducer *vars () const;
  virtual bool wants_variants () const;

protected:
  bool needs_reduce_interactions (unsigned int child_index) const
  {
    if (m_children.size () < 2 || child (child_index)->inputs ().empty ()) {
      return false;
    } else {
      return true;
    }
  }

  template <class TS, class TI>
  const shape_interactions<TS, TI> &interactions_for_child (const shape_interactions<TS, TI> &interactions, unsigned int child_index, shape_interactions<TS, TI> &child_interactions) const
  {
    if (! needs_reduce_interactions (child_index)) {
      return interactions;
    }

    for (typename shape_interactions<TS, TI>::iterator i = interactions.begin (); i != interactions.end (); ++i) {

      if (child (child_index)->on_empty_intruder_hint () == OnEmptyIntruderHint::Drop) {
        child_interactions.add_subject_shape (i->first, interactions.subject_shape (i->first));
      } else {
        //  this includes the subject-without-intruder "interaction"
        child_interactions.add_subject (i->first, interactions.subject_shape (i->first));
      }

      for (typename shape_interactions<TS, TI>::iterator2 ii = i->second.begin (); ii != i->second.end (); ++ii) {

        const std::pair<unsigned int, TI> &is = interactions.intruder_shape (*ii);

        std::map<std::pair<unsigned int, unsigned int>, unsigned int>::const_iterator lm = m_map_layer_to_child.find (std::make_pair (child_index, is.first));
        if (lm != m_map_layer_to_child.end ()) {
          child_interactions.add_intruder_shape (*ii, lm->second, is.second);
          child_interactions.add_interaction (i->first, *ii);
        }

      }

    }

    return child_interactions;
  }

  unsigned int children () const
  {
    return (unsigned int) m_children.size ();
  }

  CompoundRegionOperationNode *child (unsigned int index);
  const CompoundRegionOperationNode *child (unsigned int index) const;

private:
  geo::shared_collection<CompoundRegionOperationNode> m_children;
  //  maps child#,layer# to layer# of child:
  std::map<std::pair<unsigned int, unsigned int>, unsigned int> m_map_layer_to_child;
  std::vector<geo::Region *> m_inputs;
  CompoundTransformationReducer m_vars;

  void init ();
};


class CompoundRegionLogicalBoolOperationNode
  : public CompoundRegionMultiInputOperationNode
{
public:
  enum LogicalOp { And, Or };

  CompoundRegionLogicalBoolOperationNode (LogicalOp op, bool invert, const std::vector<CompoundRegionOperationNode *> &inputs);

  virtual std::string generated_description () const;

  //  specifies the result type
  virtual ResultType result_type () const;

  //  the different computation slots
  virtual void do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::Polygon, geo::Polygon> &interactions, std::vector<std::unordered_set<geo::Polygon> > &results, size_t max_vertex_count, double area_ratio) const
  {
    implement_compute_local (cache, layout, interactions, results, max_vertex_count, area_ratio);
  }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &interactions, std::vector<std::unordered_set<geo::PolygonRef> > &results, size_t max_vertex_count, double area_ratio) const
  {
    implement_compute_local (cache, layout, interactions, results, max_vertex_count, area_ratio);
  }

  //  non-implemented
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::Polygon, geo::Polygon> & /*interactions*/,       std::vector<std::unordered_set<geo::Edge> > & /*results*/,        size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::Polygon, geo::Polygon> & /*interactions*/,       std::vector<std::unordered_set<geo::EdgePair> > & /*results*/,    size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::PolygonRef, geo::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<geo::Edge> > & /*results*/,        size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::PolygonRef, geo::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<geo::EdgePair> > & /*results*/,    size_t /*max_vertex_count*/, double /*area_ratio*/) const { }

private:
  LogicalOp m_op;
  bool m_invert;

  template <class T>
  void implement_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<T, T> &interactions, std::vector<std::unordered_set<T> > &results, size_t max_vertex_count, double area_ratio) const;
};


class CompoundRegionCountFilterNode
  : public CompoundRegionMultiInputOperationNode
{
public:
  CompoundRegionCountFilterNode (CompoundRegionOperationNode *input, bool invert, size_t min_count, size_t max_count);
  ~CompoundRegionCountFilterNode ();

  virtual std::string generated_description () const;

  //  specifies the result type
  virtual ResultType result_type () const
  {
    return child (0)->result_type ();
  }

  //  the different computation slots
  virtual void do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::Polygon, geo::Polygon> &interactions, std::vector<std::unordered_set<geo::Polygon> > &results, size_t max_vertex_count, double area_ratio) const
  {
    implement_compute_local (cache, layout, interactions, results, max_vertex_count, area_ratio);
  }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &interactions, std::vector<std::unordered_set<geo::PolygonRef> > &results, size_t max_vertex_count, double area_ratio) const
  {
    implement_compute_local (cache, layout, interactions, results, max_vertex_count, area_ratio);
  }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::Polygon, geo::Polygon> &interactions, std::vector<std::unordered_set<geo::Edge> > &results, size_t max_vertex_count, double area_ratio) const
  {
    implement_compute_local (cache, layout, interactions, results, max_vertex_count, area_ratio);
  }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &interactions, std::vector<std::unordered_set<geo::Edge> > &results, size_t max_vertex_count, double area_ratio) const
  {
    implement_compute_local (cache, layout, interactions, results, max_vertex_count, area_ratio);
  }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::Polygon, geo::Polygon> &interactions, std::vector<std::unordered_set<geo::EdgePair> > &results, size_t max_vertex_count, double area_ratio) const
  {
    implement_compute_local (cache, layout, interactions, results, max_vertex_count, area_ratio);
  }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &interactions, std::vector<std::unordered_set<geo::EdgePair> > &results, size_t max_vertex_count, double area_ratio) const
  {
    implement_compute_local (cache, layout, interactions, results, max_vertex_count, area_ratio);
  }

private:
  size_t m_min_count, m_max_count;
  bool m_invert;

  template <class T, class TR>
  void implement_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<T, T> &interactions, std::vector<std::unordered_set<TR> > &results, size_t max_vertex_count, double area_ratio) const
  {
    std::vector<std::unordered_set<TR> > one;
    one.push_back (std::unordered_set<TR> ());

    child (0)->compute_local (cache, layout, interactions, one, max_vertex_count, area_ratio);

    if ((one.front ().size () >= m_min_count && one.front ().size () < m_max_count) != m_invert) {
      results.front ().insert (one.front ().begin (), one.front ().end ());
    }
  }
};


class CompoundRegionGeometricalBoolOperationNode
  : public CompoundRegionMultiInputOperationNode
{
public:
  enum  GeometricalOp { And, Not, Or, Xor };

  CompoundRegionGeometricalBoolOperationNode (GeometricalOp op, CompoundRegionOperationNode *a, CompoundRegionOperationNode *b);

  virtual std::string generated_description () const;

  //  specifies the result type
  virtual ResultType result_type () const;
  virtual geo::Coord computed_dist () const;

  //  the different computation slots
  virtual void do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::Polygon, geo::Polygon> &interactions, std::vector<std::unordered_set<geo::Polygon> > &results, size_t max_vertex_count, double area_ratio) const;
  virtual void do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::Polygon, geo::Polygon> &interactions, std::vector<std::unordered_set<geo::Edge> > &results, size_t max_vertex_count, double area_ratio) const;
  virtual void do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &interactions, std::vector<std::unordered_set<geo::PolygonRef> > &results, size_t max_vertex_count, double area_ratio) const;
  virtual void do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &interactions, std::vector<std::unordered_set<geo::Edge> > &results, size_t max_vertex_count, double area_ratio) const;

  //  non-implemented
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::Polygon, geo::Polygon> & /*interactions*/,       std::vector<std::unordered_set<geo::EdgePair> > & /*results*/,    size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::PolygonRef, geo::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<geo::EdgePair> > & /*results*/,    size_t /*max_vertex_count*/, double /*area_ratio*/) const { }

private:
  GeometricalOp m_op;

  template <class T, class TR>
  void implement_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<T, T> &interactions, std::vector<std::unordered_set<TR> > &results, size_t max_vertex_count, double area_ratio) const;
  template <class T, class T1, class T2, class TR>
  void implement_bool (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<T, T> &interactions, std::vector<std::unordered_set<TR> > &results, size_t max_vertex_count, double area_ratio) const;
};

template <class TS, class TI, class TR>
class compound_region_generic_operation_node
  : public CompoundRegionMultiInputOperationNode
{
public:
  /**
   *  @brief Constructor
   *  NOTE: the inputs must be given for subjects and one to many intruder layers.
   *  Input 0 is for the subject, input 1 for the first intruder etc.
   *  If original layers are required for input, CompoundRegionOperationPrimaryNode or
   *  CompoundRegionOperationSecondaryNode nodes need to be provided. If no derived node
   *  is requested for input, the input-less constructor may be used which is more efficient.
   */
  compound_region_generic_operation_node (const geo::local_operation<TS, TI, TR> *op, const std::vector<CompoundRegionOperationNode *> &inputs, const geo::TransformationReducer *vars = 0, bool want_variants = false)
    : CompoundRegionMultiInputOperationNode (inputs), m_op (op), mp_vars (vars), m_wants_variants (want_variants)
  {
    //  .. nothing yet ..
  }

  compound_region_generic_operation_node (const geo::local_operation<TS, TI, TR> *op, CompoundRegionOperationNode *input, const geo::TransformationReducer *vars = 0, bool want_variants = false)
    : CompoundRegionMultiInputOperationNode (input), m_op (op), mp_vars (vars), m_wants_variants (want_variants)
  {
    //  .. nothing yet ..
  }

  compound_region_generic_operation_node (const geo::local_operation<TS, TI, TR> *op, CompoundRegionOperationNode *a, CompoundRegionOperationNode *b, const geo::TransformationReducer *vars = 0, bool want_variants = false)
    : CompoundRegionMultiInputOperationNode (a, b), m_op (op), mp_vars (vars), m_wants_variants (want_variants)
  {
    //  .. nothing yet ..
  }

  virtual ResultType result_type () const { return compound_operation_type_traits<TR>::type (); }
  virtual const geo::TransformationReducer *vars () const  { return mp_vars; }
  virtual bool wants_variants () const { return m_wants_variants; }

  virtual geo::Coord computed_dist () const
  {
    return CompoundRegionMultiInputOperationNode::computed_dist () + m_op->dist ();
  }

  virtual std::vector<geo::Region *> inputs () const
  {
    if (! m_inputs.empty ()) {
      return m_inputs;
    } else {
      return CompoundRegionMultiInputOperationNode::inputs ();
    }
  }

protected:
  compound_region_generic_operation_node (const geo::local_operation<TS, TI, TR> *op, const std::vector<geo::Region *> &inputs, const geo::TransformationReducer *vars = 0, bool want_variants = false)
    : CompoundRegionMultiInputOperationNode (), m_op (op), mp_vars (vars), m_wants_variants (want_variants), m_inputs (inputs)
  {
    //  .. nothing yet ..
  }

  compound_region_generic_operation_node (const geo::local_operation<TS, TI, TR> *op, geo::Region *input, const geo::TransformationReducer *vars = 0, bool want_variants = false)
    : CompoundRegionMultiInputOperationNode (), m_op (op), mp_vars (vars), m_wants_variants (want_variants)
  {
    m_inputs.push_back (input);
  }

  compound_region_generic_operation_node (const geo::local_operation<TS, TI, TR> *op, geo::Region *a, geo::Region *b, const geo::TransformationReducer *vars = 0, bool want_variants = false)
    : CompoundRegionMultiInputOperationNode (), m_op (op), mp_vars (vars), m_wants_variants (want_variants)
  {
    m_inputs.push_back (a);
    m_inputs.push_back (b);
  }

  template <class TTS, class TTI, class TTR>
  void implement_compute_local (geo::CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<TTS, TTI> &interactions, std::vector<std::unordered_set<TTR> > &results, size_t max_vertex_count, double area_ratio) const;

private:
  const geo::local_operation<TS, TI, TR> *m_op;
  const geo::TransformationReducer *mp_vars;
  bool m_wants_variants;
  std::vector<geo::Region *> m_inputs;
  //  required if the inner processor is a PolygonRef type while the outer interface needs Polygon
  geo::Layout m_aux_layout;
};

class CompoundRegionInteractOperationNode
  : public compound_region_generic_operation_node<geo::Polygon, geo::Polygon, geo::Polygon>
{
public:
  CompoundRegionInteractOperationNode (CompoundRegionOperationNode *a, CompoundRegionOperationNode *b, int mode, bool touching, bool inverse, size_t min_count = 0, size_t max_count = std::numeric_limits<size_t>::max ());
  CompoundRegionInteractOperationNode (geo::Region *a, geo::Region *b, int mode, bool touching, bool inverse, size_t min_count = 0, size_t max_count = std::numeric_limits<size_t>::max ());

  std::string generated_description () const;

  virtual void do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::Polygon, geo::Polygon> &interactions, std::vector<std::unordered_set<geo::Polygon> > &results, size_t max_vertex_count, double area_ratio) const
  {
    implement_compute_local (cache, layout, interactions, results, max_vertex_count, area_ratio);
  }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &interactions, std::vector<std::unordered_set<geo::PolygonRef> > &results, size_t max_vertex_count, double area_ratio) const
  {
    implement_compute_local (cache, layout, interactions, results, max_vertex_count, area_ratio);
  }

  //  non-implemented
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::Polygon, geo::Polygon> & /*interactions*/,       std::vector<std::unordered_set<geo::Edge> > & /*results*/,        size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::Polygon, geo::Polygon> & /*interactions*/,       std::vector<std::unordered_set<geo::EdgePair> > & /*results*/,    size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::PolygonRef, geo::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<geo::Edge> > & /*results*/,        size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::PolygonRef, geo::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<geo::EdgePair> > & /*results*/,    size_t /*max_vertex_count*/, double /*area_ratio*/) const { }

private:
  typedef geo::interacting_local_operation<geo::Polygon, geo::Polygon, geo::Polygon> op_type;
  op_type m_op;
};

class CompoundRegionInteractWithEdgeOperationNode
  : public compound_region_generic_operation_node<geo::Polygon, geo::Edge, geo::Polygon>
{
public:
  CompoundRegionInteractWithEdgeOperationNode (CompoundRegionOperationNode *a, CompoundRegionOperationNode *b, bool inverse, size_t min_count = 0, size_t max_count = std::numeric_limits<size_t>::max ());

  std::string generated_description () const;

  virtual void do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::Polygon, geo::Polygon> &interactions, std::vector<std::unordered_set<geo::Polygon> > &results, size_t max_vertex_count, double area_ratio) const
  {
    implement_compute_local (cache, layout, interactions, results, max_vertex_count, area_ratio);
  }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &interactions, std::vector<std::unordered_set<geo::PolygonRef> > &results, size_t max_vertex_count, double area_ratio) const
  {
    implement_compute_local (cache, layout, interactions, results, max_vertex_count, area_ratio);
  }

  //  non-implemented
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::Polygon, geo::Polygon> & /*interactions*/,       std::vector<std::unordered_set<geo::Edge> > & /*results*/,        size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::Polygon, geo::Polygon> & /*interactions*/,       std::vector<std::unordered_set<geo::EdgePair> > & /*results*/,    size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::PolygonRef, geo::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<geo::Edge> > & /*results*/,        size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::PolygonRef, geo::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<geo::EdgePair> > & /*results*/,    size_t /*max_vertex_count*/, double /*area_ratio*/) const { }

private:
  geo::interacting_with_edge_local_operation<geo::Polygon, geo::Edge, geo::Polygon> m_op;
};

class CompoundRegionPullOperationNode
  : public compound_region_generic_operation_node<geo::Polygon, geo::Polygon, geo::Polygon>
{
public:
  CompoundRegionPullOperationNode (CompoundRegionOperationNode *a, CompoundRegionOperationNode *b, int mode, bool touching);
  CompoundRegionPullOperationNode (geo::Region *a, geo::Region *b, int mode, bool touching);

  std::string generated_description () const;

  virtual void do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::Polygon, geo::Polygon> &interactions, std::vector<std::unordered_set<geo::Polygon> > &results, size_t max_vertex_count, double area_ratio) const
  {
    implement_compute_local (cache, layout, interactions, results, max_vertex_count, area_ratio);
  }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &interactions, std::vector<std::unordered_set<geo::PolygonRef> > &results, size_t max_vertex_count, double area_ratio) const
  {
    implement_compute_local (cache, layout, interactions, results, max_vertex_count, area_ratio);
  }

  //  non-implemented
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::Polygon, geo::Polygon> & /*interactions*/,       std::vector<std::unordered_set<geo::Edge> > & /*results*/,        size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::Polygon, geo::Polygon> & /*interactions*/,       std::vector<std::unordered_set<geo::EdgePair> > & /*results*/,    size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::PolygonRef, geo::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<geo::Edge> > & /*results*/,        size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::PolygonRef, geo::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<geo::EdgePair> > & /*results*/,    size_t /*max_vertex_count*/, double /*area_ratio*/) const { }

private:
  geo::pull_local_operation<geo::Polygon, geo::Polygon, geo::Polygon> m_op;
};

class CompoundRegionPullWithEdgeOperationNode
  : public compound_region_generic_operation_node<geo::Polygon, geo::Edge, geo::Edge>
{
public:
  CompoundRegionPullWithEdgeOperationNode (CompoundRegionOperationNode *a, CompoundRegionOperationNode *b);

  std::string generated_description () const;

  virtual void do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::Polygon, geo::Polygon> &interactions, std::vector<std::unordered_set<geo::Edge> > &results, size_t max_vertex_count, double area_ratio) const
  {
    implement_compute_local (cache, layout, interactions, results, max_vertex_count, area_ratio);
  }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &interactions, std::vector<std::unordered_set<geo::Edge> > &results, size_t max_vertex_count, double area_ratio) const
  {
    implement_compute_local (cache, layout, interactions, results, max_vertex_count, area_ratio);
  }

  //  non-implemented
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::Polygon, geo::Polygon> & /*interactions*/,       std::vector<std::unordered_set<geo::Polygon> > & /*results*/,        size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::Polygon, geo::Polygon> & /*interactions*/,       std::vector<std::unordered_set<geo::EdgePair> > & /*results*/,    size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::PolygonRef, geo::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<geo::PolygonRef> > & /*results*/,        size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::PolygonRef, geo::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<geo::EdgePair> > & /*results*/,    size_t /*max_vertex_count*/, double /*area_ratio*/) const { }

private:
  geo::pull_with_edge_local_operation<geo::Polygon, geo::Edge, geo::Edge> m_op;
};

/**
 *  @brief Implements the joining of results
 *
 *  This operator joins all inputs into a common output.
 *  The types of the inputs must be the same.
 */

class CompoundRegionJoinOperationNode
  : public CompoundRegionMultiInputOperationNode
{
public:
  CompoundRegionJoinOperationNode (const std::vector<CompoundRegionOperationNode *> &inputs);

  virtual std::string generated_description () const;

  //  specifies the result type
  virtual ResultType result_type () const;

  //  the different computation slots
  virtual void do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::Polygon, geo::Polygon> &interactions, std::vector<std::unordered_set<geo::Polygon> > &results, size_t max_vertex_count, double area_ratio) const
  {
    implement_compute_local (cache, layout, interactions, results, max_vertex_count, area_ratio);
  }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &interactions, std::vector<std::unordered_set<geo::PolygonRef> > &results, size_t max_vertex_count, double area_ratio) const
  {
    implement_compute_local (cache, layout, interactions, results, max_vertex_count, area_ratio);
  }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &interactions, std::vector<std::unordered_set<geo::Edge> > &results, size_t max_vertex_count, double area_ratio) const
  {
    implement_compute_local (cache, layout, interactions, results, max_vertex_count, area_ratio);
  }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::Polygon, geo::Polygon> &interactions, std::vector<std::unordered_set<geo::Edge> > &results, size_t max_vertex_count, double area_ratio) const
  {
    implement_compute_local (cache, layout, interactions, results, max_vertex_count, area_ratio);
  }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &interactions, std::vector<std::unordered_set<geo::EdgePair> > &results, size_t max_vertex_count, double area_ratio) const
  {
    implement_compute_local (cache, layout, interactions, results, max_vertex_count, area_ratio);
  }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::Polygon, geo::Polygon> &interactions, std::vector<std::unordered_set<geo::EdgePair> > &results, size_t max_vertex_count, double area_ratio) const
  {
    implement_compute_local (cache, layout, interactions, results, max_vertex_count, area_ratio);
  }

private:
  template <class T, class TR>
  void implement_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<T, T> &interactions, std::vector<std::unordered_set<TR> > &results, size_t max_vertex_count, double area_ratio) const;
};

/**
 *  @brief Implements the case selection
 *
 *  Case selection is a sequence of if/then nodes:
 *  (if1 then1 if2 then2 ... default) or (if1 then1 if2 then2 ...)
 *  The first condition is tested. If true, the "then1" result is taken. Otherwise, the
 *  "if2" condition is tested and "then2" is taken if true etc. At the end the
 *  default is evaluated if no other condition matched.
 */

class CompoundRegionLogicalCaseSelectOperationNode
  : public CompoundRegionMultiInputOperationNode
{
public:
  CompoundRegionLogicalCaseSelectOperationNode (const std::vector<CompoundRegionOperationNode *> &inputs);

  virtual std::string generated_description () const;

  //  specifies the result type
  virtual ResultType result_type () const;

  //  the different computation slots
  virtual void do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::Polygon, geo::Polygon> &interactions, std::vector<std::unordered_set<geo::Polygon> > &results, size_t max_vertex_count, double area_ratio) const
  {
    implement_compute_local (cache, layout, interactions, results, max_vertex_count, area_ratio);
  }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &interactions, std::vector<std::unordered_set<geo::PolygonRef> > &results, size_t max_vertex_count, double area_ratio) const
  {
    implement_compute_local (cache, layout, interactions, results, max_vertex_count, area_ratio);
  }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &interactions, std::vector<std::unordered_set<geo::Edge> > &results, size_t max_vertex_count, double area_ratio) const
  {
    implement_compute_local (cache, layout, interactions, results, max_vertex_count, area_ratio);
  }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::Polygon, geo::Polygon> &interactions, std::vector<std::unordered_set<geo::Edge> > &results, size_t max_vertex_count, double area_ratio) const
  {
    implement_compute_local (cache, layout, interactions, results, max_vertex_count, area_ratio);
  }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &interactions, std::vector<std::unordered_set<geo::EdgePair> > &results, size_t max_vertex_count, double area_ratio) const
  {
    implement_compute_local (cache, layout, interactions, results, max_vertex_count, area_ratio);
  }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::Polygon, geo::Polygon> &interactions, std::vector<std::unordered_set<geo::EdgePair> > &results, size_t max_vertex_count, double area_ratio) const
  {
    implement_compute_local (cache, layout, interactions, results, max_vertex_count, area_ratio);
  }

private:
  template <class T, class TR>
  void implement_compute_local (geo::CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<T, T> &interactions, std::vector<std::unordered_set<TR> > &results, size_t max_vertex_count, double area_ratio) const;

  bool m_multi_layer;
};


class CompoundRegionFilterOperationNode
  : public CompoundRegionMultiInputOperationNode
{
public:
  CompoundRegionFilterOperationNode (PolygonFilterBase *filter, CompoundRegionOperationNode *input, bool owns_filter = false, bool sum_of_set = false);
  ~CompoundRegionFilterOperationNode ();

  //  specifies the result type
  virtual ResultType result_type () const { return Region; }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::Polygon, geo::Polygon> &interactions, std::vector<std::unordered_set<geo::Polygon> > &results, size_t max_vertex_count, double area_ratio) const;
  virtual void do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &interactions, std::vector<std::unordered_set<geo::PolygonRef> > &results, size_t max_vertex_count, double area_ratio) const;

  //  non-implemented
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::Polygon, geo::Polygon> & /*interactions*/,       std::vector<std::unordered_set<geo::Edge> > & /*results*/,        size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::Polygon, geo::Polygon> & /*interactions*/,       std::vector<std::unordered_set<geo::EdgePair> > & /*results*/,    size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::PolygonRef, geo::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<geo::Edge> > & /*results*/,        size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::PolygonRef, geo::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<geo::EdgePair> > & /*results*/,    size_t /*max_vertex_count*/, double /*area_ratio*/) const { }

  virtual const TransformationReducer *vars () const { return mp_filter->vars (); }
  virtual bool wants_variants () const { return mp_filter->wants_variants (); }

private:
  PolygonFilterBase *mp_filter;
  bool m_owns_filter;
  bool m_sum_of_set;

  template <class T>
  void implement_compute_local (geo::CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<T, T> &interactions, std::vector<std::unordered_set<T> > &results, size_t max_vertex_count, double area_ratio) const
  {
    std::vector<std::unordered_set<T> > one;
    one.push_back (std::unordered_set<T> ());

    child (0)->compute_local (cache, layout, interactions, one, max_vertex_count, area_ratio);

    if (m_sum_of_set) {
      if (mp_filter->selected_set (one.front ())) {
        results.front ().insert (one.front ().begin (), one.front ().end ());
      }
    } else {
      for (typename std::unordered_set<T>::const_iterator p = one.front ().begin (); p != one.front ().end (); ++p) {
        if (mp_filter->selected (*p)) {
          results.front ().insert (*p);
        }
      }
    }
  }
};

class CompoundRegionEdgeFilterOperationNode
  : public CompoundRegionMultiInputOperationNode
{
public:
  CompoundRegionEdgeFilterOperationNode (EdgeFilterBase *filter, CompoundRegionOperationNode *input, bool owns_filter = false, bool sum_of = false);
  ~CompoundRegionEdgeFilterOperationNode ();

  //  specifies the result type
  virtual ResultType result_type () const { return Edges; }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::Polygon, geo::Polygon> &interactions, std::vector<std::unordered_set<geo::Edge> > &results, size_t max_vertex_count, double area_ratio) const;
  virtual void do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &interactions, std::vector<std::unordered_set<geo::Edge> > &results, size_t max_vertex_count, double area_ratio) const;

  //  non-implemented
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::Polygon, geo::Polygon> & /*interactions*/,       std::vector<std::unordered_set<geo::Polygon> > & /*results*/,     size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::Polygon, geo::Polygon> & /*interactions*/,       std::vector<std::unordered_set<geo::EdgePair> > & /*results*/,    size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::PolygonRef, geo::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<geo::PolygonRef> > & /*results*/,  size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::PolygonRef, geo::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<geo::EdgePair> > & /*results*/,    size_t /*max_vertex_count*/, double /*area_ratio*/) const { }

  virtual const TransformationReducer *vars () const { return mp_filter->vars (); }
  virtual bool wants_variants () const { return mp_filter->wants_variants (); }

private:
  EdgeFilterBase *mp_filter;
  bool m_owns_filter;
  bool m_sum_of;

  template <class T>
  void implement_compute_local (geo::CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<T, T> &interactions, std::vector<std::unordered_set<geo::Edge> > &results, size_t max_vertex_count, double area_ratio) const
  {
    std::vector<std::unordered_set<geo::Edge> > one;
    one.push_back (std::unordered_set<Edge> ());

    child (0)->compute_local (cache, layout, interactions, one, max_vertex_count, area_ratio);

    if (m_sum_of) {
      if (mp_filter->selected (one.front ())) {
        results.front ().insert (one.front ().begin (), one.front ().end ());
      }
    } else {
      for (typename std::unordered_set<geo::Edge>::const_iterator p = one.front ().begin (); p != one.front ().end (); ++p) {
        if (mp_filter->selected (*p)) {
          results.front ().insert (*p);
        }
      }
    }
  }
};

class CompoundRegionEdgePairFilterOperationNode
  : public CompoundRegionMultiInputOperationNode
{
public:
  CompoundRegionEdgePairFilterOperationNode (EdgePairFilterBase *filter, CompoundRegionOperationNode *input, bool owns_filter = false);
  ~CompoundRegionEdgePairFilterOperationNode ();

  //  specifies the result type
  virtual ResultType result_type () const { return EdgePairs; }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::Polygon, geo::Polygon> &interactions, std::vector<std::unordered_set<geo::EdgePair> > &results, size_t max_vertex_count, double area_ratio) const;
  virtual void do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &interactions, std::vector<std::unordered_set<geo::EdgePair> > &results, size_t max_vertex_count, double area_ratio) const;

  //  non-implemented
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::Polygon, geo::Polygon> & /*interactions*/,       std::vector<std::unordered_set<geo::Polygon> > & /*results*/,     size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::Polygon, geo::Polygon> & /*interactions*/,       std::vector<std::unordered_set<geo::Edge> > & /*results*/,        size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::PolygonRef, geo::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<geo::PolygonRef> > & /*results*/,  size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::PolygonRef, geo::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<geo::Edge> > & /*results*/,        size_t /*max_vertex_count*/, double /*area_ratio*/) const { }

  virtual const TransformationReducer *vars () const { return mp_filter->vars (); }
  virtual bool wants_variants () const { return mp_filter->wants_variants (); }

private:
  EdgePairFilterBase *mp_filter;
  bool m_owns_filter;

  bool is_selected (const geo::EdgePair &p) const;

  template <class T>
  void implement_compute_local (geo::CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<T, T> &interactions, std::vector<std::unordered_set<geo::EdgePair> > &results, size_t max_vertex_count, double area_ratio) const
  {
    std::vector<std::unordered_set<geo::EdgePair> > one;
    one.push_back (std::unordered_set<EdgePair> ());

    child (0)->compute_local (cache, layout, interactions, one, max_vertex_count, area_ratio);

    for (typename std::unordered_set<geo::EdgePair>::const_iterator p = one.front ().begin (); p != one.front ().end (); ++p) {
      if (is_selected (*p)) {
        results.front ().insert (*p);
      }
    }
  }
};

class CompoundRegionProcessingOperationNode
  : public CompoundRegionMultiInputOperationNode
{
public:
  CompoundRegionProcessingOperationNode (PolygonProcessorBase *proc, CompoundRegionOperationNode *input, bool owns_proc = false, geo::Coord dist_adder = 0);
  ~CompoundRegionProcessingOperationNode ();

  //  specifies the result type
  virtual ResultType result_type () const { return Region; }

  virtual geo::Coord computed_dist () const { return m_dist_adder + CompoundRegionMultiInputOperationNode::computed_dist (); }

  virtual const TransformationReducer *vars () const { return mp_proc->vars (); }
  virtual bool wants_variants () const { return mp_proc->wants_variants (); }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::Polygon, geo::Polygon> &interactions, std::vector<std::unordered_set<geo::Polygon> > &results, size_t max_vertex_count, double area_ratio) const;
  virtual void do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &interactions, std::vector<std::unordered_set<geo::PolygonRef> > &results, size_t max_vertex_count, double area_ratio) const;

  //  non-implemented
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::Polygon, geo::Polygon> & /*interactions*/,       std::vector<std::unordered_set<geo::Edge> > & /*results*/,        size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::Polygon, geo::Polygon> & /*interactions*/,       std::vector<std::unordered_set<geo::EdgePair> > & /*results*/,    size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::PolygonRef, geo::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<geo::Edge> > & /*results*/,        size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::PolygonRef, geo::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<geo::EdgePair> > & /*results*/,    size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
private:
  PolygonProcessorBase *mp_proc;
  bool m_owns_proc;
  geo::Coord m_dist_adder;

  void processed (geo::Layout *, const geo::Polygon &p, std::vector<geo::Polygon> &res) const;
  void processed (geo::Layout *layout, const geo::PolygonRef &p, std::vector<geo::PolygonRef> &res) const;

  template <class T>
  void implement_compute_local (geo::CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<T, T> &interactions, std::vector<std::unordered_set<T> > &results, size_t max_vertex_count, double area_ratio) const
  {
    std::vector<std::unordered_set<T> > one;
    one.push_back (std::unordered_set<T> ());

    child (0)->compute_local (cache, layout, interactions, one, max_vertex_count, area_ratio);

    std::vector<T> res;
    for (typename std::unordered_set<T>::const_iterator p = one.front ().begin (); p != one.front ().end (); ++p) {
      res.clear ();
      processed (layout, *p, res);
      results.front ().insert (res.begin (), res.end ());
    }
  }
};

class CompoundRegionSizeOperationNode
  : public CompoundRegionProcessingOperationNode
{
public:
  CompoundRegionSizeOperationNode (geo::Coord dx, geo::Coord dy, unsigned int mode, CompoundRegionOperationNode *input)
    : CompoundRegionProcessingOperationNode (& m_proc, input), m_proc (dx, dy, mode)
  { }

  CompoundRegionSizeOperationNode (geo::Coord d, unsigned int mode, CompoundRegionOperationNode *input)
    : CompoundRegionProcessingOperationNode (& m_proc, input), m_proc (d, d, mode)
  { }

  virtual std::string description () const
  {
    return std::string ("sized") + CompoundRegionProcessingOperationNode::generated_description ();
  }

private:
  geo::PolygonSizer m_proc;
  CompoundTransformationReducer m_vars;
};

class CompoundRegionMergeOperationNode
  : public CompoundRegionMultiInputOperationNode
{
public:
  CompoundRegionMergeOperationNode (bool min_coherence, unsigned int min_wc, CompoundRegionOperationNode *input)
    : CompoundRegionMultiInputOperationNode (input), m_min_coherence (min_coherence), m_min_wc (min_wc)
  { }

  ~CompoundRegionMergeOperationNode () { }

  virtual ResultType result_type () const { return Region; }

  virtual std::string description () const
  {
    return std::string ("merged") + CompoundRegionMultiInputOperationNode::generated_description ();
  }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::Polygon, geo::Polygon> &interactions, std::vector<std::unordered_set<geo::Polygon> > &results, size_t max_vertex_count, double area_ratio) const
  {
    implement_compute_local (cache, layout, interactions, results, max_vertex_count, area_ratio);
  }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &interactions, std::vector<std::unordered_set<geo::PolygonRef> > &results, size_t max_vertex_count, double area_ratio) const
  {
    implement_compute_local (cache, layout, interactions, results, max_vertex_count, area_ratio);
  }

  //  non-implemented
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::Polygon, geo::Polygon> & /*interactions*/,       std::vector<std::unordered_set<geo::Edge> > & /*results*/,        size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::Polygon, geo::Polygon> & /*interactions*/,       std::vector<std::unordered_set<geo::EdgePair> > & /*results*/,    size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::PolygonRef, geo::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<geo::Edge> > & /*results*/,        size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::PolygonRef, geo::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<geo::EdgePair> > & /*results*/,    size_t /*max_vertex_count*/, double /*area_ratio*/) const { }

private:
  bool m_min_coherence;
  unsigned int m_min_wc;

  template <class T>
  void implement_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<T, T> &interactions, std::vector<std::unordered_set<T> > &results, size_t max_vertex_count, double area_ratio) const
  {
    std::vector<std::unordered_set<T> > one;
    one.push_back (std::unordered_set<T> ());

    child (0)->compute_local (cache, layout, interactions, one, max_vertex_count, area_ratio);

    geo::EdgeProcessor ep;

    //  count edges and reserve memory
    size_t n = 0;
    for (typename std::unordered_set<T>::const_iterator p = one.front ().begin (); p != one.front ().end (); ++p) {
      n += p->vertices ();
    }
    ep.reserve (n);

    //  insert the polygons into the processor
    n = 0;
    for (typename std::unordered_set<T>::const_iterator p = one.front ().begin (); p != one.front ().end (); ++p) {
      ep.insert (*p, n);
      ++n;
    }

    //  and run the merge step
    geo::MergeOp op (m_min_wc);
    geo::polygon_ref_generator<T> pc (layout, results.front ());
    geo::PolygonGenerator pg (pc, false /*don't resolve holes*/, m_min_coherence);
    ep.process (pg, op);
  }
};

class CompoundRegionEdgeToPolygonProcessingOperationNode
  : public CompoundRegionMultiInputOperationNode
{
public:
  CompoundRegionEdgeToPolygonProcessingOperationNode (EdgeToPolygonProcessorBase *proc, CompoundRegionOperationNode *input, bool owns_proc = false);
  ~CompoundRegionEdgeToPolygonProcessingOperationNode ();

  //  specifies the result type
  virtual ResultType result_type () const { return Region; }

  virtual const TransformationReducer *vars () const { return mp_proc->vars (); }
  virtual bool wants_variants () const { return mp_proc->wants_variants (); }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::Polygon, geo::Polygon> &interactions, std::vector<std::unordered_set<geo::Polygon> > &results, size_t max_vertex_count, double area_ratio) const;
  virtual void do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &interactions, std::vector<std::unordered_set<geo::PolygonRef> > &results, size_t max_vertex_count, double area_ratio) const;

  //  non-implemented
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::Polygon, geo::Polygon> & /*interactions*/,       std::vector<std::unordered_set<geo::Edge> > & /*results*/,        size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::Polygon, geo::Polygon> & /*interactions*/,       std::vector<std::unordered_set<geo::EdgePair> > & /*results*/,    size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::PolygonRef, geo::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<geo::Edge> > & /*results*/,        size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::PolygonRef, geo::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<geo::EdgePair> > & /*results*/,    size_t /*max_vertex_count*/, double /*area_ratio*/) const { }

private:
  EdgeToPolygonProcessorBase *mp_proc;
  bool m_owns_proc;

  void processed (geo::Layout *, const geo::Edge &p, std::vector<geo::Polygon> &res) const;
  void processed (geo::Layout *layout, const geo::Edge &p, std::vector<geo::PolygonRef> &res) const;

  template <class T>
  void implement_compute_local (geo::CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<T, T> &interactions, std::vector<std::unordered_set<T> > &results, size_t max_vertex_count, double area_ratio) const
  {
    std::vector<std::unordered_set<geo::Edge> > one;
    one.push_back (std::unordered_set<geo::Edge> ());

    child (0)->compute_local (cache, layout, interactions, one, max_vertex_count, area_ratio);

    std::vector<T> res;
    for (typename std::unordered_set<geo::Edge>::const_iterator p = one.front ().begin (); p != one.front ().end (); ++p) {
      res.clear ();
      processed (layout, *p, res);
      results.front ().insert (res.begin (), res.end ());
    }
  }
};

class CompoundRegionEdgeProcessingOperationNode
  : public CompoundRegionMultiInputOperationNode
{
public:
  CompoundRegionEdgeProcessingOperationNode (EdgeProcessorBase *proc, CompoundRegionOperationNode *input, bool owns_proc = false);
  ~CompoundRegionEdgeProcessingOperationNode ();

  //  specifies the result type
  virtual ResultType result_type () const { return Edges; }

  virtual const TransformationReducer *vars () const { return mp_proc->vars (); }
  virtual bool wants_variants () const { return mp_proc->wants_variants (); }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::Polygon, geo::Polygon> &interactions, std::vector<std::unordered_set<geo::Edge> > &results, size_t max_vertex_count, double area_ratio) const;
  virtual void do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &interactions, std::vector<std::unordered_set<geo::Edge> > &results, size_t max_vertex_count, double area_ratio) const;

  //  non-implemented
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::Polygon, geo::Polygon> & /*interactions*/,       std::vector<std::unordered_set<geo::Polygon> > & /*results*/,     size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::Polygon, geo::Polygon> & /*interactions*/,       std::vector<std::unordered_set<geo::EdgePair> > & /*results*/,    size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::PolygonRef, geo::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<geo::PolygonRef> > & /*results*/,  size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::PolygonRef, geo::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<geo::EdgePair> > & /*results*/,    size_t /*max_vertex_count*/, double /*area_ratio*/) const { }

private:
  EdgeProcessorBase *mp_proc;
  bool m_owns_proc;

  void processed (geo::Layout *, const geo::Edge &p, std::vector<geo::Edge> &res) const;

  template <class T>
  void implement_compute_local (geo::CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<T, T> &interactions, std::vector<std::unordered_set<geo::Edge> > &results, size_t max_vertex_count, double area_ratio) const
  {
    std::vector<std::unordered_set<geo::Edge> > one;
    one.push_back (std::unordered_set<geo::Edge> ());

    child (0)->compute_local (cache, layout, interactions, one, max_vertex_count, area_ratio);

    std::vector<geo::Edge> res;
    for (typename std::unordered_set<geo::Edge>::const_iterator p = one.front ().begin (); p != one.front ().end (); ++p) {
      res.clear ();
      processed (layout, *p, res);
      results.front ().insert (res.begin (), res.end ());
    }
  }
};

class CompoundRegionEdgePairToPolygonProcessingOperationNode
  : public CompoundRegionMultiInputOperationNode
{
public:
  CompoundRegionEdgePairToPolygonProcessingOperationNode (EdgePairToPolygonProcessorBase *proc, CompoundRegionOperationNode *input, bool owns_proc = false);
  ~CompoundRegionEdgePairToPolygonProcessingOperationNode ();

  //  specifies the result type
  virtual ResultType result_type () const { return Region; }

  virtual const TransformationReducer *vars () const { return mp_proc->vars (); }
  virtual bool wants_variants () const { return mp_proc->wants_variants (); }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::Polygon, geo::Polygon> &interactions, std::vector<std::unordered_set<geo::Polygon> > &results, size_t max_vertex_count, double area_ratio) const;
  virtual void do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &interactions, std::vector<std::unordered_set<geo::PolygonRef> > &results, size_t max_vertex_count, double area_ratio) const;

  //  non-implemented
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::Polygon, geo::Polygon> & /*interactions*/,       std::vector<std::unordered_set<geo::Edge> > & /*results*/,        size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::Polygon, geo::Polygon> & /*interactions*/,       std::vector<std::unordered_set<geo::EdgePair> > & /*results*/,    size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::PolygonRef, geo::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<geo::Edge> > & /*results*/,        size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::PolygonRef, geo::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<geo::EdgePair> > & /*results*/,    size_t /*max_vertex_count*/, double /*area_ratio*/) const { }

private:
  EdgePairToPolygonProcessorBase *mp_proc;
  bool m_owns_proc;

  void processed (geo::Layout *, const geo::EdgePair &p, std::vector<geo::Polygon> &res) const;
  void processed (geo::Layout *layout, const geo::EdgePair &p, std::vector<geo::PolygonRef> &res) const;

  template <class T>
  void implement_compute_local (geo::CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<T, T> &interactions, std::vector<std::unordered_set<T> > &results, size_t max_vertex_count, double area_ratio) const
  {
    std::vector<std::unordered_set<geo::EdgePair> > one;
    one.push_back (std::unordered_set<geo::EdgePair> ());

    child (0)->compute_local (cache, layout, interactions, one, max_vertex_count, area_ratio);

    std::vector<T> res;
    for (typename std::unordered_set<geo::EdgePair>::const_iterator p = one.front ().begin (); p != one.front ().end (); ++p) {
      res.clear ();
      processed (layout, *p, res);
      results.front ().insert (res.begin (), res.end ());
    }
  }
};

class CompoundRegionEdgePairToEdgeProcessingOperationNode
  : public CompoundRegionMultiInputOperationNode
{
public:
  CompoundRegionEdgePairToEdgeProcessingOperationNode (EdgePairToEdgeProcessorBase *proc, CompoundRegionOperationNode *input, bool owns_proc = false);
  ~CompoundRegionEdgePairToEdgeProcessingOperationNode ();

  //  specifies the result type
  virtual ResultType result_type () const { return Edges; }

  virtual const TransformationReducer *vars () const { return mp_proc->vars (); }
  virtual bool wants_variants () const { return mp_proc->wants_variants (); }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::Polygon, geo::Polygon> &interactions, std::vector<std::unordered_set<geo::Edge> > &results, size_t max_vertex_count, double area_ratio) const;
  virtual void do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &interactions, std::vector<std::unordered_set<geo::Edge> > &results, size_t max_vertex_count, double area_ratio) const;

  //  non-implemented
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::Polygon, geo::Polygon> & /*interactions*/,       std::vector<std::unordered_set<geo::Polygon> > & /*results*/,     size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::Polygon, geo::Polygon> & /*interactions*/,       std::vector<std::unordered_set<geo::EdgePair> > & /*results*/,    size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::PolygonRef, geo::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<geo::PolygonRef> > & /*results*/,  size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::PolygonRef, geo::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<geo::EdgePair> > & /*results*/,    size_t /*max_vertex_count*/, double /*area_ratio*/) const { }

private:
  EdgePairToEdgeProcessorBase *mp_proc;
  bool m_owns_proc;

  template <class T>
  void implement_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<T, T> &interactions, std::vector<std::unordered_set<geo::Edge> > &results, size_t max_vertex_count, double area_ratio) const
  {
    std::vector<std::unordered_set<geo::EdgePair> > one;
    one.push_back (std::unordered_set<geo::EdgePair> ());

    child (0)->compute_local (cache, layout, interactions, one, max_vertex_count, area_ratio);

    std::vector<geo::Edge> res;
    for (typename std::unordered_set<geo::EdgePair>::const_iterator p = one.front ().begin (); p != one.front ().end (); ++p) {
      res.clear ();
      mp_proc->process (*p, res);
      results.front ().insert (res.begin (), res.end ());
    }
  }
};

class CompoundRegionToEdgeProcessingOperationNode
  : public CompoundRegionMultiInputOperationNode
{
public:
  CompoundRegionToEdgeProcessingOperationNode (PolygonToEdgeProcessorBase *proc, CompoundRegionOperationNode *input, bool owns_proc = false);
  ~CompoundRegionToEdgeProcessingOperationNode ();

  //  specifies the result type
  virtual ResultType result_type () const { return Edges; }

  virtual const TransformationReducer *vars () const { return mp_proc->vars (); }
  virtual bool wants_variants () const { return mp_proc->wants_variants (); }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::Polygon, geo::Polygon> &interactions, std::vector<std::unordered_set<geo::Edge> > &results, size_t max_vertex_count, double area_ratio) const;
  virtual void do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &interactions, std::vector<std::unordered_set<geo::Edge> > &results, size_t max_vertex_count, double area_ratio) const;

  //  non-implemented
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::Polygon, geo::Polygon> & /*interactions*/,       std::vector<std::unordered_set<geo::Polygon> > & /*results*/,     size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::Polygon, geo::Polygon> & /*interactions*/,       std::vector<std::unordered_set<geo::EdgePair> > & /*results*/,    size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::PolygonRef, geo::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<geo::PolygonRef> > & /*results*/,  size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::PolygonRef, geo::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<geo::EdgePair> > & /*results*/,    size_t /*max_vertex_count*/, double /*area_ratio*/) const { }

private:
  PolygonToEdgeProcessorBase *mp_proc;
  bool m_owns_proc;

  void processed (geo::Layout *, const geo::Polygon &p, std::vector<geo::Edge> &res) const;
  void processed (geo::Layout *layout, const geo::PolygonRef &p, std::vector<geo::Edge> &res) const;

  template <class T>
  void implement_compute_local (geo::CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<T, T> &interactions, std::vector<std::unordered_set<geo::Edge> > &results, size_t max_vertex_count, double area_ratio) const
  {
    std::vector<std::unordered_set<T> > one;
    one.push_back (std::unordered_set<T> ());

    child (0)->compute_local (cache, layout, interactions, one, max_vertex_count, area_ratio);

    std::vector<geo::Edge> res;
    for (typename std::unordered_set<T>::const_iterator p = one.front ().begin (); p != one.front ().end (); ++p) {
      res.clear ();
      processed (layout, *p, res);
      results.front ().insert (res.begin (), res.end ());
    }
  }
};

/**
 *  @brief A wrapper for a generic edge-pair producing node
 *
 *  This node takes polygons for input. It will produce edge pairs by means of the PolygonToEdgePairProcessorBase object.
 */
class CompoundRegionToEdgePairProcessingOperationNode
  : public CompoundRegionMultiInputOperationNode
{
public:
  /**
   *  @brief Constructor
   *  @param proc The processor which turns polygons into edge pairs (it's reimplementation) - the node will *not* take ownership unless owns_proc is true
   *  @param input The node for the original (the node will take ownership)
   */
  CompoundRegionToEdgePairProcessingOperationNode (PolygonToEdgePairProcessorBase *proc, CompoundRegionOperationNode *input, bool owns_proc = false);
  ~CompoundRegionToEdgePairProcessingOperationNode ();

  //  specifies the result type
  virtual ResultType result_type () const { return EdgePairs; }

  virtual const TransformationReducer *vars () const { return mp_proc->vars (); }
  virtual bool wants_variants () const { return mp_proc->wants_variants (); }

  virtual void do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::Polygon, geo::Polygon> &interactions, std::vector<std::unordered_set<geo::EdgePair> > &results, size_t max_vertex_count, double area_ratio) const;
  virtual void do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &interactions, std::vector<std::unordered_set<geo::EdgePair> > &results, size_t max_vertex_count, double area_ratio) const;

  //  non-implemented
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::Polygon, geo::Polygon> & /*interactions*/,       std::vector<std::unordered_set<geo::Polygon> > & /*results*/,     size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::Polygon, geo::Polygon> & /*interactions*/,       std::vector<std::unordered_set<geo::Edge> > & /*results*/,        size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::PolygonRef, geo::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<geo::PolygonRef> > & /*results*/,  size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::PolygonRef, geo::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<geo::Edge> > & /*results*/,        size_t /*max_vertex_count*/, double /*area_ratio*/) const { }

private:
 PolygonToEdgePairProcessorBase *mp_proc;
  bool m_owns_proc;

  void processed (geo::Layout *, const geo::Polygon &p, std::vector<geo::EdgePair> &res) const;
  void processed (geo::Layout *layout, const geo::PolygonRef &p, std::vector<geo::EdgePair> &res) const;

  template <class T>
  void implement_compute_local (geo::CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<T, T> &interactions, std::vector<std::unordered_set<geo::EdgePair> > &results, size_t max_vertex_count, double area_ratio) const
  {
    std::vector<std::unordered_set<T> > one;
    one.push_back (std::unordered_set<T> ());

    child (0)->compute_local (cache, layout, interactions, one, max_vertex_count, area_ratio);

    std::vector<geo::EdgePair> res;
    for (typename std::unordered_set<T>::const_iterator p = one.front ().begin (); p != one.front ().end (); ++p) {
      res.clear ();
      processed (layout, *p, res);
      results.front ().insert (res.begin (), res.end ());
    }
  }
};

/**
 *  @brief A wrapper for a generic DRC check node
 *
 *  This node takes polygons for input. It will produce edge pairs as result of the DRC check.
 */
class CompoundRegionCheckOperationNode
  : public CompoundRegionMultiInputOperationNode
{
public:
  /**
   *  @brief Constructor
   *  @param input The node for the original (the node will take ownership)
   */
  CompoundRegionCheckOperationNode (geo::CompoundRegionOperationNode *input, geo::edge_relation_type rel, bool different_polygons, geo::Coord d, const geo::RegionCheckOptions &options);

  /**
   *  @brief Constructor for the two-layer check
   */
  CompoundRegionCheckOperationNode (geo::CompoundRegionOperationNode *input, geo::CompoundRegionOperationNode *other, geo::edge_relation_type rel, bool different_polygons, geo::Coord d, const geo::RegionCheckOptions &options);

  /**
   *  @brief Constructor for a single-polygon check (width, notch)
   */
  CompoundRegionCheckOperationNode (geo::edge_relation_type rel, bool different_polygons, geo::Coord d, const geo::RegionCheckOptions &options);

  //  specifies the result type
  virtual ResultType result_type () const { return EdgePairs; }

  virtual OnEmptyIntruderHint on_empty_intruder_hint () const;
  virtual geo::Coord computed_dist () const;

  virtual void do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::Polygon, geo::Polygon> &interactions, std::vector<std::unordered_set<geo::EdgePair> > &results, size_t max_vertex_count, double area_ratio) const;
  virtual void do_compute_local (CompoundRegionOperationCache *cache, geo::Layout *layout, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &interactions, std::vector<std::unordered_set<geo::EdgePair> > &results, size_t max_vertex_count, double area_ratio) const;

  //  non-implemented
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::Polygon, geo::Polygon> & /*interactions*/,       std::vector<std::unordered_set<geo::Polygon> > & /*results*/,     size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::Polygon, geo::Polygon> & /*interactions*/,       std::vector<std::unordered_set<geo::Edge> > & /*results*/,        size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::PolygonRef, geo::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<geo::PolygonRef> > & /*results*/,  size_t /*max_vertex_count*/, double /*area_ratio*/) const { }
  virtual void do_compute_local (CompoundRegionOperationCache * /*cache*/, geo::Layout * /*layout*/, const shape_interactions<geo::PolygonRef, geo::PolygonRef> & /*interactions*/, std::vector<std::unordered_set<geo::Edge> > & /*results*/,        size_t /*max_vertex_count*/, double /*area_ratio*/) const { }

private:
  geo::EdgeRelationFilter m_check;
  bool m_different_polygons;
  geo::RegionCheckOptions m_options;
  bool m_has_other;
  bool m_is_other_merged;
};


/**
 *  @brief The generic local operation
 *
 *  This local operation executes the operation tree within a local processor.
 *  When put into a local processor, the operation tree will be executed on each interaction.
 */
template <class TS, class TI, class TR>
class compound_local_operation
  : public local_operation<TS, TI, TR>
{
public:
  /**
   *  @brief Constructor
   *
   *  Creates a local operation which utilizes the operation tree. "node" is the root of the operation tree.
   *  Ownership of the node is *not* transferred to the local operation.
   */
  compound_local_operation<TS, TI, TR> (CompoundRegionOperationNode *node)
    : mp_node (node)
  { }

protected:
  virtual void do_compute_local (geo::Layout *layout, const shape_interactions<TS, TI> &interactions, std::vector<std::unordered_set<TR> > &results, size_t max_vertex_count, double area_ratio) const
  {
    CompoundRegionOperationCache cache;
    mp_node->compute_local (&cache, layout, interactions, results, max_vertex_count, area_ratio);
  }

  virtual geo::Coord dist () const { return mp_node->dist (); }
  virtual OnEmptyIntruderHint on_empty_intruder_hint () const { return mp_node->on_empty_intruder_hint (); }
  virtual bool requests_single_subjects () const { return true; }
  virtual std::string description () const { return mp_node->description (); }

  const TransformationReducer *vars () const { return mp_node->vars (); }
  bool wants_variants () const { return mp_node->wants_variants (); }
  std::vector<geo::Region *> inputs () const { return mp_node->inputs (); }

private:
  geo::weak_ptr<CompoundRegionOperationNode> mp_node;
};

}

namespace geo
{

template <>
struct type_traits<geo::CompoundRegionOperationNode>
  : public geo::type_traits<void>
{
  typedef false_tag has_copy_constructor;
  typedef false_tag has_default_constructor;
};

}

#endif
