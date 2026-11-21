#include "geoWriter.h"

#include "geoStream.h"

#include "utilsClassRegistry.h"
#include "utilsAssert.h"
#include "utilsStream.h"
#include "utilsTimer.h"
#include "utilsLog.h"

namespace geo
{

// -----------------------------------------------------------------------------
//  Writer implementation

Writer::Writer (const geo::SaveLayoutOptions &options)
  : mp_writer (0), m_options (options)
{
  for (geo::Registrar<geo::StreamFormatDeclaration>::iterator fmt = geo::Registrar<geo::StreamFormatDeclaration>::begin (); fmt != geo::Registrar<geo::StreamFormatDeclaration>::end () && ! mp_writer; ++fmt) {
    if (m_options.format () == fmt->format_name ()) {
      mp_writer = fmt->create_writer ();
    }
  }
  if (! mp_writer) {
    throw geo::Exception (geo::to_string (tr ("Unknown stream format: %s")), m_options.format ());
  }
}

Writer::~Writer ()
{
  if (mp_writer) {
    delete mp_writer;
  }
  mp_writer = 0;
}

void 
Writer::write (geo::Layout &layout, geo::OutputStream &stream)
{
  geo::SelfTimer timer (geo::verbosity () >= 21, geo::to_string (tr ("Writing file: ")) + stream.path ());

  tl_assert (mp_writer != 0);
  mp_writer->write (layout, stream, m_options);
}

}

