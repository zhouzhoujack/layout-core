#include "utilsStaticObjects.h"

namespace geo
{

StaticObjects StaticObjects::ms_instance;

// -----------------------------------------------------------------------------
//  StaticObjects implementation

StaticObjects::~StaticObjects ()
{
  //  nothing here. That is done too late. Use destroy() instead.
}

void
StaticObjects::register_object_base (StaticObjectReferenceBase *o)
{
  m_objects.push_back (o);
}

void
StaticObjects::do_cleanup ()
{
  //  destroy the objects the opposite order they were created
  for (std::vector<StaticObjectReferenceBase *>::iterator o = m_objects.end (); o != m_objects.begin (); ) {
    --o;
    delete (*o);
  }
  m_objects.clear ();
}

}

