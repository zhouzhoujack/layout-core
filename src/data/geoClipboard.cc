#include "geoClipboard.h"

namespace geo
{

Clipboard Clipboard::m_instance;

// -----------------------------------------------------------------------------
//  Clipboard implementation

Clipboard::Clipboard ()
{
  // .. nothing yet ..
}

Clipboard::~Clipboard ()
{
  clear ();
}

Clipboard &
Clipboard::operator+= (ClipboardObject *object)
{
  m_objects.push_back (object);
  return *this;
}

void 
Clipboard::clear ()
{
  for (iterator o = begin (); o != end (); ++o) {
    ClipboardObject *p = const_cast<ClipboardObject *> (*o);
    delete p;
  }
  m_objects.clear ();
}

}

