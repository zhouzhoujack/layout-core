#ifndef HDR_geoObjectWithProperties
#define HDR_geoObjectWithProperties

#include "utilsException.h"
#include "geoTypes.h"
#include "geoPolygon.h"
#include "geoPath.h"
#include "geoEdge.h"
#include "geoEdgePair.h"
#include "geoText.h"
#include "geoBox.h"
#include "geoArray.h"
#include "geoCellInst.h"
#include "geoPropertiesRepository.h"

namespace geo
{

class ArrayRepository;

/**
 *  @brief A object with properties template
 *
 *  This template provides some kind of "enhanced shape". A shape can be supplied
 *  with additional properties. For performance reasons, the properties are stored
 *  as an index within a lookup table. Each index refers to a set of properties.
 *  The "PropertiesRepository" class manages the properties available and associates
 *  a property set with an index. 
 *  The shape-with-properties template adds the properties repository index to a 
 *  shape and inherits all of the shape's methods.
 *  This template is not confined to be used with shapes. It can be used for
 *  instances as well.
 */

template <class Obj>
class object_with_properties
  : public Obj
{
public:
  typedef Obj object_type;

  typedef typename Obj::box_type box_type;
  typedef typename Obj::coord_type coord_type;
  typedef typename Obj::point_type point_type;

  typedef geo::object_tag< object_with_properties<Obj> > tag;

  /**
   *  @brief The default constructor
   */
  object_with_properties ()
    : Obj (), m_id (0)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Create myself from a object and an id
   */
  object_with_properties (const Obj &obj, properties_id_type id)
    : Obj (obj), m_id (id)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief The copy constructor
   */
  object_with_properties (const object_with_properties<Obj> &d)
    : Obj (d), m_id (d.m_id)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Assignment
   */
  object_with_properties &operator= (const object_with_properties<Obj> &d)
  {
    if (this != &d) {
      Obj::operator= (d);
      m_id = d.m_id;
    }
    return *this;
  }

  /**
   *  @brief Translation from a different repository space in a generic sense
   *
   *  This is required since the translation basically acts as an assignment operator
   *  whose sementics we have to provide here.
   */
  template <class Rep>
  void translate (const object_with_properties<Obj> &d, Rep &rep, geo::ArrayRepository &array_rep)
  {
    Obj::translate (d, rep, array_rep);
    m_id = d.m_id;
  }

  /**
   *  @brief Translation with transformation from a different repository space in a generic sense
   *
   *  This is required since the translation basically acts as an assignment operator
   *  whose sementics we have to provide here.
   */
  template <class Rep, class Trans>
  void translate (const object_with_properties<Obj> &d, const Trans &t, Rep &rep, geo::ArrayRepository &array_rep)
  {
    Obj::translate (d, t, rep, array_rep);
    m_id = d.m_id;
  }

  /**
   *  @brief Equality
   */
  bool operator== (const object_with_properties<Obj> &d) const
  {
    return Obj::operator== (d) && m_id == d.m_id;
  }

  /**
   *  @brief Inequality
   */
  bool operator!= (const object_with_properties<Obj> &d) const
  {
    return ! operator== (d);
  }

  /**
   *  @brief Comparison
   */
  bool operator< (const object_with_properties<Obj> &d) const
  {
    if (! Obj::operator== (d)) {
      return Obj::operator< (d);
    } 
    return m_id < d.m_id;
  }

  /**
   *  @brief Properties Id read accessor
   */
  properties_id_type properties_id () const
  {
    return m_id;
  }

  /**
   *  @brief Properties Id write accessor
   */
  void properties_id (properties_id_type id) 
  {
    m_id = id;
  }

private:
  properties_id_type m_id;
};

typedef object_with_properties<Polygon> PolygonWithProperties;
typedef object_with_properties<DPolygon> DPolygonWithProperties;
typedef object_with_properties<SimplePolygon> SimplePolygonWithProperties;
typedef object_with_properties<DSimplePolygon> DSimplePolygonWithProperties;
typedef object_with_properties<PolygonRef> PolygonRefWithProperties;
typedef object_with_properties<DPolygonRef> DPolygonRefWithProperties;
typedef object_with_properties<SimplePolygonRef> SimplePolygonRefWithProperties;
typedef object_with_properties<DSimplePolygonRef> DSimplePolygonRefWithProperties;

typedef object_with_properties<Path> PathWithProperties;
typedef object_with_properties<DPath> DPathWithProperties;
typedef object_with_properties<PathRef> PathRefWithProperties;
typedef object_with_properties<DPathRef> DPathRefWithProperties;

typedef object_with_properties<Edge> EdgeWithProperties;
typedef object_with_properties<DEdge> DEdgeWithProperties;

typedef object_with_properties<EdgePair> EdgePairWithProperties;
typedef object_with_properties<DEdgePair> DEdgePairWithProperties;

typedef object_with_properties<Text> TextWithProperties;
typedef object_with_properties<DText> DTextWithProperties;
typedef object_with_properties<TextRef> TextRefWithProperties;
typedef object_with_properties<DTextRef> DTextRefWithProperties;

typedef object_with_properties<Box> BoxWithProperties;
typedef object_with_properties<DBox> DBoxWithProperties;

typedef object_with_properties<geo::array<geo::CellInst, geo::Trans> > CellInstArrayWithProperties;
typedef object_with_properties<geo::array<geo::CellInst, geo::DTrans> > DCellInstArrayWithProperties;

} // namespace geo

#endif

