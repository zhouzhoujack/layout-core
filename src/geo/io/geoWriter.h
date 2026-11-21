#ifndef HDR_geoWriter
#define HDR_geoWriter

#include "geoCommon.h"

#include "utilsException.h"
#include "geoSaveLayoutOptions.h"

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
class DB_PUBLIC WriterBase
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
 *  @brief A generic stream format writer
 */
class DB_PUBLIC Writer
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

