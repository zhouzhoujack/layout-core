#ifndef HDR_geoObjectTag
#define HDR_geoObjectTag

namespace geo
{

/**
 *  @brief A "shape tag" class
 * 
 *  This class is just a "tag" (a class without a member).
 *  It just provides a typedef by which a shape type
 *  can be retrieved. It's only purpose is to pass a 
 *  shape type without passing an actual shape object.
 */

template <class Obj>
struct object_tag
{
  typedef Obj object_type;
};

}

#endif

