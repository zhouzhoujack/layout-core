#include "geoLoadLayoutOptions.h"
#include "geoStream.h"
#include "utilsClassRegistry.h"
#include "utilsStream.h"
#include "utilsExpression.h"

namespace geo
{
// -----------------------------------------------------------------------------
//  LoadLayoutOptions implementation

  LoadLayoutOptions::LoadLayoutOptions ()
  {
    // .. nothing yet ..
  }

  LoadLayoutOptions::LoadLayoutOptions (const LoadLayoutOptions &d)
  {
    operator= (d);
  }

  LoadLayoutOptions &
  LoadLayoutOptions::operator= (const LoadLayoutOptions &d)
  {
    if (&d != this) {

      release ();
      for (std::map <std::string, FormatSpecificReaderOptions *>::const_iterator o = d.m_options.begin (); o != d.m_options.end (); ++o) {
        m_options.insert (std::make_pair (o->first, o->second->clone ()));
      }

    }
    return *this;
  }

  LoadLayoutOptions::~LoadLayoutOptions ()
  {
    release (); 
  }

  void 
  LoadLayoutOptions::release ()
  {
    for (std::map <std::string, FormatSpecificReaderOptions *>::const_iterator o = m_options.begin (); o != m_options.end (); ++o) {
      delete o->second;
    }
    m_options.clear ();
  }

  void 
  LoadLayoutOptions::set_options (const FormatSpecificReaderOptions &options)
  {
    set_options (options.clone ());
  }

  void 
  LoadLayoutOptions::set_options (FormatSpecificReaderOptions *options)
  {
    std::map <std::string, FormatSpecificReaderOptions *>::iterator o = m_options.find (options->format_name ());
    if (o != m_options.end ()) {
      delete o->second;
      m_options.erase (o);
    }

    m_options.insert (std::make_pair (options->format_name (), options));
  }

  const FormatSpecificReaderOptions *
  LoadLayoutOptions::get_options (const std::string &format) const
  {
    std::map <std::string, FormatSpecificReaderOptions *>::const_iterator o = m_options.find (format);
    if (o != m_options.end ()) {
      return o->second;
    } else {
      return 0;
    }
  }

  FormatSpecificReaderOptions *
  LoadLayoutOptions::get_options (const std::string &format)
  {
    std::map <std::string, FormatSpecificReaderOptions *>::const_iterator o = m_options.find (format);
    if (o != m_options.end ()) {
      return o->second;
    } else {
      return 0;
    }
  }

  void
  LoadLayoutOptions::set_option_by_method (const std::string &method, const geo::Variant &value)
  {
    //  Utilizes the GSI binding to set the values
    geo::Variant ref = geo::Variant::make_variant_ref (this);

    geo::Extractor ex (method.c_str ());

    while (! ex.at_end ()) {

      std::string m;
      ex.read_word (m, "_=");
      if (! ex.at_end ()) {
        ex.expect (".");
      }

      geo::Variant out;

      std::vector<geo::Variant> args;
      if (ex.at_end ()) {
        args.push_back (value);
      }
      geo::ExpressionParserContext context;
      ref.user_cls ()->eval_cls ()->execute (context, out, ref, m, args);

      ref = out;

    }
  }

  geo::Variant
  LoadLayoutOptions::get_option_by_method (const std::string &method)
  {
    //  Utilizes the GSI binding to set the values
    geo::Variant ref = geo::Variant::make_variant_ref (this);

    geo::Extractor ex (method.c_str ());

    while (! ex.at_end ()) {

      std::string m;
      ex.read_word (m, "_=");
      if (! ex.at_end ()) {
        ex.expect (".");
      }

      geo::Variant out;

      std::vector<geo::Variant> args;
      geo::ExpressionParserContext context;
      ref.user_cls ()->eval_cls ()->execute (context, out, ref, m, args);

      ref = out;

    }

    return ref;
  }

  void
  LoadLayoutOptions::set_option_by_name (const std::string &method, const geo::Variant &value)
  {
    return set_option_by_method (method + "=", value);
  }

  geo::Variant
  LoadLayoutOptions::get_option_by_name (const std::string &method)
  {
    return get_option_by_method (method);
  }
}

