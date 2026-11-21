#ifndef HDR_geoFlatEdges
#define HDR_geoFlatEdges


#include "geoMutableEdges.h"
#include "geoShapes.h"
#include "geoShapes2.h"
#include "geoGenericShapeIterator.h"
#include "utilsCopyOnWrite.h"

namespace geo {

/**
 *  @brief An iterator delegate for the flat edge set
 */
typedef generic_shapes_iterator_delegate<geo::Edge> FlatEdgesIterator;

/**
 *  @brief A flat, edge-set delegate
 */
class FlatEdges
  : public MutableEdges
{
public:
  typedef geo::Edge value_type;

  typedef geo::layer<geo::Edge, geo::unstable_layer_tag> edge_layer_type;
  typedef edge_layer_type::iterator edge_iterator_type;

  FlatEdges ();
  FlatEdges (const geo::Shapes &edges, bool is_merged);
  FlatEdges (bool is_merged);

  FlatEdges (const FlatEdges &other);

  virtual ~FlatEdges ();

  EdgesDelegate *clone () const
  {
    return new FlatEdges (*this);
  }

  void reserve (size_t);
  void flatten () { }

  virtual EdgesIteratorDelegate *begin () const;
  virtual EdgesIteratorDelegate *begin_merged () const;

  virtual std::pair<geo::RecursiveShapeIterator, geo::ICplxTrans> begin_iter () const;
  virtual std::pair<geo::RecursiveShapeIterator, geo::ICplxTrans> begin_merged_iter () const;

  virtual bool empty () const;
  virtual size_t count () const;
  virtual size_t hier_count () const;
  virtual bool is_merged () const;

  virtual void insert_into (Layout *layout, geo::cell_index_type into_cell, unsigned int into_layer) const;

  virtual EdgesDelegate *processed_in_place (const EdgeProcessorBase &filter);
  virtual EdgesDelegate *filter_in_place (const EdgeFilterBase &filter);

  virtual EdgesDelegate *add_in_place (const Edges &other);
  virtual EdgesDelegate *add (const Edges &other) const;

  virtual const geo::Edge *nth (size_t n) const;
  virtual bool has_valid_edges () const;
  virtual bool has_valid_merged_edges () const;

  virtual const geo::RecursiveShapeIterator *iter () const;

  void do_insert (const geo::Edge &edge);

  void do_transform (const geo::Trans &t)
  {
    transform_generic (t);
  }

  void do_transform (const geo::ICplxTrans &t)
  {
    transform_generic (t);
  }

  void do_transform (const geo::IMatrix2d &t)
  {
    transform_generic (t);
  }

  void do_transform (const geo::IMatrix3d &t)
  {
    transform_generic (t);
  }

  geo::Shapes &raw_edges () { return *mp_edges; }
  const geo::Shapes &raw_edges () const { return *mp_edges; }

protected:
  virtual void merged_semantics_changed ();
  virtual Box compute_bbox () const;
  void invalidate_cache ();
  void set_is_merged (bool m);

private:
  friend class AsIfFlatEdges;

  FlatEdges &operator= (const FlatEdges &other);

  bool m_is_merged;
  mutable geo::copy_on_write_ptr<geo::Shapes> mp_edges;
  mutable geo::copy_on_write_ptr<geo::Shapes> mp_merged_edges;
  mutable bool m_merged_edges_valid;

  void init ();
  void ensure_merged_edges_valid () const;

  template <class Trans>
  void transform_generic (const Trans &trans)
  {
    if (! trans.is_unity ()) {
      geo::Shapes &e = *mp_edges;
      for (edge_iterator_type p = e.template get_layer<geo::Edge, geo::unstable_layer_tag> ().begin (); p != e.get_layer<geo::Edge, geo::unstable_layer_tag> ().end (); ++p) {
        e.get_layer<geo::Edge, geo::unstable_layer_tag> ().replace (p, p->transformed (trans));
      }
      invalidate_cache ();
    }
  }
};

}

#endif

