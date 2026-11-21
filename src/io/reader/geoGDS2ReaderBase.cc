#include "geoGDS2ReaderBase.h"

#include "geoGDS2Format.h"
#include "geoGDS2.h"
#include "geoArray.h"

#include "utilsException.h"
#include "utilsString.h"
#include "utilsClassRegistry.h"

namespace geo
{

// ---------------------------------------------------------------
//  Free helper functions (not part of a class)

inline geo::Point 
pt_conv (const GDS2XY &p) 
{
  //  TODO: this can be done more efficiently ..
  int x = (int (p.x[0]) << 24) | (int (p.x[1]) << 16) | (int (p.x[2]) << 8) | int (p.x[3]);
  int y = (int (p.y[0]) << 24) | (int (p.y[1]) << 16) | (int (p.y[2]) << 8) | int (p.y[3]);
  return geo::Point (x, y);
}

inline geo::Vector
v_conv (const GDS2XY &p)
{
  return pt_conv (p) - geo::Point ();
}

inline bool 
eq_x (const GDS2XY &a, const GDS2XY &b)
{
  /// Re-cast pointer to char[4] as pointer to 32bit int for faster comparison
  return *(( int*)a.x) == *(( int*)b.x);
}

inline bool 
eq_y (const GDS2XY &a, const GDS2XY &b)
{
  /// Re-cast pointer to char[4] as pointer to 32bit int for faster comparison
  return *(( int*)a.y) == *(( int*)b.y);
}

// ---------------------------------------------------------------
//  GDS2ReaderBase

// -----------------------------------------------------------------------------
//  GDS2ReaderBase implementation

GDS2ReaderBase::GDS2ReaderBase ()
  : m_dbu (0.001), 
    m_dbuu (1.0), 
    m_read_texts (true),
    m_read_properties (true),
    m_allow_multi_xy_records (false),
    m_box_mode (0)
{
  // .. nothing yet ..
}

GDS2ReaderBase::~GDS2ReaderBase ()
{
  // .. nothing yet ..
}

void
GDS2ReaderBase::init (const geo::LoadLayoutOptions &options)
{
  CommonReader::init (options);

  geo::GDS2ReaderOptions gds2_options = options.get_options<geo::GDS2ReaderOptions> ();

  m_read_texts = common_options ().enable_text_objects;
  m_read_properties = common_options ().enable_properties;

  m_allow_multi_xy_records = gds2_options.allow_multi_xy_records;
  m_box_mode = gds2_options.box_mode;
}

void
GDS2ReaderBase::finish_element ()
{
  while (true) {

    short rec_id = get_record ();

    if (rec_id == sENDEL) {
      break;
    } else if (rec_id == sPROPATTR) {
      //  skip this record
    } else if (rec_id == sPROPVALUE) {
      //  skip this record
    } else if (rec_id == sTEXT || rec_id == sPATH || rec_id == sBOUNDARY || rec_id == sBOX || 
               rec_id == sAREF || rec_id == sSREF || rec_id == sENDSTR) {
      unget_record (rec_id);
      warn (geo::to_string (tr ("ENDEL record expected - assuming missing ENDEL")));
      break;
    } else {
      error (geo::to_string (tr ("ENDEL, PROPATTR or PROPVALUE record expected")));
    }

  } 
}


std::pair <bool, geo::properties_id_type> 
GDS2ReaderBase::finish_element (geo::PropertiesRepository &rep)
{
  bool any = false;
  long attr = 0;

  geo::PropertiesRepository::properties_set properties;

  while (true) {

    short rec_id = get_record ();

    if (rec_id == sENDEL) {
      break;
    } else if (rec_id == sPROPATTR) {
      attr = long (get_ushort ());
    } else if (rec_id == sPROPVALUE) {

      const char *value = get_string ();
      if (m_read_properties) {
        properties.insert (std::make_pair (rep.prop_name_id (geo::Variant (attr)), 
                                           geo::Variant (value)));

        any = true;
      }

    } else if (rec_id == sTEXT || rec_id == sPATH || rec_id == sBOUNDARY || rec_id == sBOX || 
               rec_id == sAREF || rec_id == sSREF || rec_id == sENDSTR) {
      unget_record (rec_id);
      warn (geo::to_string (tr ("ENDEL record expected - assuming missing ENDEL")));
      break;
    } else {
      error (geo::to_string (tr ("ENDEL, PROPATTR or PROPVALUE record expected")));
    }

  } 

  if (any) {
    return std::make_pair (true, rep.properties_id (properties));
  } else {
    return std::make_pair (false, 0);
  }
}

void 
GDS2ReaderBase::do_read (geo::Layout &layout) 
{
  m_cellname = "";
  m_libname = "";

  //  read header
  if (get_record () != sHEADER) {
    error (geo::to_string (tr ("HEADER record expected")));
  }
  if (get_record () != sBGNLIB) {
    error (geo::to_string (tr ("BGNLIB record expected")));
  }

  unsigned int mod_time[6] = { 0, 0, 0, 0, 0, 0 };
  unsigned int access_time[6] = { 0, 0, 0, 0, 0, 0 };
  get_time (mod_time, access_time);
  layout.add_meta_info (MetaInfo ("mod_time", geo::to_string (tr ("Modification Time")), geo::sprintf ("%d/%d/%d %d:%02d:%02d", mod_time[1], mod_time[2], mod_time[0], mod_time[3], mod_time[4], mod_time[5])));
  layout.add_meta_info (MetaInfo ("access_time", geo::to_string (tr ("Access Time")), geo::sprintf ("%d/%d/%d %d:%02d:%02d", access_time[1], access_time[2], access_time[0], access_time[3], access_time[4], access_time[5])));

  long attr = 0;
  geo::PropertiesRepository::properties_set layout_properties;

  //  read until 
  short rec_id = 0;
  do {
    rec_id = get_record ();
    if (rec_id == sLIBDIRSIZE ||
        rec_id == sSRFNAME ||
        rec_id == sREFLIBS ||
        rec_id == sFONTS ||
        rec_id == sATTRTABLE ||
        rec_id == sGENERATIONS ||
        rec_id == sFORMAT ||
        rec_id == sMASK ||
        rec_id == sENDMASKS) {

      //  OK and overread
      
    } else if (rec_id == sLIBNAME) {

      m_libname = get_string ();

    } else if (rec_id == sBGNSTR || rec_id == sENDLIB) {

      //  start with cells or finish (for empty file)
      unget_record (rec_id);
      break;

    } else if (rec_id == sPROPATTR) {

      attr = long (get_ushort ());

    } else if (rec_id == sPROPVALUE) {

      const char *value = get_string ();
      if (m_read_properties) {
        layout_properties.insert (std::make_pair (layout.properties_repository ().prop_name_id (geo::Variant (attr)), geo::Variant (value)));
      }

    } else if (rec_id == sUNITS) {

      //  get units
      double dbuu = get_double ();
      double dbum = get_double ();
      
      layout.add_meta_info (MetaInfo ("dbuu", geo::to_string (tr ("Database unit in user units")), geo::to_string (dbuu)));
      layout.add_meta_info (MetaInfo ("dbum", geo::to_string (tr ("Database unit in meter")), geo::to_string (dbum)));
      layout.add_meta_info (MetaInfo ("libname", geo::to_string (tr ("Library name")), m_libname));

      m_dbuu = dbuu;
      m_dbu = dbum * 1e6; /*in micron*/
      layout.dbu (m_dbu);

    } else {
      error (geo::to_string (tr ("Invalid record or data type")));
    }

  } while (true);

  //  set the layout properties
  if (! layout_properties.empty ()) {
    layout.prop_id (layout.properties_repository ().properties_id (layout_properties));
  }

  //  this container has been found to grow quite a lot.
  //  using a list instead of a vector should make this more efficient.
  geo::uvector<geo::CellInstArray> instances;
  geo::uvector<geo::CellInstArrayWithProperties> instances_with_props;

  //  prepare a string vector for the context information
  m_context_info.clear ();

  bool first_cell = true;

  //  get cells
  while ((rec_id = get_record ()) == sBGNSTR) {

    progress_checkpoint ();

    //  erase current instance list 
    instances.erase (instances.begin (), instances.end ());
    instances_with_props.erase (instances_with_props.begin (), instances_with_props.end ());

    if (get_record () != sSTRNAME) {
      error (geo::to_string (tr ("STRNAME record expected")));
    }

    get_string (m_cellname);

    //  if the first cell is the dummy cell containing the context information
    //  read this cell in a special way and store the context information separately.
    if (first_cell && m_cellname == "$$$CONTEXT_INFO$$$") {

      read_context_info_cell ();

    } else {

      geo::cell_index_type cell_index = make_cell (layout, m_cellname);

      bool ignore_cell = false;
      std::map <geo::string, std::vector <std::string> >::const_iterator ctx = m_context_info.find (m_cellname);
      if (ctx != m_context_info.end ()) {
        CommonReaderLayerMapping layer_mapping (this, &layout);
        if (layout.recover_proxy_as (cell_index, ctx->second.begin (), ctx->second.end (), &layer_mapping)) {
          //  ignore everything in that cell since it is created by the import:
          ignore_cell = true;
        }
      }
      
      geo::Cell *cell = 0;
      if (! ignore_cell) {
        cell = &layout.cell (cell_index);
      }

      long attr = 0;
      geo::PropertiesRepository::properties_set cell_properties;

      //  read cell content
      while ((rec_id = get_record ()) != sENDSTR) { 

        progress_checkpoint ();

        if (cell == 0) {

          //  ignore everything in proxy cells: these are created from the libraries or PCells.

        } else if (rec_id == sPROPATTR) {

          attr = long (get_ushort ());

        } else if (rec_id == sPROPVALUE) {

          const char *value = get_string ();
          if (m_read_properties) {
            cell_properties.insert (std::make_pair (layout.properties_repository ().prop_name_id (geo::Variant (attr)), geo::Variant (value)));
          }

        } else if (rec_id == sBOUNDARY) {

          read_boundary (layout, *cell, false);

        } else if (rec_id == sPATH) {

          read_path (layout, *cell);

        } else if (rec_id == sSREF || rec_id == sAREF) {

          bool array = (rec_id == sAREF);
          read_ref (layout, *cell, array, instances, instances_with_props);

        } else if (rec_id == sTEXT) {

          read_text (layout, *cell);

        } else if (rec_id == sBOX) {

          if (m_box_mode == 1) {
            read_box (layout, *cell);
          } else if (m_box_mode == 2) {
            read_boundary (layout, *cell, true);
          } else if (m_box_mode == 3) {
            error (geo::to_string (tr ("BOX record encountered (reader is configured to produce an error in this case)")));
          } else {
            while (get_record () != sENDEL) { }
          }

        } else if (rec_id == sNODE) {

          //  NODE records are ignored.
          while (get_record () != sENDEL) { }

        } else {
          error (geo::to_string (tr ("Invalid record or data type")));
        }
      
      }

      //  insert all instances collected
      if (! instances.empty ()) {
        cell->insert (instances.begin (), instances.end ());
      }
      if (! instances_with_props.empty ()) {
        cell->insert (instances_with_props.begin (), instances_with_props.end ());
      }

      //  set the cell properties
      if (! cell_properties.empty ()) {
        cell->prop_id (layout.properties_repository ().properties_id (cell_properties));
      }

    }

    m_cellname = "";
    first_cell = false;

  }

  //  check, if the last record is a ENDLIB
  if (rec_id != sENDLIB) {
    error (geo::to_string (tr ("ENDLIB record expected")));
  }
}

void
GDS2ReaderBase::read_context_info_cell ()
{
  short rec_id = 0;

  //  read cell content
  while ((rec_id = get_record ()) != sENDSTR) { 

    progress_checkpoint ();

    if (rec_id == sSREF) {

      do {
        rec_id = get_record ();
      } while (rec_id == sELFLAGS || rec_id == sPLEX);
      if (rec_id != sSNAME) {
        error (geo::to_string (tr ("SNAME record expected")));
      }

      std::string cn = get_string ();

      rec_id = get_record ();
      while (rec_id == sSTRANS || rec_id == sANGLE || rec_id == sMAG) {
        rec_id = get_record ();
      }
      if (rec_id != sXY) {
        error (geo::to_string (tr ("XY record expected")));
      }

      std::vector <std::string> &strings = m_context_info.insert (std::make_pair (cn, std::vector <std::string> ())).first->second;

      size_t attr = 0;

      while (true) {

        rec_id = get_record ();

        if (rec_id == sENDEL) {
          break;
        } else if (rec_id == sPROPATTR) {
          attr = size_t (get_ushort ());
        } else if (rec_id == sPROPVALUE) {

          if (strings.size () <= attr) {
            strings.resize (attr + 1, std::string ());
          }
          strings [attr] = get_string ();

        } else {
          error (geo::to_string (tr ("ENDEL, PROPATTR or PROPVALUE record expected")));
        }

      } 

    } else {
      error (geo::to_string (tr ("Invalid record inside a context info cell")));
    }
  
  }
}

void 
GDS2ReaderBase::read_boundary (geo::Layout &layout, geo::Cell &cell, bool from_box_record)
{
  LDPair ld; 
  short rec_id = 0;

  do {
    rec_id = get_record ();
  } while (rec_id == sELFLAGS || rec_id == sPLEX);
  if (rec_id != sLAYER) {
    error (geo::to_string (tr ("LAYER record expected")));
  }
  ld.layer = get_ushort ();

  rec_id = get_record ();
  if (from_box_record) {
    if (rec_id != sBOXTYPE) {
      error (geo::to_string (tr ("BOXTYPE record expected")));
    }
  } else {
    if (rec_id != sDATATYPE) {
      error (geo::to_string (tr ("DATATYPE record expected")));
    }
  }

  ld.datatype = get_ushort ();

  if (get_record () != sXY) {
    error (geo::to_string (tr ("XY record expected")));
  }

  unsigned int xy_length = 0;
  GDS2XY *xy_data = get_xy_data (xy_length);

  std::pair<bool, unsigned int> ll = open_dl (layout, ld);
  if (ll.first) {

    //  create a box object if possible
    GDS2XY *xy = xy_data;
    if ((xy_length == 4 || 
         (xy_length == 5 && pt_conv (xy[4]) == pt_conv (xy[0]))) && 
        ((eq_x (xy[0], xy[1]) && eq_x (xy[2], xy[3]) && 
          eq_y (xy[1], xy[2]) && eq_y (xy[0], xy[3])) ||
         (eq_x (xy[1], xy[2]) && eq_x (xy[0], xy[3]) && 
          eq_y (xy[0], xy[1]) && eq_y (xy[2], xy[3])))) {

      //  we can create a box object:
      geo::Point p1 = pt_conv (*xy++);
      geo::Point p2 = p1;
       
      while (xy < xy_data + 4) {
        geo::Point p (pt_conv (*xy++));
        if (p.x () < p1.x ()) {
          p1.set_x (p.x ());
        }
        if (p.y () < p1.y ()) {
          p1.set_y (p.y ());
        }
        if (p.x () > p2.x ()) {
          p2.set_x (p.x ());
        }
        if (p.y () > p2.y ()) {
          p2.set_y (p.y ());
        }
      }

      std::pair<bool, geo::properties_id_type> pp = finish_element (layout.properties_repository ());
      if (pp.first) {
        cell.shapes (ll.second).insert (geo::BoxWithProperties (geo::Box (p1, p2), pp.second));
      } else {
        cell.shapes (ll.second).insert (geo::Box (p1, p2));
      }

    } else {

      //  convert the GDS2 record into the polygon.
      geo::SimplePolygon poly;

      //  Try to detect Multi-XY records. A good indication may be a huge point count.
      if (xy_length > 2000) {

        m_all_points.clear ();
        m_all_points.reserve (xy_length * 2); // allocate some (hopefully enough) elements

        while (true) {

          for (GDS2XY *xy = xy_data; xy < xy_data + xy_length; ++xy) {
            m_all_points.push_back (pt_conv (*xy));
          }

          if ((rec_id = get_record ()) == sXY) {
            xy_data = get_xy_data (xy_length);
            if (! m_allow_multi_xy_records) {
              error (geo::to_string (tr ("Multiple XY records detected on BOUNDARY element (reader is configured not to allow this)")));
            }
          } else {
            unget_record (rec_id);
            break;
          }

        } 

        //  remove redundant start and endpoint
        if (! m_all_points.empty () && m_all_points.back () == m_all_points.front ()) {
          m_all_points.pop_back ();
        }

        poly.assign_hull (m_all_points.begin (), m_all_points.end (), false /*no compression*/);

      } else {

        //  remove redundant start and endpoint
        if (xy_length > 1 && eq_x (xy_data [0], xy_data [xy_length - 1]) && eq_y (xy_data [0], xy_data [xy_length - 1])) {
          --xy_length;
        }

        poly.assign_hull (xy_data, xy_data + xy_length, pt_conv, false /*no compression*/);

      }

      if (poly.hull ().size () < 3) {
        warn (geo::to_string (tr ("BOUNDARY with less than 3 points ignored")));
        finish_element ();
      } else {
        //  this will copy the polyon:
        std::pair<bool, geo::properties_id_type> pp = finish_element (layout.properties_repository ());
        if (pp.first) {
          cell.shapes (ll.second).insert (geo::SimplePolygonRefWithProperties (geo::SimplePolygonRef (poly, layout.shape_repository ()), pp.second));
        } else {
          cell.shapes (ll.second).insert (geo::SimplePolygonRef (poly, layout.shape_repository ()));
        }
      }

    }

  } else {

    while ((rec_id = get_record ()) == sXY) { 
      // read over multi-XY records
      if (! m_allow_multi_xy_records) {
        error (geo::to_string (tr ("Multiple XY records detected on BOUNDARY element (reader is configured not to allow this)")));
      }
    }
    unget_record (rec_id);

    finish_element ();

  }
}

void 
GDS2ReaderBase::read_path (geo::Layout &layout, geo::Cell &cell)
{
  LDPair ld; 
  short rec_id = 0;

  do {
    rec_id = get_record ();
  } while (rec_id == sELFLAGS || rec_id == sPLEX);
  if (rec_id != sLAYER) {
    error (geo::to_string (tr ("LAYER record expected")));
  }
  ld.layer = get_ushort ();
  if (get_record () != sDATATYPE) {
    error (geo::to_string (tr ("DATATYPE record expected")));
  }
  ld.datatype = get_ushort ();
    
  rec_id = get_record ();

  short type = 0; 
  if (rec_id == sPATHTYPE) {
    type = get_ushort (); 
    rec_id = get_record ();
  }

  if (type != 0 && type != 1 && type != 2 && type != 4) {
    warn (geo::to_string (tr ("Unsupported PATHTYPE")));
    type = 0;
  }

  geo::Coord w = 0;
  if (rec_id == sWIDTH) {
    w = get_int ();
    rec_id = get_record ();
  }

  geo::Coord bgn_ext = 0;
  geo::Coord end_ext = 0;

  if (rec_id == sBGNEXTN) {
    bgn_ext = get_int ();
    rec_id = get_record ();
  } else {
    if (type == 2 || type == 1) {
      bgn_ext = w / 2;
    } 
  }

  if (rec_id == sENDEXTN) {
    end_ext = get_int ();
    rec_id = get_record ();
  } else {
    if (type == 2 || type == 1) {
      end_ext = w / 2;
    } 
  }

  if (rec_id != sXY) {
    error (geo::to_string (tr ("XY record expected")));
  }

  unsigned int xy_length = 0;
  GDS2XY *xy_data = get_xy_data (xy_length);

  std::pair<bool, unsigned int> ll = open_dl (layout, ld);
  if (ll.first) {

    //  this will copy the path:
    geo::Path path;

    //  Try to detect Multi-XY records. A good indication may be a huge point count.
    if (xy_length > 2000) {

      m_all_points.clear ();
      m_all_points.reserve (xy_length * 2); // allocate some (hopefully enough) elements

      while (true) {

        for (GDS2XY *xy = xy_data; xy < xy_data + xy_length; ++xy) {
          m_all_points.push_back (pt_conv (*xy));
        }

        if ((rec_id = get_record ()) == sXY) {
          xy_data = get_xy_data (xy_length);
          if (! m_allow_multi_xy_records) {
            error (geo::to_string (tr ("Multiple XY records detected on PATH element (reader is configured not to allow this)")));
          }
        } else {
          unget_record (rec_id);
          break;
        }

      }

      path.assign (m_all_points.begin (), m_all_points.end ());

    } else {
      path.assign (xy_data, xy_data + xy_length, pt_conv);
    }

    path.width (w);
    path.extensions (bgn_ext, end_ext);
    path.round (type == 1);

    if (path.points () < 1) {
      warn (geo::to_string (tr ("PATH with less than one point ignored")));
      finish_element ();
    } else {
      if (path.points () < 2 && type != 1) {
        warn (geo::to_string (tr ("PATH with less than two points encountered - interpretation may be different in other tools")));
      }
      std::pair<bool, geo::properties_id_type> pp = finish_element (layout.properties_repository ());
      if (pp.first) {
        cell.shapes (ll.second).insert (geo::PathRefWithProperties (geo::PathRef (path, layout.shape_repository ()), pp.second));
      } else {
        cell.shapes (ll.second).insert (geo::PathRef (path, layout.shape_repository ()));
      }
    }

  } else {

    while ((rec_id = get_record ()) == sXY) {
      // read over multi-XY records
      if (! m_allow_multi_xy_records) {
        error (geo::to_string (tr ("Multiple XY records detected on PATH element (reader is configured not to allow this)")));
      }
    }
    unget_record (rec_id);

    finish_element ();

  }
}

void 
GDS2ReaderBase::read_text (geo::Layout &layout, geo::Cell &cell)
{
  LDPair ld; 
  short rec_id = 0;

  do {
    rec_id = get_record ();
  } while (rec_id == sELFLAGS || rec_id == sPLEX);
  if (rec_id != sLAYER) {
    error (geo::to_string (tr ("LAYER record expected")));
  }
  ld.layer = get_ushort ();
  if (get_record () != sTEXTTYPE) {
    error (geo::to_string (tr ("DATATYPE record expected")));
  }
  ld.datatype = get_ushort ();

  std::pair<bool, unsigned int> ll (false, 0);

  if (m_read_texts) {
    ll = open_dl (layout, ld);
  }

  rec_id = get_record ();

  geo::HAlign ha = geo::NoHAlign;
  geo::VAlign va = geo::NoVAlign;
  geo::Font font = geo::NoFont;

  if (rec_id == sPRESENTATION) {
    short p = get_short ();
    ha = geo::HAlign (p & 3);
    va = geo::VAlign ((p >> 2) & 3);
    // HINT: currently we don't read the font since the font is not well standardized ..
    // font = (geo::Font) ((p >> 4) & 0xfff);
    rec_id = get_record ();
  }

  if (rec_id == sPATHTYPE) {
    rec_id = get_record ();
  }

  if (rec_id == sWIDTH) {
    rec_id = get_record ();
  }

  bool mirror = false;
  int angle = 0;
  geo::Coord size = 0;

  while (rec_id == sSTRANS || rec_id == sMAG || rec_id == sANGLE) {

    if (rec_id == sSTRANS) {

      short f = get_short ();
      if ((f & 0x8000) != 0) {
        mirror = true;
      }

    } else if (rec_id == sMAG) {

      size = geo::coord_traits<geo::Coord>::rounded (get_double () / m_dbuu);

    } else if (rec_id == sANGLE) {

      if (ll.first) {
        double aorg = get_double ();
        double a = aorg / 90.0;
        if (a < -4 || a > 4) {
          warn (geo::sprintf (geo::to_string (tr ("Invalid text rotation angle (%g is less than -360 or larger than 360)")), aorg));
        }
        angle = int (a < 0 ? (a - 0.5) : (a + 0.5));
        if (fabs (double (angle) - a) > 1e-9) {
          warn (geo::sprintf (geo::to_string (tr ("Invalid text rotation angle (%g is not a multiple of 90)")), aorg));
        }
        while (angle < 0) {
          angle += 4;
        }
        while (angle >= 4) {
          angle -= 4;
        }
      }

    }

    rec_id = get_record ();
      
  }

  if (rec_id != sXY) {
    error (geo::to_string (tr ("XY record expected")));
  }

  unsigned int xy_length = 0;
  GDS2XY *xy_data = get_xy_data (xy_length);
  if (xy_length == 0) {
    error (geo::to_string (tr ("No point in XY record for text")));
  } else if (xy_length > 1) {
    warn (geo::to_string (tr ("More than one point in XY record for text")));
  }

  geo::Trans t (angle, mirror, pt_conv (xy_data [0]) - geo::Point ());

  if (get_record () != sSTRING) {
    error (geo::to_string (tr ("STRING record expected")));
  }

  if (ll.first) {

    //  Create the text
    geo::Text text (get_string (), t, size, font, ha, va);

    std::pair<bool, geo::properties_id_type> pp = finish_element (layout.properties_repository ());
    if (pp.first) {
      cell.shapes (ll.second).insert (geo::TextRefWithProperties (geo::TextRef (text, layout.shape_repository ()), pp.second));
    } else {
      cell.shapes (ll.second).insert (geo::TextRef (text, layout.shape_repository ()));
    }

  } else {
    finish_element ();
  }
}

void 
GDS2ReaderBase::read_box (geo::Layout &layout, geo::Cell &cell)
{
  LDPair ld; 
  short rec_id = 0;

  do {
    rec_id = get_record ();
  } while (rec_id == sELFLAGS || rec_id == sPLEX);
  if (rec_id != sLAYER) {
    error (geo::to_string (tr ("LAYER record expected")));
  }
  ld.layer = get_ushort ();
  if (get_record () != sBOXTYPE) {
    error (geo::to_string (tr ("DATATYPE record expected")));
  }
  ld.datatype = get_ushort ();

  std::pair<bool, unsigned int> ll = open_dl (layout, ld);

  if (get_record () != sXY) {
    error (geo::to_string (tr ("XY record expected")));
  }

  unsigned int xy_length = 0;
  GDS2XY *xy_data = get_xy_data (xy_length);

  if (ll.first) {

    GDS2XY *xy = xy_data;
    geo::Box box;
    while (xy < xy_data + xy_length) {
      box += pt_conv (*xy++);
    }

    std::pair<bool, geo::properties_id_type> pp = finish_element (layout.properties_repository ());
    if (! box.empty ()) {
      if (pp.first) {
        cell.shapes (ll.second).insert (geo::BoxWithProperties (box, pp.second));
      } else {
        cell.shapes (ll.second).insert (box);
      }
    }

  } else {
    finish_element ();
  }
}

void 
GDS2ReaderBase::read_ref (geo::Layout &layout, geo::Cell & /*cell*/, bool array, geo::uvector<geo::CellInstArray> &instances, geo::uvector<geo::CellInstArrayWithProperties> &instances_with_props)
{
  short rec_id = 0;

  do {
    rec_id = get_record ();
  } while (rec_id == sELFLAGS || rec_id == sPLEX);
  if (rec_id != sSNAME) {
    error (geo::to_string (tr ("SNAME record expected")));
  }

  geo::cell_index_type ci = cell_for_instance (layout, get_string ());

  bool mirror = false;
  int angle = 0;
  double angle_deg = 0.0;
  double mag = 1.0;
  bool is_mag = false;

  rec_id = get_record ();

  while (rec_id == sSTRANS || rec_id == sMAG || rec_id == sANGLE) {

    if (rec_id == sSTRANS) {
      short f = get_short ();
      if ((f & 0x8000) != 0) {
        mirror = true;
      }
      if ((f & (4 | 2)) != 0) {
        warn (geo::to_string (tr ("Absolute transformations are not supported")));
      }  
    } else if (rec_id == sMAG) {
      mag = get_double ();
      if (fabs (mag - 1.0) > 1e-9) { 
        is_mag = true;
      }
    } else if (rec_id == sANGLE) {
      angle_deg = get_double ();
      double a = angle_deg / 90.0;
      if (a < -4 || a > 4) {
        warn (geo::sprintf (geo::to_string (tr ("Invalid rotation angle (%g is less than -360 or larger than 360)")), angle_deg));
      }
      angle = int (a < 0 ? (a - 0.5) : (a + 0.5));
      if (fabs (double (angle) - a) > 1e-9) {
        angle = -1; // indicates arbitrary orientation. Take angle_deg instead
      } else {
        if (angle < 0) {
          angle += ((4 - 1) - angle) & ~(4 - 1);
        }
        angle = angle % 4;
      }
    }

    rec_id = get_record ();

  }

  if (array) {

    //  Array reference
    if (rec_id != sCOLROW) {
      error (geo::to_string (tr ("COLROW record expected")));
    }

    int cols = get_ushort ();
    int rows = get_ushort ();

    cols = std::max (1, cols);
    rows = std::max (1, rows);

    //  Array reference
    if (get_record () != sXY) {
      error (geo::to_string (tr ("XY record expected")));
    }

    //  Create the instance
    unsigned int xy_length = 0;
    GDS2XY *xy_data = get_xy_data (xy_length);
    if (xy_length < 3) {
      error (geo::to_string (tr ("Too few points in XY record for AREF")));
    } else if (xy_length > 3) {
      warn (geo::to_string (tr ("More than three points in XY record for AREF")));
    }

    //  Create the instance
    geo::Vector xy = v_conv (xy_data [0]);
    geo::Vector c = v_conv (xy_data [1]) - xy;
    geo::Vector r = v_conv (xy_data [2]) - xy;

    //  Reduce axes with no displacement to dimension 1 - such
    //  axes only produce overlapping instances.
    if (c == geo::Vector ()) {
      cols = 1;
    }
    if (r == geo::Vector ()) {
      rows = 1;
    }

    std::pair<bool, geo::properties_id_type> pp = finish_element (layout.properties_repository ());

    bool split_cols = false, split_rows = false;

    if (cols > 1 && (c.x () % cols != 0 || c.y () % cols != 0)) {
      warn (geo::to_string (tr ("Off-grid AREF column vector - AREF will be split into subarrays to preserve locations")));
      split_cols = true;
    }
    if (rows > 1 && (r.x () % rows != 0 || r.y () % rows != 0)) {
      warn (geo::to_string (tr ("Off-grid AREF row vector - AREF will be split into subarrays to preserve locations")));
      split_rows = true;
    }

    if (split_cols || split_rows) {

      geo::DVector cd (geo::DVector (c) * (1.0 / double (cols)));
      geo::DVector rd (geo::DVector (r) * (1.0 / double (rows)));

      c = geo::Vector (cd);
      r = geo::Vector (rd);

      int ic = 0;
      while (ic < cols) {
         
        int ic0 = ic;

        if (! split_cols) {
          ic = cols;
        } else {

          geo::DPoint p1d = geo::DPoint () + cd * double (ic);
          geo::Point p1 = geo::Point (p1d);

          do {
            ++ic;
            p1d += cd;
            p1 += c;
          } while (ic < cols && fabs (p1d.x () - p1.x ()) < 0.5 && fabs (p1d.y () - p1.y ()) < 0.5);

        }

        int ir = 0;
        while (ir < rows) {
         
          int ir0 = ir;

          if (! split_rows) {
            ir = rows;
          } else {

            geo::DPoint p2d = geo::DPoint () + rd * double (ir);
            geo::Point p2 = geo::Point (p2d);

            do { 
              ++ir;
              p2d += rd;
              p2 += r;
            } while (ir < rows && fabs(p2d.x () - p2.x ()) < 0.5 && fabs(p2d.y () - p2.y ()) < 0.5);

          }

          geo::Vector p = xy + geo::Vector (cd * double (ic0) + rd * double (ir0));

          //  insert the cell array
          geo::CellInstArray inst;

          if (is_mag || angle < 0) {
            inst = geo::CellInstArray (geo::CellInst (ci), 
                                      geo::ICplxTrans (mag, angle_deg, mirror, p), r, c, ir - ir0, ic - ic0);
          } else {
            inst = geo::CellInstArray (geo::CellInst (ci), 
                                      geo::Trans (angle, mirror, p), r, c, ir - ir0, ic - ic0);
          }

          if (pp.first) {
            instances_with_props.push_back (geo::CellInstArrayWithProperties (inst, pp.second));
          } else {
            instances.push_back (inst);
          }

        }

      }

    } else {

      if (cols > 1) {
        c = geo::Vector (c.x () / cols, c.y () / cols);
      }
      if (rows > 1) {
        r = geo::Vector (r.x () / rows, r.y () / rows);
      }

      //  insert the cell array
      geo::CellInstArray inst;

      if (is_mag || angle < 0) {
        inst = geo::CellInstArray (geo::CellInst (ci), 
                                  geo::ICplxTrans (mag, angle_deg, mirror, xy), r, c, rows, cols);
      } else {
        inst = geo::CellInstArray (geo::CellInst (ci), 
                                  geo::Trans (angle, mirror, xy), r, c, rows, cols);
      }

      if (pp.first) {
        instances_with_props.push_back (geo::CellInstArrayWithProperties (inst, pp.second));
      } else {
        instances.push_back (inst);
      }

    }

  } else {

    //  Single reference
    if (rec_id != sXY) {
      error (geo::to_string (tr ("XY record expected")));
    }

    //  Create the instance
    unsigned int xy_length = 0;
    GDS2XY *xy_data = get_xy_data (xy_length);
    if (xy_length < 1) {
      error (geo::to_string (tr ("Too few points in XY record for SREF")));
    } else if (xy_length > 1) {
      warn (geo::to_string (tr ("More than one point in XY record for SREF")));
    }

    //  Create the instance
    geo::Vector xy = v_conv (xy_data [0]);

    geo::CellInstArray inst;

    if (is_mag || angle < 0) {
      inst = geo::CellInstArray (geo::CellInst (ci), geo::ICplxTrans (mag, angle_deg, mirror, xy));
    } else {
      inst = geo::CellInstArray (geo::CellInst (ci), geo::Trans (angle, mirror, xy));
    }

    std::pair<bool, geo::properties_id_type> pp = finish_element (layout.properties_repository ());
    if (pp.first) {
      instances_with_props.push_back (geo::CellInstArrayWithProperties (inst, pp.second));
    } else {
      instances.push_back (inst);
    }

  }  
}


}

