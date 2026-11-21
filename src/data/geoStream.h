#ifndef HDR_geoStream
#define HDR_geoStream


#include "geoSaveLayoutOptions.h"
#include "geoLoadLayoutOptions.h"

#include "utilsClassRegistry.h"
#include "utilsXMLParser.h"
#include "utilsXMLWriter.h"

#include <string>
#include <vector>

namespace geo
{
  class InputStream;
}

namespace geo
{

class ReaderBase;
class WriterBase;

/**
 *  @brief A stream format declaration
 */
class StreamFormatDeclaration 
{
public:
  /**
   *  @brief Constructor
   */
  StreamFormatDeclaration () { }

  /**
   *  @brief Destructor
   */
  virtual ~StreamFormatDeclaration () { }

  /**
   *  @brief Obtain the format name
   */
  virtual std::string format_name () const = 0;

  /**
   *  @brief Obtain the format description
   */
  virtual std::string format_desc () const = 0;

  /**
   *  @brief Obtain the (long) format description
   */
  virtual std::string format_title () const = 0;

  /**
   *  @brief Obtain the file dialog format contribution
   */
  virtual std::string file_format () const = 0;

  /**
   *  @brief Auto-detect this format from the stream
   */
  virtual bool detect (geo::InputStream &s) const = 0;

  /**
   *  @brief Create the reader
   */
  virtual ReaderBase *create_reader (geo::InputStream &s) const = 0;

  /**
   *  @brief Returns true, when the format supports import (has a reader)
   */
  virtual bool can_read () const = 0;

  /**
   *  @brief Create the reader
   */
  virtual WriterBase *create_writer () const = 0;

  /**
   *  @brief Returns true, when the format supports export (has a writer)
   */
  virtual bool can_write () const = 0;

  /**
   *  @brief Delivers the XMLElement object that represents the reader options within a technology XML tree
   *
   *  This method is supposed to return an instance ReaderOptionsXMLElement<RO> where RO is the
   *  specific reader options type. The return value can be 0 to indicate there is no specific reader
   *  option.
   *
   *  The returned XMLElement is destroyed by the caller and needs to be a new object.
   */
  virtual geo::XMLElementBase *xml_reader_options_element () const
  {
    return 0;
  }

  /**
   *  @brief Delivers the XMLElement object that represents this writer options within a technology XML tree
   *
   *  This method is supposed to return an instance WriterOptionsXMLElement<WO> where WO is the
   *  specific writer options type. The return value can be 0 to indicate there is no specific writer
   *  option.
   *
   *  The returned XMLElement is destroyed by the caller and needs to be a new object.
   */
  virtual geo::XMLElementBase *xml_writer_options_element () const
  {
    return 0;
  }
};

/**
 *  @brief A helper class for the XML serialization of the stream options (custom read adaptor)
 *
 *  OPT is a reader or writer options class and HOST is the host class. For example, OPT
 *  can be geo::GDS2ReaderOptions and HOST then is geo::LoadLayoutOptions.
 */
template <class OPT, class HOST>
class StreamOptionsReadAdaptor
{
public:
  typedef geo::pass_by_ref_tag tag;

  StreamOptionsReadAdaptor ()
    : mp_options (0), m_done (false)
  {
    // .. nothing yet ..
  }

  const OPT &operator () () const
  {
    return mp_options->template get_options<OPT> ();
  }

  bool at_end () const
  {
    return m_done;
  }

  void start (const HOST &options)
  {
    mp_options = &options;
    m_done = false;
  }

  void next ()
  {
    mp_options = 0;
    m_done = true;
  }

private:
  const HOST *mp_options;
  bool m_done;
};

/**
 *  @brief A helper class for the XML serialization of the stream option (custom write adaptor)
 *
 *  See StreamOptionsReadAdaptor for details.
 */
template <class OPT, class HOST>
class StreamOptionsWriteAdaptor
{
public:
  StreamOptionsWriteAdaptor ()
  {
    // .. nothing yet ..
  }

  void operator () (HOST &options, geo::XMLReaderState &reader) const
  {
    std::unique_ptr<OPT> opt (new OPT ());

    geo::XMLObjTag<OPT> tag;
    *opt = *reader.back (tag);

    options.set_options (opt.release ());
  }
};

/**
 *  @brief A XMLElement specialization for stream options
 */
template <class OPT, class HOST>
class StreamOptionsXMLElement
  : public geo::XMLElement<OPT, HOST, StreamOptionsReadAdaptor<OPT, HOST>, StreamOptionsWriteAdaptor<OPT, HOST> >
{
public:
  StreamOptionsXMLElement (const std::string &element_name, const geo::XMLElementList &children)
    : geo::XMLElement<OPT, HOST, StreamOptionsReadAdaptor<OPT, HOST>, StreamOptionsWriteAdaptor<OPT, HOST> > (StreamOptionsReadAdaptor<OPT, HOST> (), StreamOptionsWriteAdaptor<OPT, HOST> (), element_name, children)
  {
    //  .. nothing yet ..
  }

  StreamOptionsXMLElement (const StreamOptionsXMLElement &d)
    : geo::XMLElement<OPT, HOST, StreamOptionsReadAdaptor<OPT, HOST>, StreamOptionsWriteAdaptor<OPT, HOST> > (d)
  {
    //  .. nothing yet ..
  }
};

/**
 *  @brief A custom XMLElement for the serialization of reader options
 *
 *  StreamReaderPluginDeclaration::xml_element can return such an element to
 *  insert a custom XML element into the XML tree which represents the
 *  reader options.
 */
template <class OPT>
class ReaderOptionsXMLElement
  : public StreamOptionsXMLElement<OPT, geo::LoadLayoutOptions>
{
public:
  ReaderOptionsXMLElement (const std::string &element_name, const geo::XMLElementList &children)
    : StreamOptionsXMLElement<OPT, geo::LoadLayoutOptions> (element_name, children)
  {
    //  .. nothing yet ..
  }

  ReaderOptionsXMLElement (const ReaderOptionsXMLElement &d)
    : StreamOptionsXMLElement<OPT, geo::LoadLayoutOptions> (d)
  {
    //  .. nothing yet ..
  }

  virtual geo::XMLElementBase *clone () const
  {
    return new ReaderOptionsXMLElement (*this);
  }
};

/**
 *  @brief A custom XMLElement for the serialization of writer options
 *
 *  StreamWriterPluginDeclaration::xml_element can return such an element to
 *  insert a custom XML element into the XML tree which represents the
 *  writer options.
 */
template <class OPT>
class WriterOptionsXMLElement
  : public StreamOptionsXMLElement<OPT, geo::SaveLayoutOptions>
{
public:
  WriterOptionsXMLElement (const std::string &element_name, const geo::XMLElementList &children)
    : StreamOptionsXMLElement<OPT, geo::SaveLayoutOptions> (element_name, children)
  {
    //  .. nothing yet ..
  }

  WriterOptionsXMLElement (const WriterOptionsXMLElement &d)
    : StreamOptionsXMLElement<OPT, geo::SaveLayoutOptions> (d)
  {
    //  .. nothing yet ..
  }

  virtual geo::XMLElementBase *clone () const
  {
    return new WriterOptionsXMLElement (*this);
  }
};

/**
 *  @brief Returns the XMLElement list that can represent a geo::LoadLayoutOptions object
 */
geo::XMLElementList load_options_xml_element_list ();

/**
 *  @brief Returns the XMLElement list that can represent a geo::SaveLayoutOptions object
 */
geo::XMLElementList save_options_xml_element_list ();

}

#endif


