#ifndef HDR_utilsXMLWriter
#define HDR_utilsXMLWriter

#include "utilsCommon.h"

#include <iostream>

namespace geo
{

/**
 *  @brief A preliminary class for writing XML files
 */

class TL_PUBLIC XMLWriter 
{
public:
  XMLWriter (std::ostream &os);

  void start_document ();
  void start_document (const std::string &header);
  void start_element (const std::string &name);
  void write_attribute (const std::string &name, const std::string &value);
  void cdata (const std::string &text);
  void end_element (const std::string &name);
  void end_document ();

private:
  int m_indent;
  std::ostream &m_os;
  bool m_open;
  bool m_has_children;

  void write_indent ();
  void write_string (const std::string &s);
};

}

#endif

