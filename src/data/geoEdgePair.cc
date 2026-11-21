#include "geoEdgePair.h"

namespace geo
{
// -----------------------------------------------------------------------------
//  EdgePair implementation


template<> void extractor_impl (geo::Extractor &ex, geo::EdgePair &e)
{
  if (! test_extractor_impl (ex, e)) {
    ex.error (geo::to_string (tr ("Expected an edge specification")));
  }
}

template<> void extractor_impl (geo::Extractor &ex, geo::DEdgePair &e)
{
  if (! test_extractor_impl (ex, e)) {
    ex.error (geo::to_string (tr ("Expected an edge specification")));
  }
}

template<class C> bool _test_extractor_impl (geo::Extractor &ex, geo::edge_pair<C> &e)
{
  typedef geo::edge<C> edge_type;
  geo::Extractor ex_saved = ex;

  edge_type e1, e2;

  if (ex.try_read (e1)) {

    bool symmetric = false;
    if (ex.test ("|")) {
      symmetric = true;
    } else if (! ex.test ("/")) {
      ex = ex_saved;
      return false;
    }

    if (! ex.try_read (e2)) {
      ex = ex_saved;
      return false;
    }

    e = geo::edge_pair<C> (e1, e2, symmetric);
    return true;

  } else {
    return false;
  }
}

template<> bool test_extractor_impl (geo::Extractor &ex, geo::EdgePair &e)
{
  return _test_extractor_impl (ex, e);
}

template<> bool test_extractor_impl (geo::Extractor &ex, geo::DEdgePair &e)
{
  return _test_extractor_impl (ex, e);
}

} // namespace geo

