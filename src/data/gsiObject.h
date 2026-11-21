#pragma once

#include "gsiCommon.h"

#include <memory>
#include <string>

namespace gsi
{

/**
 *  Stand-in for KLayout's gsi::ObjectBase.
 *
 *  In KLayout this class manages script-side lifetime (keep/release and the
 *  client indexes). layout-io has no scripting layer, so only the interface that
 *  the geometry classes rely on is provided here.
 */
class GSI_PUBLIC ObjectBase
{
public:
  ObjectBase () { }
  virtual ~ObjectBase () { }

  //  Script side keeps a reference to this object - no-op without a script engine
  void keep () { }

  //  Script side releases its reference - no-op without a script engine
  void release () { }
};

}  // namespace gsi
