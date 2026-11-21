#ifndef HDR_geoPCellHeader
#define HDR_geoPCellHeader


#include "geoLayout.h"
#include "geoPCellDeclaration.h"
#include "utilsVariant.h"

#include <string.h>

namespace geo
{

class PCellVariant;

struct PCellParametersCompareFunc
{
  bool operator() (const pcell_parameters_type *a, const pcell_parameters_type *b) const;
};

/**
 *  @brief A PCell header
 *
 *  The PCell header manages the PCell variants along with the PCell declaration.
 */
class PCellHeader
{
public:
  typedef std::map<const pcell_parameters_type *, geo::PCellVariant *, PCellParametersCompareFunc> variant_map_t;
  typedef variant_map_t::const_iterator variant_iterator;

  /**
   *  @brief The default constructor
   */
  PCellHeader(size_t pcell_id, const std::string &name, PCellDeclaration *declaration);

  /**
   *  @brief The destructor
   */
  ~PCellHeader ();

  /**
   *  @brief Copy constructor
   */
  PCellHeader (const PCellHeader &d);

  /**
   *  @brief Get the name
   */
  const std::string &get_name () const
  {
    return m_name;
  }

  /**
   *  @brief Get the declaration
   */
  const PCellDeclaration *declaration () const
  {
    return mp_declaration;
  }

  /**
   *  @brief Set the declaration
   *
   *  The declaration can be 0, in which case the PCell cannot produce any code.
   */
  void declaration (PCellDeclaration *declaration);

  /**
   *  @brief Get the layer index list for this PCell declaration for the given parameter set
   */
  std::vector<unsigned int> get_layer_indices (geo::Layout &layout, const pcell_parameters_type &parameters, geo::ImportLayerMapping *layer_mapping = 0);

  /**
   *  @brief Get the variant for a given parameter set if it already exists.
   *
   *  If no variant is registered for this parameter set, 0 is returned.
   */
  PCellVariant *get_variant (geo::Layout &layout, const pcell_parameters_type &parameters);

  /**
   *  @brief Register a variant 
   */
  void unregister_variant (PCellVariant *variant);

  /**
   *  @brief Unregister a variant 
   */
  void register_variant (PCellVariant *variant);

  /**
   *  @brief Iterates the variants (begin)
   */
  variant_iterator begin () const
  {
    return m_variant_map.begin ();
  }

  /**
   *  @brief Iterates the variants (end)
   */
  variant_iterator end () const
  {
    return m_variant_map.end ();
  }

  /**
   *  @brief Get the PCell Id for this variant
   */
  size_t pcell_id () const
  {
    return m_pcell_id;
  }

private:
  variant_map_t m_variant_map;
  geo::PCellDeclaration *mp_declaration;
  size_t m_pcell_id;
  std::string m_name;
};
  
}

#endif

