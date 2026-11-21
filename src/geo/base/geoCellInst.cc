#include "geoCellInst.h"
#include "geoLayout.h"

namespace geo
{

// -----------------------------------------------------------------------------
//  CellInst implementation

CellInst::box_type 
CellInst::bbox (const geo::Layout &g) const
{
  return g.cell (m_cell_index).bbox ();
}

CellInst::box_type
CellInst::bbox (const geo::Layout &g, unsigned int l) const
{
  return g.cell (m_cell_index).bbox (l);
}

std::string 
CellInst::to_string () const
{
  return "[" + geo::to_string (m_cell_index) + "]";
}

}

