#include "geoGDS2Reader.h"

#include "geoGDS2Writer.h"
#include "geoStream.h"

#include "utilsClassRegistry.h"
#include "utilsGLog.h"

namespace geo
{

// ---------------------------------------------------------------
//  GDS2 format declaration

class GDS2FormatDeclaration
  : public geo::StreamFormatDeclaration
{
public:
    GDS2FormatDeclaration():StreamFormatDeclaration(){
        LOGI << "init GDS2FormatDeclaration";
    }

  virtual std::string format_name () const { return "GDS2"; }
  virtual std::string format_desc () const { return "GDS2"; }
  virtual std::string format_title () const { return "GDS2"; }
  virtual std::string file_format () const { return "GDS2 files (*.GDS *.gds *.gds.gz *.GDS.gz *.GDS2 *.gds2 *.gds2.gz *.GDS2.gz)"; }

  virtual bool detect (geo::InputStream &stream) const 
  {
    const char *hdr = stream.get (4);
    return (hdr && hdr[0] == 0x00 && hdr[1] == 0x06 && hdr[2] == 0x00 && hdr[3] == 0x02);
  }

  virtual ReaderBase *create_reader (geo::InputStream &s) const 
  {
    return new geo::GDS2Reader (s);
  }

  virtual WriterBase *create_writer () const 
  {
    return new geo::GDS2Writer ();
  }

  virtual bool can_read () const
  {
    return true;
  }

  virtual bool can_write () const
  {
    return true;
  }

  virtual geo::XMLElementBase *xml_writer_options_element () const
  {
    return new geo::WriterOptionsXMLElement<geo::GDS2WriterOptions> ("gds2",
      geo::make_member (&geo::GDS2WriterOptions::write_timestamps, "write-timestamps") +
      geo::make_member (&geo::GDS2WriterOptions::write_cell_properties, "write-cell-properties") +
      geo::make_member (&geo::GDS2WriterOptions::write_file_properties, "write-file-properties") +
      geo::make_member (&geo::GDS2WriterOptions::no_zero_length_paths, "no-zero-length-paths") +
      geo::make_member (&geo::GDS2WriterOptions::multi_xy_records, "multi-xy-records") +
      geo::make_member (&geo::GDS2WriterOptions::resolve_skew_arrays, "resolve-skew-arrays") +
      geo::make_member (&geo::GDS2WriterOptions::max_vertex_count, "max-vertex-count") +
      geo::make_member (&geo::GDS2WriterOptions::max_cellname_length, "max-cellname-length") +
      geo::make_member (&geo::GDS2WriterOptions::libname, "libname")
    );
  }

  virtual geo::XMLElementBase *xml_reader_options_element () const
  {
    return new geo::ReaderOptionsXMLElement<geo::GDS2ReaderOptions> ("gds2",
      geo::make_member (&geo::GDS2ReaderOptions::box_mode, "box-mode") +
      geo::make_member (&geo::GDS2ReaderOptions::allow_big_records, "allow-big-records") +
      geo::make_member (&geo::GDS2ReaderOptions::allow_multi_xy_records, "allow-multi-xy-records")
    );
  }
};

static geo::RegisteredClass<geo::StreamFormatDeclaration> format_decl (new GDS2FormatDeclaration (), 0, "GDS2");

} // namespace geo

