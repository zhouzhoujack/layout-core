#ifndef HDR_geoReader
#define HDR_geoReader


#include "utilsException.h"
#include "utilsInternational.h"
#include "utilsString.h"
#include "utilsStream.h"

#include "geoLoadLayoutOptions.h"

#include <vector>

namespace geo
{

class Layout;
class ReaderBase;

/**
 *  @brief Generic base class of reader exceptions
 */
class ReaderException
  : public geo::Exception 
{
public:
  ReaderException (const std::string &msg)
    : geo::Exception (msg)
  { }
};

/**
 *  @brief Joins layer names into a single, combined layer
 *  @param s The first layer name and output
 *  @param n The name to add
 */
void
join_layer_names (std::string &s, const std::string &n);

/**
 *  @brief The generic reader base class
 */
class ReaderBase
{
public:
  ReaderBase ();
  virtual ~ReaderBase ();

  virtual const geo::LayerMap &read (geo::Layout &layout, const geo::LoadLayoutOptions &options) = 0;
  virtual const geo::LayerMap &read (geo::Layout &layout) = 0;
  virtual const char *format () const = 0;

  /**
   *  @brief Sets a flag indicating that warnings shall be treated as errors
   *  If this flag is set, warnings will be handled as errors.
   */
  void set_warnings_as_errors (bool f);

  /**
   *  @brief Gets a flag indicating that warnings shall be treated as errors
   */
  bool warnings_as_errors () const
  {
    return m_warnings_as_errors;
  }

private:
  bool m_warnings_as_errors;
};

/**
 *  @brief The generic stream reader 
 *
 *  This reader is supposed to fork to one of the specific readers
 *  depending on the format detected.
 */
class Reader
{
public: 
  /**
   *  @brief Construct a reader object
   *
   *  If no valid format can be detected, the constructor will throw 
   *  an exception. The stream must be opened already in order to allow
   *  format detection.
   *
   *  @param s The stream object from which to read stream data from
   */
  Reader (geo::InputStream &s);

  /**  
   *  @brief Destructor
   */
  ~Reader ();

  /** 
   *  @brief The basic read method 
   *
   *  This method will read the stream data and translate this to
   *  insert calls into the layout object. This will not do much
   *  on the layout object beside inserting the objects.
   *  It can be passed options with a layer map which tells which
   *  OASIS layer(s) to read on which logical layers.
   *  In addition, a flag can be passed that tells whether to create 
   *  new layers. The returned map will contain all layers, the passed
   *  ones and the newly created ones.
   *
   *  @param layout The layout object to write to
   *  @param options The LayerMap object
   */
  const geo::LayerMap &read (geo::Layout &layout, const geo::LoadLayoutOptions &options);

  /** 
   *  @brief The basic read method (without mapping)
   *
   *  This method will read the stream data and translate this to
   *  insert calls into the layout object. This will not do much
   *  on the layout object beside inserting the objects.
   *  This version will read all input layers and return a map
   *  which tells which OASIS layer has been read into which logical
   *  layer.
   *
   *  @param layout The layout object to write to
   *  @return The LayerMap object
   */
  const geo::LayerMap &read (geo::Layout &layout);

  /**
   *  @brief Returns a format describing the file format found
   */
  const char *format () const
  {
    return mp_actual_reader->format ();
  }

  /**
   *  @brief Sets a flag indicating that warnings shall be treated as errors
   *  If this flag is set, warnings will be handled as errors.
   */
  void set_warnings_as_errors (bool f)
  {
    mp_actual_reader->set_warnings_as_errors (f);
  }

  /**
   *  @brief Gets a flag indicating that warnings shall be treated as errors
   */
  bool warnings_as_errors () const
  {
    return mp_actual_reader->warnings_as_errors ();
  }

private:
  ReaderBase *mp_actual_reader;
  geo::InputStream &m_stream;
};

}

#endif

