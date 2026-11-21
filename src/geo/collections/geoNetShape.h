#ifndef HDR_geoNetShape
#define HDR_geoNetShape

#include "geoPolygon.h"
#include "geoText.h"
#include "geoShapeRepository.h"
#include "geoBoxConvert.h"

namespace geo {

class Shapes;

/**
 *  @brief Provides a union of a PolygonRef and a TextRef
 *
 *  This object is used in the netlist extractor and represents either a polygon or a text.
 *  The TextRef shall utilize a StringRef to represent the string.
 */
class NetShape
{
public:
  enum shape_type { None, Text, Polygon };

  typedef geo::Point point_type;
  typedef geo::Box box_type;
  typedef geo::Coord coord_type;
  typedef geo::Disp trans_type;

  /**
   *  @brief Default constructor
   */
  NetShape ();

  /**
   *  @brief A NetShape object representing a PolygonRef
   */
  NetShape (const geo::PolygonRef &pr);

  /**
   *  @brief A NetShape object representing a Polygon from the given shape repository
   */
  NetShape (const geo::Polygon &poly, geo::GenericRepository &repo);

  /**
   *  @brief A NetShape object representing a TextRef
   */
  NetShape (const geo::TextRef &tr);

  /**
   *  @brief A NetShape object representing a Text from the given shape repository
   */
  NetShape (const geo::Text &text, geo::GenericRepository &repo);

  /**
   *  @brief Gets a code indicating the type of object stored herein
   */
  shape_type type () const;

  /**
   *  @brief Gets the PolygonRef object
   *  Asserts if the object stored is not a polygon.
   */
  geo::PolygonRef polygon_ref () const;

  /**
   *  @brief Gets the TextRef object
   *  Asserts if the object stored is not a text.
   */
  geo::TextRef text_ref () const;

  /**
   *  @brief In-place transformation
   */
  void transform (const geo::Disp &tr);

  /**
   *  @brief Equality
   */
  bool operator== (const NetShape &net_shape) const
  {
    return m_ptr == net_shape.m_ptr && m_dx == net_shape.m_dx && m_dy == net_shape.m_dy;
  }

  /**
   *  @brief Inequality
   */
  bool operator!= (const NetShape &net_shape) const
  {
    return ! operator== (net_shape);
  }

  /**
   *  @brief Less operator
   */
  bool operator< (const NetShape &net_shape) const
  {
    if (m_ptr != net_shape.m_ptr) {
      return m_ptr < net_shape.m_ptr;
    }
    if (m_dx != net_shape.m_dx) {
      return m_dx < net_shape.m_dx;
    }
    return m_dy < net_shape.m_dy;
  }

  /**
   *  @brief Gets the bounding box of the object
   */
  box_type bbox () const;

  /**
   *  @brief Inserts the object into a Shapes collection
   */
  void insert_into (geo::Shapes &shapes) const;

  /**
   *  @brief Inserts the object into a Shapes collection with the given properties ID
   */
  void insert_into (geo::Shapes &shapes, geo::properties_id_type pi) const;

  /**
   *  @brief Returns true if the object interacts with another NetShape object
   */
  bool interacts_with (const geo::NetShape &other) const;

  /**
   *  @brief Returns true if the object interacts with another NetShape object after transforming it
   */
  template <class Tr>
  bool interacts_with_transformed (const geo::NetShape &other, const Tr &trans) const;

public:
  size_t m_ptr;
  coord_type m_dx, m_dy;
};

/**
 *  @brief A box converter implementation for NetShape
 */
template <>
struct box_convert<geo::NetShape>
{
  typedef geo::NetShape::box_type box_type;
  typedef geo::NetShape::coord_type coord_type;
  typedef geo::complex_bbox_tag complexity;

  box_type operator() (const geo::NetShape &net_shape) const
  {
    return net_shape.bbox ();
  }
};

}

#endif

