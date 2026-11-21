#include "geoColdProxy.h"
#include "geoLibraryManager.h"
#include "geoLibrary.h"
#include "geoLayout.h"
#include "geoLayoutUtils.h"

#include "utilsThreads.h"

namespace geo
{

static geo::Mutex s_map_mutex;
static std::map<std::string, geo::weak_collection<ColdProxy> *> s_proxies_per_library_name;

const geo::weak_collection<ColdProxy> &
ColdProxy::cold_proxies_per_lib_name (const std::string &libname)
{
  geo::MutexLocker locker (&s_map_mutex);

  std::map<std::string, geo::weak_collection<ColdProxy> *>::const_iterator i = s_proxies_per_library_name.find (libname);
  if (i != s_proxies_per_library_name.end ()) {
    return *i->second;
  } else {
    static geo::weak_collection<ColdProxy> s_empty;
    return s_empty;
  }
}

// -----------------------------------------------------------------------------
//  ColdProxy implementation

ColdProxy::ColdProxy (geo::cell_index_type ci, geo::Layout &layout, const ProxyContextInfo &info)
  : Cell (ci, layout), mp_context_info (new ProxyContextInfo (info))
{
  if (! info.lib_name.empty ()) {
    geo::MutexLocker locker (&s_map_mutex);
    std::map<std::string, geo::weak_collection<ColdProxy> *>::iterator i = s_proxies_per_library_name.find (info.lib_name);
    if (i == s_proxies_per_library_name.end ()) {
      i = s_proxies_per_library_name.insert (std::make_pair (info.lib_name, new geo::weak_collection<ColdProxy> ())).first;
    }
    i->second->push_back (this);
  }
}

ColdProxy::~ColdProxy ()
{
  delete mp_context_info;
  mp_context_info = 0;
}

Cell *
ColdProxy::clone (Layout &layout) const
{
  Cell *cell = new ColdProxy (geo::Cell::cell_index (), layout, *mp_context_info);
  //  copy the cell content
  *cell = *this;
  return cell;
}

std::string 
ColdProxy::get_basic_name () const
{
  if (! mp_context_info->pcell_name.empty ()) {
    return "<defunct>" + mp_context_info->pcell_name;
  } else if (! mp_context_info->cell_name.empty ()) {
    return "<defunct>" + mp_context_info->cell_name;
  } else {
    return Cell::get_basic_name ();
  }
}

std::string 
ColdProxy::get_display_name () const
{
  if (! mp_context_info->lib_name.empty ()) {
    std::string stem = "<defunct>" + mp_context_info->lib_name + ".";
    if (! mp_context_info->pcell_name.empty ()) {
      return stem + mp_context_info->pcell_name;
    } else if (! mp_context_info->cell_name.empty ()) {
      return stem + mp_context_info->cell_name;
    } else {
      return stem + "<unknown>";
    }
  } else {
    return Cell::get_display_name ();
  }
}

std::string
ColdProxy::get_qualified_name () const
{
  if (! mp_context_info->lib_name.empty ()) {
    std::string stem = "<defunct>" + mp_context_info->lib_name + ".";
    if (! mp_context_info->pcell_name.empty ()) {
      if (mp_context_info->pcell_parameters.empty ()) {
        return stem + mp_context_info->pcell_name;
      } else {
        //  TODO: list parameters? Might be long.
        return stem + mp_context_info->pcell_name + "(...)";
      }
    } else if (! mp_context_info->cell_name.empty ()) {
      return stem + mp_context_info->cell_name;
    } else {
      return stem + "<unknown>";
    }
  } else {
    return Cell::get_qualified_name ();
  }
}

}

