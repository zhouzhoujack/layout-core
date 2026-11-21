#ifndef HDR_geoOASIS2Reader
#define HDR_geoOASIS2Reader

#include "geoLayout.h"
#include "geoOASIS2Format.h"
#include "geoOASIS2ReaderBase.h"
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
 *  @brief The OASIS format stream reader
 *
 *  This class supplies the byte level primitives for the OASIS record decoder
 *  (see OASIS2ReaderBase). It reads from a geo::InputStream and supports
 *  DEFLATE compressed blocks (CBLOCK records).
 */
class OASIS2Reader
  : public OASIS2ReaderBase
{
public:
  /**
   *  @brief Construct a stream reader object
   *
   *  @param s The stream delegate from which to read stream data from
   */
  OASIS2Reader (geo::InputStream &s);

  /**
   *  @brief Destructor
   */
  ~OASIS2Reader ();

  /**
   *  @brief Format
   */
  virtual const char *format () const { return "OASIS"; }

protected:
  virtual void init (const LoadLayoutOptions &options);

  virtual unsigned char get_byte ();
  virtual uint64_t get_uint ();
  virtual int64_t get_int ();
  virtual double get_real ();
  virtual std::string get_string ();
  virtual const char *get_bytes (size_t n);
  virtual void unget_byte ();
  virtual void inflate_stream ();
  virtual void progress_checkpoint ();

  virtual size_t stream_pos () const;
  virtual std::string path () const;
  virtual void error (const std::string &txt);
  virtual void warn (const std::string &txt);

private:
  geo::InputStream &m_stream;
  geo::AbsoluteProgress m_progress;
};

}

#endif
