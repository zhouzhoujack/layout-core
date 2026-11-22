#include "geoRegion.h"
#include "geoRegionUtils.h"
#include "geoOriginalLayerRegion.h"
#include "geoEmptyRegion.h"
#include "geoFlatRegion.h"
#include "geoDeepRegion.h"
#include "geoDeepEdges.h"
#include "geoFlatEdges.h"
#include "geoPolygonTools.h"
#include "geoCompoundOperation.h"
#include "utilsGlobPattern.h"

namespace geo
{

// -------------------------------------------------------------------------------------------------------------
//  Region implementation

Region::Region ()
  : mp_delegate (new EmptyRegion ())
{
  //  .. nothing yet ..
}

Region::Region (RegionDelegate *delegate)
  : mp_delegate (delegate)
{
  //  .. nothing yet ..
}

Region::Region (const Region &other)
  : geo::ShapeCollection (), mp_delegate (other.mp_delegate->clone ())
{
  //  .. nothing yet ..
}

Region::~Region ()
{
  delete mp_delegate;
  mp_delegate = 0;
}

Region &Region::operator= (const Region &other)
{
  if (this != &other) {
    set_delegate (other.mp_delegate->clone (), false);
  }
  return *this;
}

Region::Region (const RecursiveShapeIterator &si)
{
  mp_delegate = new OriginalLayerRegion (si);
}

Region::Region (const RecursiveShapeIterator &si, const geo::ICplxTrans &trans, bool merged_semantics)
{
  mp_delegate = new OriginalLayerRegion (si, trans, merged_semantics);
}

Region::Region (const RecursiveShapeIterator &si, DeepShapeStore &dss, double area_ratio, size_t max_vertex_count)
{
  mp_delegate = new DeepRegion (si, dss, area_ratio, max_vertex_count);
}

Region::Region (const RecursiveShapeIterator &si, DeepShapeStore &dss, const geo::ICplxTrans &trans, bool merged_semantics, double area_ratio, size_t max_vertex_count)
{
  mp_delegate = new DeepRegion (si, dss, trans, merged_semantics, area_ratio, max_vertex_count);
}

Region::Region (DeepShapeStore &dss)
{
  tl_assert (dss.is_singular ());
  unsigned int layout_index = 0; // singular layout index
  mp_delegate = new geo::DeepRegion (geo::DeepLayer (&dss, layout_index, dss.layout (layout_index).insert_layer ()));
}

const geo::RecursiveShapeIterator &
Region::iter () const
{
  static geo::RecursiveShapeIterator def_iter;
  const geo::RecursiveShapeIterator *i = mp_delegate->iter ();
  return *(i ? i : &def_iter);
}

void
Region::set_delegate (RegionDelegate *delegate, bool keep_attributes)
{
  if (delegate != mp_delegate) {
    if (keep_attributes && delegate && mp_delegate) {
      //  copy the basic attributes like #threads etc.
      delegate->RegionDelegate::operator= (*mp_delegate);
    }
    delete mp_delegate;
    mp_delegate = delegate;
  }
}

void
Region::clear ()
{
  set_delegate (new EmptyRegion ());
}

void
Region::reserve (size_t n)
{
  mutable_region ()->reserve (n);
}

template <class T>
Region &Region::transform (const T &trans)
{
  mutable_region ()->transform (trans);
  return *this;
}

//  explicit instantiations
template Region &Region::transform (const geo::ICplxTrans &);
template Region &Region::transform (const geo::Trans &);
template Region &Region::transform (const geo::Disp &);
template Region &Region::transform (const geo::IMatrix2d &);
template Region &Region::transform (const geo::IMatrix3d &);

template <class Sh>
void Region::insert (const Sh &shape)
{
  mutable_region ()->insert (shape);
}

template void Region::insert (const geo::Box &);
template void Region::insert (const geo::SimplePolygon &);
template void Region::insert (const geo::Polygon &);
template void Region::insert (const geo::Path &);

void Region::insert (const geo::Shape &shape)
{
  mutable_region ()->insert (shape);
}

template <class T>
void Region::insert (const geo::Shape &shape, const T &trans)
{
  mutable_region ()->insert (shape, trans);
}

template void Region::insert (const geo::Shape &, const geo::ICplxTrans &);
template void Region::insert (const geo::Shape &, const geo::Trans &);
template void Region::insert (const geo::Shape &, const geo::Disp &);

MutableRegion *
Region::mutable_region ()
{
  MutableRegion *region = dynamic_cast<MutableRegion *> (mp_delegate);
  if (! region) {

    FlatRegion *flat_region = new FlatRegion ();
    region = flat_region;

    if (mp_delegate) {
      flat_region->RegionDelegate::operator= (*mp_delegate);   //  copy basic flags
      flat_region->insert_seq (begin ());
      flat_region->set_is_merged (mp_delegate->is_merged ());
    }

    set_delegate (flat_region);

  }

  return region;
}

EdgePairs
Region::cop_to_edge_pairs (geo::CompoundRegionOperationNode &node)
{
  tl_assert (node.result_type () == geo::CompoundRegionOperationNode::EdgePairs);
  return EdgePairs (mp_delegate->cop_to_edge_pairs (node));
}

Region
Region::cop_to_region (geo::CompoundRegionOperationNode &node)
{
  tl_assert (node.result_type () == geo::CompoundRegionOperationNode::Region);
  return Region (mp_delegate->cop_to_region (node));
}

Edges
Region::cop_to_edges (geo::CompoundRegionOperationNode &node)
{
  tl_assert (node.result_type () == geo::CompoundRegionOperationNode::Edges);
  return Edges (mp_delegate->cop_to_edges (node));
}

geo::Variant
Region::cop (geo::CompoundRegionOperationNode &node)
{
  if (node.result_type () == geo::CompoundRegionOperationNode::EdgePairs) {
    return geo::Variant::make_variant (new EdgePairs (mp_delegate->cop_to_edge_pairs (node)));
  } else if (node.result_type () == geo::CompoundRegionOperationNode::Edges) {
    return geo::Variant::make_variant (new Edges (mp_delegate->cop_to_edges (node)));
  } else if (node.result_type () == geo::CompoundRegionOperationNode::Region) {
    return geo::Variant::make_variant (new Region (mp_delegate->cop_to_region (node)));
  } else {
    return geo::Variant ();
  }
}

Region &
Region::size (coord_type d, unsigned int mode)
{
  set_delegate (mp_delegate->sized (d, mode));
  return *this;
}

Region &
Region::size (coord_type dx, coord_type dy, unsigned int mode)
{
  set_delegate (mp_delegate->sized (dx, dy, mode));
  return *this;
}

Region
Region::sized (coord_type d, unsigned int mode) const
{
  return Region (mp_delegate->sized (d, mode));
}

Region
Region::sized (coord_type dx, coord_type dy, unsigned int mode) const
{
  return Region (mp_delegate->sized (dx, dy, mode));
}

void
Region::round_corners (double rinner, double router, unsigned int n)
{
  process (RoundedCornersProcessor (rinner, router, n));
}

Region
Region::rounded_corners (double rinner, double router, unsigned int n) const
{
  return processed (RoundedCornersProcessor (rinner, router, n));
}

void
Region::smooth (coord_type d, bool keep_hv)
{
  process (SmoothingProcessor (d, keep_hv));
}

Region
Region::smoothed (coord_type d, bool keep_hv) const
{
  return processed (SmoothingProcessor (d, keep_hv));
}

geo::Region &
Region::flatten ()
{
  mutable_region ()->flatten ();
  return *this;
}

void
Region::snap (geo::Coord gx, geo::Coord gy)
{
  set_delegate (mp_delegate->snapped_in_place (gx, gy));
}

Region
Region::snapped (geo::Coord gx, geo::Coord gy) const
{
  return Region (mp_delegate->snapped (gx, gy));
}

void
Region::scale_and_snap (geo::Coord gx, geo::Coord mx, geo::Coord dx, geo::Coord gy, geo::Coord my, geo::Coord dy)
{
  set_delegate (mp_delegate->scaled_and_snapped_in_place (gx, mx, dx, gy, my, dy));
}

Region
Region::scaled_and_snapped (geo::Coord gx, geo::Coord mx, geo::Coord dx, geo::Coord gy, geo::Coord my, geo::Coord dy) const
{
  return Region (mp_delegate->scaled_and_snapped (gx, mx, dx, gy, my, dy));
}

Region
Region::strange_polygon_check () const
{
  return Region (processed (StrangePolygonCheckProcessor ()));
}

Region
Region::holes () const
{
  return Region (processed (HolesExtractionProcessor ()));
}

Region
Region::hulls () const
{
  return Region (processed (HullExtractionProcessor ()));
}

namespace
{

template <class Container>
struct dot_delivery
{
  typedef Container container_type;

  dot_delivery ()
  {
    //  .. nothing yet ..
  }

  void insert (const geo::Point &pt, Container *container) const
  {
    container->insert (geo::Edge (pt, pt));
  }
};

template <class Container>
struct box_delivery
{
  typedef Container container_type;

  box_delivery (geo::Coord enl)
    : m_d (enl, enl)
  {
    //  .. nothing yet ..
  }

  void insert (const geo::Point &pt, Container *container) const
  {
    container->insert (geo::Box (pt - m_d, pt + m_d));
  }

private:
  geo::Vector m_d;
};

template <class Iter, class Delivery>
static void fill_texts (const Iter &iter, const std::string &pat, bool pattern, const Delivery &delivery, typename Delivery::container_type *container, const geo::ICplxTrans &trans, const geo::DeepRegion *org_deep)
{
  std::pair<bool, geo::property_names_id_type> text_annot_name_id;
  const geo::Layout *layout = 0;

  if (org_deep) {
    layout = &org_deep->deep_layer ().layout ();
    const geo::DeepShapeStore *store = org_deep->deep_layer ().store ();
    if (! store->text_property_name ().is_nil ()) {
      text_annot_name_id = layout->properties_repository ().get_id_of_name (store->text_property_name ());
    }
  }

  geo::GlobPattern glob_pat;
  bool all = false;
  if (pattern) {
    if (pat == "*") {
      all = true;
    } else {
      glob_pat = geo::GlobPattern (pat);
    }
  }

  for (Iter si = iter; ! si.at_end (); ++si) {

    bool is_text = false;
    std::string text_string;

    if (si->is_text ()) {

      //  a raw text
      is_text = true;
      text_string = si->text_string ();

    } else if (layout && text_annot_name_id.first && si->prop_id () > 0) {

      //  a text marker
      const geo::PropertiesRepository::properties_set &ps = layout->properties_repository ().properties (si->prop_id ());

      for (geo::PropertiesRepository::properties_set::const_iterator j = ps.begin (); j != ps.end () && ! is_text; ++j) {
        if (j->first == text_annot_name_id.second) {
          text_string = j->second.to_string ();
          is_text = true;
        }
      }

    }

    if (is_text &&
        (all || (pattern && glob_pat.match (text_string)) || (!pattern && text_string == pat))) {
      delivery.insert (si.trans () * (trans * si->bbox ().center ()), container);
    }

  }
}

template <class Delivery>
class text_shape_receiver
  : public geo::HierarchyBuilderShapeReceiver
{
public:
  text_shape_receiver (const Delivery &delivery, const std::string &pat, bool pattern, const geo::DeepRegion *org_deep)
    : m_delivery (delivery), m_glob_pat (), m_all (false), m_pattern (pattern), m_pat (pat), m_text_annot_name_id (false, 0), mp_layout (0)
  {
    if (org_deep) {
      mp_layout = & org_deep->deep_layer ().layout ();
      const geo::DeepShapeStore *store = org_deep->deep_layer ().store ();
      if (! store->text_property_name ().is_nil ()) {
        m_text_annot_name_id = mp_layout->properties_repository ().get_id_of_name (store->text_property_name ());
      }
    }

    if (pattern) {
      if (m_pat == "*") {
        m_all = true;
      } else {
        m_glob_pat = geo::GlobPattern (pat);
      }
    }
  }

  virtual void push (const geo::Shape &shape, const geo::ICplxTrans &trans, const geo::Box &region, const geo::RecursiveShapeReceiver::box_tree_type *complex_region, geo::Shapes *target)
  {
    bool is_text = false;
    std::string text_string;

    if (shape.is_text ()) {

      //  a raw text
      is_text = true;
      text_string = shape.text_string ();

    } else if (mp_layout && m_text_annot_name_id.first && shape.prop_id () > 0) {

      //  a text marker
      const geo::PropertiesRepository::properties_set &ps = mp_layout->properties_repository ().properties (shape.prop_id ());

      for (geo::PropertiesRepository::properties_set::const_iterator j = ps.begin (); j != ps.end () && ! is_text; ++j) {
        if (j->first == m_text_annot_name_id.second) {
          text_string = j->second.to_string ();
          is_text = true;
        }
      }

    }

    if (is_text &&
        (m_all || (m_pattern && m_glob_pat.match (text_string)) || (!m_pattern && text_string == m_pat))) {

      geo::Point pt = shape.bbox ().center ();

      if (! complex_region) {
        if (region.contains (pt)) {
          m_delivery.insert (pt.transformed (trans), target);
        }
      } else {
        if (! complex_region->begin_overlapping (geo::Box (pt, pt), geo::box_convert<geo::Box> ()).at_end ()) {
          m_delivery.insert (pt.transformed (trans), target);
        }
      }

    }
  }

  virtual void push (const geo::Box &, const geo::ICplxTrans &, const geo::Box &, const geo::RecursiveShapeReceiver::box_tree_type *, geo::Shapes *) { }
  virtual void push (const geo::Polygon &, const geo::ICplxTrans &, const geo::Box &, const geo::RecursiveShapeReceiver::box_tree_type *, geo::Shapes *) { }

private:
  Delivery m_delivery;
  geo::GlobPattern m_glob_pat;
  bool m_all;
  bool m_pattern;
  std::string m_pat;
  std::pair<bool, geo::property_names_id_type> m_text_annot_name_id;
  const geo::Layout *mp_layout;
};

}

Edges
Region::texts_as_dots (const std::string &pat, bool pattern) const
{
  const geo::DeepRegion *dr = dynamic_cast<const geo::DeepRegion *> (delegate ());
  if (dr) {
    return texts_as_dots (pat, pattern, const_cast<geo::DeepShapeStore &> (*dr->deep_layer ().store ()));
  }

  std::pair<geo::RecursiveShapeIterator, geo::ICplxTrans> si = begin_iter ();
  if (! dr) {
    //  some optimization
    si.first.shape_flags (si.first.shape_flags () & geo::ShapeIterator::Texts);
  }

  std::unique_ptr<geo::FlatEdges> res (new geo::FlatEdges ());
  res->set_merged_semantics (false);

  fill_texts (si.first, pat, pattern, dot_delivery<geo::FlatEdges> (), res.get (), si.second, dr);

  return Edges (res.release ());
}

Edges
Region::texts_as_dots (const std::string &pat, bool pattern, geo::DeepShapeStore &store) const
{
  const geo::DeepRegion *dr = dynamic_cast<const geo::DeepRegion *> (delegate ());

  std::pair<geo::RecursiveShapeIterator, geo::ICplxTrans> si = begin_iter ();
  if (! dr) {
    //  some optimization
    si.first.shape_flags (si.first.shape_flags () & geo::ShapeIterator::Texts);
  }

  if (! si.first.layout ()) {

    //  flat fallback if the source isn't a deep or original layer
    std::unique_ptr<geo::FlatEdges> res (new geo::FlatEdges ());
    res->set_merged_semantics (false);

    fill_texts (si.first, pat, pattern, dot_delivery<geo::FlatEdges> (), res.get (), si.second, dr);

    return Edges (res.release ());

  }

  text_shape_receiver<dot_delivery<geo::Shapes> > pipe = text_shape_receiver<dot_delivery<geo::Shapes> > (dot_delivery<geo::Shapes> (), pat, pattern, dr);
  if (dr && dr->deep_layer ().store () == &store) {
    return Edges (new geo::DeepEdges (store.create_copy (dr->deep_layer (), &pipe)));
  } else {
    return Edges (new geo::DeepEdges (store.create_custom_layer (si.first, &pipe, si.second)));
  }
}

Region
Region::texts_as_boxes (const std::string &pat, bool pattern, geo::Coord enl) const
{
  const geo::DeepRegion *dr = dynamic_cast<const geo::DeepRegion *> (delegate ());
  if (dr) {
    return texts_as_boxes (pat, pattern, enl, const_cast<geo::DeepShapeStore &> (*dr->deep_layer ().store ()));
  }

  std::pair<geo::RecursiveShapeIterator, geo::ICplxTrans> si = begin_iter ();
  if (! dr) {
    //  some optimization
    si.first.shape_flags (si.first.shape_flags () & geo::ShapeIterator::Texts);
  }

  std::unique_ptr<geo::FlatRegion> res (new geo::FlatRegion ());
  res->set_merged_semantics (false);

  fill_texts (si.first, pat, pattern, box_delivery<geo::FlatRegion> (enl), res.get (), si.second, dr);

  return Region (res.release ());
}

Region
Region::texts_as_boxes (const std::string &pat, bool pattern, geo::Coord enl, geo::DeepShapeStore &store) const
{
  const geo::DeepRegion *dr = dynamic_cast<const geo::DeepRegion *> (delegate ());

  std::pair<geo::RecursiveShapeIterator, geo::ICplxTrans> si = begin_iter ();
  if (! dr) {
    //  some optimization
    si.first.shape_flags (si.first.shape_flags () & geo::ShapeIterator::Texts);
  }

  if (! si.first.layout ()) {

    //  flat fallback if the source isn't a deep or original layer
    std::unique_ptr<geo::FlatRegion> res (new geo::FlatRegion ());
    res->set_merged_semantics (false);

    fill_texts (si.first, pat, pattern, box_delivery<geo::FlatRegion> (enl), res.get (), si.second, dr);

    return Region (res.release ());

  }

  text_shape_receiver<box_delivery<geo::Shapes> > pipe = text_shape_receiver<box_delivery<geo::Shapes> > (box_delivery<geo::Shapes> (enl), pat, pattern, dr);
  if (dr && dr->deep_layer ().store () == &store) {
    return Region (new geo::DeepRegion (store.create_copy (dr->deep_layer (), &pipe)));
  } else {
    return Region (new geo::DeepRegion (store.create_custom_layer (si.first, &pipe, si.second)));
  }
}

}

namespace geo
{
  template<> bool test_extractor_impl (geo::Extractor &ex, geo::Region &b)
  {
    geo::Polygon p;

    if (ex.at_end ()) {
      return true;
    }
    if (! ex.try_read (p)) {
      return false;
    }
    b.insert (p);

    while (ex.test (";")) {
      ex.read (p);
      b.insert (p);
    } 

    return true;
  }

  template<> void extractor_impl (geo::Extractor &ex, geo::Region &b)
  {
    if (! test_extractor_impl (ex, b)) {
      ex.error (geo::to_string (tr ("Expected a region specification")));
    }
  }
}

