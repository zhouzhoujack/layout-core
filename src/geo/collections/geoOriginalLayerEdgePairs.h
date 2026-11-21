#ifndef HDR_geoOriginalLayerEdgePairs
#define HDR_geoOriginalLayerEdgePairs

#include "geoCommon.h"

#include "geoAsIfFlatEdgePairs.h"
#include "geoShapes.h"
#include "geoRecursiveShapeIterator.h"

namespace geo {

/**
 *  @brief An original layerregion based on a RecursiveShapeIterator
 */
class DB_PUBLIC OriginalLayerEdgePairs
  : public AsIfFlatEdgePairs
{
public:
  OriginalLayerEdgePairs ();
  OriginalLayerEdgePairs (const OriginalLayerEdgePairs &other);
  OriginalLayerEdgePairs (const RecursiveShapeIterator &si);
  OriginalLayerEdgePairs (const RecursiveShapeIterator &si, const geo::ICplxTrans &trans);
  virtual ~OriginalLayerEdgePairs ();

  EdgePairsDelegate *clone () const;

  virtual EdgePairsIteratorDelegate *begin () const;
  virtual std::pair<geo::RecursiveShapeIterator, geo::ICplxTrans> begin_iter () const;

  virtual bool empty () const;

  virtual const geo::EdgePair *nth (size_t n) const;
  virtual bool has_valid_edge_pairs () const;

  virtual const geo::RecursiveShapeIterator *iter () const;

  virtual bool equals (const EdgePairs &other) const;
  virtual bool less (const EdgePairs &other) const;

private:
  OriginalLayerEdgePairs &operator= (const OriginalLayerEdgePairs &other);

  mutable geo::RecursiveShapeIterator m_iter;
  geo::ICplxTrans m_iter_trans;

  void init ();
};

}

#endif

