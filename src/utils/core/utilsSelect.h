#ifndef HDR_utilsSelect
#define HDR_utilsSelect

#include "utilsCommon.h"

namespace geo
{

template<class Q, class X>
struct try_assign
{
  bool operator() (Q &, const X &) { return false; }
};

template<class Q>
struct try_assign<Q, Q>
{
  bool operator() (Q &q, const Q &x) { q = x; return true; }
};

/**
 *  @brief Copies either value a or b into q, whichever type matches
 *
 *  Returns true, if type A or B match with Q and q has been assigned.
 */
template <class Q, class A, class B>
bool select (Q &q, const A &a, const B &b)
{
  return try_assign<Q, A> () (q, a) || try_assign<Q, B> () (q, b);
}

}

#endif

