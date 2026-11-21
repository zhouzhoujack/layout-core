#include "geoDeepTexts.h"
#include "geoCellGraphUtils.h"
#include "geoDeepEdges.h"
#include "geoDeepRegion.h"
#include "geoCellMapping.h"
#include "geoLayoutUtils.h"
#include "geoLocalOperation.h"
#include "geoTextsUtils.h"
#include "geoHierProcessor.h"
#include "geoRegion.h"

#include <sstream>
#include <unordered_set>

namespace geo
{

/**
 *  @brief An iterator delegate for the deep region
 *  TODO: this is kind of redundant with OriginalLayerIterator ..
 */
class DeepTextsIterator
  : public TextsIteratorDelegate
{
public:
  typedef geo::Text value_type;

  DeepTextsIterator (const geo::RecursiveShapeIterator &iter)
    : m_iter (iter)
  {
    set ();
  }

  virtual ~DeepTextsIterator () { }

  virtual bool at_end () const
  {
    return m_iter.at_end ();
  }

  virtual void increment ()
  {
    ++m_iter;
    set ();
  }

  virtual bool is_addressable() const
  {
    return false;
  }

  virtual const value_type *get () const
  {
    return &m_text;
  }

  virtual bool equals (const generic_shape_iterator_delegate_base<value_type> *other) const
  {
    const DeepTextsIterator *o = dynamic_cast<const DeepTextsIterator *> (other);
    return o && o->m_iter == m_iter;
  }

  virtual TextsIteratorDelegate *clone () const
  {
    return new DeepTextsIterator (*this);
  }

  virtual void do_reset (const geo::Box &region, bool overlapping)
  {
    m_iter.set_region (region);
    m_iter.set_overlapping (overlapping);
    set ();
  }

  virtual geo::Box bbox () const
  {
    return m_iter.bbox ();
  }

private:
  friend class Texts;

  geo::RecursiveShapeIterator m_iter;
  mutable value_type m_text;

  void set () const
  {
    if (! m_iter.at_end ()) {
      m_iter.shape ().text (m_text);
      m_text.transform (m_iter.trans ());
    }
  }
};

// -----------------------------------------------------------------------------
//  DeepTexts implementation

DeepTexts::DeepTexts ()
  : MutableTexts ()
{
  //  .. nothing yet ..
}

DeepTexts::DeepTexts (const geo::Texts &other, DeepShapeStore &dss)
  : MutableTexts ()
{
  set_deep_layer (dss.create_from_flat (other));
}

DeepTexts::DeepTexts (const RecursiveShapeIterator &si, DeepShapeStore &dss)
  : MutableTexts ()
{
  set_deep_layer (dss.create_text_layer (si));
}

DeepTexts::DeepTexts (const RecursiveShapeIterator &si, DeepShapeStore &dss, const geo::ICplxTrans &trans)
  : MutableTexts ()
{
  set_deep_layer (dss.create_text_layer (si, trans));
}

DeepTexts::DeepTexts (const DeepTexts &other)
  : MutableTexts (other), DeepShapeCollectionDelegateBase (other)
{
  //  .. nothing yet ..
}

DeepTexts &
DeepTexts::operator= (const DeepTexts &other)
{
  if (this != &other) {
    AsIfFlatTexts::operator= (other);
    DeepShapeCollectionDelegateBase::operator= (other);
  }
  return *this;
}

DeepTexts::DeepTexts (const DeepLayer &dl)
  : MutableTexts ()
{
  set_deep_layer (dl);
}

DeepTexts::~DeepTexts ()
{
  //  .. nothing yet ..
}

TextsDelegate *DeepTexts::clone () const
{
  return new DeepTexts (*this);
}

void DeepTexts::do_insert (const geo::Text &text)
{
  geo::Layout &layout = deep_layer ().layout ();
  if (layout.begin_top_down () != layout.end_top_down ()) {
    geo::Cell &top_cell = layout.cell (*layout.begin_top_down ());
    top_cell.shapes (deep_layer ().layer ()).insert (geo::TextRef (text, layout.shape_repository ()));
  }

  invalidate_bbox ();
  set_is_merged (false);
}

template <class Trans>
static void transform_deep_layer (geo::DeepLayer &deep_layer, const Trans &t)
{
  //  TODO: this is a pretty cheap implementation. At least a plain move can be done with orientation variants.

  geo::Layout &layout = deep_layer.layout ();
  if (layout.begin_top_down () != layout.end_top_down ()) {

    geo::Cell &top_cell = layout.cell (*layout.begin_top_down ());

    geo::Shapes flat_shapes (layout.is_editable ());
    for (geo::RecursiveShapeIterator iter (layout, top_cell, deep_layer.layer ()); !iter.at_end (); ++iter) {
      geo::Text text;
      iter->text (text);
      flat_shapes.insert (text.transformed (iter.trans ()).transformed (t));
    }

    layout.clear_layer (deep_layer.layer ());
    top_cell.shapes (deep_layer.layer ()).swap (flat_shapes);

  }
}

void DeepTexts::do_transform (const geo::Trans &t)
{
  transform_deep_layer (deep_layer (), t);
  invalidate_bbox ();
}

void DeepTexts::do_transform (const geo::ICplxTrans &t)
{
  transform_deep_layer (deep_layer (), t);
  invalidate_bbox ();
}

void DeepTexts::do_transform (const geo::IMatrix2d &t)
{
  transform_deep_layer (deep_layer (), t);
  invalidate_bbox ();
}

void DeepTexts::do_transform (const geo::IMatrix3d &t)
{
  transform_deep_layer (deep_layer (), t);
  invalidate_bbox ();
}

void DeepTexts::reserve (size_t)
{
  //  Not implemented for deep regions
}

void DeepTexts::flatten ()
{
  geo::Layout &layout = deep_layer ().layout ();
  if (layout.begin_top_down () != layout.end_top_down ()) {

    geo::Cell &top_cell = layout.cell (*layout.begin_top_down ());

    geo::Shapes flat_shapes (layout.is_editable ());
    for (geo::RecursiveShapeIterator iter (layout, top_cell, deep_layer ().layer ()); !iter.at_end (); ++iter) {
      geo::Text text;
      iter->text (text);
      flat_shapes.insert (geo::TextRef (text.transformed (iter.trans ()), layout.shape_repository ()));
    }

    layout.clear_layer (deep_layer ().layer ());
    top_cell.shapes (deep_layer ().layer ()).swap (flat_shapes);

  }
}

TextsIteratorDelegate *DeepTexts::begin () const
{
  return new DeepTextsIterator (begin_iter ().first);
}

std::pair<geo::RecursiveShapeIterator, geo::ICplxTrans> DeepTexts::begin_iter () const
{
  const geo::Layout &layout = deep_layer ().layout ();
  if (layout.cells () == 0) {

    return std::make_pair (geo::RecursiveShapeIterator (), geo::ICplxTrans ());

  } else {

    const geo::Cell &top_cell = layout.cell (*layout.begin_top_down ());
    geo::RecursiveShapeIterator iter (deep_layer ().layout (), top_cell, deep_layer ().layer ());
    return std::make_pair (iter, geo::ICplxTrans ());

  }
}

size_t DeepTexts::count () const
{
  size_t n = 0;

  const geo::Layout &layout = deep_layer ().layout ();
  geo::CellCounter cc (&layout);
  for (geo::Layout::top_down_const_iterator c = layout.begin_top_down (); c != layout.end_top_down (); ++c) {
    n += cc.weight (*c) * layout.cell (*c).shapes (deep_layer ().layer ()).size ();
  }

  return n;
}

size_t DeepTexts::hier_count () const
{
  size_t n = 0;

  const geo::Layout &layout = deep_layer ().layout ();
  for (geo::Layout::top_down_const_iterator c = layout.begin_top_down (); c != layout.end_top_down (); ++c) {
    n += layout.cell (*c).shapes (deep_layer ().layer ()).size ();
  }

  return n;
}

std::string DeepTexts::to_string (size_t nmax) const
{
  return geo::AsIfFlatTexts::to_string (nmax);
}

Box DeepTexts::bbox () const
{
  return deep_layer ().initial_cell ().bbox (deep_layer ().layer ());
}

bool DeepTexts::empty () const
{
  return begin_iter ().first.at_end ();
}

const geo::Text *DeepTexts::nth (size_t) const
{
  throw geo::Exception (geo::to_string (tr ("Random access to texts is available only for flat text collections")));
}

bool DeepTexts::has_valid_texts () const
{
  return false;
}

const geo::RecursiveShapeIterator *DeepTexts::iter () const
{
  return 0;
}

TextsDelegate *
DeepTexts::add_in_place (const Texts &other)
{
  if (other.empty ()) {
    return this;
  }

  const DeepTexts *other_deep = dynamic_cast <const DeepTexts *> (other.delegate ());
  if (other_deep) {

    deep_layer ().add_from (other_deep->deep_layer ());

  } else {

    //  non-deep to deep merge (flat)

    geo::Shapes &shapes = deep_layer ().initial_cell ().shapes (deep_layer ().layer ());
    for (geo::Texts::const_iterator p = other.begin (); ! p.at_end (); ++p) {
      shapes.insert (*p);
    }

  }

  return this;
}

TextsDelegate *DeepTexts::add (const Texts &other) const
{
  if (other.empty ()) {
    return clone ();
  } else if (empty ()) {
    return other.delegate ()->clone ();
  } else {
    DeepTexts *new_texts = dynamic_cast<DeepTexts *> (clone ());
    new_texts->add_in_place (other);
    return new_texts;
  }
}

TextsDelegate *DeepTexts::filter_in_place (const TextFilterBase &filter)
{
  //  TODO: implement as really in place
  *this = *apply_filter (filter);
  return this;
}

TextsDelegate *DeepTexts::filtered (const TextFilterBase &filter) const
{
  return apply_filter (filter);
}

DeepTexts *DeepTexts::apply_filter (const TextFilterBase &filter) const
{
  const geo::DeepLayer &texts = deep_layer ();

  std::unique_ptr<VariantsCollectorBase> vars;
  if (filter.vars ()) {

    vars.reset (new geo::VariantsCollectorBase (filter.vars ()));

    vars->collect (texts.layout (), texts.initial_cell ());

    if (filter.wants_variants ()) {
      const_cast<geo::DeepLayer &> (texts).separate_variants (*vars);
    }

  }

  geo::Layout &layout = const_cast<geo::Layout &> (texts.layout ());
  std::map<geo::cell_index_type, std::map<geo::ICplxTrans, geo::Shapes> > to_commit;

  std::unique_ptr<geo::DeepTexts> res (new geo::DeepTexts (texts.derived ()));
  for (geo::Layout::iterator c = layout.begin (); c != layout.end (); ++c) {

    const geo::Shapes &s = c->shapes (texts.layer ());

    if (vars.get ()) {

      const std::map<geo::ICplxTrans, size_t> &vv = vars->variants (c->cell_index ());
      for (std::map<geo::ICplxTrans, size_t>::const_iterator v = vv.begin (); v != vv.end (); ++v) {

        geo::Shapes *st;
        if (vv.size () == 1) {
          st = & c->shapes (res->deep_layer ().layer ());
        } else {
          st = & to_commit [c->cell_index ()] [v->first];
        }

        const geo::ICplxTrans &tr = v->first;

        for (geo::Shapes::shape_iterator si = s.begin (geo::ShapeIterator::Texts); ! si.at_end (); ++si) {
          geo::Text text;
          si->text (text);
          if (filter.selected (text.transformed (tr))) {
            st->insert (*si);
          }
        }

      }

    } else {

      geo::Shapes &st = c->shapes (res->deep_layer ().layer ());

      for (geo::Shapes::shape_iterator si = s.begin (geo::ShapeIterator::Texts); ! si.at_end (); ++si) {
        geo::Text text;
        si->text (text);
        if (filter.selected (text)) {
          st.insert (*si);
        }
      }

    }

  }

  if (! to_commit.empty () && vars.get ()) {
    res->deep_layer ().commit_shapes (*vars, to_commit);
  }

  return res.release ();
}

RegionDelegate *
DeepTexts::processed_to_polygons (const TextToPolygonProcessorBase &filter) const
{
  return shape_collection_processed_impl<geo::Text, geo::Polygon, geo::DeepRegion> (deep_layer (), filter);
}

RegionDelegate *DeepTexts::polygons (geo::Coord e) const
{
  geo::DeepLayer new_layer = deep_layer ().derived ();
  geo::Layout &layout = const_cast<geo::Layout &> (deep_layer ().layout ());

  for (geo::Layout::iterator c = layout.begin (); c != layout.end (); ++c) {
    geo::Shapes &output = c->shapes (new_layer.layer ());
    for (geo::Shapes::shape_iterator s = c->shapes (deep_layer ().layer ()).begin (geo::ShapeIterator::Texts); ! s.at_end (); ++s) {
      geo::Box box = s->bbox ();
      box.enlarge (geo::Vector (e, e));
      geo::Polygon poly (box);
      output.insert (geo::PolygonRef (poly, layout.shape_repository ()));
    }
  }

  return new geo::DeepRegion (new_layer);
}

EdgesDelegate *DeepTexts::edges () const
{
  geo::DeepLayer new_layer = deep_layer ().derived ();
  geo::Layout &layout = const_cast<geo::Layout &> (deep_layer ().layout ());

  for (geo::Layout::iterator c = layout.begin (); c != layout.end (); ++c) {
    geo::Shapes &output = c->shapes (new_layer.layer ());
    for (geo::Shapes::shape_iterator s = c->shapes (deep_layer ().layer ()).begin (geo::ShapeIterator::Texts); ! s.at_end (); ++s) {
      geo::Box box = s->bbox ();
      output.insert (geo::Edge (box.p1 (), box.p2 ()));
    }
  }

  return new geo::DeepEdges (new_layer);
}

TextsDelegate *DeepTexts::in (const Texts &other, bool invert) const
{
  //  TODO: implement
  return AsIfFlatTexts::in (other, invert);
}

bool DeepTexts::equals (const Texts &other) const
{
  const DeepTexts *other_delegate = dynamic_cast<const DeepTexts *> (other.delegate ());
  if (other_delegate && &other_delegate->deep_layer ().layout () == &deep_layer ().layout ()
      && other_delegate->deep_layer ().layer () == deep_layer ().layer ()) {
    return true;
  } else {
    return AsIfFlatTexts::equals (other);
  }
}

bool DeepTexts::less (const Texts &other) const
{
  const DeepTexts *other_delegate = dynamic_cast<const DeepTexts *> (other.delegate ());
  if (other_delegate && &other_delegate->deep_layer ().layout () == &deep_layer ().layout ()) {
    return other_delegate->deep_layer ().layer () < deep_layer ().layer ();
  } else {
    return AsIfFlatTexts::less (other);
  }
}

void DeepTexts::insert_into (Layout *layout, geo::cell_index_type into_cell, unsigned int into_layer) const
{
  deep_layer ().insert_into (layout, into_cell, into_layer);
}

void DeepTexts::insert_into_as_polygons (Layout *layout, geo::cell_index_type into_cell, unsigned int into_layer, geo::Coord enl) const
{
  deep_layer ().insert_into_as_polygons (layout, into_cell, into_layer, enl);
}

namespace {

class Text2PolygonInteractingLocalOperation
  : public local_operation<geo::TextRef, geo::PolygonRef, geo::TextRef>
{
public:
  Text2PolygonInteractingLocalOperation (bool inverse)
    : m_inverse (inverse)
  {
    //  .. nothing yet ..
  }

  virtual geo::Coord dist () const
  {
    //  touching is sufficient
    return 1;
  }

  virtual void do_compute_local (geo::Layout * /*layout*/, const shape_interactions<geo::TextRef, geo::PolygonRef> &interactions, std::vector<std::unordered_set<geo::TextRef> > &results, size_t /*max_vertex_count*/, double /*area_ratio*/) const
  {
    tl_assert (results.size () == 1);
    std::unordered_set<geo::TextRef> &result = results.front ();

    geo::box_scanner2<geo::TextRef, size_t, geo::Polygon, size_t> scanner;

    std::set<geo::PolygonRef> others;
    for (shape_interactions<geo::TextRef, geo::PolygonRef>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
      for (shape_interactions<geo::TextRef, geo::PolygonRef>::iterator2 j = i->second.begin (); j != i->second.end (); ++j) {
        others.insert (interactions.intruder_shape (*j).second);
      }
    }

    for (shape_interactions<geo::TextRef, geo::PolygonRef>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
      const geo::TextRef &subject = interactions.subject_shape (i->first);
      scanner.insert1 (&subject, 0);
    }

    std::list<geo::Polygon> heap;
    for (std::set<geo::PolygonRef>::const_iterator o = others.begin (); o != others.end (); ++o) {
      heap.push_back (o->obj ().transformed (o->trans ()));
      scanner.insert2 (& heap.back (), 1);
    }

    if (m_inverse) {

      std::unordered_set<geo::TextRef> interacting;
      text_to_region_interaction_filter<std::unordered_set<geo::TextRef>, geo::TextRef> filter (interacting);
      scanner.process (filter, 1, geo::box_convert<geo::TextRef> (), geo::box_convert<geo::Polygon> ());

      for (shape_interactions<geo::TextRef, geo::PolygonRef>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
        const geo::TextRef &subject = interactions.subject_shape (i->first);
        if (interacting.find (subject) == interacting.end ()) {
          result.insert (subject);
        }
      }

    } else {

      text_to_region_interaction_filter<std::unordered_set<geo::TextRef>, geo::TextRef> filter (result);
      scanner.process (filter, 1, geo::box_convert<geo::TextRef> (), geo::box_convert<geo::Polygon> ());

    }
  }

  virtual OnEmptyIntruderHint on_empty_intruder_hint () const
  {
    if (m_inverse) {
      return Copy;
    } else {
      return Drop;
    }
  }

  virtual std::string description () const
  {
    return geo::to_string (tr ("Select interacting texts"));
  }

private:
  bool m_inverse;
};

struct ResultInserter
{
  typedef geo::Polygon value_type;

  ResultInserter (geo::Layout *layout, std::unordered_set<geo::PolygonRef> &result)
    : mp_layout (layout), mp_result (&result)
  {
    //  .. nothing yet ..
  }

  void insert (const geo::Polygon &p)
  {
    (*mp_result).insert (geo::PolygonRef (p, mp_layout->shape_repository ()));
  }

private:
  geo::Layout *mp_layout;
  std::unordered_set<geo::PolygonRef> *mp_result;
};

class Text2PolygonPullLocalOperation
  : public local_operation<geo::TextRef, geo::PolygonRef, geo::PolygonRef>
{
public:
  Text2PolygonPullLocalOperation ()
  {
    //  .. nothing yet ..
  }

  virtual geo::Coord dist () const
  {
    //  touching is sufficient
    return 1;
  }

  virtual void do_compute_local (geo::Layout *layout, const shape_interactions<geo::TextRef, geo::PolygonRef> &interactions, std::vector<std::unordered_set<geo::PolygonRef> > &results, size_t /*max_vertex_count*/, double /*area_ratio*/) const
  {
    tl_assert (results.size () == 1);
    std::unordered_set<geo::PolygonRef> &result = results.front ();

    geo::box_scanner2<geo::TextRef, size_t, geo::Polygon, size_t> scanner;

    std::set<geo::PolygonRef> others;
    for (shape_interactions<geo::TextRef, geo::PolygonRef>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
      for (shape_interactions<geo::TextRef, geo::PolygonRef>::iterator2 j = i->second.begin (); j != i->second.end (); ++j) {
        others.insert (interactions.intruder_shape (*j).second);
      }
    }

    for (shape_interactions<geo::TextRef, geo::PolygonRef>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
      const geo::TextRef &subject = interactions.subject_shape (i->first);
      scanner.insert1 (&subject, 1);
    }

    std::list<geo::Polygon> heap;
    for (std::set<geo::PolygonRef>::const_iterator o = others.begin (); o != others.end (); ++o) {
      heap.push_back (o->obj ().transformed (o->trans ()));
      scanner.insert2 (& heap.back (), 0);
    }

    ResultInserter inserter (layout, result);
    text_to_region_interaction_filter<ResultInserter, geo::TextRef> filter (inserter);
    scanner.process (filter, 1, geo::box_convert<geo::TextRef> (), geo::box_convert<geo::Polygon> ());
  }

  virtual OnEmptyIntruderHint on_empty_intruder_hint () const
  {
    return Drop;
  }

  virtual std::string description () const
  {
    return geo::to_string (tr ("Select interacting regions"));
  }
};

}

TextsDelegate *
DeepTexts::selected_interacting_generic (const Region &other, bool inverse) const
{
  std::unique_ptr<geo::DeepRegion> dr_holder;
  const geo::DeepRegion *other_deep = dynamic_cast<const geo::DeepRegion *> (other.delegate ());
  if (! other_deep) {
    //  if the other region isn't deep, turn into a top-level only deep region to facilitate re-hierarchization
    dr_holder.reset (new geo::DeepRegion (other, const_cast<geo::DeepShapeStore &> (*deep_layer ().store ())));
    other_deep = dr_holder.get ();
  }

  const geo::DeepLayer &texts = deep_layer ();

  DeepLayer dl_out (texts.derived ());

  geo::Text2PolygonInteractingLocalOperation op (inverse);

  geo::local_processor<geo::TextRef, geo::PolygonRef, geo::TextRef> proc (const_cast<geo::Layout *> (&texts.layout ()), const_cast<geo::Cell *> (&texts.initial_cell ()), &other_deep->deep_layer ().layout (), &other_deep->deep_layer ().initial_cell ());
  proc.set_base_verbosity (other.base_verbosity ());
  proc.set_threads (texts.store ()->threads ());

  proc.run (&op, texts.layer (), other_deep->deep_layer ().layer (), dl_out.layer ());

  return new geo::DeepTexts (dl_out);
}

RegionDelegate *DeepTexts::pull_generic (const Region &other) const
{
  std::unique_ptr<geo::DeepRegion> dr_holder;
  const geo::DeepRegion *other_deep = dynamic_cast<const geo::DeepRegion *> (other.delegate ());
  if (! other_deep) {
    //  if the other region isn't deep, turn into a top-level only deep region to facilitate re-hierarchization
    dr_holder.reset (new geo::DeepRegion (other, const_cast<geo::DeepShapeStore &> (*deep_layer ().store ())));
    other_deep = dr_holder.get ();
  }

  const geo::DeepLayer &texts = deep_layer ();
  const geo::DeepLayer &other_polygons = other_deep->merged_deep_layer ();

  DeepLayer dl_out (other_polygons.derived ());

  geo::Text2PolygonPullLocalOperation op;

  geo::local_processor<geo::TextRef, geo::PolygonRef, geo::PolygonRef> proc (const_cast<geo::Layout *> (&texts.layout ()), const_cast<geo::Cell *> (&texts.initial_cell ()), &other_polygons.layout (), &other_polygons.initial_cell ());
  proc.set_base_verbosity (other.base_verbosity ());
  proc.set_threads (texts.store ()->threads ());

  proc.run (&op, texts.layer (), other_polygons.layer (), dl_out.layer ());

  return new geo::DeepRegion (dl_out);
}

}
