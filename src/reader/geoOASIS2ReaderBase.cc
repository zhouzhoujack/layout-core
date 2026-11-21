#include "geoOASIS2ReaderBase.h"
#include "geoOASIS2.h"

#include "utilsString.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

namespace geo
{

//  oasis_magic 的唯一定义在 geo/oas/geoOASIS2Magic.cc（db-io），
//  这里通过 geoOASIS2.h 的 extern 声明使用，避免两个库各自定义。

//  a safety limit for repetition expansion - protects against corrupted files
static const uint64_t max_repetition_count = 10000000;

// -----------------------------------------------------------------------------
//  OASIS2ReaderBase implementation

OASIS2ReaderBase::OASIS2ReaderBase ()
  : m_expect_strict_mode (-1),
    m_read_texts (true),
    m_read_properties (true)
{
  reset_modal_variables ();
}

OASIS2ReaderBase::~OASIS2ReaderBase ()
{
  //  .. nothing yet ..
}

void
OASIS2ReaderBase::init (const geo::LoadLayoutOptions &options)
{
  CommonReader::init (options);

  m_read_texts = common_options ().enable_text_objects;
  m_read_properties = common_options ().enable_properties;

  geo::OASIS2ReaderOptions oasis_options = options.get_options<geo::OASIS2ReaderOptions> ();
  m_expect_strict_mode = oasis_options.expect_strict_mode;
}

void
OASIS2ReaderBase::reset_modal_variables ()
{
  m_layer = 0;
  m_datatype = 0;
  m_geometry_x = 0;
  m_geometry_y = 0;
  m_geometry_w = 0;
  m_geometry_h = 0;
  m_polygon_point_list.clear ();
  m_path_point_list.clear ();
  m_path_halfwidth = 0;
  m_path_start_extension = 0;
  m_path_end_extension = 0;
  m_circle_radius = 0;
  m_placement_cell = 0;
  m_placement_cell_valid = false;
  m_placement_x = 0;
  m_placement_y = 0;
  m_repetition.clear ();
}

// -----------------------------------------------------------------------------
//  coordinate decoders

geo::Coord
OASIS2ReaderBase::get_coord ()
{
  int64_t l = get_int ();
  if (l < (int64_t) std::numeric_limits<geo::Coord>::min () ||
      l > (int64_t) std::numeric_limits<geo::Coord>::max ()) {
    error (geo::to_string (tr ("Coordinate value overflow")));
  }
  return geo::Coord (l);
}

uint64_t
OASIS2ReaderBase::get_ucoord ()
{
  return get_uint ();
}

geo::Vector
OASIS2ReaderBase::get_2delta ()
{
  uint64_t l1 = get_uint ();
  int64_t lx = int64_t (l1 >> 2);
  if (lx > (int64_t) std::numeric_limits<geo::Coord>::max ()) {
    error (geo::to_string (tr ("Coordinate value overflow")));
  }
  geo::Coord x = geo::Coord (lx);

  switch (l1 & 3) {
  case 0:
    return geo::Vector (x, 0);
  case 1:
    return geo::Vector (0, x);
  case 2:
    return geo::Vector (-x, 0);
  case 3:
  default:
    return geo::Vector (0, -x);
  }
}

geo::Vector
OASIS2ReaderBase::get_3delta ()
{
  uint64_t l1 = get_uint ();
  int64_t lx = int64_t (l1 >> 3);
  if (lx > (int64_t) std::numeric_limits<geo::Coord>::max ()) {
    error (geo::to_string (tr ("Coordinate value overflow")));
  }
  geo::Coord x = geo::Coord (lx);

  switch (l1 & 7) {
  case 0:
    return geo::Vector (x, 0);
  case 1:
    return geo::Vector (0, x);
  case 2:
    return geo::Vector (-x, 0);
  case 3:
    return geo::Vector (0, -x);
  case 4:
    return geo::Vector (x, x);
  case 5:
    return geo::Vector (-x, x);
  case 6:
    return geo::Vector (-x, -x);
  case 7:
  default:
    return geo::Vector (x, -x);
  }
}

geo::Vector
OASIS2ReaderBase::get_gdelta ()
{
  uint64_t l1 = get_uint ();

  if ((l1 & 1) != 0) {

    //  two component form: x with an explicit sign bit, y as a signed value
    int64_t lx = int64_t (l1 >> 2);
    if ((l1 & 2) != 0) {
      lx = -lx;
    }
    int64_t ly = get_int ();

    if (lx < (int64_t) std::numeric_limits<geo::Coord>::min () ||
        lx > (int64_t) std::numeric_limits<geo::Coord>::max () ||
        ly < (int64_t) std::numeric_limits<geo::Coord>::min () ||
        ly > (int64_t) std::numeric_limits<geo::Coord>::max ()) {
      error (geo::to_string (tr ("Coordinate value overflow")));
    }

    return geo::Vector (geo::Coord (lx), geo::Coord (ly));

  } else {

    //  one component form: magnitude plus a direction code
    int64_t lx = int64_t (l1 >> 4);
    if (lx > (int64_t) std::numeric_limits<geo::Coord>::max ()) {
      error (geo::to_string (tr ("Coordinate value overflow")));
    }
    geo::Coord x = geo::Coord (lx);

    switch ((l1 >> 1) & 7) {
    case 0:
      return geo::Vector (x, 0);
    case 1:
      return geo::Vector (0, x);
    case 2:
      return geo::Vector (-x, 0);
    case 3:
      return geo::Vector (0, -x);
    case 4:
      return geo::Vector (x, x);
    case 5:
      return geo::Vector (-x, x);
    case 6:
      return geo::Vector (-x, -x);
    case 7:
    default:
      return geo::Vector (x, -x);
    }

  }
}

// -----------------------------------------------------------------------------
//  point lists

void
OASIS2ReaderBase::read_pointlist (std::vector <geo::Point> &pointlist, bool for_polygon)
{
  uint32_t type = uint32_t (get_uint ());

  uint64_t n = get_uint ();
  if (n == 0) {
    error (geo::to_string (tr ("Invalid point list: length is zero")));
  }
  if (n > max_repetition_count) {
    error (geo::to_string (tr ("Invalid point list: too many points")));
  }

  pointlist.clear ();
  pointlist.reserve (size_t (n) + 2);

  //  the first point is implicitly the origin - the other points are encoded
  //  as deltas relative to the shape position
  pointlist.push_back (geo::Point ());

  if (type == plMANHATTAN_H || type == plMANHATTAN_V) {

    bool h = (type == plMANHATTAN_H);

    geo::Point pos;
    for (uint64_t i = 0; i < n; ++i) {
      geo::Coord d = get_coord ();
      if (h) {
        pos += geo::Vector (d, 0);
      } else {
        pos += geo::Vector (0, d);
      }
      h = ! h;
      pointlist.push_back (pos);
    }

    //  for polygons the closing point is implied - synthesize it
    if (for_polygon) {
      if ((n % 2) != 0) {
        warn (geo::to_string (tr ("Type 0 or 1 point list with odd number of points is illegal")));
      }
      if (h) {
        pointlist.push_back (geo::Point (0, pos.y ()));
      } else {
        pointlist.push_back (geo::Point (pos.x (), 0));
      }
    }

  } else if (type == plDOUBLE_DELTA) {

    geo::Point pos;
    for (uint64_t i = 0; i < n; ++i) {
      pos += get_2delta ();
      pointlist.push_back (pos);
    }

  } else if (type == plTRIPLE_DELTA) {

    geo::Point pos;
    for (uint64_t i = 0; i < n; ++i) {
      pos += get_3delta ();
      pointlist.push_back (pos);
    }

  } else if (type == plGENERIC) {

    geo::Point pos;
    for (uint64_t i = 0; i < n; ++i) {
      pos += get_gdelta ();
      pointlist.push_back (pos);
    }

  } else if (type == plGENERIC_ACCUMULATE) {

    geo::Point pos;
    geo::Vector delta;
    for (uint64_t i = 0; i < n; ++i) {
      delta += get_gdelta ();
      pos += delta;
      pointlist.push_back (pos);
    }

  } else {

    error (geo::sprintf (geo::to_string (tr ("Invalid point list type %d")), int (type)));

  }
}

// -----------------------------------------------------------------------------
//  repetitions
//
//  Repetitions are expanded into a plain list of displacement vectors. This is
//  simple, correct and independent of the array representations of the geometry
//  library. The modal repetition is kept in m_repetition.

bool
OASIS2ReaderBase::read_repetition ()
{
  std::vector <geo::Vector> modal = m_repetition;
  m_repetition.clear ();

  uint32_t type = uint32_t (get_uint ());

  if (type == rpREUSE) {

    m_repetition = modal;

  } else if (type == rpGRID_2D) {

    uint64_t nx = get_uint ();
    uint64_t ny = get_uint ();
    geo::Coord dx = geo::Coord (get_ucoord ());
    geo::Coord dy = geo::Coord (get_ucoord ());

    uint64_t nn = (dx == 0) ? 1 : nx + 2;
    uint64_t mm = (dy == 0) ? 1 : ny + 2;
    if (nn * mm > max_repetition_count) {
      error (geo::to_string (tr ("Repetition is too large")));
    }

    for (uint64_t j = 0; j < mm; ++j) {
      for (uint64_t i = 0; i < nn; ++i) {
        m_repetition.push_back (geo::Vector (geo::Coord (dx * int64_t (i)), geo::Coord (dy * int64_t (j))));
      }
    }

  } else if (type == rpGRID_X || type == rpGRID_Y) {

    uint64_t nx = get_uint ();
    geo::Coord d = geo::Coord (get_ucoord ());

    uint64_t nn = (d == 0) ? 1 : nx + 2;
    if (nn > max_repetition_count) {
      error (geo::to_string (tr ("Repetition is too large")));
    }

    for (uint64_t i = 0; i < nn; ++i) {
      if (type == rpGRID_X) {
        m_repetition.push_back (geo::Vector (geo::Coord (d * int64_t (i)), 0));
      } else {
        m_repetition.push_back (geo::Vector (0, geo::Coord (d * int64_t (i))));
      }
    }

  } else if (type == rpIRREGULAR_X || type == rpIRREGULAR_X_GRID ||
             type == rpIRREGULAR_Y || type == rpIRREGULAR_Y_GRID) {

    bool horizontal = (type == rpIRREGULAR_X || type == rpIRREGULAR_X_GRID);

    uint64_t cnt = get_uint ();
    uint64_t grid = 1;
    if (type == rpIRREGULAR_X_GRID || type == rpIRREGULAR_Y_GRID) {
      grid = get_uint ();
      if (grid == 0) {
        error (geo::to_string (tr ("Divider must not be zero")));
      }
    }

    if (cnt > max_repetition_count) {
      error (geo::to_string (tr ("Repetition is too large")));
    }

    geo::Coord acc = 0;
    for (uint64_t i = 0; i <= cnt; ++i) {
      geo::Coord d = geo::Coord (get_ucoord () * grid);
      acc = geo::Coord (acc + d);
      if (horizontal) {
        m_repetition.push_back (geo::Vector (acc, 0));
      } else {
        m_repetition.push_back (geo::Vector (0, acc));
      }
    }

  } else if (type == rpGENERIC_2D) {

    uint64_t nx = get_uint ();
    uint64_t ny = get_uint ();
    geo::Vector dn = get_gdelta ();
    geo::Vector dm = get_gdelta ();

    uint64_t nn = (dn == geo::Vector ()) ? 1 : nx + 2;
    uint64_t mm = (dm == geo::Vector ()) ? 1 : ny + 2;
    if (nn * mm > max_repetition_count) {
      error (geo::to_string (tr ("Repetition is too large")));
    }

    for (uint64_t j = 0; j < mm; ++j) {
      for (uint64_t i = 0; i < nn; ++i) {
        m_repetition.push_back (geo::Vector (dn * int64_t (i) + dm * int64_t (j)));
      }
    }

  } else if (type == rpGENERIC_1D) {

    uint64_t nx = get_uint ();
    geo::Vector dn = get_gdelta ();

    uint64_t nn = (dn == geo::Vector ()) ? 1 : nx + 2;
    if (nn > max_repetition_count) {
      error (geo::to_string (tr ("Repetition is too large")));
    }

    for (uint64_t i = 0; i < nn; ++i) {
      m_repetition.push_back (geo::Vector (dn * int64_t (i)));
    }

  } else if (type == rpGENERIC_ITERATE || type == rpGENERIC_ITER_GRID) {

    uint64_t cnt = get_uint ();
    uint64_t grid = 1;
    if (type == rpGENERIC_ITER_GRID) {
      grid = get_uint ();
      if (grid == 0) {
        error (geo::to_string (tr ("Divider must not be zero")));
      }
    }

    if (cnt > max_repetition_count) {
      error (geo::to_string (tr ("Repetition is too large")));
    }

    geo::Vector pos;

    //  iterated repetitions do not contain the base offset - it is implicit
    m_repetition.push_back (geo::Vector ());

    for (uint64_t i = 0; i <= cnt; ++i) {
      geo::Vector d = get_gdelta ();
      pos += geo::Vector (d.x () * int64_t (grid), d.y () * int64_t (grid));
      m_repetition.push_back (pos);
    }

  } else {

    error (geo::sprintf (geo::to_string (tr ("Invalid repetition type %d")), int (type)));

  }

  return ! m_repetition.empty ();
}

// -----------------------------------------------------------------------------
//  properties
//
//  Properties are parsed so they can be skipped properly. They are not attached
//  to the shapes (the geometry model has no property support for OASIS yet).

void
OASIS2ReaderBase::read_property ()
{
  unsigned char m = get_byte ();

  if ((m & 0x04) != 0) {
    if ((m & 0x02) != 0) {
      //  property name by reference number
      get_uint ();
    } else {
      //  property name as a direct string
      get_string ();
    }
  }

  if ((m & 0x08) != 0) {
    //  the value list is the same as that of the previous property - no data
    return;
  }

  {

    //  inline value list
    uint64_t n = uint64_t ((m >> 4) & 0x0f);
    if (n == 15) {
      n = get_uint ();
    }
    if (n > 1000000) {
      error (geo::to_string (tr ("Property value list is too long")));
    }

    for (uint64_t i = 0; i < n; ++i) {

      unsigned char t = get_byte ();
      if (t < 8) {
        //  a real number - put the type byte back and read the value
        unget_byte ();
        get_real ();
      } else if (t == 8 || t == 9) {
        //  unsigned/signed integer
        get_uint ();
      } else if (t == 10 || t == 11 || t == 12) {
        //  directly encoded string
        get_string ();
      } else if (t == 13 || t == 14 || t == 15) {
        //  string by reference number
        get_uint ();
      } else {
        error (geo::sprintf (geo::to_string (tr ("Invalid property value type %d")), int (t)));
      }

    }

  }
}

void
OASIS2ReaderBase::skip_element_properties ()
{
  while (true) {

    unsigned char m = get_byte ();

    if (m == tPAD) {
      //  skip
    } else if (m == tPROPERTY) {
      read_property ();
    } else if (m == tPROPERTY_REPEAT) {
      //  the repeated property carries no payload
    } else if (m == tCBLOCK) {
      uint64_t type = get_uint ();
      if (type != 0) {
        error (geo::sprintf (geo::to_string (tr ("Invalid CBLOCK compression type %d")), int (type)));
      }
      get_uint ();  //  uncompressed byte count
      get_uint ();  //  compressed byte count
      inflate_stream ();
    } else {
      unget_byte ();
      break;
    }

  }
}

// -----------------------------------------------------------------------------
//  shape emitters
//
//  The shapes are built with the displacement applied. This makes the handling
//  of repetitions trivial.

void
OASIS2ReaderBase::emit_box (geo::Layout &layout, geo::cell_index_type cell_index,
                            uint32_t layer, uint32_t datatype, const geo::Box &box)
{
  std::pair<bool, unsigned int> ll = open_dl (layout, LDPair (ld_type (layer), ld_type (datatype)));
  if (! ll.first || box.empty ()) {
    return;
  }

  geo::Cell &cell = layout.cell (cell_index);

  if (m_repetition.empty ()) {
    cell.shapes (ll.second).insert (box);
  } else {
    for (size_t i = 0; i < m_repetition.size (); ++i) {
      cell.shapes (ll.second).insert (box.moved (m_repetition [i]));
    }
  }
}

void
OASIS2ReaderBase::emit_polygon (geo::Layout &layout, geo::cell_index_type cell_index,
                                uint32_t layer, uint32_t datatype,
                                const std::vector <geo::Point> &points, const geo::Vector &pos)
{
  if (points.size () < 3) {
    return;
  }

  std::pair<bool, unsigned int> ll = open_dl (layout, LDPair (ld_type (layer), ld_type (datatype)));
  if (! ll.first) {
    return;
  }

  geo::Cell &cell = layout.cell (cell_index);

  size_t nrep = m_repetition.empty () ? 1 : m_repetition.size ();
  std::vector <geo::Point> shifted;
  shifted.reserve (points.size ());

  for (size_t r = 0; r < nrep; ++r) {

    geo::Vector offset = pos;
    if (! m_repetition.empty ()) {
      offset += m_repetition [r];
    }

    shifted.clear ();
    for (std::vector <geo::Point>::const_iterator p = points.begin (); p != points.end (); ++p) {
      shifted.push_back (geo::Point (geo::Coord (p->x () + offset.x ()), geo::Coord (p->y () + offset.y ())));
    }

    geo::SimplePolygon poly;
    poly.assign_hull (shifted.begin (), shifted.end (), false /*no compression*/);
    cell.shapes (ll.second).insert (geo::SimplePolygonRef (poly, layout.shape_repository ()));

  }
}

void
OASIS2ReaderBase::emit_path (geo::Layout &layout, geo::cell_index_type cell_index,
                             uint32_t layer, uint32_t datatype,
                             const std::vector <geo::Point> &points, geo::Coord width,
                             geo::Coord start_extension, geo::Coord end_extension, bool round,
                             const geo::Vector &pos)
{
  if (points.size () < 1) {
    return;
  }

  std::pair<bool, unsigned int> ll = open_dl (layout, LDPair (ld_type (layer), ld_type (datatype)));
  if (! ll.first) {
    return;
  }

  geo::Cell &cell = layout.cell (cell_index);

  size_t nrep = m_repetition.empty () ? 1 : m_repetition.size ();
  std::vector <geo::Point> shifted;
  shifted.reserve (points.size ());

  for (size_t r = 0; r < nrep; ++r) {

    geo::Vector offset = pos;
    if (! m_repetition.empty ()) {
      offset += m_repetition [r];
    }

    shifted.clear ();
    for (std::vector <geo::Point>::const_iterator p = points.begin (); p != points.end (); ++p) {
      shifted.push_back (geo::Point (geo::Coord (p->x () + offset.x ()), geo::Coord (p->y () + offset.y ())));
    }

    geo::Path moved;
    moved.width (width);
    moved.extensions (start_extension, end_extension);
    moved.round (round);
    moved.assign (shifted.begin (), shifted.end ());

    cell.shapes (ll.second).insert (geo::PathRef (moved, layout.shape_repository ()));

  }
}

void
OASIS2ReaderBase::emit_text (geo::Layout &layout, geo::cell_index_type cell_index,
                             uint32_t layer, uint32_t datatype,
                             const std::string &text, const geo::Vector &pos)
{
  if (! m_read_texts) {
    return;
  }

  std::pair<bool, unsigned int> ll = open_dl (layout, LDPair (ld_type (layer), ld_type (datatype)));
  if (! ll.first) {
    return;
  }

  geo::Cell &cell = layout.cell (cell_index);

  size_t nrep = m_repetition.empty () ? 1 : m_repetition.size ();

  for (size_t r = 0; r < nrep; ++r) {

    geo::Vector offset = pos;
    if (! m_repetition.empty ()) {
      offset += m_repetition [r];
    }

    geo::Text text_obj (text, geo::Trans (0, false, offset), 0, geo::NoFont, geo::NoHAlign, geo::NoVAlign);
    cell.shapes (ll.second).insert (geo::TextRef (text_obj, layout.shape_repository ()));

  }
}

// -----------------------------------------------------------------------------
//  geometry records

void
OASIS2ReaderBase::read_rectangle (geo::Layout &layout, geo::cell_index_type cell_index, bool xy_absolute)
{
  unsigned char m = get_byte ();

  if ((m & iHAS_LAYER) != 0) {
    m_layer = uint32_t (get_uint ());
  }
  if ((m & iHAS_DATATYPE) != 0) {
    m_datatype = uint32_t (get_uint ());
  }

  if ((m & iHAS_WIDTH) != 0) {
    m_geometry_w = get_ucoord ();
  }
  if ((m & iHAS_VERTICAL) != 0) {
    //  square
    m_geometry_h = m_geometry_w;
  } else if ((m & iHAS_HEIGHT) != 0) {
    m_geometry_h = get_ucoord ();
  }

  if ((m & iHAS_X) != 0) {
    geo::Coord x = get_coord ();
    m_geometry_x = xy_absolute ? x : geo::Coord (m_geometry_x + x);
  }
  if ((m & iHAS_Y) != 0) {
    geo::Coord y = get_coord ();
    m_geometry_y = xy_absolute ? y : geo::Coord (m_geometry_y + y);
  }

  if ((m & iHAS_REPETITION) != 0) {
    read_repetition ();
  } else {
    m_repetition.clear ();
  }

  skip_element_properties ();

  if (m_geometry_w != 0 && m_geometry_h != 0) {
    geo::Box box (geo::Point (m_geometry_x, m_geometry_y),
                  geo::Point (geo::Coord (m_geometry_x + geo::Coord (m_geometry_w)),
                              geo::Coord (m_geometry_y + geo::Coord (m_geometry_h))));
    emit_box (layout, cell_index, m_layer, m_datatype, box);
  }

  m_repetition.clear ();
}

void
OASIS2ReaderBase::read_polygon (geo::Layout &layout, geo::cell_index_type cell_index, bool xy_absolute)
{
  unsigned char m = get_byte ();

  if ((m & iHAS_LAYER) != 0) {
    m_layer = uint32_t (get_uint ());
  }
  if ((m & iHAS_DATATYPE) != 0) {
    m_datatype = uint32_t (get_uint ());
  }
  if ((m & iHAS_HEIGHT) != 0) {
    read_pointlist (m_polygon_point_list, true);
  }

  if ((m & iHAS_X) != 0) {
    geo::Coord x = get_coord ();
    m_geometry_x = xy_absolute ? x : geo::Coord (m_geometry_x + x);
  }
  if ((m & iHAS_Y) != 0) {
    geo::Coord y = get_coord ();
    m_geometry_y = xy_absolute ? y : geo::Coord (m_geometry_y + y);
  }

  if ((m & iHAS_REPETITION) != 0) {
    read_repetition ();
  } else {
    m_repetition.clear ();
  }

  skip_element_properties ();

  if (m_polygon_point_list.size () < 3) {
    warn (geo::to_string (tr ("POLYGON with less than 3 points ignored")));
  } else {
    emit_polygon (layout, cell_index, m_layer, m_datatype, m_polygon_point_list,
                  geo::Vector (m_geometry_x, m_geometry_y));
  }

  m_repetition.clear ();
}

void
OASIS2ReaderBase::read_path (geo::Layout &layout, geo::cell_index_type cell_index, bool xy_absolute)
{
  unsigned char m = get_byte ();

  if ((m & iHAS_LAYER) != 0) {
    m_layer = uint32_t (get_uint ());
  }
  if ((m & iHAS_DATATYPE) != 0) {
    m_datatype = uint32_t (get_uint ());
  }
  if ((m & iHAS_WIDTH) != 0) {
    m_path_halfwidth = geo::Coord (get_ucoord ());
  }

  if ((m & iHAS_VERTICAL) != 0) {

    uint64_t e = get_uint ();

    if ((e & 0x0c) == 0x0c) {
      m_path_start_extension = get_coord ();
    } else if ((e & 0x0c) == 0x04) {
      m_path_start_extension = 0;
    } else if ((e & 0x0c) == 0x08) {
      m_path_start_extension = m_path_halfwidth;
    }

    if ((e & 0x03) == 0x03) {
      m_path_end_extension = get_coord ();
    } else if ((e & 0x03) == 0x01) {
      m_path_end_extension = 0;
    } else if ((e & 0x03) == 0x02) {
      m_path_end_extension = m_path_halfwidth;
    }

  }

  if ((m & iHAS_HEIGHT) != 0) {
    read_pointlist (m_path_point_list, false);
  }

  if ((m & iHAS_X) != 0) {
    geo::Coord x = get_coord ();
    m_geometry_x = xy_absolute ? x : geo::Coord (m_geometry_x + x);
  }
  if ((m & iHAS_Y) != 0) {
    geo::Coord y = get_coord ();
    m_geometry_y = xy_absolute ? y : geo::Coord (m_geometry_y + y);
  }

  if ((m & iHAS_REPETITION) != 0) {
    read_repetition ();
  } else {
    m_repetition.clear ();
  }

  skip_element_properties ();

  if (m_path_point_list.size () < 2) {
    warn (geo::to_string (tr ("PATH with less than 2 points ignored")));
  } else {
    emit_path (layout, cell_index, m_layer, m_datatype, m_path_point_list,
               geo::Coord (2 * m_path_halfwidth),
               m_path_start_extension, m_path_end_extension, false /*not round*/,
               geo::Vector (m_geometry_x, m_geometry_y));
  }

  m_repetition.clear ();
}

void
OASIS2ReaderBase::read_trapezoid (unsigned char r, geo::Layout &layout, geo::cell_index_type cell_index, bool xy_absolute)
{
  unsigned char m = get_byte ();

  if ((m & iHAS_LAYER) != 0) {
    m_layer = uint32_t (get_uint ());
  }
  if ((m & iHAS_DATATYPE) != 0) {
    m_datatype = uint32_t (get_uint ());
  }
  if ((m & iHAS_WIDTH) != 0) {
    m_geometry_w = get_ucoord ();
  }
  if ((m & iHAS_HEIGHT) != 0) {
    m_geometry_h = get_ucoord ();
  }

  geo::Coord delta_a = 0, delta_b = 0;
  if (r == tTRAPEZOID || r == tTRAPEZOID_A) {
    delta_a = get_coord ();
  }
  if (r == tTRAPEZOID || r == tTRAPEZOID_B) {
    delta_b = get_coord ();
  }

  if ((m & iHAS_X) != 0) {
    geo::Coord x = get_coord ();
    m_geometry_x = xy_absolute ? x : geo::Coord (m_geometry_x + x);
  }
  if ((m & iHAS_Y) != 0) {
    geo::Coord y = get_coord ();
    m_geometry_y = xy_absolute ? y : geo::Coord (m_geometry_y + y);
  }

  if ((m & iHAS_REPETITION) != 0) {
    read_repetition ();
  } else {
    m_repetition.clear ();
  }

  skip_element_properties ();

  geo::Coord w = geo::Coord (m_geometry_w);
  geo::Coord h = geo::Coord (m_geometry_h);

  std::vector <geo::Point> points;
  points.reserve (4);

  if ((m & iHAS_VERTICAL) != 0) {
    //  vertical trapezoid
    points.push_back (geo::Point (0, std::max (delta_a, geo::Coord (0))));
    points.push_back (geo::Point (0, geo::Coord (h + std::min (delta_b, geo::Coord (0)))));
    points.push_back (geo::Point (w, geo::Coord (h - std::max (delta_b, geo::Coord (0)))));
    points.push_back (geo::Point (w, -std::min (delta_a, geo::Coord (0))));
  } else {
    //  horizontal trapezoid
    points.push_back (geo::Point (std::max (delta_a, geo::Coord (0)), h));
    points.push_back (geo::Point (geo::Coord (w + std::min (delta_b, geo::Coord (0))), h));
    points.push_back (geo::Point (geo::Coord (w - std::max (delta_b, geo::Coord (0))), 0));
    points.push_back (geo::Point (-std::min (delta_a, geo::Coord (0)), 0));
  }

  emit_polygon (layout, cell_index, m_layer, m_datatype, points, geo::Vector (m_geometry_x, m_geometry_y));

  m_repetition.clear ();
}

void
OASIS2ReaderBase::read_circle (geo::Layout &layout, geo::cell_index_type cell_index, bool xy_absolute)
{
  unsigned char m = get_byte ();

  if ((m & iHAS_LAYER) != 0) {
    m_layer = uint32_t (get_uint ());
  }
  if ((m & iHAS_DATATYPE) != 0) {
    m_datatype = uint32_t (get_uint ());
  }
  if ((m & iHAS_HEIGHT) != 0) {
    m_circle_radius = geo::Coord (get_ucoord ());
  }

  if ((m & iHAS_X) != 0) {
    geo::Coord x = get_coord ();
    m_geometry_x = xy_absolute ? x : geo::Coord (m_geometry_x + x);
  }
  if ((m & iHAS_Y) != 0) {
    geo::Coord y = get_coord ();
    m_geometry_y = xy_absolute ? y : geo::Coord (m_geometry_y + y);
  }

  if ((m & iHAS_REPETITION) != 0) {
    read_repetition ();
  } else {
    m_repetition.clear ();
  }

  skip_element_properties ();

  if (m_circle_radius > 0) {

    //  a circle is represented as a single-point round path
    std::vector <geo::Point> points;
    points.push_back (geo::Point (0, 0));

    emit_path (layout, cell_index, m_layer, m_datatype, points,
               geo::Coord (2 * m_circle_radius), m_circle_radius, m_circle_radius, true /*round*/,
               geo::Vector (m_geometry_x, m_geometry_y));

  }

  m_repetition.clear ();
}

void
OASIS2ReaderBase::read_text (geo::Layout &layout, geo::cell_index_type cell_index, bool xy_absolute)
{
  unsigned char m = get_byte ();

  std::string text;

  if ((m & iT_HAS_STRING) != 0) {

    if ((m & iT_STRING_BY_REF) != 0) {

      uint64_t id = get_uint ();
      std::map <uint64_t, std::string>::const_iterator ts = m_textstrings.find (id);
      if (ts == m_textstrings.end ()) {
        warn (geo::sprintf (geo::to_string (tr ("TEXTSTRING id %ld is not defined")), id));
      } else {
        text = ts->second;
      }

    } else {
      text = get_string ();
    }

  }

  uint32_t text_layer = m_layer;
  uint32_t text_datatype = m_datatype;

  if ((m & iHAS_LAYER) != 0) {
    text_layer = uint32_t (get_uint ());
  }
  if ((m & iHAS_DATATYPE) != 0) {
    text_datatype = uint32_t (get_uint ());
  }

  if ((m & iHAS_X) != 0) {
    geo::Coord x = get_coord ();
    m_geometry_x = xy_absolute ? x : geo::Coord (m_geometry_x + x);
  }
  if ((m & iHAS_Y) != 0) {
    geo::Coord y = get_coord ();
    m_geometry_y = xy_absolute ? y : geo::Coord (m_geometry_y + y);
  }

  if ((m & iHAS_REPETITION) != 0) {
    read_repetition ();
  } else {
    m_repetition.clear ();
  }

  skip_element_properties ();

  emit_text (layout, cell_index, text_layer, text_datatype, text,
             geo::Vector (m_geometry_x, m_geometry_y));

  m_repetition.clear ();
}

void
OASIS2ReaderBase::read_placement (unsigned char r,
                                  geo::Layout &layout,
                                  bool xy_absolute,
                                  std::vector <geo::CellInstArray> &instances,
                                  std::vector <geo::CellInstArrayWithProperties> &instances_wp)
{
  unsigned char m = get_byte ();

  //  locate the cell
  if ((m & iP_HAS_CELL) != 0) {

    if ((m & iP_CELL_BY_REF) != 0) {
      uint64_t id = get_uint ();
      m_placement_cell = cell_for_instance (layout, size_t (id));
    } else {
      std::string name = get_string ();
      m_placement_cell = cell_for_instance (layout, name);
    }

    m_placement_cell_valid = true;

  }

  if (! m_placement_cell_valid) {
    error (geo::to_string (tr ("PLACEMENT without a cell reference")));
    return;
  }

  double mag = 1.0;
  bool mag_set = false;
  double angle_deg = 0.0;
  int angle = 0;
  bool is_angle_multiple_of_90 = true;
  bool mirror = ((m & iP_MIRROR) != 0);

  if (r == tPLACEMENT_TRANSFORM) {

    //  general transform: magnification and angle follow
    if ((m & iP_HAS_MAGNIFICATION) != 0) {
      mag = get_real ();
    }
    if (fabs (mag - 1.0) > 1e-9) {
      mag_set = true;
    }

    angle_deg = (m & iP_HAS_ANGLE) != 0 ? get_real () : 0.0;
    double a = angle_deg / 90.0;
    if (a < -4 || a > 4) {
      warn (geo::sprintf (geo::to_string (tr ("Invalid rotation angle (%g is less than -360 or larger than 360)")), angle_deg));
    }
    angle = int (a < 0 ? (a - 0.5) : (a + 0.5));
    if (fabs (double (angle) - a) > 1e-9) {
      is_angle_multiple_of_90 = false;
    } else {
      while (angle < 0) {
        angle += 4;
      }
      angle %= 4;
    }

  } else {
    //  the rotation is encoded in the info byte
    angle = (m >> 1) & 3;
  }

  if ((m & iP_HAS_X) != 0) {
    geo::Coord x = get_coord ();
    m_placement_x = xy_absolute ? x : geo::Coord (m_placement_x + x);
  }
  if ((m & iP_HAS_Y) != 0) {
    geo::Coord y = get_coord ();
    m_placement_y = xy_absolute ? y : geo::Coord (m_placement_y + y);
  }

  if ((m & iP_HAS_REPETITION) != 0) {
    read_repetition ();
  } else {
    m_repetition.clear ();
  }

  skip_element_properties ();

  geo::Vector pos (m_placement_x, m_placement_y);
  bool complex_trans = mag_set || ! is_angle_multiple_of_90;

  //  the repetition offsets always contain the base offset (0,0) - so the
  //  object is emitted once for every repetition offset
  std::vector <geo::Vector> offsets;
  if (m_repetition.empty ()) {
    offsets.push_back (geo::Vector ());
  } else {
    offsets = m_repetition;
  }

  for (std::vector <geo::Vector>::const_iterator o = offsets.begin (); o != offsets.end (); ++o) {

    geo::CellInstArray inst;
    if (complex_trans) {
      inst = geo::CellInstArray (geo::CellInst (m_placement_cell),
                                 geo::ICplxTrans (mag, angle_deg, mirror, pos + *o));
    } else {
      inst = geo::CellInstArray (geo::CellInst (m_placement_cell),
                                 geo::Trans (angle, mirror, pos + *o));
    }

    instances.push_back (inst);

  }

  m_repetition.clear ();
}

// -----------------------------------------------------------------------------
//  layer names
//
//  Layer names are parsed so the records are consumed completely. They are not
//  used to name the layers - the layer identity is given by layer/datatype.

void
OASIS2ReaderBase::read_layername (bool explicit_id)
{
  get_string ();

  for (unsigned int i = 0; i < 2; ++i) {

    uint64_t it = get_uint ();

    if (it == 0) {
      //  keep the limits
    } else if (it == 1 || it == 2 || it == 3) {
      get_uint ();
    } else if (it == 4) {
      get_uint ();
      get_uint ();
    } else {
      error (geo::to_string (tr ("Invalid LAYERNAME interval mode")));
    }

  }

  if (explicit_id) {
    get_uint ();
  }
}

// -----------------------------------------------------------------------------
//  cell bodies

void
OASIS2ReaderBase::read_cell (geo::Layout &layout, geo::cell_index_type cell_index)
{
  reset_modal_variables ();

  std::vector <geo::CellInstArray> instances;
  std::vector <geo::CellInstArrayWithProperties> instances_wp;

  bool xy_absolute = true;

  while (true) {

    progress_checkpoint ();

    unsigned char r = get_byte ();

    if (r == tPAD) {

      //  skip

    } else if (r == tXYABSOLUTE) {

      xy_absolute = true;

    } else if (r == tXYRELATIVE) {

      xy_absolute = false;

    } else if (r == tPLACEMENT || r == tPLACEMENT_TRANSFORM) {

      read_placement (r, layout, xy_absolute, instances, instances_wp);

    } else if (r == tTEXT) {

      read_text (layout, cell_index, xy_absolute);

    } else if (r == tRECTANGLE) {

      read_rectangle (layout, cell_index, xy_absolute);

    } else if (r == tPOLYGON) {

      read_polygon (layout, cell_index, xy_absolute);

    } else if (r == tPATH) {

      read_path (layout, cell_index, xy_absolute);

    } else if (r == tTRAPEZOID || r == tTRAPEZOID_A || r == tTRAPEZOID_B) {

      read_trapezoid (r, layout, cell_index, xy_absolute);

    } else if (r == tCIRCLE) {

      read_circle (layout, cell_index, xy_absolute);

    } else if (r == tCTRAPEZOID) {

      error (geo::to_string (tr ("CTRAPEZOID records are not supported yet")));

    } else if (r == tPROPERTY) {

      read_property ();

    } else if (r == tPROPERTY_REPEAT) {

      //  repeated property - no payload

    } else if (r == tXELEMENT) {

      get_uint ();      //  reference number
      get_string ();    //  payload
      skip_element_properties ();

    } else if (r == tXGEOMETRY) {

      get_byte ();      //  info byte
      get_uint ();      //  number of attribute values
      skip_element_properties ();

    } else if (r == tCBLOCK) {

      uint64_t type = get_uint ();
      if (type != 0) {
        error (geo::sprintf (geo::to_string (tr ("Invalid CBLOCK compression type %d")), int (type)));
      }
      get_uint ();  //  uncompressed byte count
      get_uint ();  //  compressed byte count
      inflate_stream ();

    } else {

      //  this record does not belong to the cell - put it back
      unget_byte ();
      break;

    }

  }

  //  attach the instances
  geo::Cell &cell = layout.cell (cell_index);
  if (! instances.empty ()) {
    cell.insert (instances.begin (), instances.end ());
  }
  if (! instances_wp.empty ()) {
    cell.insert (instances_wp.begin (), instances_wp.end ());
  }
}

// -----------------------------------------------------------------------------
//  offset table
//
//  The offset table consists of 6 (offset, size) pairs: CELLNAME, TEXTSTRING,
//  PROPNAME, PROPSTRING, LAYERNAME and XNAME.

void
OASIS2ReaderBase::read_offset_table ()
{
  for (unsigned int i = 0; i < 6; ++i) {
    get_uint ();
    get_uint ();
  }
}

// -----------------------------------------------------------------------------
//  main entry

void
OASIS2ReaderBase::do_read (geo::Layout &layout)
{
  m_textstrings.clear ();
  m_visible_cellnames.clear ();

  const char *mb = get_bytes (oasis_magic_length);
  if (! mb || memcmp (mb, oasis_magic, oasis_magic_length) != 0) {
    error (geo::to_string (tr ("Format error (missing magic bytes)")));
    return;
  }

  if (get_byte () != tSTART) {
    error (geo::to_string (tr ("Format error (START record expected)")));
    return;
  }

  std::string version = get_string ();
  if (version != "1.0") {
    error (geo::sprintf (geo::to_string (tr ("Format error (only version 1.0 is supported, file has version %s)")), version));
  }

  double res = get_real ();
  if (res < 1e-6) {
    error (geo::sprintf (geo::to_string (tr ("Invalid resolution of %g")), res));
  }

  //  the resolution is given in database units per micron
  layout.dbu (1.0 / res);

  if (get_uint () != 0) {

    //  the offset table is located at the end of the file
    if (m_expect_strict_mode == 1) {
      warn (geo::to_string (tr ("Offset table found at the end of the file (non-strict mode expected)")));
    }

  } else {

    //  the (empty) offset table placeholder follows right after the START record
    read_offset_table ();

  }

  //  implicit/explicit id modes and the associated counters
  enum id_mode_type { id_any, id_explicit, id_implicit };

  id_mode_type cellname_id_mode = id_any;
  id_mode_type textstring_id_mode = id_any;
  id_mode_type propname_id_mode = id_any;
  id_mode_type propstring_id_mode = id_any;

  uint64_t cellname_id = 0;
  uint64_t textstring_id = 0;
  uint64_t propname_id = 0;
  uint64_t propstring_id = 0;

  bool done = false;
  while (! done) {

    progress_checkpoint ();

    unsigned char r = get_byte ();

    if (r == tPAD) {

      //  skip

    } else if (r == tEND) {

      done = true;

    } else if (r == tCELLNAME_IMPLICIT || r == tCELLNAME_EXPLICIT) {

      std::string name = get_string ();

      uint64_t id = cellname_id;
      if (r == tCELLNAME_IMPLICIT) {
        if (cellname_id_mode == id_explicit) {
          error (geo::to_string (tr ("Explicit and implicit CELLNAME modes cannot be mixed")));
        }
        cellname_id_mode = id_implicit;
        ++cellname_id;
      } else {
        if (cellname_id_mode == id_implicit) {
          error (geo::to_string (tr ("Explicit and implicit CELLNAME modes cannot be mixed")));
        }
        cellname_id_mode = id_explicit;
        id = get_uint ();
      }

      m_visible_cellnames.insert (std::make_pair (id, name));
      rename_cell (layout, size_t (id), name);

      skip_element_properties ();

    } else if (r == tTEXTSTRING_IMPLICIT || r == tTEXTSTRING_EXPLICIT) {

      std::string text = get_string ();

      uint64_t id = textstring_id;
      if (r == tTEXTSTRING_IMPLICIT) {
        if (textstring_id_mode == id_explicit) {
          error (geo::to_string (tr ("Explicit and implicit TEXTSTRING modes cannot be mixed")));
        }
        textstring_id_mode = id_implicit;
        ++textstring_id;
      } else {
        if (textstring_id_mode == id_implicit) {
          error (geo::to_string (tr ("Explicit and implicit TEXTSTRING modes cannot be mixed")));
        }
        textstring_id_mode = id_explicit;
        id = get_uint ();
      }

      m_textstrings.insert (std::make_pair (id, text));

    } else if (r == tPROPNAME_IMPLICIT || r == tPROPNAME_EXPLICIT) {

      //  property names are parsed so they can be skipped properly
      get_string ();
      if (r == tPROPNAME_IMPLICIT) {
        if (propname_id_mode == id_explicit) {
          error (geo::to_string (tr ("Explicit and implicit PROPNAME modes cannot be mixed")));
        }
        propname_id_mode = id_implicit;
        ++propname_id;
      } else {
        if (propname_id_mode == id_implicit) {
          error (geo::to_string (tr ("Explicit and implicit PROPNAME modes cannot be mixed")));
        }
        propname_id_mode = id_explicit;
        get_uint ();
      }

    } else if (r == tPROPSTRING_IMPLICIT || r == tPROPSTRING_EXPLICIT) {

      //  property strings are parsed so they can be skipped properly
      get_string ();
      if (r == tPROPSTRING_IMPLICIT) {
        if (propstring_id_mode == id_explicit) {
          error (geo::to_string (tr ("Explicit and implicit PROPSTRING modes cannot be mixed")));
        }
        propstring_id_mode = id_implicit;
        ++propstring_id;
      } else {
        if (propstring_id_mode == id_implicit) {
          error (geo::to_string (tr ("Explicit and implicit PROPSTRING modes cannot be mixed")));
        }
        propstring_id_mode = id_explicit;
        get_uint ();
      }

    } else if (r == tLAYERNAME_IMPLICIT || r == tLAYERNAME_EXPLICIT) {

      read_layername (r == tLAYERNAME_EXPLICIT);

    } else if (r == tCELL_REFNUM || r == tCELL_NAME) {

      geo::cell_index_type cell_index = 0;

      if (r == tCELL_REFNUM) {

        uint64_t id = get_uint ();

        std::pair<bool, geo::cell_index_type> cc = cell_by_id (size_t (id));
        if (cc.first && ! layout.cell (cc.second).is_ghost_cell ()) {
          error (geo::sprintf (geo::to_string (tr ("A cell with id %ld is defined already")), id));
        }

        cell_index = make_cell (layout, size_t (id));

        std::map <uint64_t, std::string>::const_iterator cn = m_visible_cellnames.find (id);
        if (cn != m_visible_cellnames.end ()) {
          m_cellname = cn->second;
        } else {
          //  in strict mode the CELLNAME table is located at the end of the file -
          //  the name is assigned when the CELLNAME record is read
          m_cellname = geo::sprintf ("CELL_%ld", id);
        }

      } else {

        std::string name = get_string ();
        m_cellname = name;
        cell_index = make_cell (layout, name);

      }

      read_cell (layout, cell_index);

      m_cellname.clear ();

    } else if (r == tPROPERTY) {

      read_property ();

    } else if (r == tPROPERTY_REPEAT) {

      //  repeated property - no payload

    } else if (r == tXELEMENT) {

      get_uint ();
      get_string ();
      skip_element_properties ();

    } else if (r == tXGEOMETRY) {

      get_byte ();
      get_uint ();
      skip_element_properties ();

    } else if (r == tCBLOCK) {

      uint64_t type = get_uint ();
      if (type != 0) {
        error (geo::sprintf (geo::to_string (tr ("Invalid CBLOCK compression type %d")), int (type)));
      }
      get_uint ();  //  uncompressed byte count
      get_uint ();  //  compressed byte count
      inflate_stream ();

    } else if (r == tSTART) {

      error (geo::to_string (tr ("Format error (duplicate START record)")));

    } else {

      error (geo::sprintf (geo::to_string (tr ("Invalid record type %d")), int (r)));

    }

  }
}

}

