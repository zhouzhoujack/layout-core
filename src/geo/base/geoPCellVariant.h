#ifndef HDR_geoPCellVariant
#define HDR_geoPCellVariant

#include "geoCommon.h"

#include "geoLayout.h"
#include "utilsVariant.h"
#include "geoPCellHeader.h"

#include <map>
#include <vector>
#include <string>

namespace geo
{

/**
 *  @brief A PCell variant
 */
class DB_PUBLIC PCellVariant 
  : public geo::Cell
{
public:
  /** 
   *  @brief The constructor
   *
   *  The constructor gets the parameters that are unique for this variant.
   */
  PCellVariant (geo::cell_index_type ci, geo::Layout &layout, geo::pcell_id_type pcell_id, const pcell_parameters_type &parameters);

  /**
   *  @brief The destructor
   */
  ~PCellVariant ();

  /**
   *  @brief Cloning 
   */
  virtual Cell *clone (Layout &layout) const;

  /**
   *  @brief Get the parameter name map for this variant
   */
  std::map<std::string, geo::Variant> parameters_by_name () const;

  /**
   *  @brief Gets the parameter by name for this variant
   *  Returns a nil variant if there is no parameter with that name
   */
  geo::Variant parameter_by_name (const std::string &name) const;

  /**
   *  @brief Gets the parameters for this variant
   */
  const std::vector<geo::Variant> &parameters () const
  {
    return m_parameters;
  }

  /**
   *  @brief Get the PCell Id for this variant
   */
  geo::pcell_id_type pcell_id () const
  {
    return m_pcell_id;
  }

  /**
   *  @brief Get the basic name
   *
   *  The basic name of the cell is either the cell name or the cell name in the
   *  target library (for library proxies) or the PCell name (for PCell proxies).
   *  The actual name may be different by a extension to make it unique.
   */
  virtual std::string get_basic_name () const;

  /**
   *  @brief Get the display name
   *
   *  The display name is some "nice" descriptive name of the cell (variant)
   *  For normal cells this name is equivalent to the normal cell name.
   */
  virtual std::string get_display_name () const;

  /**
   *  @brief Unregister a cell from it's context.
   */
  virtual void unregister ();

  /**
   *  @brief Reregister a cell inside it's context.
   */
  virtual void reregister ();

  /**
   *  @brief Update the layout
   */
  virtual void update (ImportLayerMapping *layer_mapping = 0);

  /**
   *  @brief Tell, if this cell is a proxy cell
   *
   *  Proxy cells are such whose layout represents a snapshot of another entity.
   *  Such cells can be PCell variants or library references for example.
   */
  virtual bool is_proxy () const 
  { 
    return true; 
  }

protected:
  /**
   *  @brief Get the PCell header for this variant
   */
  PCellHeader *pcell_header () 
  {
    return layout ()->pcell_header (m_pcell_id);
  }

  /**
   *  @brief Get the PCell header for this variant
   */
  const PCellHeader *pcell_header () const
  {
    return layout ()->pcell_header (m_pcell_id);
  }

private:
  pcell_parameters_type m_parameters;
  mutable std::string m_display_name;
  geo::pcell_id_type m_pcell_id;
  bool m_registered;
};
  
}

#endif

