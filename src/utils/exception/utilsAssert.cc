#include "utilsAssert.h"
#include "utilsException.h"
#include "utilsLog.h"

namespace geo
{
// -----------------------------------------------------------------------------
//  Assert implementation


//  to allow setting a debug breakpoint to this function, we declare it "C"
extern "C"
void _assertion_failed (const char *filename, unsigned int line, const char *condition)
{
  geo::error << filename << "," << line << "," << condition; 
}

void 
assertion_failed (const char *filename, unsigned int line, const char *condition)
{
  _assertion_failed (filename, line, condition);
  throw geo::InternalException (filename, line, condition); 
}

}

