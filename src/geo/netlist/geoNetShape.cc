#include "geoNetShape.h"
#include "geoShapes.h"
#include "geoPolygonTools.h"

namespace geo {

// -----------------------------------------------------------------------------
//  NetShape implementation

NetShape::NetShape ()
  : m_ptr (0), m_dx (0), m_dy (0)
{ }

NetShape::NetShape (const geo::PolygonRef &pr)
{
  m_ptr = size_t (&pr.obj ()) + 1;
  m_dx = pr.trans ().disp ().x ();
  m_dy = pr.trans ().disp ().y ();
}

NetShape::NetShape (const geo::Polygon &poly, geo::GenericRepository &repo)
{
  geo::PolygonRef pr (poly, repo);
  m_ptr = size_t (&pr.obj ()) + 1;
  m_dx = pr.trans ().disp ().x ();
  m_dy = pr.trans ().disp ().y ();
}

NetShape::NetShape (const geo::TextRef &tr)
{
  m_ptr = size_t (&tr.obj ());
  m_dx = tr.trans ().disp ().x ();
  m_dy = tr.trans ().disp ().y ();
}

NetShape::NetShape (const geo::Text &text, geo::GenericRepository &repo)
{
  geo::TextRef tr (text, repo);
  m_ptr = size_t (&tr.obj ());
  m_dx = tr.trans ().disp ().x ();
  m_dy = tr.trans ().disp ().y ();
}

NetShape::shape_type NetShape::type () const
{
  if (m_ptr == 0) {
    return None;
  } else if ((m_ptr & 1) != 0) {
    return Polygon;
  } else {
    return Text;
  }
}

geo::PolygonRef NetShape::polygon_ref () const
{
  if ((size_t (m_ptr) & 1) != 0) {
    return geo::PolygonRef (reinterpret_cast<geo::Polygon *> (m_ptr - 1), geo::Disp (geo::Vector (m_dx, m_dy)));
  }
  tl_assert (false);
}

geo::TextRef NetShape::text_ref () const
{
  if ((size_t (m_ptr) & 1) == 0) {
    return geo::TextRef (reinterpret_cast<geo::Text *> (m_ptr), geo::Disp (geo::Vector (m_dx, m_dy)));
  }
  tl_assert (false);
}

void NetShape::transform (const geo::Disp &tr)
{
  m_dx += tr.disp ().x ();
  m_dy += tr.disp ().y ();
}

NetShape::box_type NetShape::bbox () const
{
  if ((m_ptr & 1) != 0) {
    return polygon_ref ().box ();
  } else if (m_ptr != 0) {
    return text_ref ().box ();
  } else {
    return box_type ();
  }
}

void NetShape::insert_into (geo::Shapes &shapes) const
{
  if ((m_ptr & 1) != 0) {
    shapes.insert (polygon_ref ());
  } else if (m_ptr != 0) {
    shapes.insert (text_ref ());
  }
}

void NetShape::insert_into (geo::Shapes &shapes, geo::properties_id_type pi) const
{
  if ((m_ptr & 1) != 0) {
    shapes.insert (geo::PolygonRefWithProperties (polygon_ref (), pi));
  } else if (m_ptr != 0) {
    shapes.insert (geo::TextRefWithProperties (text_ref (), pi));
  }
}

bool NetShape::interacts_with (const geo::NetShape &other) const
{
  if (m_ptr == 0 || other.m_ptr == 0 || ! bbox ().touches (other.bbox ())) {
    return false;
  }

  if ((m_ptr & 1) != 0) {

    if ((other.m_ptr & 1) != 0) {

      //  Polygon vs. polygon
      geo::PolygonRef pr_other = other.polygon_ref ();
      geo::PolygonRef pr = polygon_ref ();
      geo::Polygon p = pr_other.obj ().transformed (pr.trans ().inverted () * pr_other.trans ());
      return geo::interact_pp (pr.obj (), p);

    } else {

      //  NOTE: we assume that the text ref's target is at 0,0
      geo::PolygonRef pr = polygon_ref ();
      geo::Point pt = geo::Point (other.m_dx, other.m_dy) - pr.trans ().disp ();
      return geo::inside_poly (pr.obj ().begin_edge (), pt) >= 0;

    }

  } else {

    if ((other.m_ptr & 1) == 0) {

      //  Text vs. text
      return m_dx == other.m_dx && m_dy == other.m_dy;

    } else {

      //  NOTE: we assume that the text ref's target is at 0,0
      geo::PolygonRef pr_other = other.polygon_ref ();
      geo::Point pt = geo::Point (m_dx, m_dy) - pr_other.trans ().disp ();
      return geo::inside_poly (pr_other.obj ().begin_edge (), pt) >= 0;

    }

  }
}

template <class Tr>
bool NetShape::interacts_with_transformed (const geo::NetShape &other, const Tr &trans) const
{
  if (m_ptr == 0 || other.m_ptr == 0 || ! bbox ().touches (other.bbox ().transformed (trans))) {
    return false;
  }

  if ((m_ptr & 1) != 0) {

    if ((other.m_ptr & 1) != 0) {

      //  Polygon vs. polygon
      geo::PolygonRef pr_other = other.polygon_ref ();
      geo::PolygonRef pr = polygon_ref ();
      geo::Polygon p = pr_other.obj ().transformed (Tr (pr.trans ().inverted ()) * trans * Tr (pr_other.trans ()));
      return geo::interact_pp (pr.obj (), p);

    } else {

      //  NOTE: we assume that the text ref's target is at 0,0
      geo::PolygonRef pr = polygon_ref ();
      geo::Point pt = trans * geo::Point (other.m_dx, other.m_dy) - pr.trans ().disp ();
      return geo::inside_poly (pr.obj ().begin_edge (), pt) >= 0;

    }

  } else {

    if ((other.m_ptr & 1) == 0) {

      //  Text vs. text
      geo::Point pt = trans * geo::Point (other.m_dx, other.m_dy);
      return geo::Point (m_dx, m_dy) == pt;

    } else {

      //  NOTE: we assume that the text ref's target is at 0,0
      geo::PolygonRef pr_other = other.polygon_ref ();
      geo::Point pt = trans.inverted () * geo::Point (m_dx, m_dy) - pr_other.trans ().disp ();
      return geo::inside_poly (pr_other.obj ().begin_edge (), pt) >= 0;

    }

  }
}

//  explicit instantiations
template DB_PUBLIC bool NetShape::interacts_with_transformed<geo::ICplxTrans> (const geo::NetShape &other, const geo::ICplxTrans &trans) const;
template DB_PUBLIC bool NetShape::interacts_with_transformed<geo::Trans> (const geo::NetShape &other, const geo::Trans &trans) const;

}
