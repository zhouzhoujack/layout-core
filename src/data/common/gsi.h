#pragma once

// ---------------------------------------------------------------------------
// Minimal stand-in for KLayout's "gsi" (generic scripting interface).
//
// layout-core 不导出对象到脚本语言，几何类只需要一个空的基类来保持定义不变；
// 真正的 gsi 层依赖 Qt，不属于 layout-core。
//
// 本文件合并了原先的 gsiCommon.h / gsiObject.h / gsiDecl.h / gsiClass.h，
// 以便所有 gsi 相关声明集中在一处。
// ---------------------------------------------------------------------------

#include "utilsVariant.h"

#include <memory>
#include <string>

namespace gsi
{

/**
 *  Stand-in for KLayout's gsi::ObjectBase.
 *
 *  In KLayout this class manages script-side lifetime (keep/release and the
 *  client indexes). layout-core has no scripting layer, so only the interface
 *  that the geometry classes rely on is provided here.
 */
class ObjectBase
{
public:
  ObjectBase () { }
  virtual ~ObjectBase () { }

  //  Script side keeps a reference to this object - no-op without a script engine
  void keep () { }

  //  Script side releases its reference - no-op without a script engine
  void release () { }
};

class ClassBase
{
public:
  ClassBase () { }
  virtual ~ClassBase () { }

  //  Descriptor for values of this type when stored inside a geo::Variant
  virtual const geo::VariantUserClassBase *var_cls (bool is_const) const = 0;
};

/**
 *  Minimal concrete descriptor for geo::Variant values holding a T.
 *
 *  KLayout's full implementation provides type conversion, string
 *  serialisation and script binding. None of that is needed by layout-core,
 *  whose public API is C++ only - the descriptor is required solely because
 *  geo::Variant::make_variant() stores a class pointer alongside the object.
 */
template <class T>
class BasicVariantClass : public geo::VariantUserClassBase
{
public:
  void *create () const { return 0; }
  void destroy (void *p) const { delete reinterpret_cast<T *> (p); }
  bool equal (const void *a, const void *b) const { return a == b; }
  bool less (const void *a, const void *b) const { return a < b; }
  void *clone (const void * /*p*/) const { return 0; }
  std::string to_string (const void * /*p*/) const { return std::string (); }
  void read (void * /*p*/, geo::Extractor & /*ex*/) const { }
  const char *name () const { return typeid (T).name (); }
  bool is_const () const { return false; }
  void assign (void * /*self*/, const void * /*other*/) const { }
  const gsi::ClassBase *gsi_cls () const { return 0; }
  const geo::EvalClass *eval_cls () const { return 0; }
  void *deref_proxy (geo::RefCounted *proxy) const { return proxy; }
};

template <class T, class A = void>
class Class : public ClassBase
{
public:
  const geo::VariantUserClassBase *var_cls (bool /*is_const*/) const
  {
    static BasicVariantClass<T> s_class;
    return &s_class;
  }
};

template <class X>
const ClassBase *cls_decl ()
{
  static Class<X> s_class;
  return &s_class;
}

}  // namespace gsi
