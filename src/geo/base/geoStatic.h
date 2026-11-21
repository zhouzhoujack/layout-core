#ifndef HDR_geoStatic
#define HDR_geoStatic

#include "geoCommon.h"

namespace geo
{

// -----------------------------------------------------------
//  editable mode

/**
 *  @brief Global database attribute: editable mode
 *
 *  This attribute reflects editable mode. In editable mode, some restrictions apply, i.e.
 *  only shapes with attributes may be created.
 */
inline bool default_editable_mode ()
{
  extern DB_PUBLIC bool ms_editable;
  return ms_editable;
}

/**
 *  @brief Set editable mode
 *
 *  Hint: this should only be done initially, not at runtime.
 */
void DB_PUBLIC set_default_editable_mode (bool editable);

// -----------------------------------------------------------
//  circle resolution

/**
 *  @brief Returns the number of points per full circle 
 *
 *  This value is used as default in some places.
 */
inline unsigned int num_circle_points () 
{
  extern DB_PUBLIC unsigned int ms_num_circle_points;
  return ms_num_circle_points < 4 ? 4 : ms_num_circle_points;
}

/**
 *  @brief Sets the number of points per full circle.
 */
void DB_PUBLIC set_num_circle_points (unsigned int n);

}

#endif

