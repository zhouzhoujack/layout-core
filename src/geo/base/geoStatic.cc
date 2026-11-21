#include "geoStatic.h"
#include "utilsException.h"

#include <cstdio>

namespace geo
{
// -----------------------------------------------------------------------------
//  Static implementation


// -----------------------------------------------------------
//  editable mode

DB_PUBLIC bool ms_editable = false;

void set_default_editable_mode (bool editable)
{
  ms_editable = editable;
}

void check_editable_mode (const char *f_str)
{
  if (! ms_editable) {
    throw geo::Exception (geo::to_string (tr ("Function '%s' is permitted only in editable mode")), f_str);
  }
}

void check_editable_mode ()
{
  if (! ms_editable) {
    throw geo::Exception (geo::to_string (tr ("Operation is permitted only in editable mode")));
  }
}

// -----------------------------------------------------------
//  number of points per circle

DB_PUBLIC unsigned int ms_num_circle_points = 32;

void set_num_circle_points (unsigned int n)
{
  ms_num_circle_points = n;
}

// -----------------------------------------------------------
//  undo enable 

DB_PUBLIC bool ms_transactions_enabled = true;

void enable_transactions (bool enable)
{
  ms_transactions_enabled = enable;
}

}

