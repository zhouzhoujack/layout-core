#pragma once

// ---------------------------------------------------------------------------
// Stand-in for gsiDecl.h.
//
// KLayout's real gsiDecl.h pulls in the Qt based GSI type system. layout-io only
// needs the part that "geo::Variant::make_variant" uses: a per-type class
// descriptor providing "var_cls()". This header supplies that minimal piece,
// so the geometry code can stay unchanged.
// ---------------------------------------------------------------------------

#include "gsiObject.h"

#include "utilsVariant.h"

namespace gsi
{

class GSI_PUBLIC ClassBase
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
 *  serialisation and script binding. None of that is needed by layout-io, whose
 *  public API is C++ only - the descriptor is required solely because
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
