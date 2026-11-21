#ifndef HDR_geoFlatEdgePairs
#define HDR_geoFlatEdgePairs

#include "geoCommon.h"

#include "geoMutableEdgePairs.h"
#include "geoShapes.h"
#include "geoGenericShapeIterator.h"
#include "utilsCopyOnWrite.h"

namespace geo {

/**
 *  @brief An iterator delegate for the flat edge pair set
 */
typedef generic_shapes_iterator_delegate<geo::EdgePair> FlatEdgePairsIterator;

/**
 *  @brief The delegate for the actual edge pair set implementation
 */
class DB_PUBLIC FlatEdgePairs
  : public MutableEdgePairs
{
public:
  typedef geo::layer<geo::EdgePair, geo::unstable_layer_tag> edge_pair_layer_type;
  typedef edge_pair_layer_type::iterator edge_pair_iterator_type;

  FlatEdgePairs ();
  FlatEdgePairs (const geo::Shapes &edges);

  FlatEdgePairs (const FlatEdgePairs &other);

  virtual ~FlatEdgePairs ();

  EdgePairsDelegate *clone () const
  {
    return new FlatEdgePairs (*this);
  }

  void reserve (size_t);

  virtual EdgePairsIteratorDelegate *begin () const;
  virtual std::pair<geo::RecursiveShapeIterator, geo::ICplxTrans> begin_iter () const;

  virtual bool empty () const;
  virtual size_t count () const;
  virtual size_t hier_count () const;

  virtual EdgePairsDelegate *filter_in_place (const EdgePairFilterBase &filter);

  virtual EdgePairsDelegate *add_in_place (const EdgePairs &other);
  virtual EdgePairsDelegate *add (const EdgePairs &other) const;

  virtual const geo::EdgePair *nth (size_t n) const;
  virtual bool has_valid_edge_pairs () const;

  virtual const geo::RecursiveShapeIterator *iter () const;

  virtual void insert_into (Layout *layout, geo::cell_index_type into_cell, unsigned int into_layer) const;
  virtual void insert_into_as_polygons (Layout *layout, geo::cell_index_type into_cell, unsigned int into_layer, geo::Coord enl) const;

  virtual void do_insert (const geo::EdgePair &edge_pair);

  virtual void do_transform (const geo::Trans &t)
  {
    transform_generic (t);
  }

  virtual void do_transform (const geo::ICplxTrans &t)
  {
    transform_generic (t);
  }

  virtual void do_transform (const geo::IMatrix2d &t)
  {
    transform_generic (t);
  }

  virtual void do_transform (const geo::IMatrix3d &t)
  {
    transform_generic (t);
  }

  virtual void flatten () { }

  geo::Shapes &raw_edge_pairs () { return *mp_edge_pairs; }
  const geo::Shapes &raw_edge_pairs () const { return *mp_edge_pairs; }

protected:
  virtual Box compute_bbox () const;
  void invalidate_cache ();

private:
  friend class AsIfFlatEdgePairs;

  FlatEdgePairs &operator= (const FlatEdgePairs &other);

  mutable geo::copy_on_write_ptr<geo::Shapes> mp_edge_pairs;

  template <class Trans>
  void transform_generic (const Trans &trans)
  {
    if (! trans.is_unity ()) {
      geo::Shapes &ep = *mp_edge_pairs;
      for (edge_pair_iterator_type p = ep.template get_layer<geo::EdgePair, geo::unstable_layer_tag> ().begin (); p != ep.template get_layer<geo::EdgePair, geo::unstable_layer_tag> ().end (); ++p) {
        ep.get_layer<geo::EdgePair, geo::unstable_layer_tag> ().replace (p, p->transformed (trans));
      }
      invalidate_cache ();
    }
  }
};

}

#endif

