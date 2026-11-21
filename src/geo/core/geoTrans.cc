#include "geoTrans.h"
#include "utilsInternational.h"

// ----------------------------------------------------------------
//  Implementation of the custom extractors

namespace {

  template <class C>
  bool _test_extractor_impl (geo::Extractor &ex, geo::simple_trans<C> &t)
  {
    bool any = false;
    geo::FTrans f;
    geo::vector<C> p;
    while (true) {
      if (ex.try_read (f)) {
        any = true;
      } else if (ex.try_read (p)) {
        any = true;
      } else {
        if (any) {
          t = geo::simple_trans<C> (f.rot (), p);
        }
        return any;
      }
    }
  }

  template <class C>
  void _extractor_impl (geo::Extractor &ex, geo::simple_trans<C> &t)
  {
    if (! _test_extractor_impl (ex, t)) {
      ex.error (geo::to_string (tr ("Expected a transformation specification")));
    }
  }

  template <class C>
  bool _test_extractor_impl (geo::Extractor &ex, geo::disp_trans<C> &t)
  {
    bool any = false;
    geo::vector<C> p;
    while (true) {
      if (ex.try_read (p)) {
        any = true;
      } else {
        if (any) {
          t = geo::disp_trans<C> (p);
        }
        return any;
      }
    }
  }

  template <class C>
  void _extractor_impl (geo::Extractor &ex, geo::disp_trans<C> &t)
  {
    if (! _test_extractor_impl (ex, t)) {
      ex.error (geo::to_string (tr ("Expected a transformation specification")));
    }
  }

  template <class I, class F, class R>
  bool _test_extractor_impl (geo::Extractor &ex, geo::complex_trans<I, F, R> &t)
  {
    t = geo::complex_trans<I, F, R> ();
    bool any = false;
    while (true) {
      geo::vector<F> p;
      if (ex.test ("*")) {
        double f = 1.0;
        ex.read (f);
        t.mag (f);
        any = true;
      } else if (ex.try_read (p)) {
        t.disp (p);
        any = true;
      } else if (ex.test ("m")) {
        double a = 0.0;
        ex.read (a);
        t.mirror (true);
        t.angle (a * 2.0);
        any = true;
      } else if (ex.test ("r")) {
        double a = 0.0;
        ex.read (a);
        t.mirror (false);
        t.angle (a);
        any = true;
      } else {
        break;
      }
    }
    return any;
  }

  template <class I, class F, class R>
  void _extractor_impl (geo::Extractor &ex, geo::complex_trans<I, F, R> &t)
  {
    if (! _test_extractor_impl (ex, t)) {
      ex.error (geo::to_string (tr ("Expected transformation specification")));
    }
  }

  template <class C1, class C2>
  bool _test_extractor_impl (geo::Extractor &ex, geo::combined_trans<C1, C2> &t)
  {
    bool any = false;
    C1 t1;
    C2 t2;
    while (true) {
      if (ex.try_read (t1)) {
        any = true;
      } else if (ex.try_read (t2)) {
        any = true;
      } else {
        if (any) {
          t = geo::combined_trans<C1, C2> (t1, t2);
        }
        return any;
      }
    }
  }

  template <class C1, class C2>
  void _extractor_impl (geo::Extractor &ex, geo::combined_trans<C1, C2> &t)
  {
    if (! _test_extractor_impl (ex, t)) {
      ex.error (geo::to_string (tr ("Expected transformation/magnification specification")));
    }
  }

}

namespace geo
{

template <>
void DB_PUBLIC 
extractor_impl (geo::Extractor &ex, geo::FTrans &t)
{
  if (! test_extractor_impl (ex, t)) {
    ex.error (geo::to_string (tr ("Expected rotation/mirror code (r0,r90,r180,r270,m0,m45,m90,m135)")));
  }
}

template <>
void DB_PUBLIC 
extractor_impl (geo::Extractor &ex, geo::Trans &t)
{
  _extractor_impl (ex, t);
}

template <>
void DB_PUBLIC 
extractor_impl (geo::Extractor &ex, geo::DTrans &t)
{
  _extractor_impl (ex, t);
}

template <>
void DB_PUBLIC 
extractor_impl (geo::Extractor &ex, geo::Disp &t)
{
  _extractor_impl (ex, t);
}

template <>
void DB_PUBLIC 
extractor_impl (geo::Extractor &ex, geo::DDisp &t)
{
  _extractor_impl (ex, t);
}

template <>
void DB_PUBLIC 
extractor_impl (geo::Extractor &ex, geo::CplxTrans &t)
{
  _extractor_impl (ex, t);
}

template <>
DB_PUBLIC void 
extractor_impl (geo::Extractor &ex, geo::ICplxTrans &t)
{
  _extractor_impl (ex, t);
}

template <>
DB_PUBLIC void 
extractor_impl (geo::Extractor &ex, geo::DCplxTrans &t)
{
  _extractor_impl (ex, t);
}

template <>
DB_PUBLIC void
extractor_impl (geo::Extractor &ex, geo::VCplxTrans &t)
{
  _extractor_impl (ex, t);
}


template <>
DB_PUBLIC bool 
test_extractor_impl (geo::Extractor &ex, geo::FTrans &t)
{
  if (ex.test ("r0")) {
    t = geo::FTrans (geo::FTrans::r0);
    return true;
  } else if (ex.test ("r90")) {
    t = geo::FTrans (geo::FTrans::r90);
    return true;
  } else if (ex.test ("r180")) {
    t = geo::FTrans (geo::FTrans::r180);
    return true;
  } else if (ex.test ("r270")) {
    t = geo::FTrans (geo::FTrans::r270);
    return true;
  } else if (ex.test ("m0")) {
    t = geo::FTrans (geo::FTrans::m0);
    return true;
  } else if (ex.test ("m45")) {
    t = geo::FTrans (geo::FTrans::m45);
    return true;
  } else if (ex.test ("m90")) {
    t = geo::FTrans (geo::FTrans::m90);
    return true;
  } else if (ex.test ("m135")) {
    t = geo::FTrans (geo::FTrans::m135);
    return true;
  } else {
    return false;
  }
}

template <>
DB_PUBLIC bool 
test_extractor_impl (geo::Extractor &ex, geo::Trans &t)
{
  return _test_extractor_impl (ex, t);
}

template <>
DB_PUBLIC bool 
test_extractor_impl (geo::Extractor &ex, geo::DTrans &t)
{
  return _test_extractor_impl (ex, t);
}

template <>
DB_PUBLIC bool 
test_extractor_impl (geo::Extractor &ex, geo::Disp &t)
{
  return _test_extractor_impl (ex, t);
}

template <>
DB_PUBLIC bool 
test_extractor_impl (geo::Extractor &ex, geo::DDisp &t)
{
  return _test_extractor_impl (ex, t);
}

template <>
DB_PUBLIC bool 
test_extractor_impl (geo::Extractor &ex, geo::CplxTrans &t)
{
  return _test_extractor_impl (ex, t);
}

template <>
DB_PUBLIC bool 
test_extractor_impl (geo::Extractor &ex, geo::ICplxTrans &t)
{
  return _test_extractor_impl (ex, t);
}

template <>
DB_PUBLIC bool 
test_extractor_impl (geo::Extractor &ex, geo::DCplxTrans &t)
{
  return _test_extractor_impl (ex, t);
}

template <>
DB_PUBLIC bool
test_extractor_impl (geo::Extractor &ex, geo::VCplxTrans &t)
{
  return _test_extractor_impl (ex, t);
}

} // namespace geo

