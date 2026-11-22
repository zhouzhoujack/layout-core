#pragma once

namespace geo
{

/**
 *  @brief A "shape tag" class
 *
 *  This class is just a "tag" (a class without a member).
 *  It provides a typedef by which a shape type can be retrieved.
 *  Its only purpose is to pass a shape type without passing an
 *  actual shape object.
 */
template <class Obj>
struct object_tag
{
  using object_type = Obj;
};

}  // namespace geo

