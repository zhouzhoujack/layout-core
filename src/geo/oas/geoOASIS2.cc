#include "geoOASIS2Reader.h"

#include "geoOASIS2Writer.h"
#include "geoStream.h"
#include "geoOASIS2.h"

#include "utilsClassRegistry.h"
#include "utilsGLog.h"

#include <cstring>

namespace geo
{

// ---------------------------------------------------------------
//  OASIS format declaration

class OASIS2FormatDeclaration
  : public geo::StreamFormatDeclaration
{
public:
  OASIS2FormatDeclaration ()
    : StreamFormatDeclaration ()
  {
    LOGI << "init OASIS2FormatDeclaration";
  }

  virtual std::string format_name () const { return "OASIS"; }
  virtual std::string format_desc () const { return "OASIS"; }
  virtual std::string format_title () const { return "OASIS"; }

  virtual std::string file_format () const
  {
    return "OASIS files (*.OAS *.oas *.OAS.gz *.oas.gz)";
  }

  virtual bool detect (geo::InputStream &stream) const
  {
    const char *hdr = stream.get (oasis_magic_length);
    return (hdr != 0 && memcmp (hdr, oasis_magic, oasis_magic_length) == 0);
  }

  virtual ReaderBase *create_reader (geo::InputStream &s) const
  {
    return new geo::OASIS2Reader (s);
  }

  virtual WriterBase *create_writer () const
  {
    return new geo::OASIS2Writer ();
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
    return new geo::WriterOptionsXMLElement<geo::OASIS2WriterOptions> ("oasis",
      geo::make_member (&geo::OASIS2WriterOptions::strict_mode, "strict-mode") +
      geo::make_member (&geo::OASIS2WriterOptions::write_cblocks, "write-cblocks") +
      geo::make_member (&geo::OASIS2WriterOptions::write_cell_bounding_boxes, "write-cell-bounding-boxes") +
      geo::make_member (&geo::OASIS2WriterOptions::max_cellname_length, "max-cellname-length")
    );
  }

  virtual geo::XMLElementBase *xml_reader_options_element () const
  {
    return new geo::ReaderOptionsXMLElement<geo::OASIS2ReaderOptions> ("oasis",
      geo::make_member (&geo::OASIS2ReaderOptions::expect_strict_mode, "expect-strict-mode") +
      geo::make_member (&geo::OASIS2ReaderOptions::read_max_vertex_count, "read-max-vertex-count")
    );
  }
};

static geo::RegisteredClass<geo::StreamFormatDeclaration> format_decl (new OASIS2FormatDeclaration (), 0, "OASIS");

} // namespace geo
