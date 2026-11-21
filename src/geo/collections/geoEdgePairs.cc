#include "geoCommon.h"

#include "geoEdgePairs.h"
#include "geoEmptyEdgePairs.h"
#include "geoFlatEdgePairs.h"
#include "geoDeepEdgePairs.h"
#include "geoOriginalLayerEdgePairs.h"
#include "geoEdges.h"
#include "geoRegion.h"

#include "utilsVariant.h"

#include <sstream>

namespace geo
{

// ---------------------------------------------------------------------------------------------------
//  EdgePairs implementation

EdgePairs::EdgePairs ()
  : mp_delegate (new EmptyEdgePairs ())
{
  //  .. nothing yet ..
}

EdgePairs::~EdgePairs ()
{
  delete mp_delegate;
  mp_delegate = 0;
}

EdgePairs::EdgePairs (EdgePairsDelegate *delegate)
  : mp_delegate (delegate)
{
  //  .. nothing yet ..
}

EdgePairs::EdgePairs (const EdgePairs &other)
  : geo::ShapeCollection (), mp_delegate (other.mp_delegate->clone ())
{
  //  .. nothing yet ..
}

EdgePairs &EdgePairs::operator= (const EdgePairs &other)
{
  if (this != &other) {
    set_delegate (other.mp_delegate->clone ());
  }
  return *this;
}

EdgePairs::EdgePairs (const RecursiveShapeIterator &si)
{
  mp_delegate = new OriginalLayerEdgePairs (si);
}

EdgePairs::EdgePairs (const RecursiveShapeIterator &si, const geo::ICplxTrans &trans)
{
  mp_delegate = new OriginalLayerEdgePairs (si, trans);
}

EdgePairs::EdgePairs (const RecursiveShapeIterator &si, DeepShapeStore &dss)
{
  mp_delegate = new DeepEdgePairs (si, dss);
}

EdgePairs::EdgePairs (const RecursiveShapeIterator &si, DeepShapeStore &dss, const geo::ICplxTrans &trans)
{
  mp_delegate = new DeepEdgePairs (si, dss, trans);
}

template <class Sh>
void EdgePairs::insert (const Sh &shape)
{
  mutable_edge_pairs ()->insert (shape);
}

template DB_PUBLIC void EdgePairs::insert (const geo::EdgePair &);

void EdgePairs::insert (const geo::Shape &shape)
{
  mutable_edge_pairs ()->insert (shape);
}

template <class T>
void EdgePairs::insert (const geo::Shape &shape, const T &trans)
{
  mutable_edge_pairs ()->insert (shape, trans);
}

template DB_PUBLIC void EdgePairs::insert (const geo::Shape &, const geo::ICplxTrans &);
template DB_PUBLIC void EdgePairs::insert (const geo::Shape &, const geo::Trans &);
template DB_PUBLIC void EdgePairs::insert (const geo::Shape &, const geo::Disp &);

void EdgePairs::clear ()
{
  set_delegate (new EmptyEdgePairs ());
}

void EdgePairs::reserve (size_t n)
{
  mutable_edge_pairs ()->reserve (n);
}

void EdgePairs::flatten ()
{
  mutable_edge_pairs ()->flatten ();
}

template <class T>
EdgePairs &EdgePairs::transform (const T &trans)
{
  mutable_edge_pairs ()->transform (trans);
  return *this;
}

//  explicit instantiations
template DB_PUBLIC EdgePairs &EdgePairs::transform (const geo::ICplxTrans &);
template DB_PUBLIC EdgePairs &EdgePairs::transform (const geo::Trans &);
template DB_PUBLIC EdgePairs &EdgePairs::transform (const geo::Disp &);
template DB_PUBLIC EdgePairs &EdgePairs::transform (const geo::IMatrix2d &);
template DB_PUBLIC EdgePairs &EdgePairs::transform (const geo::IMatrix3d &);

const geo::RecursiveShapeIterator &
EdgePairs::iter () const
{
  static geo::RecursiveShapeIterator def_iter;
  const geo::RecursiveShapeIterator *i = mp_delegate->iter ();
  return *(i ? i : &def_iter);
}

void EdgePairs::processed (Region &output, const EdgePairToPolygonProcessorBase &filter) const
{
  output = Region (mp_delegate->processed_to_polygons (filter));
}

void EdgePairs::processed (Edges &output, const EdgePairToEdgeProcessorBase &filter) const
{
  output = Edges (mp_delegate->processed_to_edges (filter));
}

void EdgePairs::polygons (Region &output, geo::Coord e) const
{
  output.set_delegate (mp_delegate->polygons (e));
}

void EdgePairs::edges (Edges &output) const
{
  output.set_delegate (mp_delegate->edges ());
}

void EdgePairs::first_edges (Edges &output) const
{
  output.set_delegate (mp_delegate->first_edges ());
}

void EdgePairs::second_edges (Edges &output) const
{
  output.set_delegate (mp_delegate->second_edges ());
}

void EdgePairs::set_delegate (EdgePairsDelegate *delegate)
{
  if (delegate != mp_delegate) {
    delete mp_delegate;
    mp_delegate = delegate;
  }
}

MutableEdgePairs *EdgePairs::mutable_edge_pairs ()
{
  MutableEdgePairs *edge_pairs = dynamic_cast<MutableEdgePairs *> (mp_delegate);
  if (! edge_pairs) {
    edge_pairs = new FlatEdgePairs ();
    if (mp_delegate) {
      edge_pairs->EdgePairsDelegate::operator= (*mp_delegate);
      edge_pairs->insert_seq (begin ());
    }
    set_delegate (edge_pairs);
  }

  return edge_pairs;
}

}

namespace geo
{
  template<> DB_PUBLIC bool test_extractor_impl (geo::Extractor &ex, geo::EdgePairs &b)
  {
    geo::EdgePair ep;

    if (ex.at_end ()) {
      return true;
    }
    if (! ex.try_read (ep)) {
      return false;
    }
    b.insert (ep);

    while (ex.test (";")) {
      ex.read (ep);
      b.insert (ep);
    } 

    return true;
  }

  template<> DB_PUBLIC void extractor_impl (geo::Extractor &ex, geo::EdgePairs &b)
  {
    if (! test_extractor_impl (ex, b)) {
      ex.error (geo::to_string (tr ("Expected an edge pair collection specification")));
    }
  }
}

