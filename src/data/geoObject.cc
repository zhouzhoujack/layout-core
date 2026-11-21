#include "geoObject.h"

namespace geo
{
// -----------------------------------------------------------------------------
//  Object implementation

  Object::~Object ()
  {
    //  release from any manager
    manager (0);
  }
 
  Object::Object (Manager *m)
    : m_id (0), mp_manager (0)
  {
    //  attach to the manager if required
    manager (m);
  }

  Object::Object (const Object &d)
    : m_id (0), mp_manager (0)
  {
    //  attach to the manager if required
    manager (d.manager ());
  }

  void 
  Object::manager (geo::Manager *p_manager)
  {
    if (p_manager != mp_manager) {

      if (mp_manager) { 
        mp_manager->release_object (m_id);
      }

      if (p_manager) {
        mp_manager = p_manager;
        m_id = mp_manager->next_id (this);
      } else {
        mp_manager = 0;
        m_id = 0;
      }

    }
  }

}

