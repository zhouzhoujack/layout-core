#include "geoPCellVariant.h"
#include "geoPCellHeader.h"

#include "utilsLog.h"

namespace geo
{

// -----------------------------------------------------------------------------
//  PCellVariant implementation

PCellVariant::PCellVariant (geo::cell_index_type ci, geo::Layout &layout, geo::pcell_id_type pcell_id, const pcell_parameters_type &parameters)
  : Cell (ci, layout), m_parameters (parameters), m_pcell_id (pcell_id), m_registered (false)
{
  PCellVariant::reregister (); // actually, no "re-register", but the first registration ..
}

PCellVariant::~PCellVariant ()
{
  PCellVariant::unregister ();
}

Cell *
PCellVariant::clone (Layout &layout) const
{
  tl_assert (m_registered);  // don't clone detached PCellVariant's
  Cell *cell = new PCellVariant (cell_index (), layout, m_pcell_id, m_parameters);
  //  copy the cell content
  *cell = *this;
  return cell;
}

void  
PCellVariant::unregister ()
{
  // A PCellVariant that is saved as a member of a transaction will explicitly be unregistered ..
  if (m_registered) {
    PCellHeader *header = pcell_header ();
    if (header) {
      header->unregister_variant (this);
    }
    m_registered = false;
  }
}

void  
PCellVariant::reregister ()
{
  if (! m_registered) {
    PCellHeader *header = pcell_header ();
    if (header) {
      header->register_variant (this);
    }
    m_registered = true;
  }
}

std::string 
PCellVariant::get_basic_name () const
{
  const PCellHeader *header = pcell_header ();
  if (header) {
    return header->get_name ();
  } else {
    return Cell::get_basic_name ();
  }
}

std::string 
PCellVariant::get_display_name () const
{
  const PCellHeader *header = pcell_header ();
  if (header) {
    if (m_display_name.empty ()) {
      return header->get_name () + "*";
    } else {
      return m_display_name;
    }
  } else {
    return Cell::get_basic_name ();
  }
}

geo::Variant
PCellVariant::parameter_by_name (const std::string &name) const
{
  const PCellHeader *header = pcell_header ();
  if (header && header->declaration ()) {

    geo::pcell_parameters_type::const_iterator pp = parameters ().begin ();
    const std::vector<geo::PCellParameterDeclaration> &pcp = header->declaration ()->parameter_declarations ();
    for (std::vector<PCellParameterDeclaration>::const_iterator pd = pcp.begin (); pd != pcp.end () && pp != parameters ().end (); ++pd, ++pp) {
      if (pd->get_name () == name) {
        return *pp;
      }
    }

  }

  return geo::Variant ();
}

std::map<std::string, geo::Variant>
PCellVariant::parameters_by_name () const
{
  std::map<std::string, geo::Variant> param_by_name;

  const PCellHeader *header = pcell_header ();
  if (header && header->declaration ()) {

    geo::pcell_parameters_type::const_iterator pp = parameters ().begin ();
    const std::vector<geo::PCellParameterDeclaration> &pcp = header->declaration ()->parameter_declarations ();
    for (std::vector<PCellParameterDeclaration>::const_iterator pd = pcp.begin (); pd != pcp.end () && pp != parameters ().end (); ++pd, ++pp) {
      param_by_name.insert (std::make_pair (pd->get_name (), *pp));
    }

  }

  return param_by_name;
}

void 
PCellVariant::update (ImportLayerMapping *layer_mapping)
{
  tl_assert (layout () != 0);

  clear_shapes ();
  clear_insts ();

  PCellHeader *header = pcell_header ();
  if (header && header->declaration ()) {

    geo::property_names_id_type pn = layout ()->properties_repository ().prop_name_id (geo::Variant ("name"));
    geo::property_names_id_type dn = layout ()->properties_repository ().prop_name_id (geo::Variant ("description"));

    std::vector<unsigned int> layer_ids;
    try {
      layer_ids = header->get_layer_indices (*layout (), m_parameters, layer_mapping);
      header->declaration ()->produce (*layout (), layer_ids, m_parameters, *this);
      m_display_name = header->declaration ()->get_display_name (m_parameters);
    } catch (geo::Exception &ex) {
      geo::error << ex.msg ();
      if (! layer_ids.empty ()) {
        //  put error messages into layout as text objects
        shapes (layer_ids [0]).insert (geo::Text (ex.msg (), geo::Trans ()));
      }
    }

    //  produce the shape parameters on the guiding shape layer so they can be edited
    size_t i = 0;
    const std::vector<geo::PCellParameterDeclaration> &pcp = header->declaration ()->parameter_declarations ();
    for (std::vector<geo::PCellParameterDeclaration>::const_iterator p = pcp.begin (); p != pcp.end (); ++p, ++i) {

      if (i < m_parameters.size () && p->get_type () == geo::PCellParameterDeclaration::t_shape && ! p->is_hidden ()) {

        //  use property with name "name" to indicate the parameter name
        geo::PropertiesRepository::properties_set props;
        props.insert (std::make_pair (pn, geo::Variant (p->get_name ())));

        if (! p->get_description ().empty ()) {
          props.insert (std::make_pair (dn, geo::Variant (p->get_description ())));
        }

        if (m_parameters[i].is_user<geo::DBox> ()) {

          shapes (layout ()->guiding_shape_layer ()).insert (geo::BoxWithProperties(geo::Box (m_parameters[i].to_user<geo::DBox> () * (1.0 / layout ()->dbu ())), layout ()->properties_repository ().properties_id (props)));

        } else if (m_parameters[i].is_user<geo::Box> ()) {

          shapes (layout ()->guiding_shape_layer ()).insert (geo::BoxWithProperties(m_parameters[i].to_user<geo::Box> (), layout ()->properties_repository ().properties_id (props)));

        } else if (m_parameters[i].is_user<geo::DEdge> ()) {

          shapes (layout ()->guiding_shape_layer ()).insert (geo::EdgeWithProperties(geo::Edge (m_parameters[i].to_user<geo::DEdge> () * (1.0 / layout ()->dbu ())), layout ()->properties_repository ().properties_id (props)));

        } else if (m_parameters[i].is_user<geo::Edge> ()) {

          shapes (layout ()->guiding_shape_layer ()).insert (geo::EdgeWithProperties(m_parameters[i].to_user<geo::Edge> (), layout ()->properties_repository ().properties_id (props)));

        } else if (m_parameters[i].is_user<geo::DPoint> ()) {

          geo::DPoint p = m_parameters[i].to_user<geo::DPoint> ();
          shapes (layout ()->guiding_shape_layer ()).insert (geo::BoxWithProperties(geo::Box (geo::DBox (p, p) * (1.0 / layout ()->dbu ())), layout ()->properties_repository ().properties_id (props)));

        } else if (m_parameters[i].is_user<geo::Point> ()) {

          geo::Point p = m_parameters[i].to_user<geo::Point> ();
          shapes (layout ()->guiding_shape_layer ()).insert (geo::BoxWithProperties(geo::Box (p, p), layout ()->properties_repository ().properties_id (props)));

        } else if (m_parameters[i].is_user<geo::DPolygon> ()) {

          geo::complex_trans<geo::DCoord, geo::Coord> dbu_trans (1.0 / layout ()->dbu ());
          geo::Polygon poly = m_parameters[i].to_user<geo::DPolygon> ().transformed (dbu_trans, false);
          //  Hint: we don't compress the polygon since we don't want to loose information
          shapes (layout ()->guiding_shape_layer ()).insert (geo::PolygonWithProperties(poly, layout ()->properties_repository ().properties_id (props)));

        } else if (m_parameters[i].is_user<geo::Polygon> ()) {

          geo::Polygon poly = m_parameters[i].to_user<geo::Polygon> ();
          //  Hint: we don't compress the polygon since we don't want to loose information
          shapes (layout ()->guiding_shape_layer ()).insert (geo::PolygonWithProperties(poly, layout ()->properties_repository ().properties_id (props)));

        } else if (m_parameters[i].is_user<geo::DPath> ()) {

          geo::complex_trans<geo::DCoord, geo::Coord> dbu_trans (1.0 / layout ()->dbu ());
          shapes (layout ()->guiding_shape_layer ()).insert (geo::PathWithProperties(dbu_trans * m_parameters[i].to_user<geo::DPath> (), layout ()->properties_repository ().properties_id (props)));

        } else if (m_parameters[i].is_user<geo::Path> ()) {

          shapes (layout ()->guiding_shape_layer ()).insert (geo::PathWithProperties(m_parameters[i].to_user<geo::Path> (), layout ()->properties_repository ().properties_id (props)));

        }

      }

    }

  }
}

}

