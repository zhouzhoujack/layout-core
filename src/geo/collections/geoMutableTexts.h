#ifndef HDR_geoMutableTexts
#define HDR_geoMutableTexts

#include "geoCommon.h"

#include "geoAsIfFlatTexts.h"

#include <set>

namespace geo {

/**
 *  @brief An interface representing mutable text collections 
 *
 *  Mutable text collections offer insert, transform, flatten and other manipulation functions.
 */
class DB_PUBLIC MutableTexts
  : public AsIfFlatTexts
{
public:
  MutableTexts ();
  MutableTexts (const MutableTexts &other);
  virtual ~MutableTexts ();

  virtual void do_insert (const geo::Text &text) = 0;

  virtual void do_transform (const geo::Trans &t) = 0;
  virtual void do_transform (const geo::ICplxTrans &t) = 0;
  virtual void do_transform (const geo::IMatrix3d &t) = 0;
  virtual void do_transform (const geo::IMatrix2d &t) = 0;

  virtual void flatten () = 0;

  virtual void reserve (size_t n) = 0;

  void transform (const geo::UnitTrans &) { }
  void transform (const geo::Disp &t) { do_transform (geo::Trans (t)); }
  void transform (const geo::Trans &t) { do_transform (t); }
  void transform (const geo::ICplxTrans &t) { do_transform (t); }
  void transform (const geo::IMatrix3d &t) { do_transform (t); }
  void transform (const geo::IMatrix2d &t) { do_transform (t); }

  void insert (const geo::Text &text) { do_insert (text); }
  void insert (const geo::Shape &shape);

  template <class T>
  void insert (const geo::Shape &shape, const T &trans)
  {
    if (shape.is_text ()) {
      geo::Text text = shape.text ();
      text.transform (trans);
      insert (text);
    }
  }

  template <class Iter>
  void insert (const Iter &b, const Iter &e)
  {
    reserve (count () + (e - b));
    for (Iter i = b; i != e; ++i) {
      insert (*i);
    }
  }

  template <class Iter>
  void insert_seq (const Iter &seq)
  {
    for (Iter i = seq; ! i.at_end (); ++i) {
      insert (*i);
    }
  }
};

}

#endif

