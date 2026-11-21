#include "geoPCellDeclaration.h"

namespace geo
{

PCellDeclaration::PCellDeclaration ()
  : m_ref_count (0), m_id (0), m_has_parameter_declarations (false)
{ 
  // .. nothing yet ..
}

void 
PCellDeclaration::add_ref ()
{
  ++m_ref_count;
}

void 
PCellDeclaration::release_ref ()
{
  --m_ref_count;
  if (m_ref_count <= 0) {
    delete this;
  }
}

const std::vector<PCellParameterDeclaration> &
PCellDeclaration::parameter_declarations () const
{
  if (! m_has_parameter_declarations || ! wants_parameter_declaration_caching ()) {
    std::vector<PCellParameterDeclaration> pcp = get_parameter_declarations ();
    //  NOTE: this ensures that reallocation of the vector only happens if the parameters
    //  change. This makes the returned reference more stable and iterators over this reference
    //  don't get invalidated so easily if wants_parameter_declaration_caching is false.
    if (m_parameter_declarations != pcp) {
      m_parameter_declarations = pcp;
    }
    m_has_parameter_declarations = true;
  }
  return m_parameter_declarations;
}

const std::string &
PCellDeclaration::parameter_name (size_t index)
{
  const std::vector<geo::PCellParameterDeclaration> &pcp = parameter_declarations ();
  if (index < pcp.size ()) {
    return pcp [index].get_name ();
  } else {
    static std::string empty;
    return empty;
  }
}

pcell_parameters_type
PCellDeclaration::map_parameters (const std::map<size_t, geo::Variant> &param_by_name) const
{
  geo::pcell_parameters_type new_param;
  size_t i = 0;
  const std::vector<geo::PCellParameterDeclaration> &pcp = parameter_declarations ();
  for (std::vector<PCellParameterDeclaration>::const_iterator pd = pcp.begin (); pd != pcp.end (); ++pd, ++i) {
    std::map<size_t, geo::Variant>::const_iterator p = param_by_name.find (i);
    if (p != param_by_name.end ()) {
      new_param.push_back (p->second);
    } else {
      new_param.push_back (pd->get_default ());
    }
  }

  return new_param;
}

pcell_parameters_type
PCellDeclaration::map_parameters (const std::map<std::string, geo::Variant> &param_by_name) const
{
  geo::pcell_parameters_type new_param;
  const std::vector<geo::PCellParameterDeclaration> &pcp = parameter_declarations ();
  for (std::vector<PCellParameterDeclaration>::const_iterator pd = pcp.begin (); pd != pcp.end (); ++pd) {
    std::map<std::string, geo::Variant>::const_iterator p = param_by_name.find (pd->get_name ());
    if (p != param_by_name.end ()) {
      new_param.push_back (p->second);
    } else {
      new_param.push_back (pd->get_default ());
    }
  }

  return new_param;
}

std::map<std::string, geo::Variant>
PCellDeclaration::named_parameters (const pcell_parameters_type &pv) const
{
  std::map<std::string, geo::Variant> np;

  const std::vector<geo::PCellParameterDeclaration> &pcp = parameter_declarations ();
  for (std::vector<PCellParameterDeclaration>::const_iterator pd = pcp.begin (); pd != pcp.end (); ++pd) {
    size_t index = pd - pcp.begin ();
    if (index >= pv.size ()) {
      break;
    }
    np.insert (std::make_pair (pd->get_name (), pv [index]));
  }

  return np;
}

}

