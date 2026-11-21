#ifndef HDR_geoShapeFlags
#define HDR_geoShapeFlags

#include "geoShapes.h"

namespace geo
{

template <class T> unsigned int shape_flags ();
template <class T> unsigned int shape_flags_pure ();

template <> inline unsigned int shape_flags<geo::PolygonRef> ()      { return 1 << geo::ShapeIterator::PolygonRef; }
template <> inline unsigned int shape_flags_pure<geo::PolygonRef> () { return 1 << geo::ShapeIterator::PolygonRef; }

template <> inline unsigned int shape_flags<geo::TextRef> ()         { return 1 << geo::ShapeIterator::TextRef; }
template <> inline unsigned int shape_flags_pure<geo::TextRef> ()    { return 1 << geo::ShapeIterator::TextRef; }

template <> inline unsigned int shape_flags<geo::Box> ()             { return geo::ShapeIterator::Boxes; }
template <> inline unsigned int shape_flags_pure<geo::Box> ()        { return 1 << geo::ShapeIterator::Box; }

template <> inline unsigned int shape_flags<geo::Path> ()            { return geo::ShapeIterator::Paths; }
template <> inline unsigned int shape_flags_pure<geo::Path> ()       { return 1 << geo::ShapeIterator::Path; }

template <> inline unsigned int shape_flags<geo::Polygon> ()         { return geo::ShapeIterator::Polygons; }
template <> inline unsigned int shape_flags_pure<geo::Polygon> ()    { return 1 << geo::ShapeIterator::Polygon; }

template <> inline unsigned int shape_flags<geo::Edge> ()            { return geo::ShapeIterator::Edges; }
template <> inline unsigned int shape_flags_pure<geo::Edge> ()       { return 1 << geo::ShapeIterator::Edge; }

template <> inline unsigned int shape_flags<geo::EdgePair> ()        { return geo::ShapeIterator::EdgePairs; }
template <> inline unsigned int shape_flags_pure<geo::EdgePair> ()   { return 1 << geo::ShapeIterator::EdgePair; }

template <> inline unsigned int shape_flags<geo::Text> ()            { return geo::ShapeIterator::Texts; }
template <> inline unsigned int shape_flags_pure<geo::Text> ()       { return 1 << geo::ShapeIterator::Text; }


template <class T>
struct shape_to_object
{
  void set (const geo::Shape &) { }
  const T *get (const geo::Shape &s) const { return s.basic_ptr (typename T::tag ()); }
};


template <>
struct shape_to_object<geo::Polygon>
{
  typedef geo::Polygon value_type;

  void set (const geo::Shape &s) { s.polygon (m_shape); }
  const value_type *get (const geo::Shape &) const { return &m_shape; }

private:
  value_type m_shape;
};

template <>
struct shape_to_object<geo::SimplePolygon>
{
  typedef geo::SimplePolygon value_type;

  void set (const geo::Shape &s) { s.simple_polygon (m_shape); }
  const value_type *get (const geo::Shape &) const { return &m_shape; }

private:
  value_type m_shape;
};

template <>
struct shape_to_object<geo::Path>
{
  typedef geo::Path value_type;

  void set (const geo::Shape &s) { s.path (m_shape); }
  const value_type *get (const geo::Shape &) const { return &m_shape; }

private:
  value_type m_shape;
};

template <>
struct shape_to_object<geo::Text>
{
  typedef geo::Text value_type;

  void set (const geo::Shape &s) { s.text (m_shape); }
  const value_type *get (const geo::Shape &) const { return &m_shape; }

private:
  value_type m_shape;
};

template <>
struct shape_to_object<geo::Box>
{
  typedef geo::Box value_type;

  void set (const geo::Shape *s) { s->box (m_shape); }
  const value_type *get (const geo::Shape *) const { return &m_shape; }

private:
  value_type m_shape;
};

}

#endif
