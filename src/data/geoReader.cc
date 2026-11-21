#include "geoReader.h"
#include "geoStream.h"
#include "utilsClassRegistry.h"
#include "utilsTimer.h"
#include "utilsLog.h"

namespace geo
{

// ---------------------------------------------------------------

void
join_layer_names (std::string &s, const std::string &n)
{
  if (s == n) {
    return;
  }

  if (! s.empty ()) {

    size_t i = s.find (n);
    if (i != std::string::npos && (i == 0 || s.c_str ()[i - 1] == ';')) {
      char after = s.c_str ()[i + n.size ()];
      if (after == 0 || after == ';') {
        //  n is already contained in s
        return;
      }
    }

    s += ";";

  }

  s += n;
}

// ---------------------------------------------------------------
//  ReaderBase implementation

ReaderBase::ReaderBase () 
  : m_warnings_as_errors (false)
{ 
}

ReaderBase::~ReaderBase () 
{ 
}

void
ReaderBase::set_warnings_as_errors (bool f)
{
  m_warnings_as_errors = f;
}

// ---------------------------------------------------------------
//  Reader implementation

Reader::Reader (geo::InputStream &stream)
  : mp_actual_reader (0), m_stream (stream)
{
  //  Detect the format by asking all reader declarations
  for (geo::Registrar<geo::StreamFormatDeclaration>::iterator rdr = geo::Registrar<geo::StreamFormatDeclaration>::begin (); rdr != geo::Registrar<geo::StreamFormatDeclaration>::end () && ! mp_actual_reader; ++rdr) {
    m_stream.reset ();
    if (rdr->detect (m_stream)) {
      m_stream.reset ();
      mp_actual_reader = rdr->create_reader (m_stream);
    }
  }

  if (! mp_actual_reader) {
    throw geo::ReaderException (geo::to_string (tr ("Stream has unknown format: ")) + stream.source ());
  }
}

Reader::~Reader ()
{
  if (mp_actual_reader) {
    delete mp_actual_reader;
    mp_actual_reader = 0;
  }
}

const geo::LayerMap &
Reader::read (geo::Layout &layout, const geo::LoadLayoutOptions &options)
{
  geo::SelfTimer timer (geo::verbosity () >= 21, geo::to_string (tr ("Reading file: ")) + m_stream.source ());

  return mp_actual_reader->read (layout, options);
}

const geo::LayerMap &
Reader::read (geo::Layout &layout)
{
  geo::SelfTimer timer (geo::verbosity () >= 21, geo::to_string (tr ("Reading file: ")) + m_stream.source ());

  return mp_actual_reader->read (layout);
}

}

