#ifndef HDR_geoOriginalLayerTexts
#define HDR_geoOriginalLayerTexts

#include "geoCommon.h"

#include "geoAsIfFlatTexts.h"
#include "geoShapes.h"
#include "geoRecursiveShapeIterator.h"

namespace geo {

/**
 *  @brief An original layer text collection based on a RecursiveShapeIterator
 */
class DB_PUBLIC OriginalLayerTexts
  : public AsIfFlatTexts
{
public:
  OriginalLayerTexts ();
  OriginalLayerTexts (const OriginalLayerTexts &other);
  OriginalLayerTexts (const RecursiveShapeIterator &si);
  OriginalLayerTexts (const RecursiveShapeIterator &si, const geo::ICplxTrans &trans);
  virtual ~OriginalLayerTexts ();

  TextsDelegate *clone () const;

  virtual TextsIteratorDelegate *begin () const;
  virtual std::pair<geo::RecursiveShapeIterator, geo::ICplxTrans> begin_iter () const;

  virtual bool empty () const;

  virtual const geo::Text *nth (size_t n) const;
  virtual bool has_valid_texts () const;

  virtual const geo::RecursiveShapeIterator *iter () const;

  virtual bool equals (const Texts &other) const;
  virtual bool less (const Texts &other) const;

private:
  OriginalLayerTexts &operator= (const OriginalLayerTexts &other);

  mutable geo::RecursiveShapeIterator m_iter;
  geo::ICplxTrans m_iter_trans;

  void init ();
};

}

#endif

