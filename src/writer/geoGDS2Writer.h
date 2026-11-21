#ifndef HDR_geoGDS2Writer
#define HDR_geoGDS2Writer

#include "geoGDS2WriterBase.h"
#include "geoWriterTools.h"
#include "utilsProgress.h"

namespace geo
{

/**
 *  @brief A GDS2 writer abstraction
 */

class GDS2Writer
  : public geo::GDS2WriterBase
{
public:
  /**
   *  @brief Instantiate the writer
   */
  GDS2Writer ();

protected:
  /**
   *  @brief Write a byte
   */
  void write_byte (unsigned char b);

  /**
   *  @brief Write a record length short
   */
  virtual void write_record_size (int16_t i);

  /**
   *  @brief Write a record type short
   */
  virtual void write_record (int16_t i);

  /**
   *  @brief Write a short
   */
  void write_short (int16_t i);

  /**
   *  @brief Write a long
   */
  void write_int (int32_t l);

  /**
   *  @brief Write a double
   */
  void write_double (double d);

  /**
   *  @brief Write the time
   */
  void write_time (const short *t);

  /**
   *  @brief Write a string
   */
  void write_string (const char *t);

  /**
   *  @brief Write a string
   */
  void write_string (const std::string &t);

  /**
   *  @brief Write a string plus record
   */
  void write_string_record (short record, const std::string &t);

  /**
   *  @brief Set the stream to write the data to
   */
  void set_stream (geo::OutputStream &stream)
  {
    mp_stream = &stream;
  }

  /**
   *  @brief Establish a checkpoint for progress reporting
   */
  void progress_checkpoint ();

private:
  geo::OutputStream *mp_stream;
  geo::AbsoluteProgress m_progress;
};

} // namespace geo

#endif

