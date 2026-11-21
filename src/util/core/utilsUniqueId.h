#ifndef HDR_utilsUniqueId
#define HDR_utilsUniqueId

#include "utilsCommon.h"
#include <cstdint>

namespace geo
{

typedef uint64_t id_type;

/**
 *  @brief An object delivering a unique ID per object
 *
 *  This class is supposed to provide a solution for the non-uniqueness
 *  issue of addresses. Instead of using addresses as keys, use the unique
 *  ID. The advantage is to be reproducible and guaranteed to be different
 *  for each allocation of an object (apart from overflow wrapping at 64bit -
 *  this this is pretty unlikely event).
 *
 *  To supply an ID, derive your class from geo::UniqueId. Use
 *  "geo::id_of (object *)" the get the ID from that object.
 *  A null-pointer will give an ID of 0 which is reserved for
 *  "nothing".
 *
 *  The type of the ID is geo::id_type.
 */
class TL_PUBLIC UniqueId
{
public:
  /**
   *  @brief @brief Creates a new object with a new ID
   */
  UniqueId ();

  UniqueId (const UniqueId &) { }
  UniqueId &operator= (const UniqueId &) { return *this; }

private:
  friend id_type id_of (const UniqueId *);

  id_type m_id;
};

/**
 *  @brief Gets the ID of the object pointed to by o
 *
 *  Returns 0 in a null pointer and only then.
 */
inline id_type id_of (const UniqueId *o)
{
  return o ? o->m_id : 0;
}

} // namespace geo

#endif

