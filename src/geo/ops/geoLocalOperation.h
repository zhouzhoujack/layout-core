#ifndef HDR_geoLocalOperation
#define HDR_geoLocalOperation

#include "geoCommon.h"

#include "geoLayout.h"
#include "geoEdgeBoolean.h"

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>

namespace geo
{

template <class TS, class TI> class shape_interactions;

/**
 *  @brief Indicates the desired behaviour for subject shapes for which there is no intruder
 */
enum OnEmptyIntruderHint {
  /**
   *  @brief Don't imply a specific behaviour
   */
  Ignore = 0,

  /**
   *  @brief Copy the subject shape
   */
  Copy,

  /**
   *  @brief Copy the subject shape to the second result
   */
  CopyToSecond,

  /**
   *  @brief Drop the subject shape
   */
  Drop
};

/**
 *  @brief A base class for "local operations"
 *  A local operation is any operation whose result can be computed by
 *  combining the results derived from individual shape pairs.
 *  The shape pairs can originate from different or the same layer.
 *  If the layers are different, one layer is the subject layer, the
 *  other layer is the "intruder" layer. Subject shapes are always
 *  considered, intruder shapes only if they interact with subject shapes.
 *  This class implements the actual operation. It receives a
 *  cluster of subject shapes vs. corresponding intruder shapes.
 */
template <class TS, class TI, class TR>
class DB_PUBLIC local_operation
{
public:
  /**
   *  @brief Constructor
   */
  local_operation () { }

  /**
   *  @brief Destructor
   */
  virtual ~local_operation () { }

  /**
   *  @brief Computes the results from a given set of interacting shapes
   *
   *  If the operation requests single subject mode, the interactions will be split into single subject/intruder clusters
   */
  void compute_local (geo::Layout *layout, const shape_interactions<TS, TI> &interactions, std::vector<std::unordered_set<TR> > &results, size_t max_vertex_count, double area_ratio, bool report_progress = false, const std::string &progress_desc = std::string ()) const;

  /**
   *  @brief Indicates the desired behaviour when a shape does not have an intruder
   */
  virtual OnEmptyIntruderHint on_empty_intruder_hint () const { return Ignore; }

  /**
   *  @brief If this method returns true, this operation requests single subjects for meal
   */
  virtual bool requests_single_subjects () const { return false; }

  /**
   *  @brief Gets a description text for this operation
   */
  virtual std::string description () const = 0;

  /**
   *  @brief Gets the interaction distance
   *  A distance of means the shapes must overlap in order to interact.
   */
  virtual geo::Coord dist () const { return 0; }

protected:
  /**
   *  @brief Computes the results from a given set of interacting shapes
   *  @param layout The layout to which the shapes belong
   *  @param interactions The interaction set
   *  @param result The container to which the results are written
   */
  virtual void do_compute_local (geo::Layout *layout, const shape_interactions<TS, TI> &interactions, std::vector<std::unordered_set<TR> > &result, size_t max_vertex_count, double area_ratio) const = 0;
};

/**
 *  @brief Implements a boolean AND or NOT operation
 */
class DB_PUBLIC BoolAndOrNotLocalOperation
  : public local_operation<geo::PolygonRef, geo::PolygonRef, geo::PolygonRef>
{
public:
  BoolAndOrNotLocalOperation (bool is_and);

  virtual void do_compute_local (geo::Layout *layout, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &interactions, std::vector<std::unordered_set<geo::PolygonRef> > &result, size_t max_vertex_count, double area_ratio) const;
  virtual OnEmptyIntruderHint on_empty_intruder_hint () const;
  virtual std::string description () const;

private:
  bool m_is_and;
};

/**
 *  @brief Implements a boolean AND plus NOT operation
 *
 *  This processor delivers two outputs: the first one having the AND result, the second
 *  one having the NOT result.
 */
class DB_PUBLIC TwoBoolAndNotLocalOperation
  : public local_operation<geo::PolygonRef, geo::PolygonRef, geo::PolygonRef>
{
public:
  TwoBoolAndNotLocalOperation ();

  virtual void do_compute_local (geo::Layout *layout, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &interactions, std::vector<std::unordered_set<geo::PolygonRef> > &result, size_t max_vertex_count, double area_ratio) const;
  virtual std::string description () const;
};

/**
 *  @brief Implements a merge operation with an overlap count
 *  With a given wrap_count, the result will only contains shapes where
 *  the original shapes overlap at least "wrap_count" times.
 */
class DB_PUBLIC SelfOverlapMergeLocalOperation
  : public local_operation<geo::PolygonRef, geo::PolygonRef, geo::PolygonRef>
{
public:
  SelfOverlapMergeLocalOperation (unsigned int wrap_count);

  virtual void do_compute_local (geo::Layout *layout, const shape_interactions<geo::PolygonRef, geo::PolygonRef> &interactions, std::vector<std::unordered_set<geo::PolygonRef> > &result, size_t max_vertex_count, double area_ratio) const;
  virtual OnEmptyIntruderHint on_empty_intruder_hint () const;
  virtual std::string description () const;

private:
  unsigned int m_wrap_count;
};

/**
 *  @brief Implements a boolean AND or NOT operation between edges
 */
class DB_PUBLIC EdgeBoolAndOrNotLocalOperation
  : public local_operation<geo::Edge, geo::Edge, geo::Edge>
{
public:
  EdgeBoolAndOrNotLocalOperation (EdgeBoolOp op);

  virtual void do_compute_local (geo::Layout *layout, const shape_interactions<geo::Edge, geo::Edge> &interactions, std::vector<std::unordered_set<geo::Edge> > &result, size_t max_vertex_count, double area_ratio) const;
  virtual OnEmptyIntruderHint on_empty_intruder_hint () const;
  virtual std::string description () const;

  //  edge interaction distance is 1 to force overlap between edges and edge/boxes
  virtual geo::Coord dist () const { return 1; }

private:
  EdgeBoolOp m_op;
};

/**
 *  @brief Implements a boolean AND or NOT operation between edges and polygons (polygons as intruders)
 *
 *  "AND" is implemented by "outside == false", "NOT" by "outside == true" with "include_borders == true".
 *  With "include_borders == false" the operations are "INSIDE" and "OUTSIDE".
 */
class DB_PUBLIC EdgeToPolygonLocalOperation
  : public local_operation<geo::Edge, geo::PolygonRef, geo::Edge>
{
public:
  EdgeToPolygonLocalOperation (bool outside, bool include_borders);

  virtual void do_compute_local (geo::Layout *layout, const shape_interactions<geo::Edge, geo::PolygonRef> &interactions, std::vector<std::unordered_set<geo::Edge> > &result, size_t max_vertex_count, double area_ratio) const;
  virtual OnEmptyIntruderHint on_empty_intruder_hint () const;
  virtual std::string description () const;

  //  edge interaction distance is 1 to force overlap between edges and edge/boxes
  virtual geo::Coord dist () const { return m_include_borders ? 1 : 0; }

private:
  bool m_outside;
  bool m_include_borders;
};

}

#endif

