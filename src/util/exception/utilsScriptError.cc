#include "utilsScriptError.h"
#include "utilsString.h"
#include "utilsInclude.h"

namespace geo
{

// -------------------------------------------------------------------
//  BacktraceElement implementation

BacktraceElement::BacktraceElement (const std::string &_file, int _line)
  : file (_file), line (_line)
{
  translate_includes ();
}

BacktraceElement::BacktraceElement (const std::string &_file, int _line, const std::string _more_info)
  : file (_file), line (_line), more_info (_more_info)
{
  translate_includes ();
}

BacktraceElement::BacktraceElement ()
  : line (0)
{
  // .. nothing yet ..
}

void
BacktraceElement::translate_includes ()
{
  if (line < 1) {
    return;
  }

  std::pair<std::string, int> fl = geo::IncludeExpander::translate_to_original (file, line);
  if (fl.second > 0) {
    file = fl.first;
    line = fl.second;
  }
}

std::string 
BacktraceElement::to_string() const
{
  if (line > 0) {
    if (! more_info.empty ()) {
      return file + ":" + geo::to_string (line) + ":" + more_info;
    } else {
      return file + ":" + geo::to_string (line);
    }
  } else {
    return more_info;
  }
}

// -------------------------------------------------------------------
//  ScriptError implementation

ScriptError::ScriptError (const char *msg, const char *cls, const std::vector<BacktraceElement> &backtrace)
  : geo::Exception (msg), m_line (-1), m_cls (cls), m_backtrace (backtrace)
{
  //  .. nothing yet ..
}

ScriptError::ScriptError (const char *msg, const char *sourcefile, int line, const char *cls, const std::vector<BacktraceElement> &backtrace)
  : geo::Exception (msg), m_sourcefile (sourcefile), m_line (line), m_cls (cls), m_backtrace (backtrace)
{
  translate_includes ();
}

ScriptError::ScriptError (const ScriptError &d)
  : geo::Exception (d), m_sourcefile (d.m_sourcefile), m_line (d.m_line), m_cls (d.m_cls), m_context (d.m_context), m_backtrace (d.m_backtrace)
{
  //  .. nothing yet ..
}

std::string
ScriptError::basic_msg () const
{
  return geo::Exception::msg ();
}

std::string
ScriptError::msg () const
{
  std::string m = basic_msg ();

  if (! m_context.empty ()) {
      m += geo::to_string (tr (" in ")) + m_context;
    }

  for (std::vector<BacktraceElement>::const_iterator bt = backtrace ().begin (); bt != backtrace ().end (); ++bt) {
    m += "\n  ";
    m += bt->to_string ();
  }

  return m;
}

void
ScriptError::translate_includes ()
{
  if (m_line < 1) {
    return;
  }

  std::pair<std::string, int> fl = geo::IncludeExpander::translate_to_original (m_sourcefile, m_line);
  if (fl.second > 0) {
    m_sourcefile = fl.first;
    m_line = fl.second;
  }
}

}


