#include "geoHierNetworkProcessor.h"
#include "geoShape.h"
#include "geoShapes.h"
#include "geoInstElement.h"
#include "geoPolygon.h"
#include "geoPolygonTools.h"
#include "geoBoxScanner.h"
#include "geoDeepRegion.h"
#include "geoNetShape.h"
#include "utilsProgress.h"
#include "utilsLog.h"
#include "utilsTimer.h"

#include <vector>
#include <map>
#include <list>
#include <set>

namespace geo
{

// ------------------------------------------------------------------------------

//  Don't cache instance to instance interaction sets beyond this size
const size_t instance_to_instance_cache_set_size_threshold = 10000;

// ------------------------------------------------------------------------------

template <class Shape, class Trans> void insert_transformed (geo::Layout &layout, geo::Shapes &shapes, const Shape &s, const Trans &t);

template <class Trans> void insert_transformed (geo::Layout &layout, geo::Shapes &shapes, const geo::PolygonRef &s, const Trans &t)
{
  geo::Polygon poly = s.obj ();
  poly.transform (s.trans ());
  if (! t.is_unity ()) {
    poly.transform (t);
  }
  shapes.insert (geo::PolygonRef (poly, layout.shape_repository ()));
}

template <class Trans> void insert_transformed (geo::Layout &layout, geo::Shapes &shapes, const geo::NetShape &s, const Trans &t)
{
  if (s.type () == geo::NetShape::Polygon) {

    geo::PolygonRef pr = s.polygon_ref ();
    geo::Polygon poly = pr.obj ();
    poly.transform (pr.trans ());
    if (! t.is_unity ()) {
      poly.transform (t);
    }
    shapes.insert (geo::PolygonRef (poly, layout.shape_repository ()));

  } else if (s.type () == geo::NetShape::Text) {

    geo::TextRef tr = s.text_ref ();
    geo::Text text = tr.obj ();
    text.transform (tr.trans ());
    if (! t.is_unity ()) {
      text.transform (t);
    }
    shapes.insert (geo::TextRef (text, layout.shape_repository ()));

  }
}

template <class Trans> void insert_transformed (geo::Layout & /*layout*/, geo::Shapes &shapes, const geo::Edge &s, const Trans &t)
{
  shapes.insert (s.transformed (t));
}

// ------------------------------------------------------------------------------

static bool is_breakout_cell (const std::set<geo::cell_index_type> *breakout_cells, geo::cell_index_type ci)
{
  return breakout_cells && breakout_cells->find (ci) != breakout_cells->end ();
}

// ------------------------------------------------------------------------------
//  Connectivity implementation

// -----------------------------------------------------------------------------
//  HierNetworkProcessor implementation

Connectivity::Connectivity ()
  : m_ec (Connectivity::EdgesConnectCollinear)
{
  //  .. nothing yet ..
}

Connectivity::Connectivity (edge_connectivity_type ec)
  : m_ec (ec)
{
  //  .. nothing yet ..
}

void
Connectivity::connect (unsigned int la, unsigned int lb)
{
  m_connected [la].insert (lb);
  m_connected [lb].insert (la);
  m_all_layers.insert (la);
  m_all_layers.insert (lb);
}

void
Connectivity::connect (unsigned int l)
{
  m_connected [l].insert (l);
  m_all_layers.insert (l);
}

void
Connectivity::connect (const geo::DeepLayer &l)
{
  connect (l.layer ());
}

void
Connectivity::connect (const geo::DeepLayer &la, const geo::DeepLayer &lb)
{
  connect (la.layer (), lb.layer ());
}

Connectivity::global_nets_type s_empty_global_nets;

Connectivity::global_nets_iterator
Connectivity::begin_global_connections (unsigned int l) const
{
  std::map<unsigned int, global_nets_type>::const_iterator g = m_global_connections.find (l);
  if (g != m_global_connections.end ()) {
    return g->second.begin ();
  } else {
    return s_empty_global_nets.begin ();
  }
}

Connectivity::global_nets_iterator
Connectivity::end_global_connections (unsigned int l) const
{
  std::map<unsigned int, global_nets_type>::const_iterator g = m_global_connections.find (l);
  if (g != m_global_connections.end ()) {
    return g->second.end ();
  } else {
    return s_empty_global_nets.end ();
  }
}

size_t
Connectivity::connect_global (unsigned int l, const std::string &gn)
{
  size_t id = global_net_id (gn);
  m_global_connections [l].insert (id);
  m_all_layers.insert (l);
  return id;
}

size_t
Connectivity::connect_global (const geo::DeepLayer &l, const std::string &gn)
{
  return connect_global (l.layer (), gn);
}

const std::string &
Connectivity::global_net_name (size_t id) const
{
  tl_assert (id < m_global_net_names.size ());
  return m_global_net_names [id];
}

size_t
Connectivity::global_net_id (const std::string &gn)
{
  for (std::vector<std::string>::const_iterator i = m_global_net_names.begin (); i != m_global_net_names.end (); ++i) {
    if (*i == gn) {
      size_t id = i - m_global_net_names.begin ();
      return id;
    }
  }

  size_t id = m_global_net_names.size ();
  m_global_net_names.push_back (gn);
  return id;
}

size_t
Connectivity::global_nets () const
{
  return m_global_net_names.size ();
}

Connectivity::layer_iterator
Connectivity::begin_layers () const
{
  return m_all_layers.begin ();
}

Connectivity::layer_iterator
Connectivity::end_layers () const
{
  return m_all_layers.end ();
}

Connectivity::layers_type s_empty_layers;

Connectivity::layer_iterator
Connectivity::begin_connected (unsigned int layer) const
{
  std::map<unsigned int, layers_type>::const_iterator i = m_connected.find (layer);
  if (i == m_connected.end ()) {
    return s_empty_layers.begin ();
  } else {
    return i->second.begin ();
  }
}

Connectivity::layer_iterator
Connectivity::end_connected (unsigned int layer) const
{
  std::map<unsigned int, layers_type>::const_iterator i = m_connected.find (layer);
  if (i == m_connected.end ()) {
    return s_empty_layers.end ();
  } else {
    return i->second.end ();
  }
}

template <class Trans>
static bool
interaction_test (const geo::PolygonRef &a, const geo::PolygonRef &b, const Trans &trans, geo::Connectivity::edge_connectivity_type)
{
  //  TODO: this could be part of geo::interact (including transformation)
  if (a.obj ().is_box () && b.obj ().is_box ()) {
    return geo::interact (a.obj ().box ().transformed (a.trans ()), b.obj ().box ().transformed (trans * Trans (b.trans ())));
  } else {
    return geo::interact (a.obj ().transformed (a.trans ()), b.obj ().transformed (trans * Trans (b.trans ())));
  }
}

template <class C>
static bool
interaction_test (const geo::PolygonRef &a, const geo::PolygonRef &b, const geo::unit_trans<C> &, geo::Connectivity::edge_connectivity_type)
{
  //  TODO: this could be part of geo::interact (including transformation)
  if (a.obj ().is_box () && b.obj ().is_box ()) {
    return geo::interact (a.obj ().box ().transformed (a.trans ()), b.obj ().box ().transformed (b.trans ()));
  } else {
    return geo::interact (a.obj ().transformed (a.trans ()), b.obj ().transformed (b.trans ()));
  }
}

template <class Trans>
static bool
interaction_test (const geo::NetShape &a, const geo::NetShape &b, const Trans &trans, geo::Connectivity::edge_connectivity_type)
{
  return a.interacts_with_transformed (b, trans);
}

template <class C>
static bool
interaction_test (const geo::NetShape &a, const geo::NetShape &b, const geo::unit_trans<C> &, geo::Connectivity::edge_connectivity_type)
{
  return a.interacts_with (b);
}

template <class Trans>
static bool
interaction_test (const geo::Edge &a, const geo::Edge &b, const Trans &trans, geo::Connectivity::edge_connectivity_type ec)
{
  geo::Edge bt = b.transformed (trans);
  if (ec == geo::Connectivity::EdgesConnectByPoints) {
    return a.p2 () == bt.p1 () || a.p1 () == bt.p2 ();
  } else {
    return a.parallel (bt) && a.intersect (bt);
  }
}

template <class C>
static bool
interaction_test (const geo::Edge &a, const geo::Edge &b, const geo::unit_trans<C> &, geo::Connectivity::edge_connectivity_type ec)
{
  if (ec == geo::Connectivity::EdgesConnectByPoints) {
    return a.p2 () == b.p1 () || a.p1 () == b.p2 ();
  } else {
    return a.parallel (b) && a.intersect (b);
  }
}

template <class T, class Trans>
bool Connectivity::interacts (const T &a, unsigned int la, const T &b, unsigned int lb, const Trans &trans) const
{
  std::map<unsigned int, layers_type>::const_iterator i = m_connected.find (la);
  if (i == m_connected.end () || i->second.find (lb) == i->second.end ()) {
    return false;
  } else {
    return interaction_test (a, b, trans, m_ec);
  }
}

bool Connectivity::interacts (const std::set<unsigned int> &la, const std::set<unsigned int> &lb) const
{
  for (std::set<unsigned int>::const_iterator i = la.begin (); i != la.end (); ++i) {
    geo::Connectivity::layer_iterator je = end_connected (*i);
    for (geo::Connectivity::layer_iterator j = begin_connected (*i); j != je; ++j) {
      if (lb.find (*j) != lb.end ()) {
        return true;
      }
    }
  }

  return false;
}

bool Connectivity::interact (const geo::Cell &a, const geo::Cell &b) const
{
  for (std::map<unsigned int, layers_type>::const_iterator i = m_connected.begin (); i != m_connected.end (); ++i) {
    if (! a.bbox (i->first).empty ()) {
      for (layers_type::const_iterator j = i->second.begin (); j != i->second.end (); ++j) {
        if (! b.bbox (*j).empty ()) {
          return true;
        }
      }
    }
  }

  return false;
}

template <class T>
bool Connectivity::interact (const geo::Cell &a, const T &ta, const geo::Cell &b, const T &tb) const
{
  for (std::map<unsigned int, layers_type>::const_iterator i = m_connected.begin (); i != m_connected.end (); ++i) {
    geo::Box ba = a.bbox (i->first);
    if (! ba.empty ()) {
      ba.transform (ta);
      for (layers_type::const_iterator j = i->second.begin (); j != i->second.end (); ++j) {
        geo::Box bb = b.bbox (*j);
        if (! bb.empty () && bb.transformed (tb).touches (ba)) {
          return true;
        }
      }
    }
  }

  return false;
}

//  explicit instantiations
template DB_PUBLIC bool Connectivity::interacts<geo::NetShape> (const geo::NetShape &a, unsigned int la, const geo::NetShape &b, unsigned int lb, const geo::UnitTrans &trans) const;
template DB_PUBLIC bool Connectivity::interacts<geo::NetShape> (const geo::NetShape &a, unsigned int la, const geo::NetShape &b, unsigned int lb, const geo::ICplxTrans &trans) const;
template DB_PUBLIC bool Connectivity::interacts<geo::PolygonRef> (const geo::PolygonRef &a, unsigned int la, const geo::PolygonRef &b, unsigned int lb, const geo::UnitTrans &trans) const;
template DB_PUBLIC bool Connectivity::interacts<geo::PolygonRef> (const geo::PolygonRef &a, unsigned int la, const geo::PolygonRef &b, unsigned int lb, const geo::ICplxTrans &trans) const;
template DB_PUBLIC bool Connectivity::interacts<geo::Edge> (const geo::Edge &a, unsigned int la, const geo::Edge &b, unsigned int lb, const geo::UnitTrans &trans) const;
template DB_PUBLIC bool Connectivity::interacts<geo::Edge> (const geo::Edge &a, unsigned int la, const geo::Edge &b, unsigned int lb, const geo::ICplxTrans &trans) const;
template DB_PUBLIC bool Connectivity::interact<geo::ICplxTrans> (const geo::Cell &a, const geo::ICplxTrans &ta, const geo::Cell &b, const geo::ICplxTrans &tb) const;

// ------------------------------------------------------------------------------
//  local_cluster implementation

template <class T>
local_cluster<T>::local_cluster (size_t id)
  : m_id (id), m_needs_update (false), m_size (0)
{
  //  .. nothing yet ..
}

template <class T>
void
local_cluster<T>::clear ()
{
  m_shapes.clear ();
  m_needs_update = false;
  m_size = 0;
  m_bbox = box_type ();
  m_attrs.clear ();
  m_global_nets.clear ();
}

template <class T>
bool
local_cluster<T>::empty () const
{
  return m_global_nets.empty () && m_shapes.empty ();
}

template <class T>
void
local_cluster<T>::set_global_nets (const global_nets &gn)
{
  m_global_nets = gn;
}

template <class T>
void
local_cluster<T>::add_attr (attr_id a)
{
  if (a > 0) {
    m_attrs.insert (a);
  }
}

template <class T>
void
local_cluster<T>::add (const T &s, unsigned int la)
{
  m_shapes[la].insert (s);
  m_needs_update = true;
  ++m_size;
}

template <class T>
void
local_cluster<T>::join_with (const local_cluster<T> &other)
{
  for (typename std::map<unsigned int, tree_type>::const_iterator s = other.m_shapes.begin (); s != other.m_shapes.end (); ++s) {
    tree_type &tree = m_shapes[s->first];
    tree.insert (s->second.begin (), s->second.end ());
  }

  m_attrs.insert (other.m_attrs.begin (), other.m_attrs.end ());
  m_global_nets.insert (other.m_global_nets.begin (), other.m_global_nets.end ());
  m_size += other.size ();

  m_needs_update = true;
}

template <class T>
void
local_cluster<T>::ensure_sorted ()
{
  if (! m_needs_update) {
    return;
  }

  //  sort the shape trees
  for (typename std::map<unsigned int, tree_type>::iterator s = m_shapes.begin (); s != m_shapes.end (); ++s) {
    s->second.sort (geo::box_convert<T> ());
  }

  //  recompute bounding box
  m_bbox = box_type ();
  geo::box_convert<T> bc;
  for (typename std::map<unsigned int, tree_type>::const_iterator s = m_shapes.begin (); s != m_shapes.end (); ++s) {
    for (typename tree_type::const_iterator i = s->second.begin (); i != s->second.end (); ++i) {
      m_bbox += bc (*i);
    }
  }

  m_needs_update = false;
}

template <class T>
void
local_cluster<T>::mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, bool no_self, void *parent) const
{
  if (! no_self) {
    stat->add (typeid (*this), (void *) this, sizeof (*this), sizeof (*this), parent, purpose, cat);
  }

  geo::mem_stat (stat, purpose, cat, m_shapes, true, (void *) this);
  geo::mem_stat (stat, purpose, cat, m_attrs, true, (void *) this);
  geo::mem_stat (stat, purpose, cat, m_global_nets, true, (void *) this);
}

template <class T>
class DB_PUBLIC hnp_interaction_receiver
  : public box_scanner_receiver2<T, unsigned int, T, unsigned int>
{
public:
  typedef typename local_cluster<T>::box_type box_type;

  hnp_interaction_receiver (const Connectivity &conn, const geo::ICplxTrans &trans)
    : mp_conn (&conn), m_any (false), m_trans (trans)
  {
    //  .. nothing yet ..
  }

  void add (const T *s1, unsigned int l1, const T *s2, unsigned int l2)
  {
    if (mp_conn->interacts (*s1, l1, *s2, l2, m_trans)) {
      m_any = true;
    }
  }

  bool stop () const
  {
    return m_any;
  }

private:
  const Connectivity *mp_conn;
  bool m_any;
  geo::ICplxTrans m_trans;
};

template <class T, class Trans>
struct transformed_box
{
  typedef geo::box_convert<T> base_bc;
  typedef typename T::box_type box_type;

  transformed_box (const Trans &trans)
    : m_trans (trans)
  {
    //  .. nothing yet ..
  }

  box_type operator() (const T &t) const
  {
    return m_bc (t).transformed (m_trans);
  }

private:
  base_bc m_bc;
  Trans m_trans;
};

template <class T>
typename local_cluster<T>::shape_iterator local_cluster<T>::begin (unsigned int l) const
{
  static tree_type s_empty_tree;

  typename std::map<unsigned int, tree_type>::const_iterator i = m_shapes.find (l);
  if (i == m_shapes.end ()) {
    return s_empty_tree.begin_flat ();
  } else {
    return i->second.begin_flat ();
  }
}

template <class T>
bool
local_cluster<T>::interacts (const geo::Cell &cell, const geo::ICplxTrans &trans, const Connectivity &conn) const
{
  geo::box_convert<T> bc;

  for (typename std::map<unsigned int, tree_type>::const_iterator s = m_shapes.begin (); s != m_shapes.end (); ++s) {

    geo::Box box;

    Connectivity::layer_iterator le = conn.end_connected (s->first);
    for (Connectivity::layer_iterator l = conn.begin_connected (s->first); l != le; ++l) {
      box += cell.bbox (*l);
    }

    if (! box.empty () && ! s->second.begin_touching (box.transformed (trans), bc).at_end ()) {
      return true;
    }

  }

  return false;
}

template <class T>
bool
local_cluster<T>::interacts (const local_cluster<T> &other, const geo::ICplxTrans &trans, const Connectivity &conn) const
{
  geo::box_convert<T> bc;

  const_cast<local_cluster<T> *> (this)->ensure_sorted ();

  box_type common = other.bbox ().transformed (trans) & bbox ();
  if (common.empty ()) {
    return false;
  }

  box_type common_for_other = common.transformed (trans.inverted ());

  //  shortcut evaluation for disjunct layers

  std::set<unsigned int> ll1;
  for (typename std::map<unsigned int, tree_type>::const_iterator s = m_shapes.begin (); s != m_shapes.end (); ++s) {
    if (! s->second.begin_touching (common, bc).at_end ()) {
      ll1.insert (s->first);
    }
  }

  if (ll1.empty ()) {
    return false;
  }

  std::set<unsigned int> ll2;
  for (typename std::map<unsigned int, tree_type>::const_iterator s = other.m_shapes.begin (); s != other.m_shapes.end (); ++s) {
    if (! s->second.begin_touching (common_for_other, bc).at_end ()) {
      ll2.insert (s->first);
    }
  }

  if (ll2.empty ()) {
    return false;
  }

  if (! conn.interacts (ll1, ll2)) {
    return false;
  }

  //  detailed analysis

  geo::box_scanner2<T, unsigned int, T, unsigned int> scanner;
  transformed_box <T, geo::ICplxTrans> bc_t (trans);

  for (typename std::map<unsigned int, tree_type>::const_iterator s = m_shapes.begin (); s != m_shapes.end (); ++s) {
    for (typename tree_type::touching_iterator i = s->second.begin_touching (common, bc); ! i.at_end (); ++i) {
      scanner.insert1 (i.operator-> (), s->first);
    }
  }

  for (typename std::map<unsigned int, tree_type>::const_iterator s = other.m_shapes.begin (); s != other.m_shapes.end (); ++s) {
    for (typename tree_type::touching_iterator i = s->second.begin_touching (common_for_other, bc); ! i.at_end (); ++i) {
      scanner.insert2 (i.operator-> (), s->first);
    }
  }

  hnp_interaction_receiver<T> rec (conn, trans);
  return ! scanner.process (rec, 1 /*==touching*/, bc, bc_t);
}

template <class T>
double local_cluster<T>::area_ratio () const
{
  box_type bx = bbox ();
  if (bx.empty ()) {
    return 0.0;
  }

  geo::box_convert<T> bc;

  //  just the sum of the areas of the bounding boxes - this is precise if no overlaps happen and the
  //  polygons are rather rectangular. This criterion is coarse enough to prevent recursion in the split
  //  algorithm and still be fine enough - consider that we a planning to use splitted polygons for
  //  which the bbox is a fairly good approximation.
  typename box_type::area_type a = 0;
  for (typename std::map<unsigned int, tree_type>::const_iterator s = m_shapes.begin (); s != m_shapes.end (); ++s) {
    for (typename tree_type::const_iterator i = s->second.begin (); i != s->second.end (); ++i) {
      a += bc (*i).area ();
    }
  }

  return (a == 0 ? 0.0 : double (bx.area ()) / double (a));
}

template <class T>
std::vector<unsigned int>
local_cluster<T>::layers () const
{
  std::vector<unsigned int> l;
  l.reserve (m_shapes.size ());
  for (typename std::map<unsigned int, tree_type>::const_iterator s = m_shapes.begin (); s != m_shapes.end (); ++s) {
    l.push_back (s->first);
  }
  return l;
}

template <class T, class Iter>
size_t split_cluster (const local_cluster<T> &cl, double max_area_ratio, Iter &output)
{
  if (cl.area_ratio () < max_area_ratio) {
    return 0;  //  no splitting happened
  }

  geo::box_convert<T> bc;
  typename local_cluster<T>::box_type bx = cl.bbox ();

  int xthr = bx.width () > bx.height () ? bx.center ().x () : bx.left ();
  int ythr = bx.width () > bx.height () ? bx.bottom () : bx.center ().y ();

  //  split along the longer axis - decide the position according to the bbox center

  local_cluster<T> a (cl.id ()), b (cl.id ());

  std::vector<unsigned int> layers = cl.layers ();

  for (std::vector<unsigned int>::const_iterator l = layers.begin (); l != layers.end (); ++l) {
    for (typename local_cluster<T>::shape_iterator s = cl.begin (*l); ! s.at_end (); ++s) {
      typename local_cluster<T>::box_type::point_type sc = bc (*s).center ();
      if (sc.x () < xthr || sc.y () < ythr) {
        a.add (*s, *l);
      } else {
        b.add (*s, *l);
      }
    }
  }

  if (a.size () == 0 || b.size () == 0) {
    //  give up to prevent infinite recursion
    return 0;
  }

  //  split further if required
  size_t na = split_cluster (a, max_area_ratio, output);
  size_t nb = split_cluster (b, max_area_ratio, output);

  if (na == 0) {
    *output++ = a;
    na = 1;
  }

  if (nb == 0) {
    *output++ = b;
    nb = 1;
  }

  return na + nb;
}

template <class T>
template <class Iter>
size_t local_cluster<T>::split (double max_area_ratio, Iter &output) const
{
  return split_cluster (*this, max_area_ratio, output);
}

//  explicit instantiations
template class DB_PUBLIC local_cluster<geo::NetShape>;
template class DB_PUBLIC local_cluster<geo::PolygonRef>;
template class DB_PUBLIC local_cluster<geo::Edge>;
template DB_PUBLIC size_t local_cluster<geo::NetShape>::split<std::back_insert_iterator<std::list<local_cluster<geo::NetShape> > > > (double, std::back_insert_iterator<std::list<local_cluster<geo::NetShape> > > &) const;
template DB_PUBLIC size_t local_cluster<geo::PolygonRef>::split<std::back_insert_iterator<std::list<local_cluster<geo::PolygonRef> > > > (double, std::back_insert_iterator<std::list<local_cluster<geo::PolygonRef> > > &) const;
template DB_PUBLIC size_t local_cluster<geo::Edge>::split<std::back_insert_iterator<std::list<local_cluster<geo::Edge> > > > (double, std::back_insert_iterator<std::list<local_cluster<geo::Edge> > > &) const;

// ------------------------------------------------------------------------------
//  local_clusters implementation

template <class T>
local_clusters<T>::local_clusters ()
  : m_needs_update (false), m_next_dummy_id (0)
{
  //  .. nothing yet ..
}

template <class T>
void local_clusters<T>::clear ()
{
  m_needs_update = false;
  m_clusters.clear ();
  m_bbox = box_type ();
  m_next_dummy_id = 0;
}

template <class T>
const local_cluster<T> &
local_clusters<T>::cluster_by_id (typename local_cluster<T>::id_type id) const
{
  tl_assert (id > 0);

  if (id > m_clusters.size ()) {

    //  dummy connectors are not real ones - they just carry an arbitrary
    //  ID. Still they need to be treated as empty ones.
    static local_cluster<T> empty_cluster;
    return empty_cluster;

  } else {

    //  by convention the ID is the index + 1 so 0 can be used as "nil"
    return m_clusters.objects ().item (id - 1);

  }
}

template <class T>
void
local_clusters<T>::remove_cluster (typename local_cluster<T>::id_type id)
{
  if (id == 0 || id > m_clusters.size ()) {
    return;
  }

  //  TODO: this const_cast is required. But we know what we're doing ...
  //  NOTE: we cannot really delete a cluster as this would shift the indexes. So
  //  we just clear them.
  local_cluster<T> *to_delete = const_cast<local_cluster<T> *> (& m_clusters.objects ().item (id - 1));
  to_delete->clear ();
  m_needs_update = true;
}

template <class T>
void
local_clusters<T>::join_cluster_with (typename local_cluster<T>::id_type id, typename local_cluster<T>::id_type with_id)
{
  tl_assert (id > 0);
  if (with_id == 0 || with_id > m_clusters.size () || id > m_clusters.size ()) {
    return;
  }

  //  TODO: this const_cast is required. But we know what we're doing ...
  local_cluster<T> *with = const_cast<local_cluster<T> *> (& m_clusters.objects ().item (with_id - 1));
  local_cluster<T> *first = const_cast<local_cluster<T> *> (& m_clusters.objects ().item (id - 1));
  first->join_with (*with);

  //  NOTE: we cannot really delete a cluster as this would shift the indexes. So
  //  we just clear them.
  with->clear ();

  m_needs_update = true;
}

template <class T>
local_cluster<T> *
local_clusters<T>::insert ()
{
  typename tree_type::iterator i = m_clusters.insert (local_cluster<T> ());
  i->set_id (i.index () + 1);
  m_needs_update = true;
  return i.operator-> ();
}

template <class T>
void
local_clusters<T>::ensure_sorted ()
{
  if (! m_needs_update) {
    return;
  }

  //  sort the shape trees
  m_clusters.sort (local_cluster_box_convert<T> ());

  //  recompute bounding box
  m_bbox = box_type ();
  for (typename tree_type::const_iterator i = m_clusters.begin (); i != m_clusters.end (); ++i) {
    m_bbox += i->bbox ();
  }

  m_needs_update = false;
}

template <class T>
void
local_clusters<T>::mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, bool no_self, void *parent) const
{
  if (! no_self) {
    stat->add (typeid (*this), (void *) this, sizeof (*this), sizeof (*this), parent, purpose, cat);
  }

  geo::mem_stat (stat, purpose, cat, m_needs_update, true, (void *) this);
  geo::mem_stat (stat, purpose, cat, m_bbox, true, (void *) this);
  geo::mem_stat (stat, purpose, cat, m_clusters, true, (void *) this);
  geo::mem_stat (stat, purpose, cat, m_next_dummy_id, true, (void *) this);
}

namespace
{

template <class T, class BoxTree>
struct cluster_building_receiver
  : public geo::box_scanner_receiver<T, std::pair<unsigned int, size_t> >
{
  typedef typename local_cluster<T>::id_type id_type;
  typedef std::pair<const T *, std::pair<unsigned int, size_t> > shape_value;
  typedef std::vector<shape_value> shape_vector;
  typedef std::set<size_t> global_nets;
  typedef std::pair<shape_vector, global_nets> cluster_value;

  cluster_building_receiver (const geo::Connectivity &conn, const geo::equivalence_clusters<size_t> *attr_equivalence)
    : mp_conn (&conn), mp_attr_equivalence (attr_equivalence)
  {
    if (mp_attr_equivalence) {
      //  cache the global nets per attribute equivalence cluster
      for (size_t gid = 0; gid < conn.global_nets (); ++gid) {
        geo::equivalence_clusters<size_t>::cluster_id_type cl = attr_equivalence->cluster_id (global_net_id_to_attr (gid));
        if (cl > 0) {
          m_global_nets_by_attribute_cluster [cl].insert (gid);
        }
      }
    }
  }

  void generate_clusters (local_clusters<T> &clusters)
  {
    //  build the resulting clusters
    for (typename std::list<cluster_value>::const_iterator c = m_clusters.begin (); c != m_clusters.end (); ++c) {

      //  TODO: reserve?
      local_cluster<T> *cluster = clusters.insert ();
      for (typename shape_vector::const_iterator s = c->first.begin (); s != c->first.end (); ++s) {
        cluster->add (*s->first, s->second.first);
        cluster->add_attr (s->second.second);
      }

      std::set<size_t> global_nets = c->second;

      //  Add the global nets we derive from the attribute equivalence (join_nets of labelled vs.
      //  global nets)
      if (mp_attr_equivalence) {

        for (typename shape_vector::const_iterator s = c->first.begin (); s != c->first.end (); ++s) {

          size_t a = s->second.second;
          if (a == 0) {
            continue;
          }

          geo::equivalence_clusters<size_t>::cluster_id_type cl = mp_attr_equivalence->cluster_id (a);
          if (cl > 0) {
            std::map<size_t, std::set<size_t> >::const_iterator gn = m_global_nets_by_attribute_cluster.find (cl);
            if (gn != m_global_nets_by_attribute_cluster.end ()) {
              global_nets.insert (gn->second.begin (), gn->second.end ());
            }
          }

        }

      }

      cluster->set_global_nets (global_nets);

    }
  }

  void add (const T *s1, std::pair<unsigned int, size_t> p1, const T *s2, std::pair<unsigned int, size_t> p2)
  {
    if (! mp_conn->interacts (*s1, p1.first, *s2, p2.first)) {
      return;
    }

    typename std::map<const T *, typename std::list<cluster_value>::iterator>::iterator ic1 = m_shape_to_clusters.find (s1);
    typename std::map<const T *, typename std::list<cluster_value>::iterator>::iterator ic2 = m_shape_to_clusters.find (s2);

    if (ic1 == m_shape_to_clusters.end ()) {

      if (ic2 == m_shape_to_clusters.end ()) {

        m_clusters.push_back (cluster_value ());
        typename std::list<cluster_value>::iterator c = --m_clusters.end ();
        c->first.push_back (std::make_pair (s1, p1));
        c->first.push_back (std::make_pair (s2, p2));

        m_shape_to_clusters.insert (std::make_pair (s1, c));
        m_shape_to_clusters.insert (std::make_pair (s2, c));

      } else {

        ic2->second->first.push_back (std::make_pair (s1, p1));
        m_shape_to_clusters.insert (std::make_pair (s1, ic2->second));

      }

    } else if (ic2 == m_shape_to_clusters.end ()) {

      ic1->second->first.push_back (std::make_pair (s2, p2));
      m_shape_to_clusters.insert (std::make_pair (s2, ic1->second));

    } else if (ic1->second != ic2->second) {

      //  join clusters: use the larger one as the target

      if (ic1->second->first.size () < ic2->second->first.size ()) {
        join (ic2->second, ic1->second);
      } else {
        join (ic1->second, ic2->second);
      }

    }
  }

  void finish (const T *s, std::pair<unsigned int, size_t> p)
  {
    //  if the shape has not been handled yet, insert a single cluster with only this shape
    typename std::map<const T *, typename std::list<cluster_value>::iterator>::iterator ic = m_shape_to_clusters.find (s);
    if (ic == m_shape_to_clusters.end ()) {

      m_clusters.push_back (cluster_value ());
      typename std::list<cluster_value>::iterator c = --m_clusters.end ();
      c->first.push_back (std::make_pair (s, p));

      ic = m_shape_to_clusters.insert (std::make_pair (s, c)).first;

    }

    //  consider connections to global nets

    geo::Connectivity::global_nets_iterator ge = mp_conn->end_global_connections (p.first);
    for (geo::Connectivity::global_nets_iterator g = mp_conn->begin_global_connections (p.first); g != ge; ++g) {

      typename std::map<size_t, typename std::list<cluster_value>::iterator>::iterator icg = m_global_to_clusters.find (*g);

      if (icg == m_global_to_clusters.end ()) {

        ic->second->second.insert (*g);
        m_global_to_clusters.insert (std::make_pair (*g, ic->second));

      } else if (ic->second != icg->second) {

        //  join clusters
        if (ic->second->first.size () < icg->second->first.size ()) {
          join (icg->second, ic->second);
        } else {
          join (ic->second, icg->second);
        }

      }

    }
  }

private:
  const geo::Connectivity *mp_conn;
  std::map<const T *, typename std::list<cluster_value>::iterator> m_shape_to_clusters;
  std::map<size_t, typename std::list<cluster_value>::iterator> m_global_to_clusters;
  std::list<cluster_value> m_clusters;
  std::map<size_t, std::set<size_t> > m_global_nets_by_attribute_cluster;
  const geo::equivalence_clusters<size_t> *mp_attr_equivalence;

  void join (typename std::list<cluster_value>::iterator ic1, typename std::list<cluster_value>::iterator ic2)
  {
    ic1->first.insert (ic1->first.end (), ic2->first.begin (), ic2->first.end ());
    ic1->second.insert (ic2->second.begin (), ic2->second.end ());

    for (typename shape_vector::const_iterator i = ic2->first.begin (); i != ic2->first.end (); ++i) {
      m_shape_to_clusters [i->first] = ic1;
    }
    for (typename global_nets::const_iterator i = ic2->second.begin (); i != ic2->second.end (); ++i) {
      m_global_to_clusters [*i] = ic1;
    }

    m_clusters.erase (ic2);
  }
};

template <class T>
struct addressable_object_from_shape
{
  const T *operator () (const geo::Shape &shape)
  {
    typename T::tag object_tag;
    return shape.basic_ptr (object_tag);
  }
};

template <>
struct addressable_object_from_shape<geo::NetShape>
{
  const NetShape *operator () (const geo::Shape &shape)
  {
    if (shape.type () == geo::Shape::TextRef) {
      m_heap.push_back (geo::NetShape (shape.text_ref ()));
      return &m_heap.back ();
    } else if (shape.type () == geo::Shape::PolygonRef) {
      m_heap.push_back (geo::NetShape (shape.polygon_ref ()));
      return &m_heap.back ();
    } else {
      tl_assert (false);
    }
  }

private:
  std::list<NetShape> m_heap;
};

template <class T>
struct attr_accessor
{
  size_t operator() (const geo::Shape &shape) const
  {
    return shape.prop_id ();
  }
};

template <>
struct attr_accessor<geo::NetShape>
{
  size_t operator() (const geo::Shape &shape) const
  {
    //  NOTE: the attribute is
    //   * odd: a StringRef pointer's value
    //   * even: a Property ID times 2
    if (shape.type () == geo::Shape::TextRef) {
      return geo::text_ref_to_attr (&shape.text_ref ().obj ());
    } else {
      return geo::prop_id_to_attr (shape.prop_id ());
    }
  }
};

template <class T> struct get_shape_flags { };

template <>
struct get_shape_flags<geo::Edge>
{
  geo::ShapeIterator::flags_type operator() () const
  {
    return geo::ShapeIterator::Edges;
  }
};

template <>
struct get_shape_flags<geo::PolygonRef>
{
  geo::ShapeIterator::flags_type operator() () const
  {
    return geo::ShapeIterator::Polygons;
  }
};

template <>
struct get_shape_flags<geo::NetShape>
{
  geo::ShapeIterator::flags_type operator() () const
  {
    return geo::ShapeIterator::flags_type (geo::ShapeIterator::Polygons | geo::ShapeIterator::Texts);
  }
};

}

template <class T>
void
local_clusters<T>::build_clusters (const geo::Cell &cell, const geo::Connectivity &conn, const geo::equivalence_clusters<size_t> *attr_equivalence, bool report_progress)
{
  static std::string desc = geo::to_string (tr ("Building local clusters"));

  geo::box_scanner<T, std::pair<unsigned int, size_t> > bs (report_progress, desc);
  geo::box_convert<T> bc;
  addressable_object_from_shape<T> heap;
  attr_accessor<T> attr;
  geo::ShapeIterator::flags_type shape_flags = get_shape_flags<T> () ();

  for (geo::Connectivity::layer_iterator l = conn.begin_layers (); l != conn.end_layers (); ++l) {
    const geo::Shapes &shapes = cell.shapes (*l);
    for (geo::Shapes::shape_iterator s = shapes.begin (shape_flags); ! s.at_end (); ++s) {
      bs.insert (heap (*s), std::make_pair (*l, attr (*s)));
    }
  }

  cluster_building_receiver<T, box_type> rec (conn, attr_equivalence);
  bs.process (rec, 1 /*==touching*/, bc);
  rec.generate_clusters (*this);

  if (attr_equivalence && attr_equivalence->size () > 0) {
    apply_attr_equivalences (*attr_equivalence);
  }
}

template <class T>
void
local_clusters<T>::apply_attr_equivalences (const geo::equivalence_clusters<size_t> &attr_equivalence)
{
  //  identify the layout clusters joined into one attribute cluster and join them

  std::map<geo::equivalence_clusters<size_t>::cluster_id_type, std::set<size_t> > c2c;

  for (const_iterator c = begin (); c != end (); ++c) {

    for (typename local_cluster<T>::attr_iterator a = c->begin_attr (); a != c->end_attr (); ++a) {
      geo::equivalence_clusters<size_t>::cluster_id_type cl = attr_equivalence.cluster_id (*a);
      if (cl > 0) {
        c2c [cl].insert (c->id ());
      }
    }

    for (typename local_cluster<T>::global_nets_iterator g = c->begin_global_nets (); g != c->end_global_nets (); ++g) {
      size_t a = global_net_id_to_attr (*g);
      geo::equivalence_clusters<size_t>::cluster_id_type cl = attr_equivalence.cluster_id (a);
      if (cl > 0) {
        c2c [cl].insert (c->id ());
      }
    }

  }

  for (std::map<geo::equivalence_clusters<size_t>::cluster_id_type, std::set<size_t> >::const_iterator c = c2c.begin (); c != c2c.end (); ++c) {
    if (c->second.size () > 1) {
      std::set<size_t>::const_iterator cl0 = c->second.begin ();
      std::set<size_t>::const_iterator cl = cl0;
      while (++cl != c->second.end ()) {
        join_cluster_with (*cl0, *cl);
      }
    }
  }
}

//  explicit instantiations
template class DB_PUBLIC local_clusters<geo::NetShape>;
template class DB_PUBLIC local_clusters<geo::PolygonRef>;
template class DB_PUBLIC local_clusters<geo::Edge>;

// ------------------------------------------------------------------------------
//  connected_clusters_iterator implementation

template <class T>
connected_clusters_iterator<T>::connected_clusters_iterator (const connected_clusters<T> &c)
  : m_lc_iter (c.begin ())
{
  size_t max_id = 0;
  for (typename connected_clusters<T>::const_iterator i = c.begin (); i != c.end (); ++i) {
    if (i->id () > max_id) {
      max_id = i->id ();
    }
  }

  m_x_iter = c.m_connections.lower_bound (max_id + 1);
  m_x_iter_end = c.m_connections.end ();
}

//  explicit instantiations
template class DB_PUBLIC connected_clusters_iterator<geo::NetShape>;
template class DB_PUBLIC connected_clusters_iterator<geo::PolygonRef>;
template class DB_PUBLIC connected_clusters_iterator<geo::Edge>;

// ------------------------------------------------------------------------------
//  connected_clusters implementation

template <class T>
const typename connected_clusters<T>::connections_type &
connected_clusters<T>::connections_for_cluster (typename local_cluster<T>::id_type id) const
{
  typename std::map<typename local_cluster<T>::id_type, connections_type>::const_iterator c = m_connections.find (id);
  if (c == m_connections.end ()) {
    static connections_type empty_connections;
    return empty_connections;
  } else {
    return c->second;
  }
}

template <class T>
void
connected_clusters<T>::add_connection (typename local_cluster<T>::id_type id, const ClusterInstance &inst)
{
  m_connections [id].push_back (inst);
  m_rev_connections [inst] = id;
}

template <class T>
void
connected_clusters<T>::join_cluster_with (typename local_cluster<T>::id_type id, typename local_cluster<T>::id_type with_id)
{
  if (id == with_id) {
    return;
  }

  local_clusters<T>::join_cluster_with (id, with_id);

  //  handle the connections by translating

  typename std::map<typename local_cluster<T>::id_type, connections_type>::iterator tc = m_connections.find (with_id);
  if (tc != m_connections.end ()) {

    connections_type &to_join = tc->second;

    for (connections_type::const_iterator c = to_join.begin (); c != to_join.end (); ++c) {
      m_rev_connections [*c] = id;
    }

    connections_type &target = m_connections [id];
    target.splice (target.end (), to_join, to_join.begin (), to_join.end ());

    m_connections.erase (tc);

  }
}

template <class T>
typename local_cluster<T>::id_type
connected_clusters<T>::find_cluster_with_connection (const ClusterInstance &inst) const
{
  typename std::map<ClusterInstance, typename local_cluster<T>::id_type>::const_iterator rc = m_rev_connections.find (inst);
  if (rc != m_rev_connections.end ()) {
    return rc->second;
  } else {
    return 0;
  }
}

template <class T>
void
connected_clusters<T>::mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, bool no_self, void *parent) const
{
  if (! no_self) {
    stat->add (typeid (*this), (void *) this, sizeof (*this), sizeof (*this), parent, purpose, cat);
  }

  local_clusters<T>::mem_stat (stat, purpose, cat, true, parent);

  geo::mem_stat (stat, purpose, cat, m_connections, true, (void *) this);
  geo::mem_stat (stat, purpose, cat, m_rev_connections, true, (void *) this);
  geo::mem_stat (stat, purpose, cat, m_connected_clusters, true, (void *) this);
}

//  explicit instantiations
template class DB_PUBLIC connected_clusters<geo::NetShape>;
template class DB_PUBLIC connected_clusters<geo::PolygonRef>;
template class DB_PUBLIC connected_clusters<geo::Edge>;

// ------------------------------------------------------------------------------
//  connected_clusters implementation

template <class T>
class DB_PUBLIC cell_clusters_box_converter
{
public:
  typedef geo::simple_bbox_tag complexity;
  typedef typename hier_clusters<T>::box_type box_type;

  cell_clusters_box_converter (const geo::Layout &layout, const hier_clusters<T> &tree)
    : mp_layout (&layout), mp_tree (&tree)
  {
    //  .. nothing yet ..
  }

  const box_type &operator() (const geo::CellInst &cell_inst) const
  {
    return (*this) (cell_inst.cell_index ());
  }

  const box_type &operator() (geo::cell_index_type cell_index) const
  {
    typename std::map<geo::cell_index_type, box_type>::const_iterator b = m_cache.find (cell_index);
    if (b != m_cache.end ()) {

      return b->second;

    } else {

      const connected_clusters<T> &clusters = mp_tree->clusters_per_cell (cell_index);
      box_type box = clusters.bbox ();

      const geo::Cell &cell = mp_layout->cell (cell_index);
      for (geo::Cell::const_iterator inst = cell.begin (); ! inst.at_end (); ++inst) {
        const geo::CellInstArray &inst_array = inst->cell_inst ();
        box += inst_array.bbox (*this);
      }

      return m_cache.insert (std::make_pair (cell_index, box)).first->second;

    }
  }

private:
  mutable std::map<geo::cell_index_type, box_type> m_cache;
  const geo::Layout *mp_layout;
  const hier_clusters<T> *mp_tree;
};

// ------------------------------------------------------------------------------
//  hier_clusters implementation

template <class T>
const geo::cell_index_type hier_clusters<T>::top_cell_index = std::numeric_limits<geo::cell_index_type>::max ();

template <class T>
hier_clusters<T>::hier_clusters ()
  : m_base_verbosity (20)
{
  //  .. nothing yet ..
}

template <class T>
void hier_clusters<T>::set_base_verbosity (int bv)
{
  m_base_verbosity = bv;
}

template <class T>
void hier_clusters<T>::clear ()
{
  m_per_cell_clusters.clear ();
}

template <class T>
void
hier_clusters<T>::mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, bool no_self, void *parent) const
{
  if (! no_self) {
    stat->add (typeid (*this), (void *) this, sizeof (*this), sizeof (*this), parent, purpose, cat);
  }

  geo::mem_stat (stat, purpose, cat, m_per_cell_clusters, true, (void *) this);
}

template <class T>
void
hier_clusters<T>::build (const geo::Layout &layout, const geo::Cell &cell, const geo::Connectivity &conn, const std::map<geo::cell_index_type, geo::equivalence_clusters<size_t> > *attr_equivalence, const std::set<geo::cell_index_type> *breakout_cells)
{
  clear ();
  cell_clusters_box_converter<T> cbc (layout, *this);
  do_build (cbc, layout, cell, conn, attr_equivalence, breakout_cells);
}

namespace
{

/**
 *  @brief The central interaction tester between clusters on a hierarchical level
 *
 *  This receiver is both used for the instance-to-instance and the local-to-instance
 *  interactions. It is employed on cell level for in two box scanners: one
 *  investigating the instance-to-instance interactions and another one investigating
 *  local cluster to instance interactions.
 */
template <class T>
struct hc_receiver
  : public geo::box_scanner_receiver<geo::Instance, unsigned int>,
    public geo::box_scanner_receiver2<local_cluster<T>, unsigned int, geo::Instance, unsigned int>
{
public:
  typedef typename hier_clusters<T>::box_type box_type;
  typedef typename local_cluster<T>::id_type id_type;
  typedef std::map<ClusterInstance, id_type> connector_map;

  struct ClusterInstanceInteraction
  {
    ClusterInstanceInteraction (size_t _cluster_id, const ClusterInstance &_other_ci)
      : cluster_id (_cluster_id), other_ci (_other_ci)
    { }

    bool operator== (const ClusterInstanceInteraction &other) const
    {
      return cluster_id == other.cluster_id && other_ci == other.other_ci;
    }

    size_t cluster_id;
    ClusterInstance other_ci;
  };

  /**
   *  @brief Constructor
   */
  hc_receiver (const geo::Layout &layout, const geo::Cell &cell, geo::connected_clusters<T> &cell_clusters, hier_clusters<T> &tree, const cell_clusters_box_converter<T> &cbc, const geo::Connectivity &conn, const std::set<geo::cell_index_type> *breakout_cells, typename hier_clusters<T>::instance_interaction_cache_type *instance_interaction_cache)
    : mp_layout (&layout), mp_cell (&cell), mp_tree (&tree), mp_cbc (&cbc), mp_conn (&conn), mp_breakout_cells (breakout_cells), m_cluster_cache_misses (0), m_cluster_cache_hits (0), mp_instance_interaction_cache (instance_interaction_cache)
  {
    mp_cell_clusters = &cell_clusters;
  }

  /**
   *  @brief Gets the cache size
   */
  size_t cluster_cache_size () const
  {
    geo::MemStatisticsSimple ms;
    ms << m_interaction_cache_for_clusters;
    return ms.used ();
  }

  /**
   *  @brief Gets the cache hits
   */
  size_t cluster_cache_hits () const
  {
    return m_cluster_cache_hits;
  }

  /**
   *  @brief Gets the cache misses
   */
  size_t cluster_cache_misses () const
  {
    return m_cluster_cache_misses;
  }

  /**
   *  @brief Receiver main event for instance-to-instance interactions
   */
  void add (const geo::Instance *i1, unsigned int /*p1*/, const geo::Instance *i2, unsigned int /*p2*/)
  {
    geo::ICplxTrans t;

    std::list<std::pair<ClusterInstance, ClusterInstance> > ic;
    consider_instance_pair (box_type::world (), *i1, t, geo::CellInstArray::iterator (), *i2, t, geo::CellInstArray::iterator (), ic);

    connect_clusters (ic);
  }

  /**
   *  @brief Single-instance treatment - may be required because of interactions between array members
   */
  void finish (const geo::Instance *i, unsigned int /*p1*/)
  {
    consider_single_inst (*i);
  }

  /**
   *  @brief Receiver main event for local-to-instance interactions
   */
  void add (const local_cluster<T> *c1, unsigned int /*p1*/, const geo::Instance *i2, unsigned int /*p2*/)
  {
    std::list<ClusterInstanceInteraction> ic;

    geo::ICplxTrans t;
    consider_cluster_instance_pair (*c1, *i2, t, ic);

    ic.unique ();
    m_ci_interactions.splice (m_ci_interactions.end (), ic, ic.begin (), ic.end ());
  }

  /**
   *  @brief Finally join the clusters in the join set
   *
   *  This step is postponed because doing this while the iteration happens would
   *  invalidate the box trees.
   */
  void finish_cluster_to_instance_interactions ()
  {
    for (typename std::list<ClusterInstanceInteraction>::const_iterator ii = m_ci_interactions.begin (); ii != m_ci_interactions.end (); ++ii) {
      mp_tree->propagate_cluster_inst (*mp_layout, *mp_cell, ii->other_ci, mp_cell->cell_index (), false);
    }

    for (typename std::list<ClusterInstanceInteraction>::const_iterator ii = m_ci_interactions.begin (); ii != m_ci_interactions.end (); ++ii) {

      id_type other = mp_cell_clusters->find_cluster_with_connection (ii->other_ci);
      if (other > 0) {

        //  we found a child cluster that connects two clusters on our own level:
        //  we must join them into one, but not now. We're still iterating and
        //  would invalidate the box trees. So keep this now and combine the clusters later.
        mark_to_join (other, ii->cluster_id);

      } else {
        mp_cell_clusters->add_connection (ii->cluster_id, ii->other_ci);
      }

    }

    for (typename std::list<std::set<id_type> >::const_iterator sc = m_cm2join_sets.begin (); sc != m_cm2join_sets.end (); ++sc) {

      if (sc->empty ()) {
        //  dropped ones are empty
        continue;
      }

      typename std::set<id_type>::const_iterator c = sc->begin ();
      typename std::set<id_type>::const_iterator cc = c;
      for (++cc; cc != sc->end (); ++cc) {
        mp_cell_clusters->join_cluster_with (*c, *cc);
      }

    }
  }

  //  needs explicit implementation because we have two base classes:
  bool stop () const { return false; }
  void initialize () { }
  void finalize (bool) { }

private:
  const geo::Layout *mp_layout;
  const geo::Cell *mp_cell;
  geo::connected_clusters<T> *mp_cell_clusters;
  hier_clusters<T> *mp_tree;
  const cell_clusters_box_converter<T> *mp_cbc;
  const geo::Connectivity *mp_conn;
  const std::set<geo::cell_index_type> *mp_breakout_cells;
  typedef std::list<std::set<id_type> > join_set_list;
  std::map<id_type, typename join_set_list::iterator> m_cm2join_map;
  join_set_list m_cm2join_sets;
  std::list<ClusterInstanceInteraction> m_ci_interactions;
  instance_interaction_cache<interaction_key_for_clusters<box_type>, std::list<std::pair<size_t, size_t> > > m_interaction_cache_for_clusters;
  size_t m_cluster_cache_misses, m_cluster_cache_hits;
  typename hier_clusters<T>::instance_interaction_cache_type *mp_instance_interaction_cache;

  /**
   *  @brief Investigate a pair of instances
   *
   *  @param common The common box of both instances
   *  @param i1 The first instance to investigate
   *  @param t1 The parent instances' cumulated transformation
   *  @param i1element selects a specific instance from i1 (unless == geo::CellInstArray::iterator())
   *  @param i2 The second instance to investigate
   *  @param t2 The parent instances' cumulated transformation
   *  @param i2element selects a specific instance from i2 (unless == geo::CellInstArray::iterator())
   *  @param interacting_clusters_out Receives the cluster interaction descriptors
   *
   *  "interacting_clusters_out" will be cluster interactions in the parent instance space of i1 and i2 respectively.
   *  Cluster ID will be valid in the parent cells containing i1 and i2 and the cluster instances property ID will be set to p1 and p2
   *  respectively.
   */
  void consider_instance_pair (const box_type &common,
                               const geo::Instance &i1, const geo::ICplxTrans &t1, const geo::CellInstArray::iterator &i1element,
                               const geo::Instance &i2, const geo::ICplxTrans &t2, const geo::CellInstArray::iterator &i2element,
                               cluster_instance_pair_list_type &interacting_clusters)
  {
    if (is_breakout_cell (mp_breakout_cells, i1.cell_index ()) || is_breakout_cell (mp_breakout_cells, i2.cell_index ())) {
      return;
    }

    box_type bb1 = (*mp_cbc) (i1.cell_index ());
    box_type b1 = i1.cell_inst ().bbox (*mp_cbc).transformed (t1);

    box_type bb2 = (*mp_cbc) (i2.cell_index ());
    box_type b2 = i2.cell_inst ().bbox (*mp_cbc).transformed (t2);

    box_type common_all = common & b1 & b2;

    if (common_all.empty ()) {
      return;
    }

    //  gross shortcut: the cells do no comprise a constellation which will ever interact
    if (! mp_conn->interact (mp_layout->cell (i1.cell_index ()), mp_layout->cell (i2.cell_index ()))) {
      return;
    }

    InstanceToInstanceInteraction ii_key;
    geo::ICplxTrans i1t, i2t;
    bool fill_cache = false;

    size_t n1 = i1element.at_end () ? i1.size () : 1;
    size_t n2 = i2element.at_end () ? i2.size () : 1;

    //  Cache is only used for single instances or simple and regular arrays.
    if ((n1 == 1 || ! i1.is_iterated_array ()) &&
        (n2 == 1 || ! i2.is_iterated_array ())) {

      i1t = i1element.at_end () ? i1.complex_trans () : i1.complex_trans (*i1element);
      geo::ICplxTrans tt1 = t1 * i1t;

      i2t = i2element.at_end () ? i2.complex_trans () : i2.complex_trans (*i2element);
      geo::ICplxTrans tt2 = t2 * i2t;

      geo::ICplxTrans cache_norm = tt1.inverted ();
      ii_key = InstanceToInstanceInteraction ((! i1element.at_end () || i1.size () == 1) ? 0 : i1.cell_inst ().delegate (),
                                              (! i2element.at_end () || i2.size () == 1) ? 0 : i2.cell_inst ().delegate (),
                                              cache_norm, cache_norm * tt2);

      const cluster_instance_pair_list_type *cached = mp_instance_interaction_cache->find (i1.cell_index (), i2.cell_index (), ii_key);
      if (cached) {

        //  use cached interactions
        interacting_clusters = *cached;
        for (cluster_instance_pair_list_type::iterator i = interacting_clusters.begin (); i != interacting_clusters.end (); ++i) {
          //  translate the property IDs
          i->first.set_inst_prop_id (i1.prop_id ());
          i->first.transform (i1t);
          i->second.set_inst_prop_id (i2.prop_id ());
          i->second.transform (i2t);
        }

        return;

      }

      //  avoid caching few-to-many interactions as this typically does not contribute anything
      fill_cache = true;

    }

    //  shortcut: if the instances cannot interact because their bounding boxes do no comprise a valid constellation,
    //  reject the pair

    bool any = false;
    for (geo::Connectivity::layer_iterator la = mp_conn->begin_layers (); la != mp_conn->end_layers () && ! any; ++la) {
      geo::box_convert<geo::CellInst, true> bca (*mp_layout, *la);
      box_type bb1 = i1.cell_inst ().bbox (bca).transformed (t1);
      if (! bb1.empty ()) {
        geo::Connectivity::layer_iterator lbe = mp_conn->end_connected (*la);
        for (geo::Connectivity::layer_iterator lb = mp_conn->begin_connected (*la); lb != lbe && ! any; ++lb) {
          geo::box_convert<geo::CellInst, true> bcb (*mp_layout, *lb);
          box_type bb2 = i2.cell_inst ().bbox (bcb).transformed (t2);
          any = bb1.touches (bb2);
        }
      }
    }

    if (! any) {
      if (fill_cache) {
        mp_instance_interaction_cache->insert (i1.cell_index (), i2.cell_index (), ii_key);
      }
      return;
    }

    //  array interactions

    geo::ICplxTrans t1i = t1.inverted ();
    geo::ICplxTrans t2i = t2.inverted ();

    //  TODO: optimize for single inst?
    for (geo::CellInstArray::iterator ii1 = i1element.at_end () ? i1.begin_touching (common_all.transformed (t1i), mp_layout) : i1element; ! ii1.at_end (); ++ii1) {

      geo::ICplxTrans i1t = i1.complex_trans (*ii1);
      geo::ICplxTrans tt1 = t1 * i1t;
      box_type ib1 = bb1.transformed (tt1);

      for (geo::CellInstArray::iterator ii2 = i2element.at_end () ? i2.begin_touching (ib1.transformed (t2i), mp_layout) : i2element; ! ii2.at_end (); ++ii2) {

        geo::ICplxTrans i2t = i2.complex_trans (*ii2);
        geo::ICplxTrans tt2 = t2 * i2t;
        if (i1.cell_index () == i2.cell_index () && tt1 == tt2) {
          //  skip interactions between identical instances (duplicate instance removal)
          continue;
        }

        box_type ib2 = bb2.transformed (tt2);

        box_type common12 = ib1 & ib2 & common;
        if (! common12.empty ()) {

          const std::list<std::pair<size_t, size_t> > &i2i_interactions = compute_instance_interactions (common12, i1.cell_index (), tt1, i2.cell_index (), tt2);

          for (std::list<std::pair<size_t, size_t> >::const_iterator ii = i2i_interactions.begin (); ii != i2i_interactions.end (); ++ii) {
            ClusterInstance k1 (ii->first, i1.cell_index (), i1t, i1.prop_id ());
            ClusterInstance k2 (ii->second, i2.cell_index (), i2t, i2.prop_id ());
            interacting_clusters.push_back (std::make_pair (k1, k2));
          }

          //  dive into cell of ii2
          const geo::Cell &cell2 = mp_layout->cell (i2.cell_index ());
          for (geo::Cell::touching_iterator jj2 = cell2.begin_touching (common12.transformed (tt2.inverted ())); ! jj2.at_end (); ++jj2) {

            std::list<std::pair<ClusterInstance, ClusterInstance> > ii_interactions;
            consider_instance_pair (common12, i1, t1, ii1, *jj2, tt2, geo::CellInstArray::iterator (), ii_interactions);

            for (std::list<std::pair<ClusterInstance, ClusterInstance> >::iterator i = ii_interactions.begin (); i != ii_interactions.end (); ++i) {
              propagate_cluster_inst (i->second, i2.cell_index (), i2t, i2.prop_id ());
            }

            ii_interactions.unique ();
            interacting_clusters.splice (interacting_clusters.end (), ii_interactions, ii_interactions.begin (), ii_interactions.end ());

          }

          //  dive into cell of ii1
          const geo::Cell &cell1 = mp_layout->cell (i1.cell_index ());
          for (geo::Cell::touching_iterator jj1 = cell1.begin_touching (common12.transformed (tt1.inverted ())); ! jj1.at_end (); ++jj1) {

            std::list<std::pair<ClusterInstance, ClusterInstance> > ii_interactions;
            consider_instance_pair (common12, *jj1, tt1, geo::CellInstArray::iterator (), i2, t2, ii2, ii_interactions);

            for (std::list<std::pair<ClusterInstance, ClusterInstance> >::iterator i = ii_interactions.begin (); i != ii_interactions.end (); ++i) {
              propagate_cluster_inst (i->first, i1.cell_index (), i1t, i1.prop_id ());
            }

            ii_interactions.unique ();
            interacting_clusters.splice (interacting_clusters.end (), ii_interactions, ii_interactions.begin (), ii_interactions.end ());

          }

        }

        if (! i2element.at_end ()) {
          break;
        }

      }

      if (! i1element.at_end ()) {
        break;
      }

    }

    //  remove duplicates (after doing a quick unique before)
    //  NOTE: duplicates may happen due to manifold child/child interactions which boil down to
    //  identical parent cluster interactions.
    std::vector<std::pair<ClusterInstance, ClusterInstance> > sorted_interactions;
    sorted_interactions.reserve (interacting_clusters.size ());
    sorted_interactions.insert (sorted_interactions.end (), interacting_clusters.begin (), interacting_clusters.end ());
    interacting_clusters.clear ();
    std::sort (sorted_interactions.begin (), sorted_interactions.end ());
    sorted_interactions.erase (std::unique (sorted_interactions.begin (), sorted_interactions.end ()), sorted_interactions.end ());

    //  return the list of unique interactions
    interacting_clusters.insert (interacting_clusters.end (), sorted_interactions.begin (), sorted_interactions.end ());

    if (fill_cache && sorted_interactions.size () < instance_to_instance_cache_set_size_threshold) {

      //  normalize transformations for cache
      geo::ICplxTrans i1ti = i1t.inverted (), i2ti = i2t.inverted ();
      for (std::vector<std::pair<ClusterInstance, ClusterInstance> >::iterator i = sorted_interactions.begin (); i != sorted_interactions.end (); ++i) {
        i->first.transform (i1ti);
        i->second.transform (i2ti);
      }

      cluster_instance_pair_list_type &cached = mp_instance_interaction_cache->insert (i1.cell_index (), i2.cell_index (), ii_key);
      cached.insert (cached.end (), sorted_interactions.begin (), sorted_interactions.end ());

    }
  }

  /**
   *  @brief Computes a list of interacting clusters for two instances
   */
  const std::list<std::pair<size_t, size_t> > &
  compute_instance_interactions (const box_type &common,
                                 geo::cell_index_type ci1, const geo::ICplxTrans &t1,
                                 geo::cell_index_type ci2, const geo::ICplxTrans &t2)
  {
    if (is_breakout_cell (mp_breakout_cells, ci1) || is_breakout_cell (mp_breakout_cells, ci2)) {
      static const std::list<std::pair<size_t, size_t> > empty;
      return empty;
    }

    geo::ICplxTrans t1i = t1.inverted ();
    geo::ICplxTrans t2i = t2.inverted ();
    geo::ICplxTrans t21 = t1i * t2;

    box_type common2 = common.transformed (t2i);

    interaction_key_for_clusters<box_type> ikey (t1i, t21, common2);

    const std::list<std::pair<size_t, size_t> > *ici = m_interaction_cache_for_clusters.find (ci1, ci2, ikey);
    if (ici) {

      return *ici;

    } else {

      const geo::Cell &cell2 = mp_layout->cell (ci2);

      const geo::local_clusters<T> &cl1 = mp_tree->clusters_per_cell (ci1);
      const geo::local_clusters<T> &cl2 = mp_tree->clusters_per_cell (ci2);

      std::list<std::pair<size_t, size_t> > &new_interactions = m_interaction_cache_for_clusters.insert (ci1, ci2, ikey);
      geo::ICplxTrans t12 = t2i * t1;

      for (typename geo::local_clusters<T>::touching_iterator i = cl1.begin_touching (common2.transformed (t21)); ! i.at_end (); ++i) {

        //  skip the test, if this cluster doesn't interact with the whole cell2
        if (! i->interacts (cell2, t21, *mp_conn)) {
          continue;
        }

        box_type bc2 = (common2 & i->bbox ().transformed (t12));
        for (typename geo::local_clusters<T>::touching_iterator j = cl2.begin_touching (bc2); ! j.at_end (); ++j) {

          if (i->interacts (*j, t21, *mp_conn)) {
            new_interactions.push_back (std::make_pair (i->id (), j->id ()));
          }

        }

      }

      return new_interactions;

    }
  }

  /**
   *  @brief Single instance treatment
   */
  void consider_single_inst (const geo::Instance &i)
  {
    if (is_breakout_cell (mp_breakout_cells, i.cell_index ())) {
      return;
    }

    box_type bb = (*mp_cbc) (i.cell_index ());
    const geo::Cell &cell = mp_layout->cell (i.cell_index ());

    for (geo::CellInstArray::iterator ii = i.begin (); ! ii.at_end (); ++ii) {

      geo::ICplxTrans tt = i.complex_trans (*ii);
      box_type ib = bb.transformed (tt);

      std::vector<ClusterInstElement> pp;
      pp.push_back (ClusterInstElement (i.cell_index (), i.complex_trans (*ii), i.prop_id ()));

      bool any = false;

      for (geo::CellInstArray::iterator ii2 = i.begin_touching (ib, mp_layout); ! ii2.at_end (); ++ii2) {

        geo::ICplxTrans tt2 = i.complex_trans (*ii2);
        if (tt.equal (tt2)) {
          //  skip the initial instance
          continue;
        }

        box_type ib2 = bb.transformed (tt2);

        if (ib.touches (ib2)) {

          std::vector<ClusterInstElement> pp2;
          pp2.push_back (ClusterInstElement (i.cell_index (), i.complex_trans (*ii2), i.prop_id ()));

          cluster_instance_pair_list_type interacting_clusters;

          box_type common = (ib & ib2);
          const std::list<std::pair<size_t, size_t> > &i2i_interactions = compute_instance_interactions (common, i.cell_index (), tt, i.cell_index (), tt2);
          for (std::list<std::pair<size_t, size_t> >::const_iterator ii = i2i_interactions.begin (); ii != i2i_interactions.end (); ++ii) {
            ClusterInstance k1 (ii->first, i.cell_index (), tt, i.prop_id ());
            ClusterInstance k2 (ii->second, i.cell_index (), tt2, i.prop_id ());
            interacting_clusters.push_back (std::make_pair (k1, k2));
          }

          for (geo::Cell::touching_iterator jj2 = cell.begin_touching (common.transformed (tt2.inverted ())); ! jj2.at_end (); ++jj2) {

            geo::ICplxTrans t;

            std::list<std::pair<ClusterInstance, ClusterInstance> > ii_interactions;
            consider_instance_pair (common, i, t, ii, *jj2, tt2, geo::CellInstArray::iterator (), ii_interactions);

            for (std::list<std::pair<ClusterInstance, ClusterInstance> >::iterator ii = ii_interactions.begin (); ii != ii_interactions.end (); ++ii) {
              propagate_cluster_inst (ii->second, i.cell_index (), tt2, i.prop_id ());
            }
            interacting_clusters.splice (interacting_clusters.end (), ii_interactions, ii_interactions.begin (), ii_interactions.end ());

          }

          connect_clusters (interacting_clusters);

          any = true;

        }

      }

      //  we don't expect more to happen on the next instance
      if (! any) {
        break;
      }

    }
  }

  /**
   *  @brief Handles a local clusters vs. the clusters of a specific child instance or instance array
   *  @param c1 The local cluster
   *  @param i2 The index of the child cell
   *  @param t2 The accumulated transformation of the path, not including i2
   *  @param interactions_out Delivers the interactions
   */
  void consider_cluster_instance_pair (const local_cluster<T> &c1, const geo::Instance &i2, const geo::ICplxTrans &t2, std::list<ClusterInstanceInteraction> &interactions_out)
  {
    if (is_breakout_cell (mp_breakout_cells, i2.cell_index ())) {
      return;
    }

    box_type bb2 = (*mp_cbc) (i2.cell_index ());

    const geo::Cell &cell2 = mp_layout->cell (i2.cell_index ());

    box_type b1 = c1.bbox ();
    box_type b2 = i2.cell_inst ().bbox (*mp_cbc).transformed (t2);

    if (! b1.touches (b2)) {
      return;
    }

    std::vector<std::pair<size_t, size_t> > c2i_interactions;

    for (geo::CellInstArray::iterator ii2 = i2.begin_touching ((b1 & b2).transformed (t2.inverted ()), mp_layout); ! ii2.at_end (); ++ii2) {

      geo::ICplxTrans i2t = i2.complex_trans (*ii2);
      geo::ICplxTrans tt2 = t2 * i2t;
      box_type ib2 = bb2.transformed (tt2);

      if (b1.touches (ib2) && c1.interacts (cell2, tt2, *mp_conn)) {

        c2i_interactions.clear ();
        compute_cluster_instance_interactions (c1, i2.cell_index (), tt2, c2i_interactions);

        for (std::vector<std::pair<size_t, size_t> >::const_iterator ii = c2i_interactions.begin (); ii != c2i_interactions.end (); ++ii) {
          ClusterInstance k (ii->second, i2.cell_index (), i2t, i2.prop_id ());
          interactions_out.push_back (ClusterInstanceInteraction (ii->first, k));
        }

        //  dive into cell of ii2
        for (geo::Cell::touching_iterator jj2 = cell2.begin_touching ((b1 & ib2).transformed (tt2.inverted ())); ! jj2.at_end (); ++jj2) {

          std::list<ClusterInstanceInteraction> ci_interactions;
          consider_cluster_instance_pair (c1, *jj2, tt2, ci_interactions);

          for (typename std::list<ClusterInstanceInteraction>::iterator ii = ci_interactions.begin (); ii != ci_interactions.end (); ++ii) {
            propagate_cluster_inst (ii->other_ci, i2.cell_index (), i2t, i2.prop_id ());
          }
          interactions_out.splice (interactions_out.end (), ci_interactions, ci_interactions.begin (), ci_interactions.end ());

        }

      }

    }
  }

  /**
   *  @brief Handles a local clusters vs. the clusters of a specific child instance
   *  @param c1 The local cluster
   *  @param ci2 The cell index of the cell investigated
   *  @param p2 The instantiation path to the child cell (last element is the instance to ci2)
   *  @param t2 The accumulated transformation of the path
   */
  void
  compute_cluster_instance_interactions (const local_cluster<T> &c1,
                                         geo::cell_index_type ci2, const geo::ICplxTrans &t2,
                                         std::vector<std::pair<size_t, size_t> > &interactions)
  {
    if (is_breakout_cell (mp_breakout_cells, ci2)) {
      return;
    }

    const geo::local_clusters<T> &cl2 = mp_tree->clusters_per_cell (ci2);

    for (typename geo::local_clusters<T>::touching_iterator j = cl2.begin_touching (c1.bbox ().transformed (t2.inverted ())); ! j.at_end (); ++j) {

      if (c1.interacts (*j, t2, *mp_conn)) {
        interactions.push_back (std::make_pair (c1.id (), j->id ()));
      }

    }
  }

  /**
   *  @brief Inserts a pair of clusters to join
   */
  void mark_to_join (id_type a, id_type b)
  {
    if (a == b) {
      //  shouldn't happen, but duplicate instances may trigger this
      return;
    }

    typename std::map<id_type, typename join_set_list::iterator>::const_iterator x = m_cm2join_map.find (a);
    typename std::map<id_type, typename join_set_list::iterator>::const_iterator y = m_cm2join_map.find (b);

    if (x == m_cm2join_map.end ()) {

      if (y == m_cm2join_map.end ()) {

        m_cm2join_sets.push_back (std::set<id_type> ());
        m_cm2join_sets.back ().insert (a);
        m_cm2join_sets.back ().insert (b);

        m_cm2join_map [a] = --m_cm2join_sets.end ();
        m_cm2join_map [b] = --m_cm2join_sets.end ();

      } else {

        y->second->insert (a);
        m_cm2join_map [a] = y->second;

      }

    } else if (y == m_cm2join_map.end ()) {

      x->second->insert (b);
      m_cm2join_map [b] = x->second;

    } else if (x->second != y->second) {

      //  join two superclusters
      typename join_set_list::iterator yset = y->second;
      x->second->insert (yset->begin (), yset->end ());
      for (typename std::set<id_type>::const_iterator i = yset->begin (); i != yset->end (); ++i) {
        m_cm2join_map [*i] = x->second;
      }
      m_cm2join_sets.erase (yset);

    }

#if defined(DEBUG_HIER_NETWORK_PROCESSOR)
    //  concistency check for debugging
    for (typename std::map<id_type, typename join_set_list::iterator>::const_iterator j = m_cm2join_map.begin (); j != m_cm2join_map.end (); ++j) {
      tl_assert (j->second->find (j->first) != j->second->end ());
    }

    for (typename std::list<std::set<id_type> >::const_iterator i = m_cm2join_sets.begin (); i != m_cm2join_sets.end (); ++i) {
      for (typename std::set<id_type>::const_iterator j = i->begin(); j != i->end(); ++j) {
        tl_assert(m_cm2join_map.find (*j) != m_cm2join_map.end ());
        tl_assert(m_cm2join_map[*j] == i);
      }
    }

    //  the sets must be disjunct
    std::set<id_type> all;
    for (typename std::list<std::set<id_type> >::const_iterator i = m_cm2join_sets.begin (); i != m_cm2join_sets.end (); ++i) {
      for (typename std::set<id_type>::const_iterator j = i->begin(); j != i->end(); ++j) {
        tl_assert(all.find (*j) == all.end());
        all.insert(*j);
      }
    }
#endif

  }

  /**
   *  @brief Makes cluster connections from all parents of a cell into this cell and to the given cluster
   *
   *  After calling this method, the cluster instance in ci is guaranteed to have connections from all
   *  parent cells. One of these connections represents the instance ci.
   */
  void propagate_cluster_inst (ClusterInstance &ci, geo::cell_index_type pci, const geo::ICplxTrans &trans, geo::properties_id_type prop_id) const
  {
    size_t id_new = mp_tree->propagate_cluster_inst (*mp_layout, *mp_cell, ci, pci, true);
    tl_assert (id_new != 0);
    ci = geo::ClusterInstance (id_new, pci, trans, prop_id);
  }

  /**
   *  @brief Establishes connections between the cluster instances listed in the argument
   */
  void connect_clusters (const cluster_instance_pair_list_type &interacting_clusters)
  {
    for (cluster_instance_pair_list_type::const_iterator ic = interacting_clusters.begin (); ic != interacting_clusters.end (); ++ic) {

      const ClusterInstance &k1 = ic->first;
      const ClusterInstance &k2 = ic->second;

      //  Note: "with_self" is false as we're going to create a connected cluster anyway
      id_type x1 = mp_tree->propagate_cluster_inst (*mp_layout, *mp_cell, k1, mp_cell->cell_index (), false);
      id_type x2 = mp_tree->propagate_cluster_inst (*mp_layout, *mp_cell, k2, mp_cell->cell_index (), false);

      if (x1 == 0) {

        if (x2 == 0) {

          id_type connector = mp_cell_clusters->insert_dummy ();
          mp_cell_clusters->add_connection (connector, k1);
          mp_cell_clusters->add_connection (connector, k2);

        } else {
          mp_cell_clusters->add_connection (x2, k1);
        }

      } else if (x2 == 0) {

        mp_cell_clusters->add_connection (x1, k2);

      } else if (x1 != x2) {

        //  for instance-to-instance interactions the number of connections is more important for the
        //  cost of the join operation: make the one with more connections the target
        //  TODO: this will be SLOW for STL's not providing a fast size()
        if (mp_cell_clusters->connections_for_cluster (x1).size () < mp_cell_clusters->connections_for_cluster (x2).size ()) {
          std::swap (x1, x2);
        }

        mp_cell_clusters->join_cluster_with (x1, x2);
        mp_cell_clusters->remove_cluster (x2);

      }

    }
  }
};

template <class T>
struct cell_inst_clusters_box_converter
{
  typedef typename local_cluster<T>::box_type box_type;
  typedef geo::simple_bbox_tag complexity;

  cell_inst_clusters_box_converter (const cell_clusters_box_converter<T> &cbc)
    : mp_cbc (&cbc)
  {
    //  .. nothing yet ..
  }

  box_type operator() (const geo::Instance &inst) const
  {
    return inst.cell_inst ().bbox (*mp_cbc);
  }

private:
  const cell_clusters_box_converter<T> *mp_cbc;
};

}

template <class T>
size_t
hier_clusters<T>::propagate_cluster_inst (const geo::Layout &layout, const geo::Cell &cell, const ClusterInstance &ci, geo::cell_index_type pci, bool with_self)
{
  connected_clusters<T> &target_cc = clusters_per_cell (pci);
  size_t parent_cluster = target_cc.find_cluster_with_connection (ci);

  size_t id_new = 0;

  if (parent_cluster > 0) {

    //  taken parent
    id_new = parent_cluster;

  } else {

    size_t id = ci.id ();

    //  if we're attaching to a child which is root yet, we need to promote the
    //  cluster to the parent in all places
    connected_clusters<T> &child_cc = clusters_per_cell (ci.inst_cell_index ());
    if (child_cc.is_root (id)) {

      std::set<std::pair<geo::cell_index_type, ClusterInstance> > seen;  //  to avoid duplicate connections

      const geo::Cell &child_cell = layout.cell (ci.inst_cell_index ());
      for (geo::Cell::parent_inst_iterator pi = child_cell.begin_parent_insts (); ! pi.at_end (); ++pi) {

        geo::Instance child_inst = pi->child_inst ();

        connected_clusters<T> &parent_cc = clusters_per_cell (pi->parent_cell_index ());
        for (geo::CellInstArray::iterator pii = child_inst.begin (); ! pii.at_end (); ++pii) {

          ClusterInstance ci2 (id, child_inst.cell_index (), child_inst.complex_trans (*pii), child_inst.prop_id ());
          if ((with_self || cell.cell_index () != pi->parent_cell_index () || ci != ci2) && seen.find (std::make_pair (pi->parent_cell_index (), ci2)) == seen.end ()) {

            size_t id_dummy;

            const typename geo::local_cluster<T>::global_nets &gn = child_cc.cluster_by_id (id).get_global_nets ();
            if (gn.empty ()) {
              id_dummy = parent_cc.insert_dummy ();
            } else {
              local_cluster<T> *lc = parent_cc.insert ();
              lc->set_global_nets (gn);
              id_dummy = lc->id ();
            }

            parent_cc.add_connection (id_dummy, ci2);
            seen.insert (std::make_pair (pi->parent_cell_index (), ci2));

            if (pci == pi->parent_cell_index () && ci == ci2) {
              id_new = id_dummy;
            }

          }

        }

      }

      child_cc.reset_root (id);

    }

  }

  return id_new;
}

template <class T>
void
hier_clusters<T>::do_build (cell_clusters_box_converter<T> &cbc, const geo::Layout &layout, const geo::Cell &cell, const geo::Connectivity &conn, const std::map<geo::cell_index_type, geo::equivalence_clusters<size_t> > *attr_equivalence, const std::set<geo::cell_index_type> *breakout_cells)
{
  geo::SelfTimer timer (geo::verbosity () > m_base_verbosity, geo::to_string (tr ("Computing shape clusters")));

  std::set<geo::cell_index_type> called;
  cell.collect_called_cells (called);
  called.insert (cell.cell_index ());

  //  first build all local clusters

  {
    geo::SelfTimer timer (geo::verbosity () > m_base_verbosity + 10, geo::to_string (tr ("Computing local shape clusters")));
    geo::RelativeProgress progress (geo::to_string (tr ("Computing local clusters")), called.size (), 1);

    for (std::set<geo::cell_index_type>::const_iterator c = called.begin (); c != called.end (); ++c) {

      //  look for the net label joining spec - for the top cell the "top_cell_index" entry is looked for.
      //  If there is no such entry or the cell is not the top cell, look for the entry by cell index.
      std::map<geo::cell_index_type, geo::equivalence_clusters<size_t> >::const_iterator ae;
      const geo::equivalence_clusters<size_t> *ec = 0;
      if (attr_equivalence) {
        if (*c == cell.cell_index ()) {
          ae = attr_equivalence->find (top_cell_index);
          if (ae != attr_equivalence->end ()) {
            ec = &ae->second;
          }
        }
        if (! ec) {
          ae = attr_equivalence->find (*c);
          if (ae != attr_equivalence->end ()) {
            ec = &ae->second;
          }
        }
      }

      build_local_cluster (layout, layout.cell (*c), conn, ec);

      ++progress;

    }
  }

  //  build the hierarchical connections bottom-up and for all cells whose children are computed already

  instance_interaction_cache_type instance_interaction_cache;

  {
    geo::SelfTimer timer (geo::verbosity () > m_base_verbosity + 10, geo::to_string (tr ("Computing hierarchical shape clusters")));
    geo::RelativeProgress progress (geo::to_string (tr ("Computing hierarchical clusters")), called.size (), 1);

    std::set<geo::cell_index_type> done;
    std::vector<geo::cell_index_type> todo;
    for (geo::Layout::bottom_up_const_iterator c = layout.begin_bottom_up (); c != layout.end_bottom_up (); ++c) {

      if (called.find (*c) != called.end ()) {

        bool all_available = true;
        const geo::Cell &cell = layout.cell (*c);
        for (geo::Cell::child_cell_iterator cc = cell.begin_child_cells (); ! cc.at_end () && all_available; ++cc) {
          all_available = (done.find (*cc) != done.end ());
        }

        if (all_available) {
          todo.push_back (*c);
        } else {
          tl_assert (! todo.empty ());
          build_hier_connections_for_cells (cbc, layout, todo, conn, breakout_cells, progress, instance_interaction_cache);
          done.insert (todo.begin (), todo.end ());
          todo.clear ();
          todo.push_back (*c);
        }

      }

    }

    build_hier_connections_for_cells (cbc, layout, todo, conn, breakout_cells, progress, instance_interaction_cache);
  }

  if (geo::verbosity () >= m_base_verbosity + 20) {
    geo::info << "Cluster build cache statistics (instance to instance cache): size=" << instance_interaction_cache.size () << ", hits=" << instance_interaction_cache.hits () << ", misses=" << instance_interaction_cache.misses ();
  }
}

template <class T>
void
hier_clusters<T>::build_local_cluster (const geo::Layout &layout, const geo::Cell &cell, const geo::Connectivity &conn, const geo::equivalence_clusters<size_t> *attr_equivalence)
{
  std::string msg = geo::to_string (tr ("Computing local clusters for cell: ")) + std::string (layout.cell_name (cell.cell_index ()));
  if (geo::verbosity () >= m_base_verbosity + 20) {
    geo::log << msg;
  }
  geo::SelfTimer timer (geo::verbosity () > m_base_verbosity + 20, msg);

  connected_clusters<T> &local = m_per_cell_clusters [cell.cell_index ()];
  local.build_clusters (cell, conn, attr_equivalence, true);
}

template <class T>
void
hier_clusters<T>::build_hier_connections_for_cells (cell_clusters_box_converter<T> &cbc, const geo::Layout &layout, const std::vector<geo::cell_index_type> &cells, const geo::Connectivity &conn, const std::set<geo::cell_index_type> *breakout_cells, geo::RelativeProgress &progress, instance_interaction_cache_type &instance_interaction_cache)
{
  for (std::vector<geo::cell_index_type>::const_iterator c = cells.begin (); c != cells.end (); ++c) {
    build_hier_connections (cbc, layout, layout.cell (*c), conn, breakout_cells, instance_interaction_cache);
    ++progress;
  }
}

namespace {

class GlobalNetClusterMaker
{
public:
  typedef std::pair<std::set<size_t>, std::set<ClusterInstance> > entry_type;
  typedef std::list<entry_type> entry_list;
  typedef entry_list::const_iterator entry_iterator;

  void
  add (const std::set<size_t> &global_nets, size_t cluster_id)
  {
    add (global_nets, ClusterInstance (cluster_id));
  }

  void
  add (const std::set<size_t> &global_nets, const ClusterInstance &inst)
  {
    if (global_nets.empty ()) {
      return;
    }

    std::set<size_t>::const_iterator g0 = global_nets.begin ();

    std::map<size_t, entry_list::iterator>::iterator k = m_global_net_to_entries.find (*g0);
    if (k == m_global_net_to_entries.end ()) {

      m_entries.push_back (entry_type ());
      m_entries.back ().first.insert (*g0);

      k = m_global_net_to_entries.insert (std::make_pair (*g0, --m_entries.end ())).first;

    }

    k->second->second.insert (inst);

    for (std::set<size_t>::const_iterator g = ++g0; g != global_nets.end (); ++g) {

      std::map<size_t, entry_list::iterator>::iterator j = m_global_net_to_entries.find (*g);
      if (j == m_global_net_to_entries.end ()) {

        k->second->first.insert (*g);
        k->second->second.insert (inst);

        m_global_net_to_entries.insert (std::make_pair (*g, k->second));

      } else if (k->second != j->second) {

        //  joining required
        k->second->first.insert (j->second->first.begin (), j->second->first.end ());
        k->second->second.insert (j->second->second.begin (), j->second->second.end ());

        m_entries.erase (j->second);
        j->second = k->second;

      }

    }
  }

  entry_iterator begin () const
  {
    return m_entries.begin ();
  }

  entry_iterator end () const
  {
    return m_entries.end ();
  }

private:
  entry_list m_entries;
  std::map<size_t, entry_list::iterator> m_global_net_to_entries;
};

}

template <class T>
void
hier_clusters<T>::build_hier_connections (cell_clusters_box_converter<T> &cbc, const geo::Layout &layout, const geo::Cell &cell, const geo::Connectivity &conn, const std::set<geo::cell_index_type> *breakout_cells, instance_interaction_cache_type &instance_interaction_cache)
{
  std::string msg = geo::to_string (tr ("Computing hierarchical clusters for cell: ")) + std::string (layout.cell_name (cell.cell_index ()));
  if (geo::verbosity () >= m_base_verbosity + 20) {
    geo::log << msg;
  }
  geo::SelfTimer timer (geo::verbosity () > m_base_verbosity + 20, msg);

  connected_clusters<T> &local = m_per_cell_clusters [cell.cell_index ()];

  //  NOTE: this is a receiver for both the child-to-child and
  //  local to child interactions.
  std::unique_ptr<hc_receiver<T> > rec (new hc_receiver<T> (layout, cell, local, *this, cbc, conn, breakout_cells, &instance_interaction_cache));
  cell_inst_clusters_box_converter<T> cibc (cbc);

  //  The box scanner needs pointers so we have to first store the instances
  //  delivered by the cell's iterator (which does not deliver real pointer).

  std::vector<geo::Instance> inst_storage;

  //  TODO: there should be a cell.size () for this ...
  size_t n = 0;
  for (geo::Cell::const_iterator inst = cell.begin (); ! inst.at_end (); ++inst) {
    n += 1;
  }

  inst_storage.reserve (n);
  for (geo::Cell::const_iterator inst = cell.begin (); ! inst.at_end (); ++inst) {
    inst_storage.push_back (*inst);
  }

  //  handle instance to instance connections

  {
    static std::string desc = geo::to_string (tr ("Instance to instance treatment"));
    geo::SelfTimer timer (geo::verbosity () > m_base_verbosity + 30, desc);

    geo::box_scanner<geo::Instance, unsigned int> bs (true, desc);

    for (std::vector<geo::Instance>::const_iterator inst = inst_storage.begin (); inst != inst_storage.end (); ++inst) {
      if (! is_breakout_cell (breakout_cells, inst->cell_index ())) {
        bs.insert (inst.operator-> (), 0);
      }
    }

    bs.process (*rec, 1 /*touching*/, cibc);
  }

  //  handle local to instance connections

  {
    std::list<local_cluster<T> > heap;
    double area_ratio = 10.0;

    static std::string desc = geo::to_string (tr ("Local to instance treatment"));
    geo::SelfTimer timer (geo::verbosity () > m_base_verbosity + 30, desc);

    geo::box_scanner2<geo::local_cluster<T>, unsigned int, geo::Instance, unsigned int> bs2 (true, desc);

    for (typename connected_clusters<T>::const_iterator c = local.begin (); c != local.end (); ++c) {

      //  we do not actually need the original clusters. For a better performance we optimize the
      //  area ratio and split, but we keep the ID the same.
      std::back_insert_iterator<std::list<local_cluster<T> > > iout = std::back_inserter (heap);
      size_t n = c->split (area_ratio, iout);
      if (n == 0) {
        bs2.insert1 (c.operator-> (), 0);
      } else {
        typename std::list<local_cluster<T> >::iterator h = heap.end ();
        while (n-- > 0) {
          bs2.insert1 ((--h).operator-> (), 0);
        }
      }
    }

    for (std::vector<geo::Instance>::const_iterator inst = inst_storage.begin (); inst != inst_storage.end (); ++inst) {
      if (! is_breakout_cell (breakout_cells, inst->cell_index ())) {
        bs2.insert2 (inst.operator-> (), 0);
      }
    }

    bs2.process (*rec, 1 /*touching*/, local_cluster_box_convert<T> (), cibc);
  }

  //  join local clusters which got connected by child clusters
  rec->finish_cluster_to_instance_interactions ();

  if (geo::verbosity () >= m_base_verbosity + 20) {
    geo::info << "Cluster build cache statistics (instance to shape cache): size=" << rec->cluster_cache_size () << ", hits=" << rec->cluster_cache_hits () << ", misses=" << rec->cluster_cache_misses ();
  }

  rec.reset (0);

  //  finally connect global nets
  {
    static std::string desc = geo::to_string (tr ("Global net treatment"));
    geo::SelfTimer timer (geo::verbosity () > m_base_verbosity + 30, desc);

    GlobalNetClusterMaker global_net_clusters;

    //  insert the global nets from the subcircuits which need connection

    for (std::vector<geo::Instance>::const_iterator inst = inst_storage.begin (); inst != inst_storage.end (); ++inst) {

      const geo::connected_clusters<T> &cc = m_per_cell_clusters [inst->cell_index ()];
      for (typename geo::connected_clusters<T>::const_iterator cl = cc.begin (); cl != cc.end (); ++cl) {

        if (! cl->get_global_nets ().empty ()) {
          for (geo::Instance::cell_inst_array_type::iterator i = inst->begin (); !i.at_end (); ++i) {
            global_net_clusters.add (cl->get_global_nets (), geo::ClusterInstance (cl->id (), inst->cell_index (), inst->complex_trans (*i), inst->prop_id ()));
          }
        }

      }
    }

    //  insert the global nets from here

    for (typename geo::connected_clusters<T>::const_iterator cl = local.begin (); cl != local.end (); ++cl) {
      const typename geo::local_cluster<T>::global_nets &gn = cl->get_global_nets ();
      if (! gn.empty ()) {
        global_net_clusters.add (gn, geo::ClusterInstance (cl->id ()));
      }
    }

    //  now global_net_clusters knows what clusters need to be made for the global nets

    for (GlobalNetClusterMaker::entry_iterator ge = global_net_clusters.begin (); ge != global_net_clusters.end (); ++ge) {

      geo::local_cluster<T> *gc = local.insert ();
      gc->set_global_nets (ge->first);
      //  NOTE: don't use the gc pointer - it may become invalid during make_path (will also do a local.insert)
      size_t gcid = gc->id ();

      for (std::set<ClusterInstance>::const_iterator i = ge->second.begin (); i != ge->second.end (); ++i) {

        if (! i->has_instance ()) {

          local.join_cluster_with (gcid, i->id ());
          local.remove_cluster (i->id ());

        } else {

          //  ensures the cluster is propagated so we can connect it with another
          size_t other_id = propagate_cluster_inst (layout, cell, *i, cell.cell_index (), false);

          if (other_id == gcid) {
            //  shouldn't happen, but duplicate instances may trigger this
          } else if (other_id) {
            local.join_cluster_with (gcid, other_id);
            local.remove_cluster (other_id);
          } else {
            local.add_connection (gcid, *i);
          }

        }

      }

    }

  }
}

template <class T>
const connected_clusters<T> &
hier_clusters<T>::clusters_per_cell (geo::cell_index_type cell_index) const
{
  typename std::map<geo::cell_index_type, connected_clusters<T> >::const_iterator c = m_per_cell_clusters.find (cell_index);
  if (c == m_per_cell_clusters.end ()) {
    static connected_clusters<T> empty;
    return empty;
  } else {
    return c->second;
  }
}

template <class T>
connected_clusters<T> &
hier_clusters<T>::clusters_per_cell (geo::cell_index_type cell_index)
{
  typename std::map<geo::cell_index_type, connected_clusters<T> >::iterator c = m_per_cell_clusters.find (cell_index);
  if (c == m_per_cell_clusters.end ()) {
    c = m_per_cell_clusters.insert (std::make_pair (cell_index, connected_clusters<T> ())).first;
  }
  return c->second;
}

template <class T>
void
hier_clusters<T>::return_to_hierarchy (geo::Layout &layout, const std::map<unsigned int, unsigned int> &lm) const
{
  for (geo::Layout::bottom_up_iterator c = layout.begin_bottom_up (); c != layout.end_bottom_up (); ++c) {

    const geo::connected_clusters<T> &cc = clusters_per_cell (*c);
    geo::Cell &target_cell = layout.cell (*c);

    for (typename geo::connected_clusters<T>::all_iterator lc = cc.begin_all (); ! lc.at_end (); ++lc) {

      if (cc.is_root (*lc)) {

        for (typename std::map<unsigned int, unsigned int>::const_iterator m = lm.begin (); m != lm.end (); ++m) {

          geo::Shapes &shapes = target_cell.shapes (m->second);

          for (recursive_cluster_shape_iterator<T> si (*this, m->first, *c, *lc); ! si.at_end (); ++si) {
            insert_transformed (layout, shapes, *si, si.trans ());
          }

        }

      }

    }

  }
}

//  explicit instantiations
template class DB_PUBLIC hier_clusters<geo::NetShape>;
template class DB_PUBLIC hier_clusters<geo::PolygonRef>;
template class DB_PUBLIC hier_clusters<geo::Edge>;

// ------------------------------------------------------------------------------
//  recursive_cluster_shape_iterator implementation

template <class T>
recursive_cluster_shape_iterator<T>::recursive_cluster_shape_iterator (const hier_clusters<T> &hc, unsigned int layer, geo::cell_index_type ci, typename local_cluster<T>::id_type id, const CircuitCallback *callback)
  : mp_hc (&hc), m_layer (layer), m_id (id), mp_callback (callback)
{
  if (id == 0) {
    return;
  }

  down (ci, id, geo::ICplxTrans ());

  while (m_shape_iter.at_end () && ! m_conn_iter_stack.empty ()) {
    next_conn ();
  }
}

template <class T>
std::vector<ClusterInstance> recursive_cluster_shape_iterator<T>::inst_path () const
{
  std::vector<geo::ClusterInstance> p;
  if (!m_conn_iter_stack.empty ()) {
    p.reserve (m_conn_iter_stack.size ());
    for (size_t i = 0; i < m_conn_iter_stack.size () - 1; ++i) {
      p.push_back (*m_conn_iter_stack [i].first);
    }
  }
  return p;
}

template <class T>
recursive_cluster_shape_iterator<T> &recursive_cluster_shape_iterator<T>::operator++ ()
{
  ++m_shape_iter;

  while (m_shape_iter.at_end () && ! m_conn_iter_stack.empty ()) {
    next_conn ();
  }

  return *this;
}

template <class T>
void recursive_cluster_shape_iterator<T>::skip_cell ()
{
  m_shape_iter = typename geo::local_cluster<T>::shape_iterator ();

  do {

    up ();
    if (m_conn_iter_stack.empty ()) {
      return;
    }

    ++m_conn_iter_stack.back ().first;

  } while (m_conn_iter_stack.back ().first == m_conn_iter_stack.back ().second);

  while (m_shape_iter.at_end () && ! m_conn_iter_stack.empty ()) {
    next_conn ();
  }
}

template <class T>
void recursive_cluster_shape_iterator<T>::next_conn ()
{
  if (m_conn_iter_stack.back ().first != m_conn_iter_stack.back ().second) {

    const ClusterInstance &cli = *m_conn_iter_stack.back ().first;
    if (mp_callback && ! mp_callback->new_cell (cli.inst_cell_index ())) {
      //  skip this cell
      ++m_conn_iter_stack.back ().first;
    } else {
      down (cli.inst_cell_index (), cli.id (), cli.inst_trans ());
    }

  } else {

    while (m_conn_iter_stack.back ().first == m_conn_iter_stack.back ().second) {

      up ();
      if (m_conn_iter_stack.empty ()) {
        return;
      }

      ++m_conn_iter_stack.back ().first;

    }

  }
}

template <class T>
void recursive_cluster_shape_iterator<T>::up ()
{
  m_conn_iter_stack.pop_back ();
  m_trans_stack.pop_back ();
  m_cell_index_stack.pop_back ();
}

template <class T>
void recursive_cluster_shape_iterator<T>::down (geo::cell_index_type ci, typename geo::local_cluster<T>::id_type id, const geo::ICplxTrans &t)
{
  const connected_clusters<T> &clusters = mp_hc->clusters_per_cell (ci);
  const typename connected_clusters<T>::connections_type &conn = clusters.connections_for_cluster (id);

  if (! m_trans_stack.empty ()) {
    m_trans_stack.push_back (m_trans_stack.back () * t);
  } else {
    m_trans_stack.push_back (t);
  }

  m_cell_index_stack.push_back (ci);
  m_conn_iter_stack.push_back (std::make_pair (conn.begin (), conn.end ()));

  const local_cluster<T> &cluster = mp_hc->clusters_per_cell (cell_index ()).cluster_by_id (cluster_id ());
  m_shape_iter = cluster.begin (m_layer);
}

//  explicit instantiations
template class DB_PUBLIC recursive_cluster_shape_iterator<geo::NetShape>;
template class DB_PUBLIC recursive_cluster_shape_iterator<geo::PolygonRef>;
template class DB_PUBLIC recursive_cluster_shape_iterator<geo::Edge>;

// ------------------------------------------------------------------------------
//  recursive_cluster_iterator implementation

template <class T>
recursive_cluster_iterator<T>::recursive_cluster_iterator (const hier_clusters<T> &hc, geo::cell_index_type ci, typename local_cluster<T>::id_type id)
  : mp_hc (&hc), m_id (id)
{
  down (ci, id);
}

template <class T>
std::vector<ClusterInstance> recursive_cluster_iterator<T>::inst_path () const
{
  std::vector<geo::ClusterInstance> p;
  if (!m_conn_iter_stack.empty ()) {
    p.reserve (m_conn_iter_stack.size ());
    for (size_t i = 0; i < m_conn_iter_stack.size () - 1; ++i) {
      p.push_back (*m_conn_iter_stack [i].first);
    }
  }
  return p;
}

template <class T>
recursive_cluster_iterator<T> &recursive_cluster_iterator<T>::operator++ ()
{
  next_conn ();
  return *this;
}

template <class T>
void recursive_cluster_iterator<T>::next_conn ()
{
  while (m_conn_iter_stack.back ().first == m_conn_iter_stack.back ().second) {

    up ();
    if (m_conn_iter_stack.empty ()) {
      return;
    }

    ++m_conn_iter_stack.back ().first;

  }

  if (m_conn_iter_stack.back ().first != m_conn_iter_stack.back ().second) {

    const ClusterInstance &cli = *m_conn_iter_stack.back ().first;
    down (cli.inst_cell_index (), cli.id ());

  }
}

template <class T>
void recursive_cluster_iterator<T>::up ()
{
  m_conn_iter_stack.pop_back ();
  m_cell_index_stack.pop_back ();
}

template <class T>
void recursive_cluster_iterator<T>::down (geo::cell_index_type ci, typename geo::local_cluster<T>::id_type id)
{
  const connected_clusters<T> &clusters = mp_hc->clusters_per_cell (ci);
  const typename connected_clusters<T>::connections_type &conn = clusters.connections_for_cluster (id);

  m_cell_index_stack.push_back (ci);
  m_conn_iter_stack.push_back (std::make_pair (conn.begin (), conn.end ()));
}

//  explicit instantiations
template class DB_PUBLIC recursive_cluster_iterator<geo::NetShape>;
template class DB_PUBLIC recursive_cluster_iterator<geo::PolygonRef>;
template class DB_PUBLIC recursive_cluster_iterator<geo::Edge>;

// ------------------------------------------------------------------------------
//  incoming_cluster_connections implementation

template <class T>
incoming_cluster_connections<T>::incoming_cluster_connections (const geo::Layout &layout, const geo::Cell &cell, const hier_clusters<T> &hc)
  : mp_layout (const_cast<geo::Layout *> (&layout)), mp_hc (const_cast<hier_clusters<T> *> (&hc))
{
  cell.collect_called_cells (m_called_cells);
  m_called_cells.insert (cell.cell_index ());
}

template <class T>
bool
incoming_cluster_connections<T>::has_incoming (geo::cell_index_type ci, size_t cluster_id) const
{
  std::map<geo::cell_index_type, std::map<size_t, incoming_connections> >::const_iterator i = m_incoming.find (ci);
  if (i == m_incoming.end ()) {
    ensure_computed (ci);
    i = m_incoming.find (ci);
    tl_assert (i != m_incoming.end ());
  }

  tl_assert (i != m_incoming.end ());
  return (i->second.find (cluster_id) != i->second.end ());
}

template <class T>
const typename incoming_cluster_connections<T>::incoming_connections &
incoming_cluster_connections<T>::incoming (geo::cell_index_type ci, size_t cluster_id) const
{
  std::map<geo::cell_index_type, std::map<size_t, incoming_connections> >::const_iterator i = m_incoming.find (ci);
  if (i == m_incoming.end ()) {
    ensure_computed (ci);
    i = m_incoming.find (ci);
    tl_assert (i != m_incoming.end ());
  }

  std::map<size_t, incoming_connections>::const_iterator ii = i->second.find (cluster_id);
  if (ii != i->second.end ()) {
    return ii->second;
  } else {
    static incoming_connections empty;
    return empty;
  }
}

template <class T>
void
incoming_cluster_connections<T>::ensure_computed (geo::cell_index_type ci) const
{
  tl_assert (mp_layout.get () != 0);
  m_incoming.insert (std::make_pair (ci, std::map<size_t, incoming_connections> ()));

  const geo::Cell &cell = mp_layout->cell (ci);
  for (geo::Cell::parent_cell_iterator pc = cell.begin_parent_cells (); pc != cell.end_parent_cells (); ++pc) {
    if (m_called_cells.find (*pc) != m_called_cells.end ()) {
      ensure_computed_parent (*pc);
    }
  }

  m_called_cells.erase (ci);
}

template <class T>
void
incoming_cluster_connections<T>::ensure_computed_parent (geo::cell_index_type ci) const
{
  ensure_computed (ci);

  const connected_clusters<T> &cc = ((const hier_clusters<T> *) mp_hc.get ())->clusters_per_cell (ci);
  for (typename connected_clusters<T>::connections_iterator x = cc.begin_connections (); x != cc.end_connections (); ++x) {
    for (typename connected_clusters<T>::connections_type::const_iterator xx = x->second.begin (); xx != x->second.end (); ++xx) {
      m_incoming [xx->inst_cell_index ()][xx->id ()].push_back (IncomingClusterInstance (ci, x->first, *xx));
    }
  }
}

//  explicit instantiations
template class DB_PUBLIC incoming_cluster_connections<geo::NetShape>;
template class DB_PUBLIC incoming_cluster_connections<geo::PolygonRef>;
template class DB_PUBLIC incoming_cluster_connections<geo::Edge>;

}
