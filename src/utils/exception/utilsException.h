#ifndef HDR_utilsException
#define HDR_utilsException

#include "utilsCommon.h"

#include "utilsInternational.h"
#include "utilsVariant.h"

namespace geo
{

/**
 *  @brief The unspecific exception class
 *
 *  This class is the base class for all exceptions in this 
 *  framework. It does not carry further information except 
 *  a message string that can be created through different 
 *  constructor methods.
 */

class TL_PUBLIC Exception 
{
public:
  Exception (const std::string &msg)
    : m_msg (msg)
  { }

  Exception (const std::string &fmt, const std::vector<geo::Variant> &a)
  {
    init (fmt, a);
  }

  Exception (const std::string &fmt, const geo::Variant &a1)
  {
    std::vector<geo::Variant> a;
    a.push_back (a1);
    init (fmt, a);
  }

  Exception (const std::string &fmt, const geo::Variant &a1, const geo::Variant &a2)
  {
    std::vector<geo::Variant> a;
    a.push_back (a1);
    a.push_back (a2);
    init (fmt, a);
  }

  Exception (const std::string &fmt, const geo::Variant &a1, const geo::Variant &a2, const geo::Variant &a3)
  {
    std::vector<geo::Variant> a;
    a.push_back (a1);
    a.push_back (a2);
    a.push_back (a3);
    init (fmt, a);
  }

  Exception (const std::string &fmt, const geo::Variant &a1, const geo::Variant &a2, const geo::Variant &a3, const geo::Variant &a4)
  {
    std::vector<geo::Variant> a;
    a.push_back (a1);
    a.push_back (a2);
    a.push_back (a3);
    a.push_back (a4);
    init (fmt, a);
  }

  Exception (const std::string &fmt, const geo::Variant &a1, const geo::Variant &a2, const geo::Variant &a3, const geo::Variant &a4, const geo::Variant &a5)
  {
    std::vector<geo::Variant> a;
    a.push_back (a1);
    a.push_back (a2);
    a.push_back (a3);
    a.push_back (a4);
    a.push_back (a5);
    init (fmt, a);
  }

  Exception (const std::string &fmt, const geo::Variant &a1, const geo::Variant &a2, const geo::Variant &a3, const geo::Variant &a4, const geo::Variant &a5, const geo::Variant &a6)
  {
    std::vector<geo::Variant> a;
    a.push_back (a1);
    a.push_back (a2);
    a.push_back (a3);
    a.push_back (a4);
    a.push_back (a5);
    a.push_back (a6);
    init (fmt, a);
  }

  Exception (const std::string &fmt, const geo::Variant &a1, const geo::Variant &a2, const geo::Variant &a3, const geo::Variant &a4, const geo::Variant &a5, const geo::Variant &a6, const geo::Variant &a7)
  {
    std::vector<geo::Variant> a;
    a.push_back (a1);
    a.push_back (a2);
    a.push_back (a3);
    a.push_back (a4);
    a.push_back (a5);
    a.push_back (a6);
    a.push_back (a7);
    init (fmt, a);
  }

  virtual ~Exception () { }

  virtual std::string msg () const { return m_msg; }

private:
  std::string m_msg;
  void init (const std::string &fmt, const std::vector<geo::Variant> &a);
};

/**
 *  @brief A "neutral" exception thrown to terminate some operation
 *  This exception is not shown.
 */
struct TL_PUBLIC CancelException
  : public Exception
{
  CancelException ()
    : Exception (geo::to_string (tr ("Operation cancelled")))
  { }
};

/**
 *  @brief A special "internal" exception class used by tl_assert
 */
struct TL_PUBLIC InternalException
  : public Exception
{
  InternalException (const char *file, int line, const char *cond)
    : Exception (geo::to_string (tr ("Internal error: %s:%d %s was not true")).c_str (), file, line, cond)
  { }
};

} // namespace geo

#endif

