#include "geoVector.h"

// ----------------------------------------------------------------
//  Implementation of the custom extractors

namespace {

  template <class C>
  bool _test_extractor_impl (geo::Extractor &ex, geo::vector<C> &p)
  {
    //  TODO: this is not really a "test": we should move back if we did not
    //  receive a ",".
    C x = 0;
    if (ex.try_read (x)) {
      ex.expect (",");
      C y = 0;
      ex.read (y);
      p = geo::vector<C> (x, y);
      return true;
    } else {
      return false;
    }
  }

  template <class C>
  void _extractor_impl (geo::Extractor &ex, geo::vector<C> &p)
  {
    if (! _test_extractor_impl (ex, p)) {
      ex.error (geo::to_string (tr ("Expected a vector specification")));
    }
  }

}

namespace geo 
{
  
template <> 
void 
extractor_impl (geo::Extractor &ex, geo::Vector &p)
{
  _extractor_impl (ex, p);
}

template <> 
void 
extractor_impl (geo::Extractor &ex, geo::DVector &p)
{
  _extractor_impl (ex, p);
}

template <> 
bool 
test_extractor_impl (geo::Extractor &ex, geo::Vector &p)
{
  return _test_extractor_impl (ex, p);
}

template <> 
bool 
test_extractor_impl (geo::Extractor &ex, geo::DVector &p)
{
  return _test_extractor_impl (ex, p);
}

} // namespace geo

