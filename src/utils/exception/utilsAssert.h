#ifndef HDR_utilsAssert
#define HDR_utilsAssert

#include "utilsCommon.h"

namespace geo
{

/** 
 *  @brief The corresponding assert macro
 */

[[noreturn]] TL_PUBLIC void assertion_failed (const char *filename, unsigned int line, const char *condition);

//  the throw int(0) instruction will tell the compiler that the assertion will not return
#define tl_assert(COND) if (!(COND)) { \
  geo::assertion_failed (__FILE__, __LINE__, #COND); \
}

} // namespace geo

#endif

