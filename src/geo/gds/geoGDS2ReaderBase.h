#ifndef HDR_geoGDS2ReaderBase
#define HDR_geoGDS2ReaderBase

#include "geoLayout.h"
#include "geoReader.h"
#include "geoStreamLayers.h"
#include "geoCommonReader.h"

#include "utilsException.h"
#include "utilsInternational.h"
#include "utilsProgress.h"
#include "utilsString.h"
#include "utilsStream.h"

namespace geo
{

struct GDS2XY
{
  unsigned char x[4];
  unsigned char y[4];
};

/**
 *  @brief The GDS2 format basic stream reader
 */
class GDS2ReaderBase
  : public CommonReader
{
public: 
  /**
   *  @brief Constructor
   */
  GDS2ReaderBase ();

  /**  
   *  @brief Destructor
   */
  ~GDS2ReaderBase ();

  /**
   *  @brief Accessor method to the library name
   */
  const std::string &libname () const { return m_libname; }

protected:
  /**
   *  @brief Accessor method to the current cellname
   */
  const std::string &cellname () const { return m_cellname; }

  virtual void do_read (geo::Layout &layout);
  virtual void init (const LoadLayoutOptions &options);

private:
  friend class GDS2ReaderLayerMapping;

  std::string m_cellname;
  std::string m_libname;
  double m_dbu, m_dbuu;
  bool m_read_texts;
  bool m_read_properties;
  bool m_allow_multi_xy_records;
  unsigned int m_box_mode;
  std::map <geo::string, std::vector<std::string> > m_context_info;
  std::vector <geo::Point> m_all_points;

  void read_context_info_cell ();
  void read_boundary (geo::Layout &layout, geo::Cell &cell, bool from_box_record);
  void read_path (geo::Layout &layout, geo::Cell &cell);
  void read_text (geo::Layout &layout, geo::Cell &cell);
  void read_box (geo::Layout &layout, geo::Cell &cell);
  void read_ref (geo::Layout &layout, geo::Cell &cell, bool array, geo::uvector<geo::CellInstArray> &instances, geo::uvector<geo::CellInstArrayWithProperties> &insts_wp);

  std::pair <bool, geo::properties_id_type> finish_element (geo::PropertiesRepository &rep);
  void finish_element ();

  virtual void common_reader_error (const std::string &msg) { error (msg); }
  virtual void common_reader_warn (const std::string &msg) { warn (msg); }

  virtual void error (const std::string &txt) = 0;
  virtual void warn (const std::string &txt) = 0;

  virtual std::string path () const = 0;
  virtual const char *get_string () = 0;
  virtual void get_string (std::string &s) const = 0;
  virtual int get_int () = 0;
  virtual short get_short () = 0;
  virtual unsigned short get_ushort () = 0;
  virtual double get_double () = 0;
  virtual short get_record () = 0;
  virtual void unget_record (short rec_id) = 0;
  virtual void get_time (unsigned int *mod_time, unsigned int *access_time) = 0;
  virtual GDS2XY *get_xy_data (unsigned int &xy_length) = 0;
  virtual void progress_checkpoint () = 0;
};

}

#endif

