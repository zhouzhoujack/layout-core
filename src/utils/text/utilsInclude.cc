#include "utilsInclude.h"

#include "utilsAssert.h"
#include "utilsString.h"
#include "utilsFileUtils.h"
#include "utilsStream.h"
#include "utilsExpression.h"
#include "utilsUri.h"

namespace geo
{

static const char *valid_fn_chars = "@_:,.\\/-+";

// -----------------------------------------------------------------------------
//  Include implementation

IncludeExpander::IncludeExpander ()
{
  //  .. nothing yet ..
}

IncludeExpander
IncludeExpander::expand (const std::string &path, std::string &expanded_text)
{
  IncludeExpander ie;
  int lc = 1;
  geo::InputStream is (path);
  ie.read (path, is, expanded_text, ie, lc);
  return ie;
}

IncludeExpander
IncludeExpander::expand (const std::string &path, const std::string &original_text, std::string &expanded_text)
{
  IncludeExpander ie;
  int lc = 1;
  geo::InputMemoryStream ms (original_text.c_str (), original_text.size ());
  geo::InputStream is (ms);
  ie.read (path, is, expanded_text, ie, lc);
  return ie;
}

void
IncludeExpander::read (const std::string &path, geo::InputStream &is, std::string &expanded_text, IncludeExpander &ie, int &line_counter)
{
  ie.m_sections [line_counter] = std::make_pair (path, 1 - line_counter);

  geo::TextInputStream text (is);

  int lnum = 0;
  bool emit_section = false;

  while (! text.at_end ()) {

    std::string l = text.get_line ();
    ++lnum;

    geo::Extractor ex (l.c_str ());
    if (ex.test ("#") && ex.test ("%include")) {

      std::string include_path;
      if (*ex.skip () == '"' || *ex.skip () == '\'') {
        ex.read_quoted (include_path);
        ex.expect_end ();
      } else {
        include_path = geo::trim (ex.skip ());
      }

      //  allow some customization by employing expression interpolation
      include_path = geo::Eval ().interpolate (include_path);

      //  NOTE: by using URI's we can basically read from HTTP etc.
      geo::URI current_uri (path);
      geo::URI new_uri (include_path);
      if (current_uri.scheme ().empty () && new_uri.scheme ().empty ()) {
        if (! geo::is_absolute (include_path)) {
          include_path = geo::combine_path (geo::dirname (path), include_path);
        }
      } else {
        include_path = current_uri.resolved (new_uri).to_abstract_path ();
      }

      geo::InputStream is (include_path);
      read (include_path, is, expanded_text, ie, line_counter);

      emit_section = true;

    } else {

      if (emit_section) {
        emit_section = false;
        ie.m_sections [line_counter] = std::make_pair (path, lnum - line_counter);
      }

      expanded_text += l;
      expanded_text += "\n";
      ++line_counter;

    }

  }
}

std::string
IncludeExpander::to_string () const
{
  if (m_sections.empty ()) {

    return std::string ();

  } else if (m_sections.size () == 1) {

    tl_assert (m_sections.begin ()->first == 1);
    tl_assert (m_sections.begin ()->second.second == 0);

    std::string fn = m_sections.begin ()->second.first;
    return geo::to_word_or_quoted_string (fn, valid_fn_chars);

  } else {

    //  "@" indicates a mapping table
    std::string res ("@");

    for (std::map<int, std::pair<std::string, int> >::const_iterator m = m_sections.begin (); m != m_sections.end (); ++m) {
      res += geo::to_string (m->first);
      res += "*";
      res += geo::to_word_or_quoted_string (m->second.first, valid_fn_chars);
      res += "*";
      res += geo::to_string (m->second.second);
      res += ";";
    }

    return res;

  }
}

IncludeExpander
IncludeExpander::from_string (const std::string &s)
{
  IncludeExpander ie;

  geo::Extractor ex (s.c_str ());

  if (*ex == '"' || *ex == '\'') {

    ex.read_quoted (ie.m_sections [1].first);

  } else if (ex.test ("@")) {

    while (! ex.at_end ()) {

      int ln = 0;
      ex.read (ln);

      std::pair<std::string, int> &si = ie.m_sections [ln];

      ex.expect ("*");
      ex.read_word_or_quoted (si.first, valid_fn_chars);
      ex.expect ("*");
      ex.read (si.second);

      ex.test (";");

    }

  } else {

    ie.m_sections [1].first = s;

  }

  return ie;
}

std::pair<std::string, int>
IncludeExpander::translate_to_original (int line_number)
{
  std::map<int, std::pair<std::string, int> >::const_iterator m = m_sections.lower_bound (line_number);
  if (m != m_sections.begin () && (m == m_sections.end () || m->first > line_number)) {
    --m;
  }
  if (m == m_sections.end ()) {
    return std::make_pair (std::string (), 0);
  } else {
    return std::make_pair (m->second.first, line_number + m->second.second);
  }
}

}
