#ifndef HDR_geoOASIS2ReaderBase
#define HDR_geoOASIS2ReaderBase

#include "geoLayout.h"
#include "geoReader.h"
#include "geoStreamLayers.h"
#include "geoCommonReader.h"
#include "geoOASIS2Format.h"

#include "utilsException.h"
#include "utilsInternational.h"
#include "utilsProgress.h"
#include "utilsString.h"
#include "utilsStream.h"

#include <map>
#include <string>
#include <vector>

namespace geo
{

/**
 *  @brief Generic base class of OASIS reader exceptions
 */
class OASIS2ReaderException
  : public ReaderException
{
public:
  OASIS2ReaderException (const std::string &msg, size_t p, const std::string &cell)
    : ReaderException (geo::sprintf (geo::to_string (tr ("%s (position=%ld, cell=%s)")), msg, p, cell))
  { }
};

/**
 *  @brief The OASIS format basic stream reader
 *
 *  This class implements the OASIS record decoding (SEMI P39). It only
 *  depends on a few byte level primitives which are provided by the
 *  concrete stream reader (see OASIS2Reader).
 */
class OASIS2ReaderBase
  : public CommonReader
{
public:
  /**
   *  @brief Constructor
   */
  OASIS2ReaderBase ();

  /**
   *  @brief Destructor
   */
  ~OASIS2ReaderBase ();

protected:
  /**
   *  @brief Implementation of the CommonReader interface
   */
  virtual void do_read (geo::Layout &layout);
  virtual void init (const geo::LoadLayoutOptions &options);

  //  -------------------------------------------------------------------
  //  Byte level primitives (must be provided by the concrete reader)
  //  -------------------------------------------------------------------

  virtual unsigned char get_byte () = 0;
  virtual uint64_t get_uint () = 0;
  virtual int64_t get_int () = 0;
  virtual double get_real () = 0;
  virtual std::string get_string () = 0;
  virtual const char *get_bytes (size_t n) = 0;
  virtual void unget_byte () = 0;
  virtual void inflate_stream () = 0;
  virtual void progress_checkpoint () = 0;

  virtual size_t stream_pos () const = 0;
  virtual std::string path () const = 0;
  virtual void error (const std::string &txt) = 0;
  virtual void warn (const std::string &txt) = 0;

  virtual void common_reader_error (const std::string &msg) { error (msg); }
  virtual void common_reader_warn (const std::string &msg) { warn (msg); }

  //  -------------------------------------------------------------------
  //  Coordinate decoders
  //  -------------------------------------------------------------------

  /**
   *  @brief Reads a signed coordinate ("1-delta" representation)
   */
  geo::Coord get_coord ();

  /**
   *  @brief Reads an unsigned coordinate
   */
  uint64_t get_ucoord ();

  /**
   *  @brief Reads a "2-delta" (horizontal or vertical vector)
   */
  geo::Vector get_2delta ();

  /**
   *  @brief Reads a "3-delta" (octangular vector)
   */
  geo::Vector get_3delta ();

  /**
   *  @brief Reads a "g-delta" (arbitrary vector)
   */
  geo::Vector get_gdelta ();

private:
  int m_expect_strict_mode;
  bool m_read_texts;
  bool m_read_properties;
  std::string m_cellname;

  //  the modal variables (SEMI P39 "modal variables")
  uint32_t m_layer;
  uint32_t m_datatype;
  geo::Coord m_geometry_x;
  geo::Coord m_geometry_y;
  uint64_t m_geometry_w;
  uint64_t m_geometry_h;
  std::vector <geo::Point> m_polygon_point_list;
  std::vector <geo::Point> m_path_point_list;
  geo::Coord m_path_halfwidth;
  geo::Coord m_path_start_extension;
  geo::Coord m_path_end_extension;
  geo::Coord m_circle_radius;
  geo::cell_index_type m_placement_cell;
  bool m_placement_cell_valid;
  geo::Coord m_placement_x;
  geo::Coord m_placement_y;
  std::vector <geo::Vector> m_repetition;

  //  tables
  std::map <uint64_t, std::string> m_textstrings;
  std::map <uint64_t, std::string> m_visible_cellnames;

  void reset_modal_variables ();

  void read_cell (geo::Layout &layout, geo::cell_index_type cell_index);

  void read_placement (unsigned char r,
                       geo::Layout &layout,
                       bool xy_absolute,
                       std::vector <geo::CellInstArray> &instances,
                       std::vector <geo::CellInstArrayWithProperties> &instances_wp);

  void read_text (geo::Layout &layout, geo::cell_index_type cell_index, bool xy_absolute);
  void read_rectangle (geo::Layout &layout, geo::cell_index_type cell_index, bool xy_absolute);
  void read_polygon (geo::Layout &layout, geo::cell_index_type cell_index, bool xy_absolute);
  void read_path (geo::Layout &layout, geo::cell_index_type cell_index, bool xy_absolute);
  void read_trapezoid (unsigned char r, geo::Layout &layout, geo::cell_index_type cell_index, bool xy_absolute);
  void read_circle (geo::Layout &layout, geo::cell_index_type cell_index, bool xy_absolute);

  void read_layername (bool explicit_id);

  /**
   *  @brief Reads a point list into the given container
   *
   *  The points are delivered relative to the shape position.
   */
  void read_pointlist (std::vector <geo::Point> &pointlist, bool for_polygon);

  /**
   *  @brief Reads a repetition into m_repetition
   *
   *  @return false if the repetition is singular (no repetition)
   */
  bool read_repetition ();

  /**
   *  @brief Reads and discards a PROPERTY record
   */
  void read_property ();

  /**
   *  @brief Skips the properties attached to the last element
   */
  void skip_element_properties ();

  /**
   *  @brief Reads the offset table (5 tables + XNAME)
   */
  void read_offset_table ();

  /**
   *  @brief Emits a shape at the shape position and at all repetition offsets
   */

  void emit_box (geo::Layout &layout, geo::cell_index_type cell_index,
                 uint32_t layer, uint32_t datatype, const geo::Box &box);

  void emit_polygon (geo::Layout &layout, geo::cell_index_type cell_index,
                     uint32_t layer, uint32_t datatype,
                     const std::vector <geo::Point> &points, const geo::Vector &pos);

  void emit_path (geo::Layout &layout, geo::cell_index_type cell_index,
                  uint32_t layer, uint32_t datatype,
                  const std::vector <geo::Point> &points, geo::Coord width,
                  geo::Coord start_extension, geo::Coord end_extension, bool round,
                  const geo::Vector &pos);

  void emit_text (geo::Layout &layout, geo::cell_index_type cell_index,
                  uint32_t layer, uint32_t datatype,
                  const std::string &text, const geo::Vector &pos);
};

}

#endif
