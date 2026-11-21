#include "geoLayoutContextHandler.h"
#include "geoLibraryManager.h"
#include "geoLibrary.h"

namespace geo
{

// -----------------------------------------------------------------------------
//  LayoutContextHandler implementation

LayoutContextHandler::LayoutContextHandler (const geo::Layout *layout)
  : geo::ContextHandler (), mp_layout (layout), mp_layout_nc (0)
{
}

LayoutContextHandler::LayoutContextHandler (geo::Layout *layout, bool can_modify)
  : geo::ContextHandler (), mp_layout (layout), mp_layout_nc (0)
{
  if (can_modify) {
    mp_layout_nc = layout;
  }
}

geo::Variant LayoutContextHandler::eval_bracket (const std::string &content) const
{
  geo::Extractor ex (content.c_str ());
  geo::LayerProperties lp;
  lp.read (ex);

  if (! ex.at_end ()) {
    throw geo::Exception (geo::to_string (tr ("Not a valid layer source expression: ")) + content);
  }

  for (geo::LayerIterator l = mp_layout->begin_layers (); l != mp_layout->end_layers (); ++l) {
    if ((*l).second->log_equal (lp)) {
      return geo::Variant ((*l).first);
    }
  }

  if (mp_layout_nc) {
    return geo::Variant (mp_layout_nc->insert_layer (lp));
  } else {
    throw geo::Exception (geo::to_string (tr ("Not a valid layer: ")) + lp.to_string ());
  }
}

geo::Variant LayoutContextHandler::eval_double_bracket (const std::string &s) const
{
  std::pair<bool, geo::cell_index_type> ci = mp_layout->cell_by_name (s.c_str ());
  if (! ci.first) {

    if (mp_layout_nc) {

      std::string libname;
      const char *cp;
      for (cp = s.c_str (); *cp && *cp != '.'; ++cp) {
        libname += *cp;
      }
      if (*cp == '.') {

        std::string tail = cp + 1;

        geo::Library *lib = geo::LibraryManager::instance ().lib_ptr_by_name (libname, mp_layout->technology_name ());
        if (! lib) {
          throw geo::Exception (geo::to_string (tr ("Not a valid library name: ")) + libname);
        }

        geo::LayoutContextHandler lib_context (&lib->layout (), true);
        geo::Variant r = lib_context.eval_double_bracket (tail);
        if (r.is_nil ()) {
          return r;
        } else {
          geo::cell_index_type lib_cell_id = r.to<geo::cell_index_type> ();
          return geo::Variant (mp_layout_nc->get_lib_proxy (lib, lib_cell_id));
        }

      } else {
        return geo::Variant (mp_layout_nc->add_cell (s.c_str ()));
      }


    } else {
      throw geo::Exception (geo::to_string (tr ("Not a valid cell name: ")) + s);
    }

  }

  return geo::Variant (ci.second);
}

}

