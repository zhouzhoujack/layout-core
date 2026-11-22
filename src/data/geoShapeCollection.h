#ifndef HDR_geoShapeCollection
#define HDR_geoShapeCollection

#include "geoDeepShapeStore.h"
#include "utilsUniqueId.h"

namespace geo
{

/**
 *  @brief A base class for the deep collection delegates
 */
class DeepShapeCollectionDelegateBase
{
public:
  DeepShapeCollectionDelegateBase () { }

  DeepShapeCollectionDelegateBase (const DeepShapeCollectionDelegateBase &other)
  {
    m_deep_layer = other.m_deep_layer.copy ();
  }

  DeepShapeCollectionDelegateBase &operator= (const DeepShapeCollectionDelegateBase &other)
  {
    if (this != &other) {
      m_deep_layer = other.m_deep_layer.copy ();
    }
    return *this;
  }

  const geo::DeepLayer &deep_layer () const
  {
    return m_deep_layer;
  }

  geo::DeepLayer &deep_layer ()
  {
    return m_deep_layer;
  }

protected:
  virtual void set_deep_layer (const geo::DeepLayer &dl)
  {
    m_deep_layer = dl;
  }

private:
  geo::DeepLayer m_deep_layer;
};

/**
 *  @brief A base class for the shape collection delegates
 */
class ShapeCollectionDelegateBase
  : public geo::UniqueId
{
public:
  ShapeCollectionDelegateBase () { }
  virtual ~ShapeCollectionDelegateBase () { }

  virtual DeepShapeCollectionDelegateBase *deep () { return 0; }
};

/**
 *  @brief A base class for the shape collections such as Region, Edges, EdgePairs etc.
 */
class ShapeCollection
{
public:
  ShapeCollection () { }
  virtual ~ShapeCollection () { }

  virtual ShapeCollectionDelegateBase *get_delegate () const = 0;
};

}

namespace geo
{

template<> struct type_traits<geo::ShapeCollection> : public geo::type_traits<void>
{
  //  mark "NetlistDeviceExtractor" as having a default ctor and no copy ctor
  typedef geo::false_tag has_copy_constructor;
  typedef geo::false_tag has_default_constructor;
};

}

#endif
