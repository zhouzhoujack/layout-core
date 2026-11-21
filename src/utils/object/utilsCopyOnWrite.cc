#include "utilsCopyOnWrite.h"

namespace geo
{
// -----------------------------------------------------------------------------
//  CopyOnWrite implementation


geo::Mutex geo::CopyOnWritePtrBase::ms_lock;

}
