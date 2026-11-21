#include "geoCommon.h"

#include "geoTexts.h"
#include "geoEmptyTexts.h"
#include "geoFlatTexts.h"
#include "geoDeepTexts.h"
#include "geoOriginalLayerTexts.h"
#include "geoEdges.h"
#include "geoRegion.h"

#include "utilsVariant.h"

#include <sstream>

namespace geo
{

// -----------------------------------------------------------------------------
//  Texts implementation

Texts::Texts ()
  : mp_delegate (new EmptyTexts ())
{
  //  .. nothing yet ..
}

Texts::~Texts ()
{
  delete mp_delegate;
  mp_delegate = 0;
}

Texts::Texts (TextsDelegate *delegate)
  : mp_delegate (delegate)
{
  //  .. nothing yet ..
}

Texts::Texts (const Texts &other)
  : geo::ShapeCollection (), mp_delegate (other.mp_delegate->clone ())
{
  //  .. nothing yet ..
}

Texts &Texts::operator= (const Texts &other)
{
  if (this != &other) {
    set_delegate (other.mp_delegate->clone ());
  }
  return *this;
}

Texts::Texts (const RecursiveShapeIterator &si)
{
  mp_delegate = new OriginalLayerTexts (si);
}

Texts::Texts (const RecursiveShapeIterator &si, const geo::ICplxTrans &trans)
{
  mp_delegate = new OriginalLayerTexts (si, trans);
}

Texts::Texts (const RecursiveShapeIterator &si, DeepShapeStore &dss)
{
  mp_delegate = new DeepTexts (si, dss);
}

Texts::Texts (const RecursiveShapeIterator &si, DeepShapeStore &dss, const geo::ICplxTrans &trans)
{
  mp_delegate = new DeepTexts (si, dss, trans);
}

template <class Sh>
void Texts::insert (const Sh &shape)
{
  mutable_texts ()->insert (shape);
}

template DB_PUBLIC void Texts::insert (const geo::Text &);

void Texts::insert (const geo::Shape &shape)
{
  mutable_texts ()->insert (shape);
}

template <class T>
void Texts::insert (const geo::Shape &shape, const T &trans)
{
  mutable_texts ()->insert (shape, trans);
}

template DB_PUBLIC void Texts::insert (const geo::Shape &, const geo::ICplxTrans &);
template DB_PUBLIC void Texts::insert (const geo::Shape &, const geo::Trans &);
template DB_PUBLIC void Texts::insert (const geo::Shape &, const geo::Disp &);
template DB_PUBLIC void Texts::insert (const geo::Shape &, const geo::IMatrix2d &);
template DB_PUBLIC void Texts::insert (const geo::Shape &, const geo::IMatrix3d &);

void Texts::clear ()
{
  set_delegate (new EmptyTexts ());
}

void Texts::reserve (size_t n)
{
  mutable_texts ()->reserve (n);
}

void Texts::flatten ()
{
  mutable_texts ()->flatten ();
}

template <class T>
Texts &Texts::transform (const T &trans)
{
  mutable_texts ()->transform (trans);
  return *this;
}

//  explicit instantiations
template DB_PUBLIC Texts &Texts::transform (const geo::ICplxTrans &);
template DB_PUBLIC Texts &Texts::transform (const geo::Trans &);
template DB_PUBLIC Texts &Texts::transform (const geo::Disp &);

const geo::RecursiveShapeIterator &
Texts::iter () const
{
  static geo::RecursiveShapeIterator def_iter;
  const geo::RecursiveShapeIterator *i = mp_delegate->iter ();
  return *(i ? i : &def_iter);
}

void Texts::polygons (Region &output, geo::Coord e) const
{
  output.set_delegate (mp_delegate->polygons (e));
}

void Texts::edges (Edges &output) const
{
  output.set_delegate (mp_delegate->edges ());
}

void Texts::set_delegate (TextsDelegate *delegate)
{
  if (delegate != mp_delegate) {
    delete mp_delegate;
    mp_delegate = delegate;
  }
}

MutableTexts *Texts::mutable_texts ()
{
  MutableTexts *texts = dynamic_cast<MutableTexts *> (mp_delegate);
  if (! texts) {
    texts = new FlatTexts ();
    if (mp_delegate) {
      texts->TextsDelegate::operator= (*mp_delegate);
      texts->insert_seq (begin ());
    }
    set_delegate (texts);
  }

  return texts;
}

void Texts::processed (Region &output, const TextToPolygonProcessorBase &filter) const
{
  output = Region (mp_delegate->processed_to_polygons (filter));
}

void Texts::pull_interacting (Region &output, const Region &other) const
{
  output = Region (mp_delegate->pull_interacting (other));
}

}

namespace geo
{
  template<> DB_PUBLIC bool test_extractor_impl (geo::Extractor &ex, geo::Texts &b)
  {
    geo::Text ep;

    if (ex.at_end ()) {
      return true;
    }
    if (! ex.try_read (ep)) {
      return false;
    }
    b.insert (ep);

    while (ex.test (";")) {
      ex.read (ep);
      b.insert (ep);
    } 

    return true;
  }

  template<> DB_PUBLIC void extractor_impl (geo::Extractor &ex, geo::Texts &b)
  {
    if (! test_extractor_impl (ex, b)) {
      ex.error (geo::to_string (tr ("Expected an edge pair collection specification")));
    }
  }
}

