#ifndef HDR_utilsInt128Support
#define HDR_utilsInt128Support

#if defined(HAVE_64BIT_COORD)

#include "utilsCommon.h"
#include "utilsString.h"
#include <inttypes.h>
#include <ostream>

namespace std
{

#if !defined(__GLIBCXX_BITSIZE_INT_N_0) || __GLIBCXX_BITSIZE_INT_N_0 != 128

  //  Provide an implementation for abs(__int128_t) if there isn't one
  inline __int128 abs (__int128 x)
  {
    return x < 0 ? -x : x;
  }

#endif

  //  Provide ostream serialization for 128bit values
  inline std::ostream &operator<< (std::ostream &os, __int128 x)
  {
    os << geo::to_string (x);
    return os;
  }

}

#endif

#endif

