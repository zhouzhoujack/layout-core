#include "geoText.h"

namespace geo
{

// -----------------------------------------------------------------------------
//  Text implementation

StringRef::~StringRef ()
{
  if (mp_rep) {
    mp_rep->unregister_ref (this);
  }
}

}

namespace geo
{

template<> void extractor_impl (geo::Extractor &ex, geo::Text &p)
{
  if (! test_extractor_impl (ex, p)) {
    ex.error (geo::to_string (tr ("Expected a text specification")));
  }
}

template<> void extractor_impl (geo::Extractor &ex, geo::DText &p)
{
  if (! test_extractor_impl (ex, p)) {
    ex.error (geo::to_string (tr ("Expected a text specification")));
  }
}


template<class C> bool _test_extractor_impl (geo::Extractor &ex, geo::text<C> &t)
{
  if (ex.test ("(")) {

    std::string s;
    ex.read_word_or_quoted (s);
    t.string (s);

    ex.expect (",");

    typename geo::text<C>::trans_type tt;
    ex.read (tt);
    t.trans (tt);

    ex.expect (")");

    return true;

  } else {
    return false;
  }
}

template<> bool test_extractor_impl (geo::Extractor &ex, geo::Text &p)
{
  return _test_extractor_impl (ex, p);
}

template<> bool test_extractor_impl (geo::Extractor &ex, geo::DText &p)
{
  return _test_extractor_impl (ex, p);
}

}

