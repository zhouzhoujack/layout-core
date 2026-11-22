#include "utilsXMLParser.h"
#include "utilsString.h"
#include "utilsLog.h"
#include "utilsAssert.h"
#include "utilsProgress.h"

#include <cstring>
#include <memory>


#include <QtGlobal>
#include <QFile>
#include <QIODevice>

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED

#include <QXmlContentHandler>

namespace geo
{

// --------------------------------------------------------------------
//  A SAX handler for the Qt implementation

class SAXHandler
  : public QXmlDefaultHandler
{
public:
  SAXHandler (XMLStructureHandler *sh);

  bool characters (const QString &ch);
  bool endElement (const QString &namespaceURI, const QString &localName, const QString &qName);
  bool startElement (const QString &namespaceURI, const QString &localName, const QString &qName, const QXmlAttributes &atts);
  bool error (const QXmlParseException &exception);
  bool fatalError (const QXmlParseException &exception);
  bool warning (const QXmlParseException &exception);

  void setDocumentLocator (QXmlLocator *locator);

private:
  QXmlLocator *mp_locator;
  XMLStructureHandler *mp_struct_handler;
};

// --------------------------------------------------------------------------------------------------------
//  trureHandler implementation

SAXHandler::SAXHandler (XMLStructureHandler *sh)
  : QXmlDefaultHandler (), mp_locator (0), mp_struct_handler (sh)
{
  // .. nothing yet ..
}

void
SAXHandler::setDocumentLocator (QXmlLocator *locator)
{
  mp_locator = locator;
}

bool
SAXHandler::startElement (const QString &qs_uri, const QString &qs_lname, const QString &qs_qname, const QXmlAttributes & /*atts*/)
{
  std::string uri (geo::to_string (qs_uri));
  std::string lname (geo::to_string (qs_lname));
  std::string qname (geo::to_string (qs_qname));

  try {
    mp_struct_handler->start_element (uri, lname, qname);
  } catch (geo::XMLException &ex) {
    throw geo::XMLLocatedException (ex.raw_msg (), mp_locator->lineNumber (), mp_locator->columnNumber ());
  } catch (geo::Exception &ex) {
    throw geo::XMLLocatedException (ex.msg (), mp_locator->lineNumber (), mp_locator->columnNumber ());
  }

  //  successful
  return true;
}

bool
SAXHandler::endElement (const QString &qs_uri, const QString &qs_lname, const QString &qs_qname)
{
  std::string uri (geo::to_string (qs_uri));
  std::string lname (geo::to_string (qs_lname));
  std::string qname (geo::to_string (qs_qname));

  try {
    mp_struct_handler->end_element (uri, lname, qname);
  } catch (geo::XMLException &ex) {
    throw geo::XMLLocatedException (ex.raw_msg (), mp_locator->lineNumber (), mp_locator->columnNumber ());
  } catch (geo::Exception &ex) {
    throw geo::XMLLocatedException (ex.msg (), mp_locator->lineNumber (), mp_locator->columnNumber ());
  }

  //  successful
  return true;
}

bool
SAXHandler::characters (const QString &t)
{
  try {
    mp_struct_handler->characters (geo::to_string (t));
  } catch (geo::XMLException &ex) {
    throw geo::XMLLocatedException (ex.raw_msg (), mp_locator->lineNumber (), mp_locator->columnNumber ());
  } catch (geo::Exception &ex) {
    throw geo::XMLLocatedException (ex.msg (), mp_locator->lineNumber (), mp_locator->columnNumber ());
  }

  //  successful
  return true;
}

bool
SAXHandler::error (const QXmlParseException &ex)
{
  throw geo::XMLLocatedException (geo::to_string (ex.message ()), ex.lineNumber (), ex.columnNumber ());
}

bool
SAXHandler::fatalError (const QXmlParseException &ex)
{
  throw geo::XMLLocatedException (geo::to_string (ex.message ()), ex.lineNumber (), ex.columnNumber ());
}

bool
SAXHandler::warning (const QXmlParseException &ex)
{
  geo::XMLLocatedException lex (geo::to_string (ex.message ()), ex.lineNumber (), ex.columnNumber ());
  geo::warn << lex.msg ();
  //  continue
  return true;
}

// --------------------------------------------------------------------
//  SourcePrivateData implementation

class XMLSourcePrivateData
  : public QXmlInputSource
{
public:
  XMLSourcePrivateData ()
    : QXmlInputSource ()
  {
    //  .. nothing yet ..
  }

  XMLSourcePrivateData (QIODevice *dev)
    : QXmlInputSource (dev)
  {
    //  .. nothing yet ..
  }
};

// --------------------------------------------------------------------
//  XMLSource implementation

XMLSource::XMLSource ()
  : mp_source (0)
{
  //  .. nothing yet ..
}

XMLSource::~XMLSource ()
{
  delete mp_source;
  mp_source = 0;
}

void
XMLSource::reset ()
{
  mp_source->reset ();
}

// --------------------------------------------------------------------
//  XMLStringSource implementation

XMLStringSource::XMLStringSource (const std::string &string)
{
  XMLSourcePrivateData *source = new XMLSourcePrivateData ();
  source->setData (QByteArray (string.c_str ()));
  set_source (source);
}

XMLStringSource::XMLStringSource (const char *cp)
{
  XMLSourcePrivateData *source = new XMLSourcePrivateData ();
  source->setData (QByteArray (cp));
  set_source (source);
}

XMLStringSource::XMLStringSource (const char *cp, size_t len)
{
  XMLSourcePrivateData *source = new XMLSourcePrivateData ();
  source->setData (QByteArray (cp, int (len)));
  set_source (source);
}

XMLStringSource::~XMLStringSource ()
{
  //  .. nothing yet ..
}

// --------------------------------------------------------------------
//  StreamIODevice definition and implementation

class StreamIODevice
  : public QIODevice
{
public:
  StreamIODevice (geo::InputStream &stream)
    : mp_stream (&stream),
      mp_progress (0),
      m_has_error (false)
  {
    open (QIODevice::ReadOnly);
  }

  StreamIODevice (geo::InputStream &stream, const std::string &progress_message)
    : mp_stream (&stream),
      mp_progress (new AbsoluteProgress (progress_message, 100)),
      m_has_error (false)
  {
    mp_progress->set_format (geo::to_string (tr ("%.0f MB")));
    mp_progress->set_unit (1024 * 1024);
    open (QIODevice::ReadOnly);
  }

  StreamIODevice (const std::string &path)
    : mp_stream_holder (new geo::InputStream (path)),
      mp_progress (0),
      m_has_error (false)
  {
    mp_stream = mp_stream_holder.get ();
    open (QIODevice::ReadOnly);
  }

  StreamIODevice (const std::string &path, const std::string &progress_message)
    : mp_stream_holder (new geo::InputStream (path)),
      mp_progress (new AbsoluteProgress (progress_message, 100)),
      m_has_error (false)
  {
    mp_stream = mp_stream_holder.get ();
    mp_progress->set_format (geo::to_string (tr ("%.0f MB")));
    mp_progress->set_unit (1024 * 1024);
    open (QIODevice::ReadOnly);
  }

  ~StreamIODevice ()
  {
    if (mp_progress) {
      delete mp_progress;
      mp_progress = 0;
    }
  }

  virtual bool isSequential () const
  {
    return true;
  }

  qint64 writeData (const char *, qint64) 
  {
    tl_assert (false);
  }

  qint64 readData (char *data, qint64 n)
  {
    try {

      if (mp_progress) {
        mp_progress->set (mp_stream->pos ());
      }

      qint64 n0 = n;
      for (const char *rd = 0; n > 0 && (rd = mp_stream->get (1)) != 0; ) {
        //  NOTE: we skip CR to compensate for Windows CRLF line terminators (issue #419).
        if (*rd != '\r') {
          *data++ = *rd;
          --n;
        }
      }

      if (n0 == n) {
        return -1;
      } else {
        return n0 - n;
      }

    } catch (geo::Exception &ex) {
      setErrorString (geo::to_qstring (ex.msg ()));
      m_has_error = true;
      return -1;
    }
  }

  bool has_error () const
  {
    return m_has_error;
  }

private:
  geo::InputStream *mp_stream;
  std::unique_ptr<geo::InputStream> mp_stream_holder;
  geo::AbsoluteProgress *mp_progress;
  bool m_has_error;
};

// --------------------------------------------------------------------
//  XMLFileSource implementation

class XMLStreamSourcePrivateData
  : public XMLSourcePrivateData
{
public:
  XMLStreamSourcePrivateData (StreamIODevice *io)
    : XMLSourcePrivateData (io), mp_io (io)
  {
    //  .. nothing yet ..
  }

  virtual void fetchData ()
  {
    QXmlInputSource::fetchData ();

    //  This feature is actually missing in the original implementation: throw an exception on error
    if (mp_io->has_error ()) {
      throw geo::Exception (geo::to_string (mp_io->errorString ()));
    }
  }

private:
  std::unique_ptr<StreamIODevice> mp_io;
};

// --------------------------------------------------------------------
//  XMLFileSource implementation

XMLFileSource::XMLFileSource (const std::string &path, const std::string &progress_message)
{
  set_source (new XMLStreamSourcePrivateData (new StreamIODevice (path, progress_message)));
}

XMLFileSource::XMLFileSource (const std::string &path)
{
  set_source (new XMLStreamSourcePrivateData (new StreamIODevice (path)));
}

XMLFileSource::~XMLFileSource ()
{
  //  .. nothing yet ..
}

// --------------------------------------------------------------------
//  XMLStreamSource implementation

XMLStreamSource::XMLStreamSource (geo::InputStream &s, const std::string &progress_message)
{
  set_source (new XMLStreamSourcePrivateData (new StreamIODevice (s, progress_message)));
}

XMLStreamSource::XMLStreamSource (geo::InputStream &s)
{
  set_source (new XMLStreamSourcePrivateData (new StreamIODevice (s)));
}

XMLStreamSource::~XMLStreamSource ()
{
  //  .. nothing yet ..
}

// --------------------------------------------------------------------
//  XMLParser implementation

class XMLParserPrivateData
  : public QXmlSimpleReader
{
public:
  XMLParserPrivateData () : QXmlSimpleReader () { }
};

XMLParser::XMLParser ()
  : mp_data (new XMLParserPrivateData ())
{
  //  .. nothing yet ..
}

XMLParser::~XMLParser ()
{
  delete mp_data;
  mp_data = 0;
}

void 
XMLParser::parse (XMLSource &source, XMLStructureHandler &struct_handler)
{
  SAXHandler handler (&struct_handler);

  mp_data->setContentHandler (&handler);
  mp_data->setErrorHandler (&handler);

  mp_data->parse (source.source (), false /*=not incremental*/);
}

bool
XMLParser::is_available ()
{
  return true;
}

}

QT_WARNING_POP


namespace geo {

// -----------------------------------------------------------------
//  The C++ structure definition interface (for use cases see tlXMLParser.ut)

//  XMLElementProxy implementation

XMLElementProxy::XMLElementProxy (const XMLElementProxy &d)
  : mp_ptr (d.mp_ptr->clone ())
{
  //  .. nothing yet ..
}

XMLElementProxy::XMLElementProxy (const XMLElementBase &d)
  : mp_ptr (d.clone ())
{
  //  .. nothing yet ..
}

XMLElementProxy::XMLElementProxy (XMLElementBase *d)
  : mp_ptr (d)
{
  //  .. nothing yet ..
}

XMLElementProxy::~XMLElementProxy ()
{
  delete mp_ptr;
  mp_ptr = 0;
}

//  XMLElementBase implementation

void 
XMLElementBase::write_indent (geo::OutputStream &os, int indent)
{
  for (int i = 0; i < indent; ++i) {
    os << " ";
  }
}

void 
XMLElementBase::write_string (geo::OutputStream &os, const std::string &s)
{
  for (const char *cp = s.c_str (); *cp; ++cp) {
    unsigned char c = (unsigned char) *cp;
    if (c == '&') {
      os << "&amp;";
    } else if (c == '<') {
      os << "&lt;";
    } else if (c == '>') {
      os << "&gt;";
    } else if (c == '\r') {
      //  ignore CR characters (#13)
    } else if (c == '\t' || c == '\n') {
      os << c;
    } else if (c < ' ') {
      os << "&#" << int (c) << ";";
    } else {
      os << c;
    }
  }
}

// --------------------------------------------------------------------------------------------------------
//  trureHandler implementation

XMLStructureHandler::XMLStructureHandler (const XMLElementBase *root, XMLReaderState *reader_state)
  : mp_root (root), mp_state (reader_state)
{ 
  // .. nothing yet ..
}

void
XMLStructureHandler::start_element (const std::string &uri, const std::string &lname, const std::string &qname)
{
  const XMLElementBase *new_element = 0;
  const XMLElementBase *parent = 0;

  if (m_stack.size () == 0) {
    if (! mp_root->check_name (uri, lname, qname)) {
      throw geo::XMLException (geo::to_string (tr ("Root element must be ")) + mp_root->name ());
    }
    new_element = mp_root;
  } else {
    parent = m_stack.back ();
    if (parent) {
      for (XMLElementBase::iterator c = parent->begin (); c != parent->end (); ++c) {
        if ((*c)->check_name (uri, lname, qname)) {
          new_element = (*c).get ();
          break;
        }
      }
    }
  }

  if (new_element) {
    new_element->create (parent, *mp_state, uri, lname, qname);
  }

  m_stack.push_back (new_element);
}

void
XMLStructureHandler::end_element (const std::string &uri, const std::string &lname, const std::string &qname)
{
  if (m_stack.empty ()) {
    return;
  }

  const XMLElementBase *element = m_stack.back ();
  m_stack.pop_back ();

  if (! element) {
    //  inside unknown element
  } else if (m_stack.size () == 0) {
    element->finish (0, *mp_state, uri, lname, qname);
  } else {
    element->finish (m_stack.back (), *mp_state, uri, lname, qname);
  }
}

void
XMLStructureHandler::characters (const std::string &t)
{
  if (! m_stack.empty () && m_stack.back ()) {
    m_stack.back ()->cdata (t, *mp_state);
  }
}

// --------------------------------------------------------------------
//  XMLReaderState implementation

XMLReaderState::XMLReaderState ()
{
  //  .. nothing yet ..
}

XMLReaderState::~XMLReaderState ()
{
  for (std::vector <XMLReaderProxyBase *>::const_iterator o = m_objects.begin (); o != m_objects.end (); ++o) {
    (*o)->release ();
    delete *o;
  }
  m_objects.clear ();
}

// --------------------------------------------------------------------
//  XMLWriterState implementation

XMLWriterState::XMLWriterState ()
{
  //  .. nothing yet ..
}

}
