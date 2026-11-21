#ifndef HDR_geoOASIS2Format
#define HDR_geoOASIS2Format

#include "geoSaveLayoutOptions.h"
#include "geoLoadLayoutOptions.h"

namespace geo
{

/**
 *  @brief Structure that holds the OASIS specific options for the reader
 *  NOTE: this structure is non-public linkage by intention. This way it's instantiated
 *  in all compile units and the shared object does not need to be linked.
 */
class OASIS2ReaderOptions
  : public FormatSpecificReaderOptions
{
public:
  /**
   *  @brief The constructor
   */
  OASIS2ReaderOptions ()
    : expect_strict_mode (-1),
      read_max_vertex_count (0)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Tells the reader whether the file is expected to be in strict mode
   *
   *  0: the file is expected to be in strict mode
   *  1: the file is expected to be not in strict mode
   * -1: don't care (default)
   *
   *  This option only controls the diagnostics, not the actual reading.
   */
  int expect_strict_mode;

  /**
   *  @brief Limits the number of vertices of polygons read
   *
   *  A value of 0 (the default) means "unlimited".
   *  Polygons with more vertices are split into smaller ones.
   */
  unsigned int read_max_vertex_count;

  /**
   *  @brief Implementation of FormatSpecificReaderOptions
   */
  virtual FormatSpecificReaderOptions *clone () const
  {
    return new OASIS2ReaderOptions (*this);
  }

  /**
   *  @brief Implementation of FormatSpecificReaderOptions
   */
  virtual const std::string &format_name () const
  {
    static const std::string n ("OASIS");
    return n;
  }
};

/**
 *  @brief Structure that holds the OASIS specific options for the writer
 *  NOTE: this structure is non-public linkage by intention. This way it's instantiated
 *  in all compile units and the shared object does not need to be linked.
 */
class OASIS2WriterOptions
  : public FormatSpecificWriterOptions
{
public:
  /**
   *  @brief The constructor
   */
  OASIS2WriterOptions ()
    : strict_mode (false),
      write_cblocks (false),
      write_cell_bounding_boxes (false),
      max_cellname_length (0)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Write the file in strict mode
   *
   *  NOTE: the current implementation always produces non-strict files
   *  (tables at the beginning, empty offset table). Turning this option on
   *  only issues a warning.
   */
  bool strict_mode;

  /**
   *  @brief Wrap the cell bodies into CBLOCK (DEFLATE compressed) records
   *
   *  NOTE: compression is not implemented yet - the flag is accepted but
   *  ignored. Files are written uncompressed, which is fully compliant.
   */
  bool write_cblocks;

  /**
   *  @brief Write the "S_BOUNDING_BOX" standard property for each cell
   *
   *  NOTE: not implemented yet - the flag is accepted but ignored.
   */
  bool write_cell_bounding_boxes;

  /**
   *  @brief The maximum length of a cell name
   *
   *  0 means "unlimited".
   */
  unsigned int max_cellname_length;

  /**
   *  @brief Implementation of FormatSpecificWriterOptions
   */
  virtual FormatSpecificWriterOptions *clone () const
  {
    return new OASIS2WriterOptions (*this);
  }

  /**
   *  @brief Implementation of FormatSpecificWriterOptions
   */
  virtual const std::string &format_name () const
  {
    static const std::string n ("OASIS");
    return n;
  }
};

}

#endif
