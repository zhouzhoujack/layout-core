#ifndef HDR_geoOriginalLayerEdges
#define HDR_geoOriginalLayerEdges

#include "geoCommon.h"

#include "geoAsIfFlatEdges.h"
#include "geoShapes.h"
#include "geoRecursiveShapeIterator.h"

namespace geo {

/**
 *  @brief An original layerregion based on a RecursiveShapeIterator
 */
class DB_PUBLIC OriginalLayerEdges
  : public AsIfFlatEdges
{
public:
  OriginalLayerEdges ();
  OriginalLayerEdges (const OriginalLayerEdges &other);
  OriginalLayerEdges (const RecursiveShapeIterator &si, bool is_merged = false);
  OriginalLayerEdges (const RecursiveShapeIterator &si, const geo::ICplxTrans &trans, bool merged_semantics, bool is_merged = false);
  virtual ~OriginalLayerEdges ();

  EdgesDelegate *clone () const;

  virtual EdgesIteratorDelegate *begin () const;
  virtual EdgesIteratorDelegate *begin_merged () const;

  virtual std::pair<geo::RecursiveShapeIterator, geo::ICplxTrans> begin_iter () const;
  virtual std::pair<geo::RecursiveShapeIterator, geo::ICplxTrans> begin_merged_iter () const;

  virtual bool empty () const;

  virtual bool is_merged () const;

  virtual const geo::Edge *nth (size_t n) const;
  virtual bool has_valid_edges () const;
  virtual bool has_valid_merged_edges () const;

  virtual const geo::RecursiveShapeIterator *iter () const;

  virtual bool equals (const Edges &other) const;
  virtual bool less (const Edges &other) const;

protected:
  virtual void merged_semantics_changed ();

private:
  OriginalLayerEdges &operator= (const OriginalLayerEdges &other);

  bool m_is_merged;
  mutable geo::Shapes m_merged_edges;
  mutable bool m_merged_edges_valid;
  mutable geo::RecursiveShapeIterator m_iter;
  geo::ICplxTrans m_iter_trans;

  void init ();
  void ensure_merged_edges_valid () const;
};

}

#endif

