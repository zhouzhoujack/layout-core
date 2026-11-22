 
#include "utilsClassRegistry.h"

#include <cassert>
#include <map>

namespace geo
{
// -----------------------------------------------------------------------------
//  ClassRegistry implementation

struct ti_compare_f
{
  bool operator() (const std::type_info *a, const std::type_info *b) const
  {
    assert(a != nullptr);
    assert(b != nullptr);
    return a->before (*b);
  }
};

typedef std::map<const std::type_info *, RegistrarBase *, ti_compare_f> inst_map_type;

inst_map_type &inst_map()
{
    static inst_map_type map;
    return map;
}

TL_PUBLIC void set_registrar_instance_by_type (const std::type_info &ti, RegistrarBase *rb)
{
  auto &map = inst_map();

  if (rb) {
    map[&ti] = rb;
  } else {
    map.erase (&ti);
  }

}

TL_PUBLIC RegistrarBase *registrar_instance_by_type (const std::type_info &ti)
{
    inst_map_type::const_iterator im = inst_map().find (&ti);
    if (im != inst_map().end ()) {
        return im->second;
    } else {
        return 0;
    }
}

}
