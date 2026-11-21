#ifndef HDR_geoMutableRegion
#define HDR_geoMutableRegion

#include "geoCommon.h"

#include "geoAsIfFlatRegion.h"

#include <set>

namespace geo {

/**
 *  @brief An interface representing mutable regions
 *
 *  Mutable regions offer insert, transform, flatten and other manipulation functions.
 */
class DB_PUBLIC MutableRegion
  : public AsIfFlatRegion
{
public:
  MutableRegion ();
  MutableRegion (const MutableRegion &other);
  virtual ~MutableRegion ();

  virtual void do_insert (const geo::Polygon &polygon) = 0;

  void transform (const geo::UnitTrans &) { }
  void transform (const geo::Disp &t) { do_transform (geo::Trans (t)); }
  void transform (const geo::Trans &t) { do_transform (t); }
  void transform (const geo::ICplxTrans &t) { do_transform (t); }
  void transform (const geo::IMatrix2d &t) { do_transform (t); }
  void transform (const geo::IMatrix3d &t) { do_transform (t); }

  virtual void do_transform (const geo::Trans &t) = 0;
  virtual void do_transform (const geo::ICplxTrans &t) = 0;
  virtual void do_transform (const geo::IMatrix2d &t) = 0;
  virtual void do_transform (const geo::IMatrix3d &t) = 0;

  virtual void flatten () = 0;

  virtual void reserve (size_t n) = 0;

  void insert (const geo::Polygon &polygon) { do_insert (polygon); }
  void insert (const geo::Box &box);
  void insert (const geo::Path &path);
  void insert (const geo::SimplePolygon &polygon);

  void insert (const geo::Shape &shape);

  template <class T>
  void insert (const geo::Shape &shape, const T &trans)
  {
    if (shape.is_polygon () || shape.is_path () || shape.is_box ()) {
      geo::Polygon poly;
      shape.polygon (poly);
      poly.transform (trans);
      insert (poly);
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

