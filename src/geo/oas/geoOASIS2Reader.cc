#include "geoOASIS2Reader.h"
#include "geoOASIS2.h"

#include "utilsException.h"
#include "utilsGLog.h"
#include "utilsString.h"

#include <cstring>

namespace geo
{

// -----------------------------------------------------------------------------
//  OASIS2Reader implementation

OASIS2Reader::OASIS2Reader (geo::InputStream &s)
  : m_stream (s),
    m_progress (geo::to_string (tr ("Reading OASIS file")), 10000)
{
  m_progress.set_format (geo::to_string (tr ("%.0f MB")));
  m_progress.set_unit (1024 * 1024);
}

OASIS2Reader::~OASIS2Reader ()
{
  //  .. nothing yet ..
}

void
OASIS2Reader::init (const geo::LoadLayoutOptions &options)
{
  OASIS2ReaderBase::init (options);
}

unsigned char
OASIS2Reader::get_byte ()
{
  const char *b = m_stream.get (1);
  if (! b) {
    error (geo::to_string (tr ("Unexpected end-of-file")));
    return 0;
  }
  return (unsigned char) *b;
}

uint64_t
OASIS2Reader::get_uint ()
{
  uint64_t v = 0;
  uint64_t vm = 1;
  unsigned char c;

  do {

    c = get_byte ();
    v += uint64_t (c & 0x7f) * vm;
    vm <<= 7;

    if (vm == 0) {
      error (geo::to_string (tr ("Unsigned integer value overflow")));
      return 0;
    }

  } while ((c & 0x80) != 0);

  return v;
}

int64_t
OASIS2Reader::get_int ()
{
  uint64_t u = get_uint ();
  if ((u & 1) != 0) {
    return -int64_t (u >> 1);
  } else {
    return int64_t (u >> 1);
  }
}

double
OASIS2Reader::get_real ()
{
  uint64_t t = get_uint ();

  if (t == rZERO) {

    return double (get_uint ());

  } else if (t == rZERO_NEG) {

    return -double (get_uint ());

  } else if (t == rRECIPROCAL || t == rRECIPROCAL_NEG) {

    uint64_t d = get_uint ();
    if (d == 0) {
      error (geo::to_string (tr ("Divider must not be zero")));
      return 0.0;
    }
    return (t == rRECIPROCAL ? 1.0 : -1.0) / double (d);

  } else if (t == rRATIO || t == rRATIO_NEG) {

    double n = double (get_uint ());
    uint64_t d = get_uint ();
    if (d == 0) {
      error (geo::to_string (tr ("Divider must not be zero")));
      return 0.0;
    }
    return (t == rRATIO ? n : -n) / double (d);

  } else if (t == rFLOAT) {

    const unsigned char *b = (const unsigned char *) m_stream.get (4);
    if (! b) {
      error (geo::to_string (tr ("Unexpected end-of-file")));
      return 0.0;
    }

    union {
      float f;
      uint32_t i;
    } i2f;

    i2f.i = 0;
    for (int i = 0; i < 4; ++i) {
      i2f.i |= uint32_t (b [i]) << (8 * i);
    }

    return double (i2f.f);

  } else if (t == rDOUBLE) {

    const unsigned char *b = (const unsigned char *) m_stream.get (8);
    if (! b) {
      error (geo::to_string (tr ("Unexpected end-of-file")));
      return 0.0;
    }

    union {
      double d;
      uint64_t i;
    } i2f;

    i2f.i = 0;
    for (int i = 0; i < 8; ++i) {
      i2f.i |= uint64_t (b [i]) << (8 * i);
    }

    return i2f.d;

  } else {

    error (geo::sprintf (geo::to_string (tr ("Invalid real number type %d")), int (t)));
    return 0.0;

  }
}

std::string
OASIS2Reader::get_string ()
{
  uint64_t l = get_uint ();

  if (l == 0) {
    return std::string ();
  }

  if (l > 0x10000000) {
    error (geo::to_string (tr ("String is too long")));
    return std::string ();
  }

  const char *b = m_stream.get (size_t (l));
  if (! b) {
    error (geo::to_string (tr ("Unexpected end-of-file")));
    return std::string ();
  }

  return std::string (b, size_t (l));
}

const char *
OASIS2Reader::get_bytes (size_t n)
{
  const char *b = m_stream.get (n);
  if (! b) {
    error (geo::to_string (tr ("Unexpected end-of-file")));
  }
  return b;
}

void
OASIS2Reader::unget_byte ()
{
  m_stream.unget (1);
}

void
OASIS2Reader::inflate_stream ()
{
  m_stream.inflate ();
}

void
OASIS2Reader::progress_checkpoint ()
{
  m_progress.set (m_stream.pos ());
}

size_t
OASIS2Reader::stream_pos () const
{
  return m_stream.pos ();
}

std::string
OASIS2Reader::path () const
{
  return m_stream.source ();
}

void
OASIS2Reader::error (const std::string &txt)
{
  throw OASIS2ReaderException (txt, m_stream.pos (), std::string ());
}

void
OASIS2Reader::warn (const std::string &txt)
{
  if (warnings_as_errors ()) {
    throw OASIS2ReaderException (txt, m_stream.pos (), std::string ());
  }

  LOGW << txt << " (position=" << m_stream.pos () << ")";
}

}
