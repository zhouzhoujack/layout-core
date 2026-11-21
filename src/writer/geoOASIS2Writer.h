#ifndef HDR_geoOASIS2Writer
#define HDR_geoOASIS2Writer

#include "geoOASIS2Format.h"

#include "geoWriter.h"
#include "geoWriterTools.h"

#include "geoBox.h"
#include "geoEdge.h"
#include "geoInstances.h"
#include "geoPath.h"
#include "geoPolygon.h"
#include "geoText.h"
#include "geoVector.h"

#include "utilsProgress.h"

#include <string>
#include <vector>

namespace geo
{
  class OutputStream;
}

namespace geo
{

class Layout;
class SaveLayoutOptions;

/**
 *  @brief An OASIS writer abstraction
 *
 *  The writer produces compliant OASIS (SEMI P39) version 1.0 files in
 *  "non-strict" mode: the tables (CELLNAME, TEXTSTRING, ...) are written at the
 *  beginning of the file and the offset table is left empty.
 *
 *  All geometry is written with absolute coordinates and explicit info bytes,
 *  i.e. the modal variable compression is not used. This keeps the writer simple
 *  while producing files that are still fully compliant.
 */
class OASIS2Writer
  : public geo::WriterBase
{
public:
  /**
   *  @brief Instantiate the writer
   */
  OASIS2Writer ();

  /**
   *  @brief Write the layout object
   */
  virtual void write (geo::Layout &layout, geo::OutputStream &stream, const geo::SaveLayoutOptions &options);

private:
  geo::OutputStream *mp_stream;
  geo::OASIS2WriterOptions m_options;
  double m_sf;
  geo::AbsoluteProgress m_progress;

  void write_byte (unsigned char b);
  void write_bytes (const char *b, size_t n);
  void write_record (unsigned char r);
  void write_uint (uint64_t n);
  void write_int (int64_t n);
  void write_real (double d);
  void write_string (const std::string &s);
  void write_coord (geo::Coord c);
  void write_ucoord (geo::Coord c);
  void write_gdelta (const geo::Vector &p);
  void write_pointlist (const std::vector <geo::Vector> &pointlist);
  void write_repetition (const geo::Vector &a, const geo::Vector &b,
                         unsigned long na, unsigned long nb);
  void progress_checkpoint ();

  void write_instance (const geo::Instance &inst);
  void write_box (int layer, int datatype, const geo::Box &box);
  void write_polygon (int layer, int datatype, const geo::Polygon &polygon);
  void write_path (int layer, int datatype, const geo::Path &path);
  void write_edge (int layer, int datatype, const geo::Edge &edge);
  void write_text (int layer, int datatype, const geo::Text &text);
  void write_end_record ();
};

}

#endif
