#ifndef HDR_geoShapeCollectionUtils
#define HDR_geoShapeCollectionUtils

#include "geoCommon.h"

#include "geoShape.h"
#include "geoShapes.h"
#include "geoLayout.h"
#include "geoCellVariants.h"
#include "geoShapeCollection.h"
#include "geoDeepShapeStore.h"
#include "utilsObject.h"

#include <list>

namespace geo {

/**
 *  @brief A template base class for edge processors
 *
 *  A polygon processor can turn a edge into something else.
 */
template <class Shape, class Result>
class DB_PUBLIC_TEMPLATE shape_collection_processor
  : public geo::RefCounted
{
public:
  /**
   *  @brief Constructor
   */
  shape_collection_processor () { }

  /**
   *  @brief Destructor
   */
  virtual ~shape_collection_processor () { }

  /**
   *  @brief Performs the actual processing
   *  This method will take the input edge from "edge" and puts the results into "res".
   *  "res" can be empty - in this case, the edge will be skipped.
   */
  virtual void process (const Shape &shape, std::vector<Result> &res) const = 0;

  /**
   *  @brief Returns the transformation reducer for building cell variants
   *  This method may return 0. In this case, not cell variants are built.
   */
  virtual const TransformationReducer *vars () const { return 0; }

  /**
   *  @brief Returns true, if the result of this operation can be regarded "merged" always.
   */
  virtual bool result_is_merged () const { return false; }

  /**
   *  @brief Returns true, if the result of this operation must not be merged.
   *  This feature can be used, if the result represents "degenerated" objects such
   *  as point-like edges. These must not be merged. Otherwise they disappear.
   */
  virtual bool result_must_not_be_merged () const { return false; }

  /**
   *  @brief Returns true, if the processor wants raw (not merged) input
   */
  virtual bool requires_raw_input () const { return false; }

  /**
   *  @brief Returns true, if the processor wants to build variants
   *  If not true, the processor accepts shape propagation as variant resolution.
   */
  virtual bool wants_variants () const { return false; }
};

/**
 *  @brief A shape delivery class for the shape collection processor
 */
template <class Result> struct shape_collection_processor_delivery;

/**
 *  @brief A shape delivery implementation for polygons
 */
template <>
struct DB_PUBLIC shape_collection_processor_delivery<geo::Polygon>
{
  shape_collection_processor_delivery (geo::Layout *layout, geo::Shapes *shapes)
    : mp_layout (layout), mp_shapes (shapes)
  { }

  void put (const geo::Polygon &result)
  {
    geo::MutexLocker locker (&mp_layout->lock ());
    mp_shapes->insert (geo::PolygonRef (result, mp_layout->shape_repository ()));
  }

private:
  geo::Layout *mp_layout;
  geo::Shapes *mp_shapes;
};

/**
 *  @brief A shape delivery implementation for texts
 */
template <>
struct DB_PUBLIC shape_collection_processor_delivery<geo::Text>
{
  shape_collection_processor_delivery (geo::Layout *layout, geo::Shapes *shapes)
    : mp_layout (layout), mp_shapes (shapes)
  { }

  void put (const geo::Text &result)
  {
    geo::MutexLocker locker (&mp_layout->lock ());
    mp_shapes->insert (geo::TextRef (result, mp_layout->shape_repository ()));
  }

private:
  geo::Layout *mp_layout;
  geo::Shapes *mp_shapes;
};

/**
 *  @brief A generic delivery
 */
template <class Result>
struct DB_PUBLIC shape_collection_processor_delivery
{
  shape_collection_processor_delivery (geo::Layout *, geo::Shapes *shapes)
    : mp_shapes (shapes)
  { }

  void put (const Result &result)
  {
    mp_shapes->insert (result);
  }

private:
  geo::Shapes *mp_shapes;
};

/**
 *  @brief Provides a generic implementation of the shape collection processor
 */
template <class Shape, class Result, class OutputContainer>
DB_PUBLIC_TEMPLATE
OutputContainer *
shape_collection_processed_impl (const geo::DeepLayer &input, const shape_collection_processor<Shape, Result> &filter)
{
  std::unique_ptr<VariantsCollectorBase> vars;
  if (filter.vars ()) {

    vars.reset (new geo::VariantsCollectorBase (filter.vars ()));

    vars->collect (input.layout (), input.initial_cell ());

    if (filter.wants_variants ()) {
      const_cast<geo::DeepLayer &> (input).separate_variants (*vars);
    }

  }

  geo::Layout &layout = const_cast<geo::Layout &> (input.layout ());

  std::vector<Result> heap;
  std::map<geo::cell_index_type, std::map<geo::ICplxTrans, geo::Shapes> > to_commit;

  std::unique_ptr<OutputContainer> res (new OutputContainer (input.derived ()));
  if (filter.result_must_not_be_merged ()) {
    res->set_merged_semantics (false);
  }

  for (geo::Layout::iterator c = layout.begin (); c != layout.end (); ++c) {

    const geo::Shapes &s = c->shapes (input.layer ());

    if (vars.get ()) {

      const std::map<geo::ICplxTrans, size_t> &vv = vars->variants (c->cell_index ());
      for (std::map<geo::ICplxTrans, size_t>::const_iterator v = vv.begin (); v != vv.end (); ++v) {

        geo::Shapes *st;
        if (vv.size () == 1) {
          st = & c->shapes (res->deep_layer ().layer ());
        } else {
          st = & to_commit [c->cell_index ()] [v->first];
        }

        shape_collection_processor_delivery<Result> delivery (&layout, st);

        const geo::ICplxTrans &tr = v->first;
        geo::ICplxTrans trinv = tr.inverted ();

        for (geo::Shapes::shape_iterator si = s.begin (geo::ShapeIterator::All); ! si.at_end (); ++si) {
          Shape s;
          si->instantiate (s);
          s.transform (tr);
          heap.clear ();
          filter.process (s, heap);
          for (typename std::vector<Result>::const_iterator i = heap.begin (); i != heap.end (); ++i) {
            delivery.put (i->transformed (trinv));
          }
        }

      }

    } else {

      geo::Shapes &st = c->shapes (res->deep_layer ().layer ());
      shape_collection_processor_delivery<Result> delivery (&layout, &st);

      for (geo::Shapes::shape_iterator si = s.begin (geo::ShapeIterator::All); ! si.at_end (); ++si) {
        Shape s;
        si->instantiate (s);
        heap.clear ();
        filter.process (s, heap);
        for (typename std::vector<Result>::const_iterator i = heap.begin (); i != heap.end (); ++i) {
          delivery.put (*i);
        }
      }

    }

  }

  if (! to_commit.empty () && vars.get ()) {
    res->deep_layer ().commit_shapes (*vars, to_commit);
  }

  if (filter.result_is_merged ()) {
    res->set_is_merged (true);
  }
  return res.release ();
}

/**
 *  @brief A generic processor to compute the extents of an object
 */
template <class Shape>
class extents_processor
  : public geo::shape_collection_processor<Shape, geo::Polygon>
{
public:
  extents_processor (geo::Coord dx, geo::Coord dy)
    : m_dx (dx), m_dy (dy)
  { }

  virtual void process (const Shape &s, std::vector<geo::Polygon> &res) const
  {
    geo::box_convert<Shape> bc;
    geo::Box box = bc (s).enlarged (geo::Vector (m_dx, m_dy));
    if (! box.empty ()) {
      res.push_back (geo::Polygon (box));
    }
  }

  virtual const geo::TransformationReducer *vars () const
  {
    if (m_dx == 0 && m_dy == 0) {
      return 0;
    } else if (m_dx == m_dy) {
      return & m_isotropic_reducer;
    } else {
      return & m_anisotropic_reducer;
    }
  }

  virtual bool result_is_merged () const { return false; }
  virtual bool requires_raw_input () const { return false; }
  virtual bool result_must_not_be_merged () const { return false; }
  virtual bool wants_variants () const { return true; }

private:
  geo::XYAnisotropyAndMagnificationReducer m_anisotropic_reducer;
  geo::MagnificationReducer m_isotropic_reducer;
  geo::Coord m_dx, m_dy;
};

}

#endif

