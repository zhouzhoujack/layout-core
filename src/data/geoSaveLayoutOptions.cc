#include "geoSaveLayoutOptions.h"
#include "geoStream.h"
#include "utilsClassRegistry.h"
#include "utilsStream.h"
#include "utilsExpression.h"
#include "utilsInternational.h"

namespace geo
{

// -----------------------------------------------------------------------------
//  SaveLayoutOptions implementation

SaveLayoutOptions::SaveLayoutOptions ()
  : m_format ("GDS2"), m_all_layers (true), m_all_cells (true), m_dbu (0.0), m_scale_factor (1.0),
    m_keep_instances (false), m_write_context_info (true), m_dont_write_empty_cells (false)
{
  // .. nothing yet ..
}

SaveLayoutOptions::~SaveLayoutOptions ()
{
  release ();
}

SaveLayoutOptions::SaveLayoutOptions (const SaveLayoutOptions &d)
{
  operator= (d);
}

SaveLayoutOptions &
SaveLayoutOptions::operator= (const SaveLayoutOptions &d)
{
  if (this != &d) {

    m_format = d.m_format;
    m_layers = d.m_layers;
    m_cells = d.m_cells;
    m_implied_childred = d.m_implied_childred;
    m_all_layers = d.m_all_layers;
    m_all_cells = d.m_all_cells;
    m_dbu = d.m_dbu;
    m_scale_factor = d.m_scale_factor;
    m_keep_instances = d.m_keep_instances;
    m_write_context_info = d.m_write_context_info;
    m_dont_write_empty_cells = d.m_dont_write_empty_cells;

    release ();
    for (std::map <std::string, FormatSpecificWriterOptions *>::const_iterator o = d.m_options.begin (); o != d.m_options.end (); ++o) {
      m_options.insert (std::make_pair (o->first, o->second->clone ()));
    }

  }
  return *this;
}

void
SaveLayoutOptions::release ()
{
  for (std::map <std::string, FormatSpecificWriterOptions *>::const_iterator o = m_options.begin (); o != m_options.end (); ++o) {
    delete o->second;
  }
  m_options.clear ();
}

void
SaveLayoutOptions::set_options (const FormatSpecificWriterOptions &options)
{
  set_options (options.clone ());
}

void
SaveLayoutOptions::set_options (FormatSpecificWriterOptions *options)
{
  if (!options) {
    return;
  }

  std::map <std::string, FormatSpecificWriterOptions *>::iterator o = m_options.find (options->format_name ());
  if (o != m_options.end ()) {
    delete o->second;
    m_options.erase (o);
  }

  m_options.insert (std::make_pair (options->format_name (), options));
}

const FormatSpecificWriterOptions *
SaveLayoutOptions::get_options (const std::string &format) const
{
  std::map <std::string, FormatSpecificWriterOptions *>::const_iterator o = m_options.find (format);
  if (o != m_options.end ()) {
    return o->second;
  } else {
    return 0;
  }
}

FormatSpecificWriterOptions *
SaveLayoutOptions::get_options (const std::string &format)
{
  std::map <std::string, FormatSpecificWriterOptions *>::const_iterator o = m_options.find (format);
  if (o != m_options.end ()) {
    return o->second;
  } else {
    return 0;
  }
}

void
SaveLayoutOptions::set_option_by_name (const std::string &method, const geo::Variant &value)
{
  //  Utilizes the GSI binding to set the values
  geo::Variant options_ref = geo::Variant::make_variant_ref (this);
  const geo::EvalClass *eval_cls = options_ref.user_cls ()->eval_cls ();
  geo::ExpressionParserContext context;

  geo::Variant out;
  std::vector<geo::Variant> args;
  args.push_back (value);
  eval_cls->execute (context, out, options_ref, method + "=", args);
}

geo::Variant
SaveLayoutOptions::get_option_by_name (const std::string &method)
{
  //  Utilizes the GSI binding to set the values
  geo::Variant options_ref = geo::Variant::make_variant_ref (this);
  const geo::EvalClass *eval_cls = options_ref.user_cls ()->eval_cls ();
  geo::ExpressionParserContext context;

  geo::Variant out;
  std::vector<geo::Variant> args;
  eval_cls->execute (context, out, options_ref, method, args);

  return out;
}

void 
SaveLayoutOptions::set_format (const std::string &format_name)
{
  m_format = format_name;
}

void 
SaveLayoutOptions::add_layer (unsigned int layer, const geo::LayerProperties &props)
{
  m_all_layers = false;
  m_layers.insert (std::make_pair (layer, props));
}

void
SaveLayoutOptions::select_all_layers ()
{
  m_all_layers = true;
  m_layers.clear ();
}

void
SaveLayoutOptions::deselect_all_layers ()
{
  m_all_layers = false;
  m_layers.clear ();
}

void 
SaveLayoutOptions::add_cell (geo::cell_index_type cell_index)
{
  m_all_cells = false;
  m_cells.insert (cell_index);
  m_implied_childred.insert (cell_index);
}

void 
SaveLayoutOptions::add_this_cell (geo::cell_index_type cell_index)
{
  m_all_cells = false;
  m_cells.insert (cell_index);
}

void
SaveLayoutOptions::clear_cells ()
{
  m_all_cells = false;
  m_cells.clear ();
  m_implied_childred.clear ();
}

void
SaveLayoutOptions::select_all_cells ()
{
  m_all_cells = true;
  m_cells.clear ();
  m_implied_childred.clear ();
}

void 
SaveLayoutOptions::set_dbu (double dbu)
{
  m_dbu = dbu;
}

void 
SaveLayoutOptions::set_scale_factor (double f)
{
  m_scale_factor = f;
}

void 
SaveLayoutOptions::set_dont_write_empty_cells (bool f)
{
  m_dont_write_empty_cells = f;
}

void 
SaveLayoutOptions::get_valid_layers (const geo::Layout &layout, std::vector <std::pair <unsigned int, geo::LayerProperties> > &layers, geo::SaveLayoutOptions::LayerAssignmentMode lm) const
{
  std::vector<std::pair <unsigned int, geo::LayerProperties> > all_layers;

  if (m_all_layers) {

    //  collect all layers if necessary
    for (unsigned int l = 0; l < layout.layers (); ++l) {
      if (layout.is_valid_layer (l)) {
        const geo::LayerProperties &prop = layout.get_properties (l);
        if (! prop.is_null ()) {
          all_layers.push_back (std::make_pair (l, prop));
        }
      }
    }

  } else {

    //  collect the selected layers 
    for (unsigned int l = 0; l < layout.layers (); ++l) {
      if (layout.is_valid_layer (l)) {
        const geo::LayerProperties &prop = layout.get_properties (l);
        std::map<unsigned int, geo::LayerProperties>::const_iterator ll = m_layers.find (l);
        if (ll != m_layers.end ()) {
          if (! ll->second.is_null ()) {
            all_layers.push_back (*ll);
          } else if (! prop.is_null ()) {
            all_layers.push_back (std::make_pair (ll->first, prop));
          }
        }
      }
    }

  }

  if (lm == LP_OnlyNumbered) {

    for (std::vector<std::pair <unsigned int, geo::LayerProperties> >::const_iterator l = all_layers.begin (); l != all_layers.end (); ++l) {
      if (l->second.layer >= 0 && l->second.datatype >= 0) {
        layers.push_back (*l);
      }
    }

  } else if (lm == LP_OnlyNamed) {

    for (std::vector<std::pair <unsigned int, geo::LayerProperties> >::const_iterator l = all_layers.begin (); l != all_layers.end (); ++l) {
      if (! l->second.name.empty ()) {
        layers.push_back (*l);
      }
    }

  } else if (lm == LP_AssignName) {

    for (std::vector<std::pair <unsigned int, geo::LayerProperties> >::const_iterator l = all_layers.begin (); l != all_layers.end (); ++l) {
      layers.push_back (*l);
      if (l->second.name.empty ()) {
        layers.back ().second = geo::sprintf ("L%dD%d", l->second.layer, l->second.datatype);
      } else if (l->second.layer >= 0 && l->second.datatype >= 0) {
        layers.back ().second = geo::sprintf ("L%dD%d", l->second.layer, l->second.datatype) + "_" + l->second.name;
      }
    }

  } else if (lm == LP_AssignNameWithPriority) {

    for (std::vector<std::pair <unsigned int, geo::LayerProperties> >::const_iterator l = all_layers.begin (); l != all_layers.end (); ++l) {
      layers.push_back (*l);
      if (l->second.name.empty ()) {
        layers.back ().second = geo::sprintf ("L%dD%d", l->second.layer, l->second.datatype);
      } else if (l->second.layer >= 0 && l->second.datatype >= 0) {
        layers.back ().second = l->second.name;
      }
    }

  } else if (lm == LP_AssignNumber) {

    int next_layer = 0;
    for (unsigned int l = 0; l < layout.layers (); ++l) {
      if (layout.is_valid_layer (l)) {
        const geo::LayerProperties &prop = layout.get_properties (l);
        next_layer = std::max (prop.layer, next_layer);
      }
    }

    for (std::vector<std::pair <unsigned int, geo::LayerProperties> >::const_iterator l = all_layers.begin (); l != all_layers.end (); ++l) {
      layers.push_back (*l);
      if (! (l->second.layer >= 0 && l->second.datatype >= 0)) {
        layers.back ().second.layer = ++next_layer;
        layers.back ().second.datatype = 0;
      }
    }

  }
}

void 
SaveLayoutOptions::get_cells (const geo::Layout &layout, std::set <geo::cell_index_type> &cells, const std::vector <std::pair <unsigned int, geo::LayerProperties> > &valid_layers, bool require_unique_names) const
{
  if (m_all_cells) {

    for (geo::Layout::const_iterator cell = layout.begin (); cell != layout.end (); ++cell) {
      cells.insert (cell->cell_index ());
    }

  } else {

    for (std::set <geo::cell_index_type>::const_iterator c = m_cells.begin (); c != m_cells.end (); ++c) {
      cells.insert (*c);
      if (m_implied_childred.find (*c) != m_implied_childred.end ()) {
        layout.cell (*c).collect_called_cells (cells);
      }
    }

  }

  if (m_dont_write_empty_cells) {

    std::set <geo::cell_index_type> empty_cells;

    for (std::set <geo::cell_index_type>::const_iterator c = cells.begin (); c != cells.end (); ++c) {

      const geo::Cell &cref (layout.cell (*c));

      bool is_empty = true;
      for (std::vector <std::pair <unsigned int, geo::LayerProperties> >::const_iterator l = valid_layers.begin (); l != valid_layers.end () && is_empty; ++l) {
        if (! cref.shapes (l->first).empty ()) {
          is_empty = false;
        }
      }

      if (is_empty) {

        //  treat only cells as empty which are referenced within the cell set we have defined - other cells
        //  will become top cells and must not be deleted
        bool is_top_cell = true;

        for (geo::Cell::parent_cell_iterator p = cref.begin_parent_cells (); p != cref.end_parent_cells () && is_top_cell; ++p) {
          if (cells.find (*p) != cells.end ()) {
            is_top_cell = false;
          }
        }

        if (! is_top_cell) {
          empty_cells.insert (*c);
        }

      }

    }

    bool repeat;
    do {

      repeat = false;

      for (std::set <geo::cell_index_type>::const_iterator c = empty_cells.begin (); c != empty_cells.end (); ) {
        const geo::Cell *cell = &layout.cell (*c);
        ++c;
        bool is_empty = true;
        for (geo::Cell::child_cell_iterator cc = cell->begin_child_cells (); ! cc.at_end () && is_empty; ++cc) {
          if (empty_cells.find (*cc) == empty_cells.end ()) {
            is_empty = false;
          }
        }
        if (! is_empty) {
          empty_cells.erase (cell->cell_index ());
          repeat = true;
        }
      }

    } while (repeat);

    for (std::set <geo::cell_index_type>::const_iterator c = empty_cells.begin (); c != empty_cells.end (); ++c) {
      cells.erase (*c);
    }

  }

  if (require_unique_names) {

    std::map<std::string, unsigned int> use_count;
    for (std::set <geo::cell_index_type>::const_iterator c = cells.begin (); c != cells.end (); ++c) {
      use_count.insert (std::make_pair (std::string (layout.cell_name (*c)), 0)).first->second += 1;
    }

    std::vector<std::string> multi;
    for (std::map<std::string, unsigned int>::const_iterator u = use_count.begin (); u != use_count.end (); ++u) {
      if (u->second > 1) {
        multi.push_back (u->first);
      }
    }

    if (! multi.empty ()) {
      throw geo::Exception (geo::to_string (tr ("The following cell name(s) are used for more than one cell - can't write this layout:\n  ")) + geo::join (multi, "\n  "));
    }

  }
}

bool 
SaveLayoutOptions::set_format_from_filename (const std::string &fn)
{
  for (geo::Registrar<geo::StreamFormatDeclaration>::iterator fmt = geo::Registrar<geo::StreamFormatDeclaration>::begin (); fmt != geo::Registrar<geo::StreamFormatDeclaration>::end (); ++fmt) {
    if (geo::match_filename_to_format (fn, fmt->file_format ())) {
      m_format = fmt->format_name ();
      return true;
    }
  }
  return false;
}

}

