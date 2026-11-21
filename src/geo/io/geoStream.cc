#include "geoStream.h"
#include "utilsClassRegistry.h"
#include "utilsXMLParser.h"

#include <string.h>

namespace geo
{

// ------------------------------------------------------------------
//  Implementation of load_options_xml_element_list


geo::XMLElementList load_options_xml_element_list ()
{
  geo::XMLElementList elements;

  for (geo::Registrar<geo::StreamFormatDeclaration>::iterator cls = geo::Registrar<geo::StreamFormatDeclaration>::begin (); cls != geo::Registrar<geo::StreamFormatDeclaration>::end (); ++cls) {
    const geo::StreamFormatDeclaration *decl = dynamic_cast <const geo::StreamFormatDeclaration *> (&*cls);
    if (decl) {
      elements.append (decl->xml_reader_options_element ());
    }
  }

  return elements;
}

geo::XMLElementList save_options_xml_element_list ()
{
  geo::XMLElementList elements;

  for (geo::Registrar<geo::StreamFormatDeclaration>::iterator cls = geo::Registrar<geo::StreamFormatDeclaration>::begin (); cls != geo::Registrar<geo::StreamFormatDeclaration>::end (); ++cls) {
    const StreamFormatDeclaration *decl = dynamic_cast <const StreamFormatDeclaration *> (&*cls);
    if (decl) {
      elements.append (decl->xml_writer_options_element ());
    }
  }

  return elements;
}

}


