#include "utilsException.h"
#include "utilsString.h"

namespace geo
{

// -----------------------------------------------------------------------------
//  Exception implementation

void
Exception::init (const std::string &fmt, const std::vector<geo::Variant> &a)
{ 
  m_msg = geo::sprintf (fmt, a);
}

}

