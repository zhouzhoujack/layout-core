#ifndef HDR_geoColdProxy
#define HDR_geoColdProxy

#include "geoCommon.h"

#include "geoTypes.h"
#include "geoCell.h"

#include "utilsObject.h"
#include "utilsTypeTraits.h"

namespace geo
{

struct ProxyContextInfo;

/**
 *  @brief A cell specialization: a cold proxy representing a library or PCell which has gone out of scope
 *
 *  If a PCell or library cell gets disconnected - for example, because the technology has changed or during
 *  development of PCell code - this proxy replaces the original one. It stores the connection information, so
 *  it can be regenerated when it becomes valid again.
 */
class DB_PUBLIC ColdProxy 
  : public Cell, public geo::RefCounted
{
public:
  /** 
   *  @brief The constructor
   *
   *  Creates a cold proxy represented by the ProxyContextInfo data.
   */
  ColdProxy (geo::cell_index_type ci, geo::Layout &layout, const ProxyContextInfo &info);

  /**
   *  @brief The destructor
   */
  ~ColdProxy ();

  /**
   *  @brief Cloning 
   */
  virtual Cell *clone (Layout &layout) const;

  /**
   *  @brief Get the library id 
   */
  const ProxyContextInfo &context_info () const
  {
    return *mp_context_info;
  }

  /**
   *  @brief Indicates that this cell is a proxy cell
   */
  virtual bool is_proxy () const 
  { 
    return true; 
  }

  /**
   *  @brief Gets a list of cold proxies for a given library name
   */
  static const geo::weak_collection<ColdProxy> &cold_proxies_per_lib_name (const std::string &libname);

  /**
   *  @brief Gets the basic name
   */
  virtual std::string get_basic_name () const;

  /**
   *  @brief Gets the display name
   */
  virtual std::string get_display_name () const;

  /**
   *  @brief Gets the qualified name
   */
  virtual std::string get_qualified_name () const;

private:
  ProxyContextInfo *mp_context_info;

  ColdProxy (const ColdProxy &d);
  ColdProxy &operator= (const ColdProxy &d);
};

}

namespace geo
{
  template <>
  struct type_traits<geo::ColdProxy> : public type_traits<void>
  {
    typedef geo::false_tag has_default_constructor;
    typedef geo::false_tag has_copy_constructor;
  };
}

#endif


