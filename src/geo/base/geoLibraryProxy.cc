#include "geoLibraryProxy.h"
#include "geoLibraryManager.h"
#include "geoLibrary.h"
#include "geoLayout.h"
#include "geoLayoutUtils.h"

namespace geo
{

// -----------------------------------------------------------------------------
//  LibraryProxy implementation

LibraryProxy::LibraryProxy (geo::cell_index_type ci, geo::Layout &layout, lib_id_type lib_id, cell_index_type lib_cell_index)
  : Cell (ci, layout), m_lib_id (lib_id), m_library_cell_index (lib_cell_index)
{
  geo::Library *lib = geo::LibraryManager::instance ().lib (lib_id);
  if (lib) {
    lib->register_proxy (this, &layout);
  }
  layout.register_lib_proxy (this);
}

LibraryProxy::~LibraryProxy ()
{
  if (layout ()) {
    layout ()->unregister_lib_proxy (this);
  }
  if (geo::LibraryManager::initialized ()) {
    geo::Library *lib = geo::LibraryManager::instance ().lib (m_lib_id);
    if (lib) {
      lib->unregister_proxy (this, layout ());
    }
  }
}

void 
LibraryProxy::unregister ()
{
  if (layout ()) {
    layout ()->unregister_lib_proxy (this);
  }
  if (geo::LibraryManager::initialized ()) {
    geo::Library *lib = geo::LibraryManager::instance ().lib (m_lib_id);
    if (lib) {
      lib->retire_proxy (this);
    }
  }
}

void 
LibraryProxy::reregister ()
{
  if (layout ()) {
    layout ()->register_lib_proxy (this);
  }
  if (geo::LibraryManager::initialized ()) {
    geo::Library *lib = geo::LibraryManager::instance ().lib (m_lib_id);
    if (lib) {
      lib->unretire_proxy (this);
    }
  }
}

void 
LibraryProxy::remap (lib_id_type lib_id, cell_index_type lib_cell_index)
{
  if (lib_id == m_lib_id && m_library_cell_index == lib_cell_index) {
    return;
  }

  if (layout ()) {
    layout ()->unregister_lib_proxy (this);
  }
  geo::Library *old_lib = geo::LibraryManager::instance ().lib (m_lib_id);
  if (old_lib) {
    old_lib->unregister_proxy (this, layout ());
  }

  m_lib_id = lib_id;
  m_library_cell_index = lib_cell_index;

  geo::Library *lib = geo::LibraryManager::instance ().lib (m_lib_id);
  if (lib) {
    lib->register_proxy (this, layout ());
  }
  if (layout ()) {
    layout ()->register_lib_proxy (this);
  }

  update ();
}

Cell *
LibraryProxy::clone (Layout &layout) const
{
  Cell *cell = new LibraryProxy (geo::Cell::cell_index (), layout, lib_id (), library_cell_index ());
  //  copy the cell content
  *cell = *this;
  return cell;
}

std::vector<int> 
LibraryProxy::get_layer_indices (geo::Layout &layout, geo::ImportLayerMapping *layer_mapping)
{
  std::vector<int> m_layer_indices; // TODO: should be somewhere "global" ..

  Library *lib = LibraryManager::instance ().lib (lib_id ());
  tl_assert (lib != 0);

  const geo::Cell &cell = lib->layout ().cell (library_cell_index ());

  bool reuse_layer_list = (m_layer_indices.size () == lib->layout ().layers ());
  for (unsigned int i = 0; i < m_layer_indices.size () && reuse_layer_list; ++i) {
    reuse_layer_list = layout.is_valid_layer (m_layer_indices[i]) 
                         && lib->layout ().is_valid_layer (i)
                         && layout.get_properties (m_layer_indices[i]).log_equal (lib->layout ().get_properties (i));
  }

  if (! reuse_layer_list) {

    geo::DirectLayerMapping direct_layer_mapping (&layout);
    if (! layer_mapping) {
      layer_mapping = &direct_layer_mapping;
    }

    m_layer_indices.clear ();
    m_layer_indices.reserve (lib->layout ().layers ());

    for (unsigned int i = 0; i < lib->layout ().layers (); ++i) {

      if (i == lib->layout ().guiding_shape_layer ()) {

        //  map guiding shape layer
        m_layer_indices.push_back ((int) layout.guiding_shape_layer ());

      } else if (! lib->layout ().is_valid_layer (i) || cell.bbox (i).empty ()) {

        m_layer_indices.push_back (-1);

      } else {

        std::pair<bool, unsigned int> lm = layer_mapping->map_layer (lib->layout ().get_properties (i));
        if (lm.first) {
          m_layer_indices.push_back (lm.second);
        } else {
          m_layer_indices.push_back (layout.waste_layer ());
        }

      }

    }

  }

  return m_layer_indices;
}

class LibraryCellIndexMapper
{
public:
  LibraryCellIndexMapper (Layout &layout, Library *lib)
    : mp_lib (lib), mp_layout (&layout)
  {
    // .. nothing yet ..
  }

  cell_index_type operator() (cell_index_type cell_index_in_lib)
  {
    return mp_layout->get_lib_proxy (mp_lib, cell_index_in_lib);
  }

private:
  Library *mp_lib;
  Layout *mp_layout;
};

void 
LibraryProxy::update (geo::ImportLayerMapping *layer_mapping)
{
  tl_assert (layout () != 0);
  std::vector<int> layer_indices (get_layer_indices (*layout (), layer_mapping));

  Library *lib = LibraryManager::instance ().lib (lib_id ());
  const geo::Cell &source_cell = lib->layout ().cell (library_cell_index ());

  geo::ICplxTrans tr;
  bool need_transform = false;
  if (fabs (layout ()->dbu () - lib->layout ().dbu ()) > 1e-6) {
    need_transform = true;
    tr = geo::ICplxTrans (lib->layout ().dbu () / layout ()->dbu ());
  }

  clear_shapes ();
  clear_insts ();

  PropertyMapper prop_id_map (*layout (), lib->layout ());

  for (unsigned int l = 0; l < lib->layout ().layers (); ++l) {
    if (layer_indices [l] >= 0) {
      shapes ((unsigned int) layer_indices [l]).assign_transformed (source_cell.shapes (l), tr, prop_id_map);
    }
  }

  LibraryCellIndexMapper cell_index_mapper (*layout (), lib);

  for (Cell::const_iterator inst = source_cell.begin (); !inst.at_end (); ++inst) {
    geo::Instance new_inst = insert (*inst, cell_index_mapper, prop_id_map);
    if (need_transform) {
      replace (new_inst, new_inst.cell_inst ().transformed_into (tr));
    }
  }
}

std::string 
LibraryProxy::get_basic_name () const
{
  Library *lib = LibraryManager::instance ().lib (lib_id ());
  if (lib) {
    const geo::Cell *lib_cell = &lib->layout ().cell (library_cell_index ());
    if (! lib_cell) {
      return "<defunct>";
    } else {
      return lib_cell->get_basic_name ();
    }
  } else {
    return Cell::get_basic_name ();
  }
}

std::string 
LibraryProxy::get_display_name () const
{
  Library *lib = LibraryManager::instance ().lib (lib_id ());
  if (lib) {
    const geo::Cell *lib_cell = &lib->layout ().cell (library_cell_index ());
    if (! lib_cell) {
      return lib->get_name () + "." + "<defunct>";
    } else {
      return lib->get_name () + "." + lib_cell->get_display_name ();
    }
  } else {
    return Cell::get_display_name ();
  }
}

std::string
LibraryProxy::get_qualified_name () const
{
  Library *lib = LibraryManager::instance ().lib (lib_id ());
  if (lib) {
    const geo::Cell *lib_cell = &lib->layout ().cell (library_cell_index ());
    if (! lib_cell) {
      return lib->get_name () + "." + "<defunct>";
    } else {
      return lib->get_name () + "." + lib_cell->get_qualified_name ();
    }
  } else {
    return Cell::get_qualified_name ();
  }
}

}

