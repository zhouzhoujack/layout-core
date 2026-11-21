#include "geoUserObject.h"

#include "utilsClassRegistry.h"

namespace geo
{
// -----------------------------------------------------------------------------
//  UserObject implementation

  static unsigned int s_class_id = 1;

  unsigned int 
  get_unique_user_object_class_id ()
  {
    return s_class_id++;
  }
}


