#ifndef HDR_geoBoxConvert
#define HDR_geoBoxConvert

#include "geoText.h"
#include "geoPolygon.h"
#include "geoEdge.h"
#include "geoEdgePair.h"
#include "geoUserObject.h"
#include "geoBox.h"
#include "geoPath.h"
#include "geoArray.h"
#include "geoObjectWithProperties.h"

namespace geo
{

class Cell;
class CellInst;
class Layout;

/**
 *  @brief A tag class to indicate "simple" box computation
 *
 *  Using this tag as a typedef for box_convert<T>::complexity
 *  enables algorithms to simply use the box without caching it.
 */

struct DB_PUBLIC simple_bbox_tag { };

/**
 *  @brief A tag class to indicate "complex" box computation
 *
 *  Using this tag as a typedef for box_convert<T>::complexity
 *  enables algorithms to use cached bboxes, i.e. for the dbBoxTree.
 */

struct DB_PUBLIC complex_bbox_tag { };

/**
 *  @brief The generic box converter for the shapes
 *
 *  The box converter is supposed to convert a given
 *  shape (i.e. edge, polygon etc) into a box covering
 *  the shape as close as possible).
 *  This is just the template declaration. The specializations
 *  provide the correct implementation.
 *
 *  The AllowEmpty flag is ignored currently.
 */
template <class Sh, bool AllowEmpty>
struct box_convert 
{
};

/**
 *  @brief The shape-with-properties box converter
 *
 *  The AllowEmpty flag is ignored currently.
 */
template <class Sh, bool AllowEmpty>
struct box_convert< geo::object_with_properties<Sh>, AllowEmpty >
{
  typedef geo::box_convert<Sh> base_convert;

  typedef typename base_convert::complexity complexity;
  typedef typename base_convert::box_type box_type;

  box_type operator() (const geo::object_with_properties<Sh> &s) const
  {
    return bconvert (s);
  }

  base_convert bconvert;
};

/**
 *  @brief The text box converter
 *
 *  The text is just a point, so the box returned is
 *  degenerated, but usable.
 *
 *  The AllowEmpty flag is ignored currently.
 */
template <class C, bool AllowEmpty>
struct box_convert< geo::text<C>, AllowEmpty >
{
  typedef geo::text<C> text_type;
  typedef geo::box<C> box_type;

  typedef simple_bbox_tag complexity;

  box_type operator() (const text_type &t) const
  {
    return t.box ();
  }
};

/**
 *  @brief The text reference box converter
 *
 *  The text is just a point, so the box returned is
 *  degenerated, but usable.
 *
 *  The AllowEmpty flag is ignored currently.
 */
template <class Text, class Trans, bool AllowEmpty>
struct box_convert< geo::text_ref<Text, Trans>, AllowEmpty >
{
  typedef geo::text_ref<Text, Trans> text_ref_type;
  typedef typename Text::coord_type coord_type;
  typedef geo::box<coord_type> box_type;

  typedef simple_bbox_tag complexity;

  box_type operator() (const text_ref_type &t) const
  {
    return t.box ();
  }
};

/**
 *  @brief The text reference array box converter
 *
 *  The text is just a point, so the box returned is
 *  degenerated, but usable.
 *
 *  The AllowEmpty flag is ignored currently.
 */
template <class Text, class Trans, class ArrayTrans, bool AllowEmpty>
struct box_convert< geo::array< geo::text_ref<Text, Trans>, ArrayTrans>, AllowEmpty >
{
  typedef geo::text_ref<Text, Trans> text_ref_type;
  typedef geo::array<text_ref_type, ArrayTrans> text_ref_array_type;
  typedef typename Text::coord_type coord_type;
  typedef geo::box<coord_type> box_type;

  typedef simple_bbox_tag complexity;

  box_type operator() (const text_ref_array_type &t) const
  {
    box_convert< geo::text_ref<Text, Trans> > bc;
    return t.bbox (bc);
  }
};

/**
 *  @brief The path box converter
 *
 *  This maps the box to the bounding box of the polygon.
 *
 *  The AllowEmpty flag is ignored currently.
 */
template <class C, bool AllowEmpty>
struct box_convert < geo::path<C>, AllowEmpty >
{
  typedef geo::path <C> path_type;
  typedef geo::box <C> box_type;

  typedef complex_bbox_tag complexity;

  box_type operator() (const path_type &p) const
  {
    return p.box ();
  }
};

/**
 *  @brief The path reference box converter
 *
 *  This maps the box to the bounding box of the polygon.
 *
 *  The AllowEmpty flag is ignored currently.
 */
template <class Path, class Trans, bool AllowEmpty>
struct box_convert < geo::path_ref<Path, Trans>, AllowEmpty >
{
  typedef geo::path_ref <Path, Trans> path_ref_type;
  typedef typename Path::coord_type coord_type;
  typedef geo::box <coord_type> box_type;

  typedef complex_bbox_tag complexity;

  box_type operator() (const path_ref_type &p) const
  {
    return p.box ();
  }
};

/**
 *  @brief The path reference array box converter
 *
 *  This maps the box to the bounding box of the polygon.
 *
 *  The AllowEmpty flag is ignored currently.
 */
template <class Path, class Trans, class ArrayTrans, bool AllowEmpty>
struct box_convert < geo::array< geo::path_ref<Path, Trans>, ArrayTrans >, AllowEmpty >
{
  typedef geo::path_ref <Path, Trans> path_ref_type;
  typedef geo::array<path_ref_type, ArrayTrans> path_ref_array_type;
  typedef typename Path::coord_type coord_type;
  typedef geo::box <coord_type> box_type;

  typedef complex_bbox_tag complexity;

  box_type operator() (const path_ref_array_type &p) const
  {
    box_convert< geo::path_ref<Path, Trans> > bc;
    return p.bbox (bc);
  }
};

/**
 *  @brief The polygon box converter
 *
 *  This maps the box to the bounding box of the polygon.
 *
 *  The AllowEmpty flag is ignored currently.
 */
template <class C, bool AllowEmpty>
struct box_convert< geo::polygon<C>, AllowEmpty >
{
  typedef geo::polygon<C> polygon_type;
  typedef geo::box<C> box_type;

  typedef simple_bbox_tag complexity;

  const box_type &operator() (const polygon_type &p) const
  {
    return p.box ();
  }
};

/**
 *  @brief The simple polygon box converter
 *
 *  This maps the box to the bounding box of the polygon.
 *
 *  The AllowEmpty flag is ignored currently.
 */
template <class C, bool AllowEmpty>
struct box_convert< geo::simple_polygon<C>, AllowEmpty >
{
  typedef geo::simple_polygon<C> simple_polygon_type;
  typedef geo::box<C> box_type;

  typedef simple_bbox_tag complexity;

  const box_type &operator() (const simple_polygon_type &p) const
  {
    return p.box ();
  }
};

/**
 *  @brief The polygon reference box converter
 *
 *  This maps the box to the bounding box of the polygon.
 *
 *  The AllowEmpty flag is ignored currently.
 */
template <class Poly, class Trans, bool AllowEmpty>
struct box_convert< geo::polygon_ref<Poly, Trans>, AllowEmpty >
{
  typedef geo::polygon_ref<Poly, Trans> polygon_ref_type;
  typedef typename Poly::coord_type coord_type;
  typedef geo::box<coord_type> box_type;

  typedef simple_bbox_tag complexity;

  box_type operator() (const polygon_ref_type &p) const
  {
    return p.box ();
  }
};

/**
 *  @brief The polygon reference box converter
 *
 *  This maps the box to the bounding box of the polygon.
 *
 *  The AllowEmpty flag is ignored currently.
 */
template <class Poly, class Trans, class ArrayTrans, bool AllowEmpty>
struct box_convert< geo::array< geo::polygon_ref<Poly, Trans>, ArrayTrans >, AllowEmpty >
{
  typedef geo::polygon_ref<Poly, Trans> polygon_ref_type;
  typedef geo::array<polygon_ref_type, ArrayTrans> polygon_ref_array_type;
  typedef typename Poly::coord_type coord_type;
  typedef geo::box<coord_type> box_type;

  typedef simple_bbox_tag complexity;

  box_type operator() (const polygon_ref_array_type &p) const
  {
    box_convert< geo::polygon_ref<Poly, Trans> > bc;
    return p.bbox (bc);
  }
};

/**
 *  @brief The point box converter
 *
 *  This maps the box to a degenerated box with just one point.
 *
 *  The AllowEmpty flag is ignored currently.
 */
template <class C, bool AllowEmpty>
struct box_convert< geo::point<C>, AllowEmpty >
{
  typedef geo::point<C> point_type;
  typedef geo::box<C> box_type;

  typedef simple_bbox_tag complexity;

  box_type operator() (const point_type &p) const
  {
    return box_type (p, p);
  }
};

/**
 *  @brief The vector box converter
 *
 *  This maps the box to a degenerated box with just one point.
 *
 *  The AllowEmpty flag is ignored currently.
 */
template <class C, bool AllowEmpty>
struct box_convert< geo::vector<C>, AllowEmpty >
{
  typedef geo::vector<C> vector_type;
  typedef geo::point<C> point_type;
  typedef geo::box<C> box_type;

  typedef simple_bbox_tag complexity;

  box_type operator() (const vector_type &p) const
  {
    return box_type (point_type () + p, point_type () + p);
  }
};

/**
 *  @brief The edge pair box converter
 *
 *  This maps the box to the bounding box of the edge pair, which may
 *  be degenerate, i.e. for horizontal edges.
 *
 *  The AllowEmpty flag is ignored currently.
 */
template <class C, bool AllowEmpty>
struct box_convert< geo::edge_pair<C>, AllowEmpty >
{
  typedef geo::edge_pair<C> edge_pair_type;
  typedef geo::box<C> box_type;

  typedef simple_bbox_tag complexity;

  box_type operator() (const edge_pair_type &e) const
  {
    return e.bbox ();
  }
};

/**
 *  @brief The edge box converter
 *
 *  This maps the box to the bounding box of the edge, which may
 *  be degenerate, i.e. for horizontal edges.
 *
 *  The AllowEmpty flag is ignored currently.
 */
template <class C, bool AllowEmpty>
struct box_convert< geo::edge<C>, AllowEmpty >
{
  typedef geo::edge<C> edge_type;
  typedef geo::box<C> box_type;

  typedef simple_bbox_tag complexity;

  box_type operator() (const edge_type &e) const
  {
    return box_type (e.p1 (), e.p2 ());
  }
};

/**
 *  @brief The user object box converter
 *
 *  The AllowEmpty flag is ignored currently.
 */
template <class C, bool AllowEmpty>
struct box_convert< geo::user_object<C>, AllowEmpty >
{
  typedef geo::user_object<C> user_object_type;
  typedef geo::box<C> box_type;

  typedef complex_bbox_tag complexity;

  box_type operator() (const user_object_type &r) const
  {
    return r.box ();
  }
};

/**
 *  @brief The box-to-box converter
 *
 *  This is the trivial case where a box becomes a box.
 *
 *  The AllowEmpty flag is ignored currently.
 */
template <class C, bool AllowEmpty>
struct box_convert <geo::box <C>, AllowEmpty >
{
  typedef geo::box <C> box_type;

  typedef simple_bbox_tag complexity;

  const box_type &operator() (const box_type &b) const
  {
    return b;
  }
};

/**
 *  @brief The box array box converter
 *
 *  The AllowEmpty flag is ignored currently.
 */
template <class C, class ArrayTrans, bool AllowEmpty>
struct box_convert< geo::array< geo::box<C>, ArrayTrans >, AllowEmpty >
{
  typedef C coord_type;
  typedef geo::box<coord_type> box_type;
  typedef geo::array<box_type, ArrayTrans> box_array_type;

  typedef simple_bbox_tag complexity;

  box_type operator() (const box_array_type &a) const
  {
    box_convert<box_type> bc;
    return a.bbox (bc);
  }
};

/**
 *  @brief The box-to-box converter for the case of boxes with special internal representation
 *
 *  This is the trivial case where a box becomes a box.
 *
 *  The AllowEmpty flag is ignored currently.
 */
template <class C, class R, bool AllowEmpty>
struct box_convert <geo::box <C, R>, AllowEmpty >
{
  typedef geo::box <C> box_type;

  typedef simple_bbox_tag complexity;

  box_type operator() (const geo::box<C, R> &b) const
  {
    return box_type (b);
  }
};

/**
 *  @brief The box array box converter for boxes with special internal representation
 *
 *  The AllowEmpty flag is ignored currently.
 */
template <class C, class R, class ArrayTrans, bool AllowEmpty>
struct box_convert< geo::array< geo::box<C, R>, ArrayTrans >, AllowEmpty >
{
  typedef geo::box<C> box_type;
  typedef geo::array< geo::box<C, R>, ArrayTrans> box_array_type;

  typedef simple_bbox_tag complexity;

  box_type operator() (const box_array_type &a) const
  {
    box_convert< box<C, R> > bc;
    return a.bbox (bc);
  }
};

//  Note: the implementation is delegated, so we don't need to make box_convert dllexport.
DB_PUBLIC geo::Box cell_box_convert_impl (const geo::Cell &c, int layer, bool allow_empty);

/**
 *  @brief The cell box converter
 *
 *  This class is used as a function to obtain the bounding box of
 *  a cell, whether layer-wise or global.
 *
 *  If AllowEmpty is false, the box converter treats empty cells as single points at 0, 0.
 */

template <bool AllowEmpty>
struct DB_PUBLIC box_convert <geo::Cell, AllowEmpty>
{
  typedef geo::Cell cell_type;
  typedef geo::Box box_type;

  typedef complex_bbox_tag complexity;

  box_convert ()
    : m_layer (-1)
  { }
  
  box_convert (unsigned int l)
    : m_layer (l)
  { }
  
  box_type operator() (const cell_type &c) const
  {
    return cell_box_convert_impl (c, m_layer, AllowEmpty);
  }

private:
  int m_layer;
};

//  Note: the implementation is delegated, so we don't need to make box_convert dllexport.
DB_PUBLIC geo::Box cellinst_box_convert_impl (const geo::CellInst &inst, const geo::Layout *layout, int layer, bool allow_empty);

/**
 *  @brief The cell inst box converter
 *
 *  This class is used as a function to convert a cell instance
 *  to a box for a given layer. This requires that the per-layer 
 *  bounding boxes of the cell have been computed already.
 *
 *  If AllowEmpty is false, the box converter treats empty cells as single points at 0, 0.
 */

template <bool AllowEmpty>
struct box_convert <geo::CellInst, AllowEmpty>
{
  typedef geo::CellInst cell_inst_type;
  typedef geo::Layout layout_type;
  typedef geo::Box box_type;

  typedef complex_bbox_tag complexity;

  box_convert ()
    : mp_layout (0), m_layer (-1)
  { }
  
  box_convert (const layout_type &ly, unsigned int l)
    : mp_layout (&ly), m_layer (l)
  { }
  
  box_convert (const layout_type &ly)
    : mp_layout (&ly), m_layer (-1)
  { }
  
  box_type operator() (const cell_inst_type &t) const
  {
    return cellinst_box_convert_impl (t, mp_layout, m_layer, AllowEmpty);
  }

private:
  const layout_type *mp_layout;
  int m_layer;
};

/**
 *  @brief The cell inst array box converter
 *
 *  This class is used as a function to convert a cell instance array
 *  to a box for a given layer or for all layers. This requires that 
 *  the per-layer or overall bounding boxes of the cell have been 
 *  computed already.
 *
 *  If AllowEmpty is false, the box converter treats empty cells as single points at 0, 0.
 */

template <class ArrayTrans, bool AllowEmpty>
struct box_convert <geo::array <geo::CellInst, ArrayTrans>, AllowEmpty>
{
  typedef geo::CellInst cell_inst_type;
  typedef geo::Layout layout_type;
  typedef geo::Box box_type;

  typedef complex_bbox_tag complexity;

  box_convert ()
    : m_bc ()
  { }
  
  box_convert (const layout_type &g, unsigned int l)
    : m_bc (g, l)
  { }
  
  box_convert (const layout_type &g)
    : m_bc (g)
  { }
  
  box_type operator() (const geo::array <cell_inst_type, ArrayTrans> &t) const
  {
    return t.bbox (m_bc);
  }

private:
  box_convert <cell_inst_type, AllowEmpty> m_bc;
};

/**
 *  @brief The cell-inst-array-with-properties box converter
 *
 *  If AllowEmpty is false, the box converter treats empty cells as single points at 0, 0.
 */
template <class ArrayTrans, bool AllowEmpty>
struct box_convert< geo::object_with_properties< geo::array <geo::CellInst, ArrayTrans> >, AllowEmpty>
{
  typedef geo::array <geo::CellInst, ArrayTrans> cell_inst_array;
  typedef geo::box_convert<cell_inst_array, AllowEmpty> base_convert;
  typedef geo::Layout layout_type;

  typedef typename base_convert::complexity complexity;
  typedef typename base_convert::box_type box_type;

  box_convert ()
    : bconvert ()
  { }

  box_convert (const layout_type &g, unsigned int l)
    : bconvert (g, l)
  { }

  box_convert (const layout_type &g)
    : bconvert (g)
  { }

  box_type operator() (const geo::object_with_properties<cell_inst_array> &s) const
  {
    return bconvert (s);
  }

  base_convert bconvert;
};

}

#endif

