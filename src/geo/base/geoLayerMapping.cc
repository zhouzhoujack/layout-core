#include "geoLayout.h"
#include "geoLayerMapping.h"

namespace geo
{

// -------------------------------------------------------------------------------------
//  LayerMapping implementation

LayerMapping::LayerMapping ()
{
  // .. nothing yet ..
}

void LayerMapping::clear ()
{
  m_b2a_mapping.clear ();
}

void 
LayerMapping::create (const geo::Layout &layout_a, const geo::Layout &layout_b)
{
  clear ();

  if (&layout_a == &layout_b) {

    for (geo::Layout::layer_iterator la = layout_a.begin_layers (); la != layout_a.end_layers (); ++la) {
      m_b2a_mapping.insert (std::make_pair ((*la).first, (*la).first));
    }

  } else {

    std::map<geo::LayerProperties, unsigned int, geo::LPLogicalLessFunc> layers;

    for (geo::Layout::layer_iterator la = layout_a.begin_layers (); la != layout_a.end_layers (); ++la) {
      if (! (*la).second->is_null ()) {
        layers.insert (std::make_pair (*(*la).second, (*la).first));
      }
    }

    for (geo::Layout::layer_iterator lb = layout_b.begin_layers (); lb != layout_b.end_layers (); ++lb) {
      if (! (*lb).second->is_null ()) {
        std::map<geo::LayerProperties, unsigned int, geo::LPLogicalLessFunc>::const_iterator l = layers.find (*(*lb).second);
        if (l != layers.end ()) {
          m_b2a_mapping.insert (std::make_pair ((*lb).first, l->second));
        }
      }
    }

  }
}

std::vector<unsigned int> 
LayerMapping::create_full (geo::Layout &layout_a, const geo::Layout &layout_b)
{
  clear ();

  std::vector<unsigned int> new_layers;

  if (&layout_a == &layout_b) {

    for (geo::Layout::layer_iterator la = layout_a.begin_layers (); la != layout_a.end_layers (); ++la) {
      m_b2a_mapping.insert (std::make_pair ((*la).first, (*la).first));
    }

  } else {

    std::map<geo::LayerProperties, unsigned int, geo::LPLogicalLessFunc> layers;

    for (geo::Layout::layer_iterator la = layout_a.begin_layers (); la != layout_a.end_layers (); ++la) {
      if (! (*la).second->is_null ()) {
        layers.insert (std::make_pair (*(*la).second, (*la).first));
      }
    }

    for (geo::Layout::layer_iterator lb = layout_b.begin_layers (); lb != layout_b.end_layers (); ++lb) {
      if (! (*lb).second->is_null ()) {
        std::map<geo::LayerProperties, unsigned int, geo::LPLogicalLessFunc>::const_iterator l = layers.find (*(*lb).second);
        if (l != layers.end ()) {
          m_b2a_mapping.insert (std::make_pair ((*lb).first, l->second));
        } else {
          unsigned int nl = layout_a.insert_layer (*(*lb).second);
          new_layers.push_back (nl);
          m_b2a_mapping.insert (std::make_pair ((*lb).first, nl));
        }
      }
    }

  }

  return new_layers;
}

std::pair<bool, unsigned int> 
LayerMapping::layer_mapping_pair (unsigned int layer_b) const
{
  std::map <unsigned int, unsigned int>::const_iterator m = m_b2a_mapping.find (layer_b);
  if (m == m_b2a_mapping.end ()) {
    return std::make_pair (false, 0);
  } else {
    return std::make_pair (true, m->second);
  }
}

bool 
LayerMapping::has_mapping (unsigned int layer_b) const
{
  std::map <unsigned int, unsigned int>::const_iterator m = m_b2a_mapping.find (layer_b);
  return (m != m_b2a_mapping.end ());
}

unsigned int
LayerMapping::layer_mapping (unsigned int layer_b) const
{
  std::map <unsigned int, unsigned int>::const_iterator m = m_b2a_mapping.find (layer_b);
  tl_assert (m != m_b2a_mapping.end ());
  return m->second;
}

}

