#ifndef HDR_geoGDS2WriterBase
#define HDR_geoGDS2WriterBase

#include "geoWriter.h"
#include "geoWriterTools.h"
#include "utilsProgress.h"

namespace geo
{
  class OutputStream;
}

namespace geo
{

class Layout;
class SaveLayoutOptions;

/**
 *  @brief A GDS2 writer abstraction
 */

class GDS2WriterBase
  : public geo::WriterBase
{
public:
  /**
   *  @brief Instantiate the writer
   */
  GDS2WriterBase ();

  /**
   *  @brief Write the layout object
   */
  void write (geo::Layout &layout, geo::OutputStream &stream, const geo::SaveLayoutOptions &options);

protected:
  /**
   *  @brief Write a byte
   */
  virtual void write_byte (unsigned char b) = 0;

  /**
   *  @brief Write a record length short
   */
  virtual void write_record_size (int16_t i) = 0;

  /**
   *  @brief Write a record type short
   */
  virtual void write_record (int16_t i) = 0;

  /**
   *  @brief Write a short
   */
  virtual void write_short (int16_t i) = 0;

  /**
   *  @brief Write a long
   */
  virtual void write_int (int32_t l) = 0;

  /**
   *  @brief Write a double
   */
  virtual void write_double (double d) = 0;

  /**
   *  @brief Write the time
   */
  virtual void write_time (const short *time) = 0;

  /**
   *  @brief Write a string
   */
  virtual void write_string (const char *t) = 0;

  /**
   *  @brief Write a string
   */
  virtual void write_string (const std::string &t) = 0;

  /**
   *  @brief Set the stream to write the data to
   */
  virtual void set_stream (geo::OutputStream &stream) = 0;

  /**
   *  @brief Establish a checkpoint
   */
  virtual void progress_checkpoint () = 0;

  /**
   *  @brief Write a string plus record
   */
  void write_string_record (short record, const std::string &t);

  /**
   *  @brief Write an instance 
   */
  void write_inst (double sf, const geo::Instance &instance, bool normalize, bool resolve_skew_arrays, const geo::Layout &layout, geo::properties_id_type prop_id);

  /**
   *  @brief Write a shape as box
   */
  void write_box (int layer, int datatype, double sf, const geo::Shape &shape, const geo::Layout &layout, geo::properties_id_type prop_id);

  /**
   *  @brief Write a shape as edge
   */
  void write_edge (int layer, int datatype, double sf, const geo::Shape &shape, const geo::Layout &layout, geo::properties_id_type prop_id);

  /**
   *  @brief Writes an edge
   */
  void write_edge (int layer, int datatype, double sf, const geo::Edge &edge, const geo::Layout &layout, geo::properties_id_type prop_id);

  /**
   *  @brief Write a shape as path
   */
  void write_path (int layer, int datatype, double sf, const geo::Shape &shape, bool multi_xy, const geo::Layout &layout, geo::properties_id_type prop_id);

  /**
   *  @brief Write a shape as text
   */
  void write_text (int layer, int datatype, double sf, double dbu, const geo::Shape &shape, const geo::Layout &layout, geo::properties_id_type prop_id);

  /**
   *  @brief Write a shape as polygon
   */
  void write_polygon (int layer, int datatype, double sf, const geo::Shape &shape, bool multi_xy, size_t max_vertex, const geo::Layout &layout, geo::properties_id_type prop_id);

  /**
   *  @brief Write a polygon
   */
  void write_polygon (int layer, int datatype, double sf, const geo::Polygon &polygon, bool multi_xy, size_t max_vertex, const geo::Layout &layout, geo::properties_id_type prop_id, bool merged);

  /**
   *  @brief Finish an element by writing the properties and ENDEL
   */
  void finish (const geo::Layout &layout, geo::properties_id_type prop_id);

private:
  geo::WriterCellNameMap m_cell_name_map;

  void write_properties (const geo::Layout &layout, geo::properties_id_type prop_id);
};

} // namespace geo

#endif

