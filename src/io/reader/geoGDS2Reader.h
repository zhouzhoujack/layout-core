#ifndef HDR_geoGDS2Reader
#define HDR_geoGDS2Reader

#include "geoLayout.h"
#include "geoGDS2Format.h"
#include "geoGDS2ReaderBase.h"
#include "geoCommonReader.h"
#include "geoStreamLayers.h"

#include "utilsException.h"
#include "utilsInternational.h"
#include "utilsProgress.h"
#include "utilsString.h"
#include "utilsStream.h"

namespace geo
{

/**
 *  @brief Generic base class of GDS2 reader exceptions
 */
class GDS2ReaderException
  : public ReaderException 
{
public:
  GDS2ReaderException (const std::string &msg, size_t p, size_t n, const std::string &cell)
    : ReaderException (geo::sprintf (geo::to_string (tr ("%s (position=%ld, record number=%ld, cell=%s)")), msg, p, n, cell))
  { }
};

/**
 *  @brief The GDS2 format stream reader
 */
class GDS2Reader
  : public GDS2ReaderBase
{
public: 
  /**
   *  @brief Construct a stream reader object
   *
   *  @param s The stream delegate from which to read stream data from
   */
  GDS2Reader (geo::InputStream &s);

  /**  
   *  @brief Destructor
   */
  ~GDS2Reader ();

  /**
   *  @brief Format
   */
  virtual const char *format () const { return "GDS2"; }

protected:
  virtual void init (const LoadLayoutOptions &options);

private:
  geo::InputStream &m_stream;
  size_t m_recnum;
  size_t m_reclen;
  size_t m_recptr;
  unsigned char *mp_rec_buf;
  geo::string m_string_buf;
  short m_stored_rec;
  bool m_allow_big_records;
  geo::AbsoluteProgress m_progress;

  virtual void error (const std::string &txt);
  virtual void warn (const std::string &txt);

  virtual std::string path () const;
  virtual const char *get_string ();
  virtual void get_string (std::string &s) const;
  virtual int get_int ();
  virtual short get_short ();
  virtual unsigned short get_ushort ();
  virtual double get_double ();
  virtual short get_record ();
  virtual void unget_record (short rec_id);
  virtual void get_time (unsigned int *mod_time, unsigned int *access_time);
  virtual GDS2XY *get_xy_data (unsigned int &length);
  virtual void progress_checkpoint ();
};

}

#endif

