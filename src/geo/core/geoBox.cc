#include "geoBox.h"
#include "utilsString.h"
#include "utilsVariant.h"

namespace geo
{
// -----------------------------------------------------------------------------
//  Box implementation


template<> void extractor_impl (geo::Extractor &ex, geo::Box &b)
{
  if (! test_extractor_impl (ex, b)) {
    ex.error (geo::to_string (tr ("Expected an box specification")));
  }
}

template<> void extractor_impl (geo::Extractor &ex, geo::DBox &b)
{
  if (! test_extractor_impl (ex, b)) {
    ex.error (geo::to_string (tr ("Expected an box specification")));
  }
}

template<class C> bool _test_extractor_impl (geo::Extractor &ex, geo::box<C> &b)
{
  typedef geo::point<C> point_type;

  if (ex.test ("(")) {

    if (ex.test (")")) {
      b = geo::box<C> ();
    } else {

      point_type p1, p2;
      ex.read (p1);
      ex.expect (";");
      ex.read (p2);

      b = geo::box<C> (p1, p2);

      ex.expect (")");

    }

    return true;

  } else {
    return false;
  }
}

template<> bool test_extractor_impl (geo::Extractor &ex, geo::Box &b)
{
  return _test_extractor_impl (ex, b);
}

template<> bool test_extractor_impl (geo::Extractor &ex, geo::DBox &b)
{
  return _test_extractor_impl (ex, b);
}

}


