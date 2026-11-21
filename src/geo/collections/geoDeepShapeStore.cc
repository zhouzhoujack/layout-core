#include "geoDeepShapeStore.h"
#include "geoCellMapping.h"
#include "geoLayoutUtils.h"
#include "geoRegion.h"
#include "geoEdges.h"
#include "geoEdgePairs.h"
#include "geoTexts.h"
#include "geoDeepRegion.h"
#include "geoDeepEdges.h"
#include "geoDeepEdgePairs.h"
#include "geoDeepTexts.h"
#include "geoShapeCollection.h"

#include "utilsTimer.h"

namespace geo
{

// ----------------------------------------------------------------------------------

DeepLayer::DeepLayer ()
  : mp_store (), m_layout (0), m_layer (0)
{
  //  .. nothing yet ..
}

DeepLayer::DeepLayer (const Region &region)
  : mp_store (), m_layout (0), m_layer (0)
{
  const geo::DeepRegion *dr = dynamic_cast<geo::DeepRegion *> (region.delegate ());
  tl_assert (dr != 0);
  *this = dr->deep_layer ();
}

DeepLayer::DeepLayer (const Texts &texts)
  : mp_store (), m_layout (0), m_layer (0)
{
  const geo::DeepTexts *dr = dynamic_cast<geo::DeepTexts *> (texts.delegate ());
  tl_assert (dr != 0);
  *this = dr->deep_layer ();
}

DeepLayer::DeepLayer (const Edges &edges)
  : mp_store (), m_layout (0), m_layer (0)
{
  const geo::DeepEdges *dr = dynamic_cast<geo::DeepEdges *> (edges.delegate ());
  tl_assert (dr != 0);
  *this = dr->deep_layer ();
}

DeepLayer::DeepLayer (const EdgePairs &edge_pairs)
  : mp_store (), m_layout (0), m_layer (0)
{
  const geo::DeepEdgePairs *dr = dynamic_cast<geo::DeepEdgePairs *> (edge_pairs.delegate ());
  tl_assert (dr != 0);
  *this = dr->deep_layer ();
}

DeepLayer::DeepLayer (const DeepLayer &x)
  : mp_store (x.mp_store), m_layout (x.m_layout), m_layer (x.m_layer)
{
  if (mp_store.get ()) {
    mp_store->add_ref (m_layout, m_layer);
  }
}

DeepLayer::DeepLayer (DeepShapeStore *store, unsigned int layout, unsigned int layer)
  : mp_store (store), m_layout (layout), m_layer (layer)
{
  if (store) {
    store->add_ref (layout, layer);
  }
}

DeepLayer &DeepLayer::operator= (const DeepLayer &other)
{
  if (this != &other) {
    if (mp_store.get ()) {
      mp_store->remove_ref (m_layout, m_layer);
    }
    mp_store = other.mp_store;
    m_layout = other.m_layout;
    m_layer = other.m_layer;
    if (mp_store.get ()) {
      mp_store->add_ref (m_layout, m_layer);
    }
  }

  return *this;
}

DeepLayer::~DeepLayer ()
{
  if (mp_store.get ()) {
    mp_store->remove_ref (m_layout, m_layer);
  }
}

DeepLayer
DeepLayer::derived () const
{
  return DeepLayer (const_cast <geo::DeepShapeStore *> (mp_store.get ()), m_layout, const_cast <geo::Layout &> (layout ()).insert_layer ());
}

DeepLayer
DeepLayer::copy () const
{
  DeepLayer new_layer (derived ());

  geo::DeepShapeStore *non_const_store = const_cast<geo::DeepShapeStore *> (mp_store.get ());
  non_const_store->layout (m_layout).copy_layer (m_layer, new_layer.layer ());

  return new_layer;
}

void
DeepLayer::add_from (const DeepLayer &dl)
{
  if (&dl.layout () == &layout ()) {

    //  intra-layout merge

    layout ().copy_layer (dl.layer (), layer ());

  } else {

    //  inter-layout merge

    geo::cell_index_type into_cell = initial_cell ().cell_index ();
    geo::Layout *into_layout = &layout ();
    geo::cell_index_type source_cell = dl.initial_cell ().cell_index ();
    const geo::Layout *source_layout = &dl.layout ();

    geo::CellMapping cm;
    cm.create_from_geometry_full (*into_layout, into_cell, *source_layout, source_cell);

    //  Actually copy the shapes

    std::map<unsigned int, unsigned int> lm;
    lm.insert (std::make_pair (dl.layer (), layer ()));

    std::vector <geo::cell_index_type> source_cells;
    source_cells.push_back (source_cell);
    geo::copy_shapes (*into_layout, *source_layout, geo::ICplxTrans (), source_cells, cm.table (), lm);

  }
}

const std::set<geo::cell_index_type> *
DeepLayer::breakout_cells () const
{
  return store ()->breakout_cells (layout_index ());
}

void
DeepLayer::insert_into (geo::Layout *into_layout, geo::cell_index_type into_cell, unsigned int into_layer) const
{
  check_dss ();
  const_cast<geo::DeepShapeStore *> (mp_store.get ())->insert (*this, into_layout, into_cell, into_layer);
}

void
DeepLayer::insert_into_as_polygons (geo::Layout *into_layout, geo::cell_index_type into_cell, unsigned int into_layer, geo::Coord enl) const
{
  check_dss ();
  const_cast<geo::DeepShapeStore *> (mp_store.get ())->insert_as_polygons (*this, into_layout, into_cell, into_layer, enl);
}

bool DeepLayer::operator< (const DeepLayer &other) const
{
  if (mp_store.get () != other.mp_store.get ()) {
    return mp_store.get () < other.mp_store.get ();
  }
  if (m_layout != other.m_layout) {
    return m_layout < other.m_layout;
  }
  if (m_layer != other.m_layer) {
    return m_layer < other.m_layer;
  }
  return false;
}

bool DeepLayer::operator== (const DeepLayer &other) const
{
  if (mp_store.get () != other.mp_store.get ()) {
    return false;
  }
  if (m_layout != other.m_layout) {
    return false;
  }
  if (m_layer != other.m_layer) {
    return false;
  }
  return true;
}

geo::Layout &
DeepLayer::layout ()
{
  check_dss ();
  return mp_store->layout (m_layout);
}

const geo::Layout &
DeepLayer::layout () const
{
  check_dss ();
  return const_cast<geo::DeepShapeStore *> (mp_store.get ())->layout (m_layout);
}

geo::Cell &
DeepLayer::initial_cell ()
{
  check_dss ();
  return mp_store->initial_cell (m_layout);
}

const geo::Cell &
DeepLayer::initial_cell () const
{
  check_dss ();
  return mp_store->const_initial_cell (m_layout);
}

void
DeepLayer::check_dss () const
{
  if (mp_store.get () == 0) {
    throw geo::Exception (geo::to_string (tr ("Heap lost: the DeepShapeStore container no longer exists")));
  }
}

// ----------------------------------------------------------------------------------

struct DeepShapeStore::LayoutHolder
{
  LayoutHolder (const geo::ICplxTrans &trans)
    : refs (0), layout (false), builder (&layout, trans)
  {
    //  .. nothing yet ..
  }

  void add_layer_ref (unsigned int layer)
  {
    layer_refs [layer] += 1;
  }

  bool remove_layer_ref (unsigned int layer)
  {
    if ((layer_refs[layer] -= 1) <= 0) {
      layout.delete_layer (layer);
      layer_refs.erase (layer);
      return true;
    } else {
      return false;
    }
  }

  int refs;
  geo::Layout layout;
  geo::HierarchyBuilder builder;
  std::map<unsigned int, int> layer_refs;
};

// ----------------------------------------------------------------------------------

DeepShapeStoreState::DeepShapeStoreState ()
  : m_threads (1), m_max_area_ratio (3.0), m_max_vertex_count (16), m_reject_odd_polygons (false), m_text_property_name (), m_text_enlargement (-1)
{
  //  .. nothing yet ..
}

void DeepShapeStoreState::set_reject_odd_polygons (bool f)
{
  m_reject_odd_polygons = f;
}

bool DeepShapeStoreState::reject_odd_polygons () const
{
  return m_reject_odd_polygons;
}

void DeepShapeStoreState::set_text_enlargement (int enl)
{
  m_text_enlargement = enl;
}

int DeepShapeStoreState::text_enlargement () const
{
  return m_text_enlargement;
}

void DeepShapeStoreState::set_text_property_name (const geo::Variant &pn)
{
  m_text_property_name = pn;
}

const geo::Variant &
DeepShapeStoreState::text_property_name () const
{
  return m_text_property_name;
}

const std::set<geo::cell_index_type> *
DeepShapeStoreState::breakout_cells (unsigned int layout_index) const
{
  const std::set<geo::cell_index_type> &boc = (const_cast<DeepShapeStoreState *> (this))->ensure_breakout_cells (layout_index);
  if (boc.empty ()) {
    return 0;
  } else {
    return &boc;
  }
}

void
DeepShapeStoreState::clear_breakout_cells (unsigned int layout_index)
{
  ensure_breakout_cells (layout_index).clear ();
}

void
DeepShapeStoreState::set_breakout_cells (unsigned int layout_index, const std::set<geo::cell_index_type> &boc)
{
  ensure_breakout_cells (layout_index) = boc;
}

void
DeepShapeStoreState::add_breakout_cell (unsigned int layout_index, geo::cell_index_type ci)
{
  ensure_breakout_cells (layout_index).insert (ci);
}

void
DeepShapeStoreState::add_breakout_cells (unsigned int layout_index, const std::set<geo::cell_index_type> &cc)
{
  ensure_breakout_cells (layout_index).insert (cc.begin (), cc.end ());
}

void
DeepShapeStoreState::set_threads (int n)
{
  m_threads = n;
}

int
DeepShapeStoreState::threads () const
{
  return m_threads;
}

void
DeepShapeStoreState::set_max_area_ratio (double ar)
{
  m_max_area_ratio = ar;
}

double
DeepShapeStoreState::max_area_ratio () const
{
  return m_max_area_ratio;
}

void
DeepShapeStoreState::set_max_vertex_count (size_t n)
{
  m_max_vertex_count = n;
}

size_t
DeepShapeStoreState::max_vertex_count () const
{
  return m_max_vertex_count;
}

// ----------------------------------------------------------------------------------

static size_t s_instance_count = 0;

static unsigned int init_layer (geo::Layout &layout, const geo::RecursiveShapeIterator &si)
{
  unsigned int layer_index = layout.insert_layer ();

  if (si.layout ()) {
    //  try to preserve the layer properties
    if (! si.multiple_layers ()) {
      if (si.layer () < si.layout ()->layers ()) {
        layout.set_properties (layer_index, si.layout ()->get_properties (si.layer ()));
      }
    } else if (! si.layers ().empty ()) {
      layout.set_properties (layer_index, si.layout ()->get_properties (si.layers ().front ()));
    }
  }

  return layer_index;
}

// -----------------------------------------------------------------------------
//  DeepShapeStore implementation

DeepShapeStore::DeepShapeStore ()
{
  ++s_instance_count;
}

DeepShapeStore::DeepShapeStore (const std::string &topcell_name, double dbu)
{
  ++s_instance_count;

  m_layouts.push_back (new LayoutHolder (geo::ICplxTrans ()));
  m_layouts.back ()->layout.dbu (dbu);
  m_layouts.back ()->layout.add_cell (topcell_name.c_str ());
}

DeepShapeStore::~DeepShapeStore ()
{
  --s_instance_count;

  for (std::vector<LayoutHolder *>::iterator h = m_layouts.begin (); h != m_layouts.end (); ++h) {
    delete *h;
  }
  m_layouts.clear ();
}

DeepLayer DeepShapeStore::create_from_flat (const geo::Region &region, bool for_netlist, double max_area_ratio, size_t max_vertex_count, const geo::ICplxTrans &trans)
{
  //  reuse existing layer
  std::pair<bool, DeepLayer> lff = layer_for_flat (region);
  if (lff.first) {
    return lff.second;
  }

  require_singular ();

  unsigned int layer = init_layer (layout (), region.iter ());

  //  attempt to initialize the layer properties

  if (max_area_ratio == 0.0) {
    max_area_ratio = m_state.max_area_ratio ();
  }
  if (max_vertex_count == 0) {
    max_vertex_count = m_state.max_vertex_count ();
  }

  geo::Shapes *shapes = &initial_cell ().shapes (layer);
  geo::Box world = geo::Box::world ();

  //  The chain of operators for producing clipped and reduced polygon references
  geo::PolygonReferenceHierarchyBuilderShapeReceiver refs (&layout (), text_enlargement (), text_property_name ());
  geo::ReducingHierarchyBuilderShapeReceiver red (&refs, max_area_ratio, max_vertex_count, m_state.reject_odd_polygons ());

  //  try to maintain the texts on top level - go through shape iterator
  std::pair<geo::RecursiveShapeIterator, geo::ICplxTrans> ii = region.begin_iter ();
  geo::ICplxTrans ttop = trans * ii.second;
  while (! ii.first.at_end ()) {

    if (for_netlist && ii.first->is_text () && ii.first.layout () && ii.first.cell () != ii.first.top_cell ()) {
      //  Skip texts on levels below top cell. For the reasoning see the description of this method.
    } else {
      red.push (*ii.first, ttop * ii.first.trans (), world, 0, shapes);
    }

    ++ii.first;

  }

  DeepLayer dl (this, 0 /*singular layout index*/, layer);
  m_layers_for_flat [geo::id_of (region.delegate ())] = std::make_pair (dl.layout_index (), dl.layer ());
  m_flat_region_id [std::make_pair (dl.layout_index (), dl.layer ())] = geo::id_of (region.delegate ());
  return dl;
}

DeepLayer DeepShapeStore::create_from_flat (const geo::Edges &edges, const geo::ICplxTrans &trans)
{
  //  reuse existing layer
  std::pair<bool, DeepLayer> lff = layer_for_flat (geo::id_of (edges.delegate ()));
  if (lff.first) {
    return lff.second;
  }

  require_singular ();

  unsigned int layer = init_layer (layout (), edges.iter ());

  geo::Shapes *shapes = &initial_cell ().shapes (layer);
  geo::Box world = geo::Box::world ();

  geo::EdgeBuildingHierarchyBuilderShapeReceiver eb (false);

  std::pair<geo::RecursiveShapeIterator, geo::ICplxTrans> ii = edges.begin_iter ();
  geo::ICplxTrans ttop = trans * ii.second;
  while (! ii.first.at_end ()) {
    eb.push (*ii.first, ttop * ii.first.trans (), world, 0, shapes);
    ++ii.first;
  }

  DeepLayer dl (this, 0 /*singular layout index*/, layer);
  m_layers_for_flat [geo::id_of (edges.delegate ())] = std::make_pair (dl.layout_index (), dl.layer ());
  m_flat_region_id [std::make_pair (dl.layout_index (), dl.layer ())] = geo::id_of (edges.delegate ());
  return dl;
}

DeepLayer DeepShapeStore::create_from_flat (const geo::Texts &texts, const geo::ICplxTrans &trans)
{
  //  reuse existing layer
  std::pair<bool, DeepLayer> lff = layer_for_flat (geo::id_of (texts.delegate ()));
  if (lff.first) {
    return lff.second;
  }

  require_singular ();

  unsigned int layer = init_layer (layout (), texts.iter ());

  geo::Shapes *shapes = &initial_cell ().shapes (layer);
  geo::Box world = geo::Box::world ();

  geo::TextBuildingHierarchyBuilderShapeReceiver tb (&layout ());

  std::pair<geo::RecursiveShapeIterator, geo::ICplxTrans> ii = texts.begin_iter ();
  geo::ICplxTrans ttop = trans * ii.second;
  while (! ii.first.at_end ()) {
    tb.push (*ii.first, ttop * ii.first.trans (), world, 0, shapes);
    ++ii.first;
  }

  DeepLayer dl (this, 0 /*singular layout index*/, layer);
  m_layers_for_flat [geo::id_of (texts.delegate ())] = std::make_pair (dl.layout_index (), dl.layer ());
  m_flat_region_id [std::make_pair (dl.layout_index (), dl.layer ())] = geo::id_of (texts.delegate ());
  return dl;
}

std::pair<bool, DeepLayer> DeepShapeStore::layer_for_flat (const ShapeCollection &coll) const
{
  return layer_for_flat (geo::id_of (coll.get_delegate ()));
}

std::pair<bool, DeepLayer> DeepShapeStore::layer_for_flat (size_t region_id) const
{
  std::map<size_t, std::pair<unsigned int, unsigned int> >::const_iterator lff = m_layers_for_flat.find (region_id);
  if (lff == m_layers_for_flat.end ()) {
    return std::make_pair (false, DeepLayer ());
  } else {
    return std::make_pair (true, DeepLayer (const_cast<DeepShapeStore *> (this), lff->second.first, lff->second.second));
  }
}

bool DeepShapeStore::is_singular () const
{
  return m_layouts.size () == 1;
}

void DeepShapeStore::require_singular () const
{
  if (! is_singular ()) {
    throw geo::Exception (geo::to_string (tr ("Internal error: deep shape store isn't singular. This may happen if you try to mix hierarchical layers from different sources our you use clipping.")));
  }
}

Cell &DeepShapeStore::initial_cell(unsigned int n)
{
  geo::Layout &ly = layout (n);
  tl_assert (ly.cells () > 0);
  return ly.cell (*ly.begin_top_down ());
}

const geo::Cell &DeepShapeStore::const_initial_cell (unsigned int n) const
{
  const geo::Layout &ly = const_layout (n);
  tl_assert (ly.cells () > 0);
  return ly.cell (*ly.begin_top_down ());
}

void DeepShapeStore::set_text_enlargement (int enl)
{
  m_state.set_text_enlargement (enl);
}

int DeepShapeStore::text_enlargement () const
{
  return m_state.text_enlargement ();
}

void DeepShapeStore::set_text_property_name (const geo::Variant &pn)
{
  m_state.set_text_property_name (pn);
}

const geo::Variant &DeepShapeStore::text_property_name () const
{
  return m_state.text_property_name ();
}

const std::set<geo::cell_index_type> *
DeepShapeStore::breakout_cells (unsigned int layout_index) const
{
  return m_state.breakout_cells (layout_index);
}

void
DeepShapeStore::clear_breakout_cells (unsigned int layout_index)
{
  m_state.clear_breakout_cells (layout_index);
}

void
DeepShapeStore::set_breakout_cells (unsigned int layout_index, const std::set<geo::cell_index_type> &boc)
{
  m_state.set_breakout_cells (layout_index, boc);
}

void
DeepShapeStore::add_breakout_cell (unsigned int layout_index, geo::cell_index_type ci)
{
  m_state.add_breakout_cell (layout_index, ci);
}

void
DeepShapeStore::add_breakout_cells (unsigned int layout_index, const std::set<geo::cell_index_type> &cc)
{
  m_state.add_breakout_cells (layout_index, cc);
}

void DeepShapeStore::set_threads (int n)
{
  m_state.set_threads (n);
}

int DeepShapeStore::threads () const
{
  return m_state.threads ();
}

void DeepShapeStore::set_max_area_ratio (double ar)
{
  m_state.set_max_area_ratio (ar);
}

double DeepShapeStore::max_area_ratio () const
{
  return m_state.max_area_ratio ();
}

void DeepShapeStore::set_reject_odd_polygons (bool f)
{
  m_state.set_reject_odd_polygons (f);
}

bool DeepShapeStore::reject_odd_polygons () const
{
  return m_state.reject_odd_polygons ();
}

void DeepShapeStore::set_max_vertex_count (size_t n)
{
  m_state.set_max_vertex_count (n);
}

size_t DeepShapeStore::max_vertex_count () const
{
  return m_state.max_vertex_count ();
}

void DeepShapeStore::push_state ()
{
  m_state_stack.push_back (m_state);
}

void DeepShapeStore::pop_state ()
{
  if (! m_state_stack.empty ()) {
    m_state = m_state_stack.back ();
    m_state_stack.pop_back ();
  }
}

bool DeepShapeStore::is_valid_layout_index (unsigned int n) const
{
  return (n < (unsigned int) m_layouts.size () && m_layouts[n] != 0);
}

const geo::Layout &DeepShapeStore::const_layout (unsigned int n) const
{
  tl_assert (is_valid_layout_index (n));
  return m_layouts [n]->layout;
}

geo::Layout &DeepShapeStore::layout (unsigned int n)
{
  tl_assert (is_valid_layout_index (n));
  return m_layouts [n]->layout;
}

size_t DeepShapeStore::instance_count ()
{
  return s_instance_count;
}

void DeepShapeStore::add_ref (unsigned int layout, unsigned int layer)
{
  geo::MutexLocker locker (&m_lock);

  tl_assert (layout < (unsigned int) m_layouts.size () && m_layouts[layout] != 0);

  m_layouts[layout]->refs += 1;
  m_layouts[layout]->add_layer_ref (layer);
}

void DeepShapeStore::remove_ref (unsigned int layout, unsigned int layer)
{
  geo::MutexLocker locker (&m_lock);

  tl_assert (layout < (unsigned int) m_layouts.size () && m_layouts[layout] != 0);

  if (m_layouts[layout]->remove_layer_ref (layer)) {

    //  remove from flat region cross ref if required
    std::map<std::pair<unsigned int, unsigned int>, size_t>::iterator fri = m_flat_region_id.find (std::make_pair (layout, layer));
    if (fri != m_flat_region_id.end ()) {
      m_layers_for_flat.erase (fri->second);
      m_flat_region_id.erase (fri);
    }

  }

  if ((m_layouts[layout]->refs -= 1) <= 0) {
    delete m_layouts[layout];
    m_layouts[layout] = 0;
    clear_breakout_cells (layout);
  }
}

unsigned int
DeepShapeStore::layout_for_iter (const geo::RecursiveShapeIterator &si, const geo::ICplxTrans &trans)
{
  layout_map_type::iterator l = m_layout_map.find (std::make_pair (si, trans));
  if (l == m_layout_map.end () || m_layouts[l->second] == 0) {

    unsigned int layout_index;

    if (l != m_layout_map.end ()) {
      //  reuse discarded entry
      layout_index = l->second;
      m_layouts[layout_index] = new LayoutHolder (trans);
    } else {
      layout_index = (unsigned int) m_layouts.size ();
      m_layouts.push_back (new LayoutHolder (trans));
    }

    geo::Layout &layout = m_layouts[layout_index]->layout;
    layout.hier_changed_event.add (this, &DeepShapeStore::invalidate_hier);
    if (si.layout ()) {
      layout.dbu (si.layout ()->dbu () / trans.mag ());
    }

    m_layout_map[std::make_pair (si, trans)] = layout_index;
    return layout_index;

  } else {
    return l->second;
  }
}

void DeepShapeStore::make_layout (unsigned int layout_index, const geo::RecursiveShapeIterator &si, const geo::ICplxTrans &trans)
{
  tl_assert (m_layout_map.find (std::make_pair (si, trans)) == m_layout_map.end ());

  while (m_layouts.size () <= layout_index) {
    m_layouts.push_back (0);
  }

  m_layouts[layout_index] = new LayoutHolder (trans);

  geo::Layout &layout = m_layouts[layout_index]->layout;
  layout.hier_changed_event.add (this, &DeepShapeStore::invalidate_hier);
  if (si.layout ()) {
    layout.dbu (si.layout ()->dbu () / trans.mag ());
  }

  m_layout_map[std::make_pair (si, trans)] = layout_index;
}

DeepLayer DeepShapeStore::create_polygon_layer (const geo::RecursiveShapeIterator &si, double max_area_ratio, size_t max_vertex_count, const geo::ICplxTrans &trans)
{
  if (max_area_ratio == 0.0) {
    max_area_ratio = m_state.max_area_ratio ();
  }
  if (max_vertex_count == 0) {
    max_vertex_count = m_state.max_vertex_count ();
  }

  unsigned int layout_index = layout_for_iter (si, trans);

  geo::Layout &layout = m_layouts[layout_index]->layout;
  geo::HierarchyBuilder &builder = m_layouts[layout_index]->builder;

  unsigned int layer_index = init_layer (layout, si);
  builder.set_target_layer (layer_index);

  //  The chain of operators for producing clipped and reduced polygon references
  geo::PolygonReferenceHierarchyBuilderShapeReceiver refs (& layout, text_enlargement (), text_property_name ());
  geo::ReducingHierarchyBuilderShapeReceiver red (&refs, max_area_ratio, max_vertex_count, m_state.reject_odd_polygons ());
  geo::ClippingHierarchyBuilderShapeReceiver clip (&red);

  //  Build the working hierarchy from the recursive shape iterator
  try {

    geo::SelfTimer timer (geo::verbosity () >= 41, geo::to_string (tr ("Building working hierarchy")));
    geo::LayoutLocker ll (&layout, true /*no update*/);

    builder.set_shape_receiver (&clip);
    geo::RecursiveShapeIterator (si).push (& builder);
    builder.set_shape_receiver (0);

  } catch (...) {
    builder.set_shape_receiver (0);
    throw;
  }

  return DeepLayer (this, layout_index, layer_index);
}

DeepLayer DeepShapeStore::create_custom_layer (const geo::RecursiveShapeIterator &si, HierarchyBuilderShapeReceiver *pipe, const geo::ICplxTrans &trans)
{
  unsigned int layout_index = layout_for_iter (si, trans);

  geo::Layout &layout = m_layouts[layout_index]->layout;
  geo::HierarchyBuilder &builder = m_layouts[layout_index]->builder;

  unsigned int layer_index = init_layer (layout, si);
  builder.set_target_layer (layer_index);

  //  Build the working hierarchy from the recursive shape iterator
  try {

    geo::SelfTimer timer (geo::verbosity () >= 41, geo::to_string (tr ("Building working hierarchy")));
    geo::LayoutLocker ll (&layout, true /*no update*/);

    builder.set_shape_receiver (pipe);
    geo::RecursiveShapeIterator (si).push (& builder);
    builder.set_shape_receiver (0);

  } catch (...) {
    builder.set_shape_receiver (0);
    throw;
  }

  return DeepLayer (this, layout_index, layer_index);
}

DeepLayer DeepShapeStore::create_copy (const DeepLayer &source, HierarchyBuilderShapeReceiver *pipe)
{
  tl_assert (source.store () == this);

  unsigned int from_layer_index = source.layer ();
  geo::Layout &ly = layout ();

  unsigned int layer_index = ly.insert_layer ();

  //  Build the working hierarchy from the recursive shape iterator
  geo::SelfTimer timer (geo::verbosity () >= 41, geo::to_string (tr ("Building working hierarchy")));

  geo::Box region = geo::Box::world ();
  geo::ICplxTrans trans;

  for (geo::Layout::iterator c = ly.begin (); c != ly.end (); ++c) {
    geo::Shapes &into = c->shapes (layer_index);
    const geo::Shapes &from = c->shapes (from_layer_index);
    for (geo::Shapes::shape_iterator s = from.begin (geo::ShapeIterator::All); ! s.at_end (); ++s) {
      pipe->push (*s, trans, region, 0, &into);
    }
  }

  return DeepLayer (this, source.layout_index (), layer_index);
}

DeepLayer DeepShapeStore::create_edge_layer (const geo::RecursiveShapeIterator &si, bool as_edges, const geo::ICplxTrans &trans)
{
  geo::EdgeBuildingHierarchyBuilderShapeReceiver refs (as_edges);
  return create_custom_layer (si, &refs, trans);
}

DeepLayer DeepShapeStore::create_edge_pair_layer (const geo::RecursiveShapeIterator &si, const geo::ICplxTrans &trans)
{
  geo::EdgePairBuildingHierarchyBuilderShapeReceiver refs;
  return create_custom_layer (si, &refs, trans);
}

DeepLayer DeepShapeStore::create_text_layer (const geo::RecursiveShapeIterator &si, const geo::ICplxTrans &trans)
{
  unsigned int layout_index = layout_for_iter (si, trans);
  geo::Layout &layout = m_layouts[layout_index]->layout;

  geo::TextBuildingHierarchyBuilderShapeReceiver refs (&layout);
  return create_custom_layer (si, &refs, trans);
}

void
DeepShapeStore::invalidate_hier ()
{
  m_delivery_mapping_cache.clear ();
}

void
DeepShapeStore::issue_variants (unsigned int layout_index, const std::map<geo::cell_index_type, std::map<geo::ICplxTrans, geo::cell_index_type> > &var_map)
{
  invalidate_hier ();

  geo::HierarchyBuilder &builder = m_layouts [layout_index]->builder;
  for (std::map<geo::cell_index_type, std::map<geo::ICplxTrans, geo::cell_index_type> >::const_iterator i = var_map.begin (); i != var_map.end (); ++i) {
    for (std::map<geo::ICplxTrans, geo::cell_index_type>::const_iterator j = i->second.begin (); j != i->second.end (); ++j) {
      builder.register_variant (i->first, j->second);
    }
  }
}

const geo::CellMapping &
DeepShapeStore::cell_mapping_to_original (unsigned int layout_index, geo::Layout *into_layout, geo::cell_index_type into_cell, const std::set<geo::cell_index_type> *excluded_cells, const std::set<geo::cell_index_type> *included_cells)
{
  geo::Layout *source_layout = &m_layouts [layout_index]->layout;
  if (source_layout->begin_top_down () == source_layout->end_top_cells ()) {
    //  empty source - nothing to do.
    static geo::CellMapping cm;
    return cm;
  }

  geo::cell_index_type source_top = *source_layout->begin_top_down();

  geo::HierarchyBuilder &original_builder = m_layouts [layout_index]->builder;

  //  Derive a cell mapping for source to target. We reuse any existing mapping for returning the
  //  shapes into the original layout.

  DeliveryMappingCacheKey key (layout_index, geo::id_of (into_layout), into_cell);

  std::map<DeliveryMappingCacheKey, geo::CellMapping>::iterator cm = m_delivery_mapping_cache.find (key);
  if (cm == m_delivery_mapping_cache.end ()) {

    cm = m_delivery_mapping_cache.insert (std::make_pair (key, geo::CellMapping ())).first;

    //  collects the cell mappings we skip because they are variants (variant building or box variants)
    std::map<geo::cell_index_type, geo::HierarchyBuilder::CellMapKey> cm_skipped_variants;

    if (into_layout == original_builder.source ().layout () && &into_layout->cell (into_cell) == original_builder.source ().top_cell () && original_builder.source ().global_trans ().is_unity ()) {

      //  This is the case of mapping back to the original. In this case we can use the information
      //  provided inside the original hierarchy builders. They list the source cells and the target cells
      //  create from them. We need to consider however, that the hierarchy builder is allowed to create
      //  variants which we cannot map.

      for (HierarchyBuilder::cell_map_type::const_iterator m = original_builder.begin_cell_map (); m != original_builder.end_cell_map (); ) {

        HierarchyBuilder::cell_map_type::const_iterator mm = m;
        ++mm;
        bool skip = original_builder.is_variant (m->second);   //  skip variant cells
        while (mm != original_builder.end_cell_map () && mm->first.original_cell == m->first.original_cell) {
          //  we have cell (box) variants and cannot simply map
          ++mm;
          skip = true;
        }

        if (! skip) {
          cm->second.map (m->second, m->first.original_cell);
        } else {
          for (HierarchyBuilder::cell_map_type::const_iterator n = m; n != mm; ++n) {
            tl_assert (cm_skipped_variants.find (n->second) == cm_skipped_variants.end ());
            cm_skipped_variants [n->second] = n->first;
          }
        }

        m = mm;

      }

    } else if (into_layout->cells () == 1) {

      //  Another simple case is mapping into an empty (or single-top-cell-only) layout, where we can use "create_from_single_full".
      cm->second.create_single_mapping (*into_layout, into_cell, *source_layout, source_top);

    } else {

      cm->second.create_from_geometry (*into_layout, into_cell, *source_layout, source_top);

    }

    //  Add new cells for the variants and (possible) devices which are cells added during the device
    //  extraction process
    std::vector<std::pair<geo::cell_index_type, geo::cell_index_type> > new_pairs = cm->second.create_missing_mapping2 (*into_layout, *source_layout, source_top, excluded_cells, included_cells);

    //  the variant's originals we are going to delete
    std::set<geo::cell_index_type> cells_to_delete;

    //  We now need to fix the cell map from the hierarchy builder, so we can import back from the modified layout.
    //  This is in particular important if we created new cells for known variants.
    for (std::vector<std::pair<geo::cell_index_type, geo::cell_index_type> >::const_iterator np = new_pairs.begin (); np != new_pairs.end (); ++np) {

      geo::cell_index_type var_org = original_builder.original_target_for_variant (np->first);

      std::map<geo::cell_index_type, geo::HierarchyBuilder::CellMapKey>::const_iterator icm = cm_skipped_variants.find (var_org);
      if (icm != cm_skipped_variants.end ()) {

        //  create the variant clone in the original layout too and delete this cell
        VariantsCollectorBase::copy_shapes (*into_layout, np->second, icm->second.original_cell);
        cells_to_delete.insert (icm->second.original_cell);

        //  forget the original cell (now separated into variants) and map the variants back into the
        //  DSS layout
        original_builder.unmap (icm->second);
        geo::HierarchyBuilder::CellMapKey k = icm->second;
        k.original_cell = np->second;
        original_builder.map (k, np->first);

        //  forget the variant as now it's a real cell in the source layout
        original_builder.unregister_variant (np->first);

        //  rename the cell because it may be a different one now
        source_layout->rename_cell (np->first, into_layout->cell_name (np->second));

      }

    }

    //  delete the variant's original cell
    if (! cells_to_delete.empty ()) {
      into_layout->delete_cells (cells_to_delete);
    }

  }

  return cm->second;
}

namespace
{
  class DeepShapeStoreToShapeTransformer
    : public ShapesTransformer
  {
  public:
    DeepShapeStoreToShapeTransformer (const DeepShapeStore &dss, const geo::Layout &layout)
      : mp_layout (& layout)
    {
      //  gets the text annotation property ID -
      //  this is how the texts are passed for annotating the net names
      m_text_annot_name_id = std::pair<bool, geo::property_names_id_type> (false, 0);
      if (! dss.text_property_name ().is_nil ()) {
        m_text_annot_name_id = mp_layout->properties_repository ().get_id_of_name (dss.text_property_name ());
      }
    }

    void insert_transformed (Shapes &into, const Shapes &from, const ICplxTrans &trans, PropertyMapper &pm) const
    {
      if (! m_text_annot_name_id.first) {

        //  fast shortcut
        into.insert_transformed (from, trans, pm);

      } else {

        for (geo::Shapes::shape_iterator i = from.begin (geo::ShapeIterator::All); ! i.at_end (); ++i) {

          bool is_text = false;

          if (i->prop_id () > 0) {

            const geo::PropertiesRepository::properties_set &ps = mp_layout->properties_repository ().properties (i->prop_id ());

            for (geo::PropertiesRepository::properties_set::const_iterator j = ps.begin (); j != ps.end () && ! is_text; ++j) {
              if (j->first == m_text_annot_name_id.second) {

                geo::Text text (j->second.to_string (), geo::Trans (i->bbox ().center () - geo::Point ()));
                text.transform (trans);
                if (into.layout ()) {
                  into.insert (geo::TextRef (text, into.layout ()->shape_repository ()));
                } else {
                  into.insert (text);
                }

                is_text = true;

              }
            }

          }

          if (! is_text) {
            into.insert (*i, trans, pm);
          }

        }

      }

    }

  private:
    std::pair<bool, geo::property_names_id_type> m_text_annot_name_id;
    const geo::Layout *mp_layout;
  };
}

void
DeepShapeStore::insert (const DeepLayer &deep_layer, geo::Layout *into_layout, geo::cell_index_type into_cell, unsigned int into_layer)
{
  geo::LayoutLocker locker (into_layout);

  const geo::Layout &source_layout = deep_layer.layout ();
  if (source_layout.begin_top_down () == source_layout.end_top_cells ()) {
    //  empty source - nothing to do.
    return;
  }

  //  prepare the transformation
  geo::ICplxTrans trans (source_layout.dbu () / into_layout->dbu ());

  //  prepare a layer map
  std::map<unsigned int, unsigned int> lm;
  lm.insert (std::make_pair (deep_layer.layer (), into_layer));

  //  prepare a cell mapping
  const geo::CellMapping &cm = cell_mapping_to_original (deep_layer.layout_index (), into_layout, into_cell);

  //  prepare a vector with the source cells
  std::vector <geo::cell_index_type> source_cells;
  source_cells.push_back (*source_layout.begin_top_down());

  //  prepare a transformer to convert text-annotated markers back to texts (without transformation however)
  DeepShapeStoreToShapeTransformer dsst (*this, source_layout);

  //  actually copy the shapes
  geo::copy_shapes (*into_layout, source_layout, trans, source_cells, cm.table (), lm, &dsst);
}

void
DeepShapeStore::insert_as_polygons (const DeepLayer &deep_layer, geo::Layout *into_layout, geo::cell_index_type into_cell, unsigned int into_layer, geo::Coord enl)
{
  //  prepare a temporary layer with the polygons
  DeepLayer tmp = deep_layer.derived ();

  geo::Layout &layout = const_cast<geo::Layout &> (deep_layer.layout ());

  for (geo::Layout::iterator c = layout.begin (); c != layout.end (); ++c) {

    geo::Shapes &out = c->shapes (tmp.layer ());
    for (geo::Shapes::shape_iterator s = c->shapes (deep_layer.layer ()); ! s.at_end (); ++s) {

      if (s->is_edge_pair ()) {

        out.insert (s->edge_pair ().normalized ().to_simple_polygon (enl));

      } else if (s->is_path () || s->is_polygon () || s->is_box ()) {

        geo::Polygon poly;
        s->polygon (poly);
        out.insert (poly);

      } else if (s->is_text ()) {

        geo::Text t;
        s->text (t);
        out.insert (geo::SimplePolygon (t.box ().enlarged (geo::Vector (enl, enl))));

      }

    }

  }

  //  and insert this one
  insert (tmp, into_layout, into_cell, into_layer);
}

}

