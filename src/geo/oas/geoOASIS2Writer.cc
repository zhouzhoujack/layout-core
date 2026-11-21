#include "geoOASIS2Writer.h"
#include "geoOASIS2.h"

#include "geoLayout.h"
#include "geoShape.h"
#include "geoSaveLayoutOptions.h"

#include "utilsStream.h"
#include "utilsAssert.h"
#include "utilsException.h"
#include "utilsGLog.h"
#include "utilsString.h"

#include <cmath>
#include <cstring>
#include <limits>
#include <map>
#include <set>
#include <vector>

namespace geo
{

// -----------------------------------------------------------------------------
//  OASIS2Writer implementation

OASIS2Writer::OASIS2Writer ()
  : mp_stream (0),
    m_sf (1.0),
    m_progress (geo::to_string (tr ("Writing OASIS file")), 10000)
{
  m_progress.set_format (geo::to_string (tr ("%.0f MB")));
  m_progress.set_unit (1024 * 1024);
}

// ------------------------------------------------------------------
//  low level output

void
OASIS2Writer::write_byte (unsigned char b)
{
  char c = char (b);
  mp_stream->put (&c, 1);
}

void
OASIS2Writer::write_bytes (const char *b, size_t n)
{
  mp_stream->put (b, n);
}

void
OASIS2Writer::write_record (unsigned char r)
{
  write_byte (r);
}

void
OASIS2Writer::write_uint (uint64_t n)
{
  char buffer [16];
  size_t i = 0;

  do {
    unsigned char b = n & 0x7f;
    n >>= 7;
    if (n > 0) {
      b |= 0x80;
    }
    buffer [i++] = char (b);
  } while (n > 0);

  write_bytes (buffer, i);
}

void
OASIS2Writer::write_int (int64_t n)
{
  if (n < 0) {
    write_uint ((uint64_t (-n) << 1) | 1);
  } else {
    write_uint (uint64_t (n) << 1);
  }
}

void
OASIS2Writer::write_real (double d)
{
  if (fabs (d) >= 0.5 &&
      fabs (floor (d + 0.5) - d) < 1e-10 &&
      fabs (d) < double (std::numeric_limits<int64_t>::max ())) {

    //  whole number
    if (d < 0.0) {
      write_byte (rZERO_NEG);
      write_uint (uint64_t (floor (-d + 0.5)));
    } else {
      write_byte (rZERO);
      write_uint (uint64_t (floor (d + 0.5)));
    }

  } else {

    write_byte (rDOUBLE);

    union {
      double d;
      uint64_t i;
    } f2i;

    f2i.d = d;

    uint64_t v = f2i.i;
    char buffer [8];
    for (int i = 0; i < 8; ++i) {
      buffer [i] = char (v & 0xff);
      v >>= 8;
    }

    write_bytes (buffer, 8);

  }
}

void
OASIS2Writer::write_string (const std::string &s)
{
  write_uint (uint64_t (s.size ()));
  if (! s.empty ()) {
    write_bytes (s.c_str (), s.size ());
  }
}

void
OASIS2Writer::write_coord (geo::Coord c)
{
  write_int (int64_t (m_sf == 1.0 ? c : geo::Coord (floor (m_sf * double (c) + 0.5))));
}

void
OASIS2Writer::write_ucoord (geo::Coord c)
{
  geo::Coord s = (m_sf == 1.0 ? c : geo::Coord (floor (m_sf * double (c) + 0.5)));
  write_uint (uint64_t (s < 0 ? -s : s));
}

void
OASIS2Writer::write_gdelta (const geo::Vector &p)
{
  geo::Coord x = (m_sf == 1.0 ? p.x () : geo::Coord (floor (m_sf * double (p.x ()) + 0.5)));
  geo::Coord y = (m_sf == 1.0 ? p.y () : geo::Coord (floor (m_sf * double (p.y ()) + 0.5)));

  if (x == -y || x == y || x == 0 || y == 0) {

    //  single component form
    uint64_t dir = 0;
    uint64_t l = 0;

    if (x > 0) {
      l = uint64_t (x);
      if (y == 0) {
        dir = 0;
      } else if (y < 0) {
        dir = 7;
      } else {
        dir = 4;
      }
    } else if (x == 0) {
      if (y < 0) {
        l = uint64_t (-y);
        dir = 3;
      } else {
        l = uint64_t (y);
        dir = 1;
      }
    } else {
      l = uint64_t (-x);
      if (y == 0) {
        dir = 2;
      } else if (y < 0) {
        dir = 6;
      } else {
        dir = 5;
      }
    }

    write_uint ((l << 4) | (dir << 1));

  } else {

    //  two component form
    uint64_t d;
    if (x < 0) {
      d = (uint64_t (-x) << 2) | 3;
    } else {
      d = (uint64_t (x) << 2) | 1;
    }

    write_uint (d);
    write_int (int64_t (y));

  }
}

void
OASIS2Writer::write_pointlist (const std::vector <geo::Vector> &pointlist)
{
  //  always use the generic point list form
  write_byte (plGENERIC);
  write_uint (uint64_t (pointlist.size ()));

  geo::Vector last;
  for (std::vector <geo::Vector>::const_iterator p = pointlist.begin (); p != pointlist.end (); ++p) {
    write_gdelta (*p - last);
    last = *p;
  }
}

void
OASIS2Writer::write_repetition (const geo::Vector &a_in, const geo::Vector &b_in,
                                unsigned long na_in, unsigned long nb_in)
{
  geo::Vector a = a_in, b = b_in;
  unsigned long na = na_in, nb = nb_in;

  //  reduce trivial dimensions
  if (na <= 1 && nb <= 1) {
    //  no repetition - express it as a singular array
    write_byte (rpGENERIC_1D);
    write_uint (0);
    write_gdelta (geo::Vector ());
    return;
  }

  if (nb <= 1) {
    b = geo::Vector ();
    nb = 1;
  } else if (na <= 1) {
    a = b;
    na = nb;
    b = geo::Vector ();
    nb = 1;
  }

  if (b == geo::Vector ()) {

    //  one dimensional
    if (a.y () == 0 && a.x () >= 0) {
      write_byte (rpGRID_X);
      write_uint (uint64_t (na) - 2);
      write_ucoord (a.x ());
    } else if (a.x () == 0 && a.y () >= 0) {
      write_byte (rpGRID_Y);
      write_uint (uint64_t (na) - 2);
      write_ucoord (a.y ());
    } else {
      write_byte (rpGENERIC_1D);
      write_uint (uint64_t (na) - 2);
      write_gdelta (a);
    }

  } else if (a.y () == 0 && a.x () >= 0 && b.x () == 0 && b.y () >= 0) {

    write_byte (rpGRID_2D);
    write_uint (uint64_t (na) - 2);
    write_uint (uint64_t (nb) - 2);
    write_ucoord (a.x ());
    write_ucoord (b.y ());

  } else if (b.y () == 0 && b.x () >= 0 && a.x () == 0 && a.y () >= 0) {

    write_byte (rpGRID_2D);
    write_uint (uint64_t (nb) - 2);
    write_uint (uint64_t (na) - 2);
    write_ucoord (b.x ());
    write_ucoord (a.y ());

  } else {

    write_byte (rpGENERIC_2D);
    write_uint (uint64_t (na) - 2);
    write_uint (uint64_t (nb) - 2);
    write_gdelta (a);
    write_gdelta (b);

  }
}

void
OASIS2Writer::progress_checkpoint ()
{
  m_progress.set (mp_stream->pos ());
}

// ------------------------------------------------------------------
//  instances

void
OASIS2Writer::write_instance (const geo::Instance &inst)
{
  const geo::Trans &t = inst.front ();

  geo::Vector a, b;
  unsigned long na = 1, nb = 1;

  bool rep = inst.is_regular_array (a, b, na, nb) && (na > 1 || nb > 1);

  if (inst.is_complex ()) {

    geo::ICplxTrans ct = inst.complex_trans ();

    unsigned char info = iP_HAS_CELL | iP_CELL_BY_REF |
                         iP_HAS_MAGNIFICATION | iP_HAS_ANGLE |
                         iP_HAS_X | iP_HAS_Y;

    if (ct.is_mirror ()) {
      info |= iP_MIRROR;
    }
    if (rep) {
      info |= iP_HAS_REPETITION;
    }

    write_record (tPLACEMENT_TRANSFORM);
    write_byte (info);
    write_uint (uint64_t (inst.cell_index ()));
    write_real (ct.mag ());
    write_real (ct.angle ());
    write_coord (ct.disp ().x ());
    write_coord (ct.disp ().y ());

  } else {

    unsigned char info = iP_HAS_CELL | iP_CELL_BY_REF | iP_HAS_X | iP_HAS_Y;
    info = (unsigned char) (info | (unsigned char) (((t.rot () & 0x03) << 1)));

    if (t.is_mirror ()) {
      info |= iP_MIRROR;
    }
    if (rep) {
      info |= iP_HAS_REPETITION;
    }

    write_record (tPLACEMENT);
    write_byte (info);
    write_uint (uint64_t (inst.cell_index ()));
    write_coord (t.disp ().x ());
    write_coord (t.disp ().y ());

  }

  if (rep) {
    write_repetition (a, b, na, nb);
  }
}

// ------------------------------------------------------------------
//  shapes

void
OASIS2Writer::write_box (int layer, int datatype, const geo::Box &box)
{
  write_record (tRECTANGLE);
  write_byte (iHAS_LAYER | iHAS_DATATYPE | iHAS_WIDTH | iHAS_HEIGHT | iHAS_X | iHAS_Y);
  write_uint (uint64_t (layer));
  write_uint (uint64_t (datatype));
  write_ucoord (geo::Coord (box.width ()));
  write_ucoord (geo::Coord (box.height ()));
  write_coord (box.left ());
  write_coord (box.bottom ());
}

void
OASIS2Writer::write_polygon (int layer, int datatype, const geo::Polygon &polygon)
{
  geo::Polygon::polygon_contour_iterator e = polygon.begin_hull ();
  if (e == polygon.end_hull ()) {
    return;
  }

  if (polygon.holes () > 0) {
    //  OASIS polygons cannot have holes - only the hull is written
    throw geo::Exception (geo::to_string (tr ("Polygons with holes cannot be written to OASIS files (cell ")) +
                            geo::to_string (tr ("unknown")) + ")");
  }

  geo::Point start = *e;

  std::vector <geo::Vector> pointlist;
  pointlist.reserve (polygon.hull ().size ());

  while (++e != polygon.end_hull ()) {
    pointlist.push_back (geo::Vector (*e - start));
  }

  if (pointlist.size () < 2) {
    //  a degenerated polygon - ignore
    return;
  }

  write_record (tPOLYGON);
  write_byte (iHAS_LAYER | iHAS_DATATYPE | iHAS_HEIGHT | iHAS_X | iHAS_Y);
  write_uint (uint64_t (layer));
  write_uint (uint64_t (datatype));
  write_pointlist (pointlist);
  write_coord (start.x ());
  write_coord (start.y ());
}

void
OASIS2Writer::write_path (int layer, int datatype, const geo::Path &path)
{
  if (path.begin () == path.end ()) {
    return;
  }

  geo::Point start = *path.begin ();

  std::vector <geo::Vector> pointlist;
  pointlist.reserve (path.points ());

  for (geo::Path::iterator p = path.begin (); p != path.end (); ++p) {
    if (p != path.begin ()) {
      pointlist.push_back (geo::Vector (*p - start));
    }
  }

  if (pointlist.empty ()) {
    //  single point paths are not representable as OASIS paths
    return;
  }

  geo::Coord width = path.width ();
  geo::Coord hw = geo::Coord (width / 2);
  std::pair <geo::Coord, geo::Coord> ext = path.extensions ();

  unsigned char ext_scheme = 0;
  if (ext.first == 0) {
    ext_scheme |= 0x04;
  } else if (ext.first == hw) {
    ext_scheme |= 0x08;
  } else {
    ext_scheme |= 0x0c;
  }
  if (ext.second == 0) {
    ext_scheme |= 0x01;
  } else if (ext.second == hw) {
    ext_scheme |= 0x02;
  } else {
    ext_scheme |= 0x03;
  }

  write_record (tPATH);
  write_byte (iHAS_LAYER | iHAS_DATATYPE | iHAS_WIDTH | iHAS_VERTICAL | iHAS_HEIGHT | iHAS_X | iHAS_Y);
  write_uint (uint64_t (layer));
  write_uint (uint64_t (datatype));
  write_ucoord (hw);
  write_byte (ext_scheme);
  if ((ext_scheme & 0x0c) == 0x0c) {
    write_coord (ext.first);
  }
  if ((ext_scheme & 0x03) == 0x03) {
    write_coord (ext.second);
  }
  write_pointlist (pointlist);
  write_coord (start.x ());
  write_coord (start.y ());
}

void
OASIS2Writer::write_edge (int layer, int datatype, const geo::Edge &edge)
{
  //  an edge becomes a zero-width path
  write_record (tPATH);
  write_byte (iHAS_LAYER | iHAS_DATATYPE | iHAS_WIDTH | iHAS_HEIGHT | iHAS_X | iHAS_Y);
  write_uint (uint64_t (layer));
  write_uint (uint64_t (datatype));
  write_ucoord (0);
  write_byte (0x05);   //  no extensions
  std::vector <geo::Vector> pointlist;
  pointlist.push_back (geo::Vector (edge.p2 () - edge.p1 ()));
  write_pointlist (pointlist);
  write_coord (edge.p1 ().x ());
  write_coord (edge.p1 ().y ());
}

void
OASIS2Writer::write_text (int layer, int datatype, const geo::Text &text)
{
  write_record (tTEXT);
  write_byte (iT_HAS_STRING | iHAS_LAYER | iHAS_DATATYPE | iHAS_X | iHAS_Y);
  write_string (text.string ());
  write_uint (uint64_t (layer));
  write_uint (uint64_t (datatype));
  write_coord (text.trans ().disp ().x ());
  write_coord (text.trans ().disp ().y ());
}

// ------------------------------------------------------------------
//  the writer

void
OASIS2Writer::write (geo::Layout &layout, geo::OutputStream &stream, const geo::SaveLayoutOptions &options)
{
  mp_stream = &stream;
  m_options = options.get_options<geo::OASIS2WriterOptions> ();

  if (m_options.strict_mode) {
    LOGW << "OASIS strict mode is not implemented - writing a non-strict file";
  }

  double dbu = (options.dbu () == 0.0) ? layout.dbu () : options.dbu ();
  m_sf = options.scale_factor () * (layout.dbu () / dbu);
  if (fabs (m_sf - 1.0) < 1e-9) {
    m_sf = 1.0;
  }

  std::vector <std::pair <unsigned int, geo::LayerProperties> > layers;
  options.get_valid_layers (layout, layers, geo::SaveLayoutOptions::LP_AssignNumber);

  std::set <geo::cell_index_type> cell_set;
  options.get_cells (layout, cell_set, layers);

  //  collect the cells to write (bottom-up)
  std::vector <geo::cell_index_type> cells;
  cells.reserve (cell_set.size ());

  for (geo::Layout::bottom_up_const_iterator cell = layout.begin_bottom_up (); cell != layout.end_bottom_up (); ++cell) {
    if (cell_set.find (*cell) != cell_set.end ()) {
      cells.push_back (*cell);
    }
  }

  //  cell name map - the names must be unique
  std::map <geo::cell_index_type, std::string> cell_names;
  std::set <std::string> used_names;

  for (std::vector <geo::cell_index_type>::const_iterator c = cells.begin (); c != cells.end (); ++c) {

    std::string name = layout.cell_name (*c);

    if (m_options.max_cellname_length > 4 && name.size () > m_options.max_cellname_length) {
      name = name.substr (0, m_options.max_cellname_length);
    }

    if (name.empty () || used_names.find (name) != used_names.end ()) {

      std::string prefix = name.empty () ? std::string ("CELL") : name;
      for (int n = 0; ; ++n) {
        name = prefix + geo::sprintf ("_%d", n);
        if (used_names.find (name) == used_names.end ()) {
          break;
        }
      }

    }

    used_names.insert (name);
    cell_names.insert (std::make_pair (*c, name));

  }

  //  header

  write_bytes (oasis_magic, oasis_magic_length);

  write_record (tSTART);
  write_string ("1.0");
  write_real (1.0 / dbu);
  write_uint (0);   //  offset table at the beginning (non-strict mode)

  //  offset table placeholder: 6 pairs of (offset, size) - CELLNAME,
  //  TEXTSTRING, PROPNAME, PROPSTRING, LAYERNAME, XNAME
  for (unsigned int i = 0; i < 12; ++i) {
    write_byte (0);
  }

  //  CELLNAME table (explicit ids so the CELL records can refer to them)

  for (std::vector <geo::cell_index_type>::const_iterator c = cells.begin (); c != cells.end (); ++c) {
    write_record (tCELLNAME_EXPLICIT);
    write_string (cell_names [*c]);
    write_uint (uint64_t (*c));
  }

  //  cells

  for (std::vector <geo::cell_index_type>::const_iterator c = cells.begin (); c != cells.end (); ++c) {

    progress_checkpoint ();

    const geo::Cell &cref (layout.cell (*c));

    //  skip ghost cells unless they have content
    if (cref.is_ghost_cell () && cref.empty ()) {
      continue;
    }

    write_record (tCELL_REFNUM);
    write_uint (uint64_t (*c));

    //  instances
    for (geo::Cell::const_iterator inst = cref.begin (); ! inst.at_end (); ++inst) {

      //  write only instances to selected cells
      if (options.keep_instances () || cell_set.find (inst->cell_index ()) != cell_set.end ()) {
        progress_checkpoint ();
        write_instance (*inst);
      }

    }

    //  shapes
    for (std::vector <std::pair <unsigned int, geo::LayerProperties> >::const_iterator l = layers.begin (); l != layers.end (); ++l) {

      if (! layout.is_valid_layer (l->first)) {
        continue;
      }

      int layer = l->second.layer;
      int datatype = l->second.datatype;

      geo::ShapeIterator shape (cref.shapes (l->first).begin (geo::ShapeIterator::Boxes |
                                                             geo::ShapeIterator::Polygons |
                                                             geo::ShapeIterator::Paths |
                                                             geo::ShapeIterator::Texts |
                                                             geo::ShapeIterator::Edges |
                                                             geo::ShapeIterator::EdgePairs));

      while (! shape.at_end ()) {

        progress_checkpoint ();

        if (shape->is_box ()) {
          write_box (layer, datatype, shape->box ());
        } else if (shape->is_polygon ()) {
          geo::Polygon poly;
          shape->polygon (poly);
          write_polygon (layer, datatype, poly);
        } else if (shape->is_path ()) {
          geo::Path path;
          shape->path (path);
          write_path (layer, datatype, path);
        } else if (shape->is_text ()) {
          geo::Text text;
          shape->text (text);
          write_text (layer, datatype, text);
        } else if (shape->is_edge ()) {
          geo::Edge edge;
          shape->edge (edge);
          write_edge (layer, datatype, edge);
        } else if (shape->is_edge_pair ()) {
          geo::EdgePair ep;
          shape->edge_pair (ep);
          write_edge (layer, datatype, ep.first ());
          write_edge (layer, datatype, ep.second ());
        }

        ++shape;

      }

    }

  }

  //  END record and the padding that brings the file size to a multiple of
  //  256 bytes (the padding must be at least 254 bytes)
  write_end_record ();

  progress_checkpoint ();
}

void
OASIS2Writer::write_end_record ()
{
  write_record (tEND);

  size_t pad = 256 - (mp_stream->pos () % 256);
  while (pad < 254) {
    pad += 256;
  }

  for (size_t i = 0; i < pad; ++i) {
    write_byte (0);
  }
}

}

