#ifndef HDR_geoInstElement
#define HDR_geoInstElement


#include "geoLayout.h"

namespace geo {

/**
 *  @brief A struct that describes a level of instantiation in the selection, hence a specific instance of an array (if the instance is one) 
 */
struct InstElement
{
  geo::Instance inst_ptr;
  geo::CellInstArray::iterator array_inst;

  /**
   *  @brief Default constructor
   */
  InstElement ()
    : inst_ptr (), array_inst ()
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Constructor providing an instance 
   *
   *  The iterator will be set to the first element (the only one if it is a single instance)
   */
  InstElement (const geo::Instance &ip)
    : inst_ptr (ip), array_inst (ip.begin ())
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Constructor providing an instance and an iterator within this instance (which is likely to be an array)
   */
  InstElement (const geo::Instance &ip, const geo::CellInstArray::iterator &ai)
    : inst_ptr (ip), array_inst (ai)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Copy ctor
   */
  InstElement (const InstElement &d)
    : inst_ptr (d.inst_ptr),
      array_inst (d.array_inst)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Compute the bounding box of the instance path element 
   *
   *  If the instance path element is a whole array, the bounding box is computed for the array, 
   *  otherwise for the instance.
   *
   *  @param bc The bounding box converter for the cell instance (geo::box_convert<geo::CellInst>)
   */
  geo::Box bbox (const geo::box_convert<geo::CellInst> &bc) const
  {
    if (whole_array ()) {
      //  this is the whole array
      return inst_ptr.cell_inst ().bbox (bc);
    } else {
      //  this is a single instance
      return geo::Box (inst_ptr.cell_inst ().complex_trans (*array_inst) * bc (inst_ptr.cell_inst ().object ()));
    }
  }

  /**
   *  @brief Tell, if this instance describes the whole array or just one instance of it
   */
  bool whole_array () const
  {
    return array_inst.at_end ();
  }

  /**
   *  @brief Assignment
   */
  InstElement &operator= (const InstElement &d) 
  {
    if (&d != this) {
      inst_ptr = d.inst_ptr;
      array_inst = d.array_inst;
    }
    return *this;
  }

  /**
   *  @brief "less" operator to establish an order for sets etc.
   */
  bool operator< (const InstElement &d) const
  {
    if (inst_ptr != d.inst_ptr) { 
      return inst_ptr < d.inst_ptr; 
    }
    return *array_inst < *d.array_inst;
  }

  /**
   *  @brief Equality
   */
  bool operator== (const InstElement &d) const
  {
    return inst_ptr == d.inst_ptr && *array_inst == *d.array_inst;
  }

  /**
   *  @brief Inequality
   */
  bool operator!= (const InstElement &d) const
  {
    return !operator== (d);
  }

  /**
   *  @brief Return the complex transformation induced by this instance path element
   */
  geo::ICplxTrans complex_trans () const
  {
    return inst_ptr.cell_inst ().complex_trans (*array_inst);
  }
};

} // namespace geo

#endif

