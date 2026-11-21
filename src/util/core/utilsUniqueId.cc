#include "utilsUniqueId.h"
#include "utilsThreads.h"

namespace geo
{

static geo::Mutex s_lock;
static id_type s_next_id = 0;

// -----------------------------------------------------------------------------
//  UniqueId implementation

UniqueId::UniqueId ()
{
  geo::MutexLocker locker (&s_lock);
  do {
    m_id = ++s_next_id;
  } while (m_id == 0);
}

} // namespace geo


