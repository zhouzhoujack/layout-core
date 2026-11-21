#include "geoOriginalLayerRegion.h"
#include "geoFlatRegion.h"
#include "geoFlatEdges.h"
#include "geoRegion.h"
#include "geoShapeProcessor.h"
#include "geoDeepEdges.h"
#include "geoDeepRegion.h"
#include "geoDeepShapeStore.h"
#include "geoCellGraphUtils.h"
#include "utilsGlobPattern.h"

namespace geo
{

// -------------------------------------------------------------------------------------------------------------
//  OriginalLayerRegion implementation

namespace
{

  class OriginalLayerRegionIterator
    : public RegionIteratorDelegate
  {
  public:
    OriginalLayerRegionIterator (const geo::RecursiveShapeIterator &iter, const geo::ICplxTrans &trans)
      : m_rec_iter (iter), m_iter_trans (trans)
    {
      set ();
    }

    virtual bool is_addressable() const
    {
      return false;
    }

    virtual bool at_end () const
    {
      return m_rec_iter.at_end ();
    }

    virtual void increment ()
    {
      do_increment ();
      set ();
    }

    virtual const value_type *get () const
    {
      return &m_polygon;
    }

    virtual RegionIteratorDelegate *clone () const
    {
      return new OriginalLayerRegionIterator (*this);
    }

    virtual bool equals (const generic_shape_iterator_delegate_base<value_type> *other) const
    {
      const OriginalLayerRegionIterator *o = dynamic_cast<const OriginalLayerRegionIterator *> (other);
      return o && o->m_rec_iter == m_rec_iter && o->m_iter_trans.equal (m_iter_trans);
    }

    virtual void do_reset (const geo::Box &region, bool overlapping)
    {
      if (region == geo::Box::world ()) {
        m_rec_iter.set_region (region);
      } else {
        m_rec_iter.set_region (m_iter_trans.inverted () * region);
      }
      m_rec_iter.set_overlapping (overlapping);
      set ();
    }

    virtual geo::Box bbox () const
    {
      return m_iter_trans * m_rec_iter.bbox ();
    }

  private:
    friend class Region;

    geo::RecursiveShapeIterator m_rec_iter;
    geo::ICplxTrans m_iter_trans;
    geo::Polygon m_polygon;

    void set ()
    {
      while (! m_rec_iter.at_end () && ! (m_rec_iter.shape ().is_polygon () || m_rec_iter.shape ().is_path () || m_rec_iter.shape ().is_box ())) {
        ++m_rec_iter;
      }
      if (! m_rec_iter.at_end ()) {
        m_rec_iter.shape ().polygon (m_polygon);
        m_polygon.transform (m_iter_trans * m_rec_iter.trans (), false);
      }
    }

    void do_increment ()
    {
      if (! m_rec_iter.at_end ()) {
        ++m_rec_iter;
      }
    }
  };

}

OriginalLayerRegion::OriginalLayerRegion ()
  : AsIfFlatRegion (), m_merged_polygons (false)
{
  init ();
}

OriginalLayerRegion::OriginalLayerRegion (const OriginalLayerRegion &other)
  : AsIfFlatRegion (other),
    m_is_merged (other.m_is_merged),
    m_merged_polygons (other.m_merged_polygons),
    m_merged_polygons_valid (other.m_merged_polygons_valid),
    m_iter (other.m_iter),
    m_iter_trans (other.m_iter_trans)
{
  //  .. nothing yet ..
}

OriginalLayerRegion::OriginalLayerRegion (const RecursiveShapeIterator &si, bool is_merged)
  : AsIfFlatRegion (), m_merged_polygons (false), m_iter (si)
{
  init ();

  m_is_merged = is_merged;
}

OriginalLayerRegion::OriginalLayerRegion (const RecursiveShapeIterator &si, const geo::ICplxTrans &trans, bool merged_semantics, bool is_merged)
  : AsIfFlatRegion (), m_merged_polygons (false), m_iter (si), m_iter_trans (trans)
{
  init ();

  m_is_merged = is_merged;
  set_merged_semantics (merged_semantics);
}

OriginalLayerRegion::~OriginalLayerRegion ()
{
  //  .. nothing yet ..
}

RegionDelegate *
OriginalLayerRegion::clone () const
{
  return new OriginalLayerRegion (*this);
}

void
OriginalLayerRegion::merged_semantics_changed ()
{
  m_merged_polygons.clear ();
  m_merged_polygons_valid = false;
}

void
OriginalLayerRegion::min_coherence_changed ()
{
  m_is_merged = false;
  m_merged_polygons.clear ();
  m_merged_polygons_valid = false;
}

size_t
OriginalLayerRegion::count () const
{
  //  NOTE: we should to make sure the iterator isn't validated as this would spoil the usability or OriginalLayerRegion upon
  //  layout changes
  geo::RecursiveShapeIterator iter = m_iter;

  if (iter.has_complex_region () || iter.region () != geo::Box::world () || ! iter.enables ().empty () || ! iter.disables ().empty ()) {

    //  complex case with a search region - use the iterator to determine the count (expensive)
    size_t n = 0;
    for (geo::RecursiveShapeIterator i = iter; ! i.at_end (); ++i) {
      ++n;
    }

    return n;

  } else {

    //  otherwise we can utilize the CellCounter

    size_t n = 0;

    const geo::Layout &layout = *iter.layout ();

    std::set<geo::cell_index_type> cells;
    iter.top_cell ()->collect_called_cells (cells);
    cells.insert (iter.top_cell ()->cell_index ());

    geo::CellCounter cc (&layout);
    for (geo::Layout::top_down_const_iterator c = layout.begin_top_down (); c != layout.end_top_down (); ++c) {
      if (cells.find (*c) == cells.end ()) {
        continue;
      }
      size_t nn = 0;
      if (iter.multiple_layers ()) {
        for (std::vector<unsigned int>::const_iterator l = iter.layers ().begin (); l != iter.layers ().end (); ++l) {
          nn += layout.cell (*c).shapes (*l).size (iter.shape_flags () & geo::ShapeIterator::Regions);
        }
      } else if (iter.layer () < layout.layers ()) {
        nn += layout.cell (*c).shapes (iter.layer ()).size (iter.shape_flags () & geo::ShapeIterator::Regions);
      }
      n += cc.weight (*c) * nn;
    }

    return n;

  }
}

size_t
OriginalLayerRegion::hier_count () const
{
  //  NOTE: we should to make sure the iterator isn't validated as this would spoil the usability or OriginalLayerRegion upon
  //  layout changes
  geo::RecursiveShapeIterator iter = m_iter;

  if (iter.has_complex_region () || iter.region () != geo::Box::world ()) {

    //  TODO: how to establish a "hierarchical" interpretation in this case?
    return count ();

  } else {

    size_t n = 0;

    const geo::Layout &layout = *iter.layout ();

    std::set<geo::cell_index_type> cells;
    iter.top_cell ()->collect_called_cells (cells);
    cells.insert (iter.top_cell ()->cell_index ());

    for (geo::Layout::top_down_const_iterator c = layout.begin_top_down (); c != layout.end_top_down (); ++c) {
      if (cells.find (*c) == cells.end ()) {
        continue;
      }
      if (iter.multiple_layers ()) {
        for (std::vector<unsigned int>::const_iterator l = iter.layers ().begin (); l != iter.layers ().end (); ++l) {
          n += layout.cell (*c).shapes (*l).size (iter.shape_flags () & geo::ShapeIterator::Regions);
        }
      } else if (iter.layer () < layout.layers ()) {
        n += layout.cell (*c).shapes (iter.layer ()).size (iter.shape_flags () & geo::ShapeIterator::Regions);
      }
    }

    return n;

  }
}

RegionIteratorDelegate *
OriginalLayerRegion::begin () const
{
  return new OriginalLayerRegionIterator (m_iter, m_iter_trans);
}

RegionIteratorDelegate *
OriginalLayerRegion::begin_merged () const
{
  if (! merged_semantics () || m_is_merged) {
    return begin ();
  } else {
    ensure_merged_polygons_valid ();
    return new FlatRegionIterator (& m_merged_polygons);
  }
}

std::pair<geo::RecursiveShapeIterator, geo::ICplxTrans>
OriginalLayerRegion::begin_iter () const
{
  return std::make_pair (m_iter, m_iter_trans);
}

std::pair<geo::RecursiveShapeIterator, geo::ICplxTrans>
OriginalLayerRegion::begin_merged_iter () const
{
  if (! merged_semantics () || m_is_merged) {
    return begin_iter ();
  } else {
    ensure_merged_polygons_valid ();
    return std::make_pair (geo::RecursiveShapeIterator (m_merged_polygons), geo::ICplxTrans ());
  }
}

bool
OriginalLayerRegion::empty () const
{
  return m_iter.at_end ();
}

bool
OriginalLayerRegion::is_merged () const
{
  return m_is_merged;
}

const geo::Polygon *
OriginalLayerRegion::nth (size_t) const
{
  throw geo::Exception (geo::to_string (tr ("Random access to polygons is available only for flat regions")));
}

bool
OriginalLayerRegion::has_valid_polygons () const
{
  return false;
}

bool
OriginalLayerRegion::has_valid_merged_polygons () const
{
  return merged_semantics () && ! m_is_merged;
}

const geo::RecursiveShapeIterator *
OriginalLayerRegion::iter () const
{
  return &m_iter;
}

bool
OriginalLayerRegion::equals (const Region &other) const
{
  const OriginalLayerRegion *other_delegate = dynamic_cast<const OriginalLayerRegion *> (other.delegate ());
  if (other_delegate && other_delegate->m_iter == m_iter && other_delegate->m_iter_trans == m_iter_trans) {
    return true;
  } else {
    return AsIfFlatRegion::equals (other);
  }
}

bool
OriginalLayerRegion::less (const Region &other) const
{
  const OriginalLayerRegion *other_delegate = dynamic_cast<const OriginalLayerRegion *> (other.delegate ());
  if (other_delegate && other_delegate->m_iter == m_iter && other_delegate->m_iter_trans == m_iter_trans) {
    return false;
  } else {
    return AsIfFlatRegion::less (other);
  }
}

void
OriginalLayerRegion::init ()
{
  m_is_merged = true;
  m_merged_polygons_valid = false;
}

void
OriginalLayerRegion::insert_into (Layout *layout, geo::cell_index_type into_cell, unsigned int into_layer) const
{
  //  explicitly update the layout if the source is equal to the target
  //  (needed because we lock the layout below and no update would happen)
  if (layout == m_iter.layout ()) {
    layout->update ();
  }

  geo::Shapes &sh = layout->cell (into_cell).shapes (into_layer);

  //  NOTE: if the source (r) is from the same layout than the shapes live in, we better
  //  lock the layout against updates while inserting
  geo::LayoutLocker locker (layout);
  for (geo::RecursiveShapeIterator i = m_iter; !i.at_end (); ++i) {
    geo::ident_map<geo::properties_id_type> pm;
    sh.insert (*i, i.trans (), pm);
  }
}

void
OriginalLayerRegion::ensure_merged_polygons_valid () const
{
  if (! m_merged_polygons_valid) {

    m_merged_polygons.clear ();

    geo::EdgeProcessor ep (report_progress (), progress_desc ());
    ep.set_base_verbosity (base_verbosity ());

    //  count edges and reserve memory
    size_t n = 0;
    for (RegionIterator p (begin ()); ! p.at_end (); ++p) {
      n += p->vertices ();
    }
    ep.reserve (n);

    //  insert the polygons into the processor
    n = 0;
    for (RegionIterator p (begin ()); ! p.at_end (); ++p, ++n) {
      ep.insert (*p, n);
    }

    //  and run the merge step
    geo::MergeOp op (0);
    geo::ShapeGenerator pc (m_merged_polygons);
    geo::PolygonGenerator pg (pc, false /*don't resolve holes*/, min_coherence ());
    ep.process (pg, op);

    m_merged_polygons_valid = true;

  }
}

}
