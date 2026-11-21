#ifndef HDR_geoWriter
#define HDR_geoWriter


#include "utilsException.h"
#include "geoSaveLayoutOptions.h"

#include <string>

namespace geo 
{
  class OutputStream;
  class OutputStreamBase;
}

namespace geo
{

class Layout;

/**
 *  @brief The generic writer base class
 */
class WriterBase
{
public:
  /**
   *  @brief Constructor
   */
  WriterBase () { }

  /**
   *  @brief Destructor
   */
  virtual ~WriterBase () { }

  /**
   *  @brief Actually write the layout
   *  The layout is non-const since the writer may modify the meta information of the layout.
   */
  virtual void write (geo::Layout &layout, geo::OutputStream &stream, const geo::SaveLayoutOptions &options) = 0;
};

/**
 *  @brief Type of a writer factory function
 */
typedef WriterBase *(*writer_factory_type) ();

/**
 *  @brief Creates a writer for the given format name
 *
 *  Returns 0 if no writer implementation is registered for that format.
 *  Writer implementations register themselves with WriterFactoryRegistration,
 *  so the reader library does not have to link against the writer library.
 */
WriterBase *create_writer_for_format (const std::string &format_name);

/**
 *  @brief Registers a writer factory for a format name
 *
 *  Instantiate this class as a static object inside a writer implementation
 *  translation unit to make the writer available through
 *  create_writer_for_format.
 */
class WriterFactoryRegistration
{
public:
  WriterFactoryRegistration (const std::string &format_name, writer_factory_type factory);
};

/**
 *  @brief A generic stream format writer
 */
class Writer
{
public:
  typedef std::vector<MetaInfo> meta_info;
  typedef meta_info::const_iterator meta_info_iterator;

  /**
   *  @brief The constructor
   */
  Writer (const SaveLayoutOptions &options);

  /**
   *  @brief The destructor
   */
  ~Writer ();

  /**
   *  @brief The generic write method
   *  The layout is non-const since the writer may modify the meta information of the layout.
   */
  void write (geo::Layout &layout, geo::OutputStream &stream);

  /**
   *  @brief True, if for this format a valid writer is provided
   */
  bool is_valid () const
  {
    return mp_writer != 0;
  }

private:
  WriterBase *mp_writer;
  geo::SaveLayoutOptions m_options;
};

}

#endif

