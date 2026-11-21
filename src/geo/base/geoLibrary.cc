#include "geoLibrary.h"
#include "geoLibraryProxy.h"
#include "geoPCellDeclaration.h"
#include "geoPCellVariant.h"

namespace geo
{

// -----------------------------------------------------------------------------
//  Library implementation

Library::Library()
  : m_id (0), m_layout (true)
{
  // .. nothing yet ..
}

Library::Library(const Library &d)
  : gsi::ObjectBase (), geo::RefCounted (), m_name (d.m_name), m_description (d.m_description), m_id (0), m_layout (d.m_layout)
{
  // .. nothing yet ..
}

Library::~Library ()
{
  // .. nothing yet ..
}

bool
Library::is_for_technology (const std::string &name) const
{
  return m_technologies.find (name) != m_technologies.end ();
}

bool
Library::for_technologies () const
{
  return ! m_technologies.empty ();
}

void
Library::set_technology (const std::string &t)
{
  m_technologies.clear ();
  if (! t.empty ()) {
    m_technologies.insert (t);
  }
}

void
Library::clear_technologies ()
{
  m_technologies.clear ();
}

void
Library::add_technology (const std::string &tech)
{
  m_technologies.insert (tech);
}

void 
Library::register_proxy (geo::LibraryProxy *lib_proxy, geo::Layout *ly)
{
  m_referrers.insert (std::make_pair (ly, 0)).first->second += 1;
  m_refcount.insert (std::make_pair (lib_proxy->library_cell_index (), 0)).first->second += 1;
  retired_state_changed_event ();
}

void 
Library::unregister_proxy (geo::LibraryProxy *lib_proxy, geo::Layout *ly)
{
  std::map<geo::Layout *, int>::iterator r = m_referrers.find (ly);
  if (r != m_referrers.end ()) {
    if (! --r->second) {
      m_referrers.erase (r);
    }
  }

  std::map<geo::cell_index_type, int>::iterator c = m_refcount.find (lib_proxy->library_cell_index ());
  if (c != m_refcount.end ()) {
    if (! --c->second) {
      geo::cell_index_type ci = c->first;
      m_refcount.erase (c);
      //  remove cells which are itself proxies and are no longer used
      geo::Cell *lib_cell = &layout ().cell (ci);
      if (lib_cell && lib_cell->is_proxy () && lib_cell->parent_cells () == 0) {
        layout ().delete_cell (ci);
      }
    }
    retired_state_changed_event ();
  }
}

void
Library::retire_proxy (geo::LibraryProxy *lib_proxy)
{
  m_retired_count.insert (std::make_pair (lib_proxy->library_cell_index (), 0)).first->second += 1;
  retired_state_changed_event ();
}

void
Library::unretire_proxy (geo::LibraryProxy *lib_proxy)
{
  std::map<geo::cell_index_type, int>::iterator c = m_retired_count.find (lib_proxy->library_cell_index ());
  if (c != m_retired_count.end ()) {
    if (! --c->second) {
      m_retired_count.erase (c);
    }
    retired_state_changed_event ();
  }
}

bool
Library::is_retired (const geo::cell_index_type library_cell_index) const
{
  std::map<geo::cell_index_type, int>::const_iterator i = m_refcount.find (library_cell_index);
  std::map<geo::cell_index_type, int>::const_iterator j = m_retired_count.find (library_cell_index);
  return (i != m_refcount.end () && j != m_retired_count.end () && i->second == j->second);
}

void
Library::remap_to (geo::Library *other)
{
  //  Hint: in the loop over the referrers we might unregister (delete from m_referrers) a referrer because no more cells refer to us.
  //  Hence we must not directly iterate of m_referrers.
  std::vector<std::pair<geo::Layout *, int> > referrers;
  for (std::map<geo::Layout *, int>::const_iterator r = m_referrers.begin (); r != m_referrers.end (); ++r) {
    referrers.push_back (*r);
  }

  //  Remember the layouts that will finally need a cleanup
  std::set<geo::Layout *> needs_cleanup;

  for (std::vector<std::pair<geo::Layout *, int> >::const_iterator r = referrers.begin (); r != referrers.end (); ++r) {

    std::vector<std::pair<geo::LibraryProxy *, geo::PCellVariant *> > pcells_to_map;
    std::vector<geo::LibraryProxy *> lib_cells_to_map;

    for (geo::Layout::iterator c = r->first->begin (); c != r->first->end (); ++c) {

      geo::LibraryProxy *lib_proxy = dynamic_cast<geo::LibraryProxy *> (&*c);
      if (lib_proxy && lib_proxy->lib_id () == get_id ()) {

        geo::Cell *lib_cell = &layout ().cell (lib_proxy->library_cell_index ());
        geo::PCellVariant *lib_pcell = dynamic_cast <geo::PCellVariant *> (lib_cell);
        if (lib_pcell) {
          pcells_to_map.push_back (std::make_pair (lib_proxy, lib_pcell));
        } else {
          lib_cells_to_map.push_back (lib_proxy);
        }

        needs_cleanup.insert (r->first);

      }

    }

    //  We do PCell resolution before the library proxy resolution. The reason is that 
    //  PCells may generate library proxies in their instantiation. Hence we must instantiate
    //  the PCells before we can resolve them.
    for (std::vector<std::pair<geo::LibraryProxy *, geo::PCellVariant *> >::const_iterator lp = pcells_to_map.begin (); lp != pcells_to_map.end (); ++lp) {

      geo::cell_index_type ci = lp->first->Cell::cell_index ();
      geo::PCellVariant *lib_pcell = lp->second;

      std::pair<bool, pcell_id_type> pn (false, 0);
      if (other) {
        pn = other->layout ().pcell_by_name (lp->first->get_basic_name ().c_str ());
      }

      if (! pn.first) {

        //  substitute by a cold proxy
        geo::ProxyContextInfo info;
        r->first->get_context_info (ci, info);
        r->first->create_cold_proxy_as (info, ci);

      } else {

        const geo::PCellDeclaration *old_pcell_decl = layout ().pcell_declaration (lib_pcell->pcell_id ());
        const geo::PCellDeclaration *new_pcell_decl = other->layout ().pcell_declaration (pn.second);
        if (! old_pcell_decl || ! new_pcell_decl) {

          //  substitute by a cold proxy
          geo::ProxyContextInfo info;
          r->first->get_context_info (ci, info);
          r->first->create_cold_proxy_as (info, ci);

        } else {

          //  map pcell parameters by name
          std::map<std::string, geo::Variant> param_by_name = lib_pcell->parameters_by_name ();
          lp->first->remap (other->get_id (), other->layout ().get_pcell_variant (pn.second, new_pcell_decl->map_parameters (param_by_name)));

        }

      }

    }

    for (std::vector<geo::LibraryProxy *>::const_iterator lp = lib_cells_to_map.begin (); lp != lib_cells_to_map.end (); ++lp) {

      geo::cell_index_type ci = (*lp)->Cell::cell_index ();

      std::pair<bool, cell_index_type> cn (false, 0);
      if (other) {
        cn = other->layout ().cell_by_name ((*lp)->get_basic_name ().c_str ());
      }

      if (! cn.first) {

        //  substitute by a cold proxy
        geo::ProxyContextInfo info;
        r->first->get_context_info (ci, info);
        r->first->create_cold_proxy_as (info, ci);

      } else {

        (*lp)->remap (other->get_id (), cn.second);

      }

    }

  }

  //  Do a cleanup later since the referrers now might have invalid proxy instances
  for (std::set<geo::Layout *>::const_iterator c = needs_cleanup.begin (); c != needs_cleanup.end (); ++c) {
    (*c)->cleanup ();
  }
}

}

