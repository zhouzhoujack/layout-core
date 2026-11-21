#ifndef HDR_geoFlatRegion
#define HDR_geoFlatRegion

#include "geoCommon.h"

#include "geoMutableRegion.h"
#include "geoShapes.h"
#include "geoShapes2.h"
#include "utilsCopyOnWrite.h"

namespace geo {

/**
 *  @brief An iterator delegate for the flat region
 */
typedef generic_shapes_iterator_delegate<geo::Polygon> FlatRegionIterator;

/**
 *  @brief A flat, polygon-set delegate
 */
class DB_PUBLIC FlatRegion
  : public MutableRegion
{
public:
  typedef geo::Polygon value_type;
  typedef geo::layer<geo::Polygon, geo::unstable_layer_tag> polygon_layer_type;
  typedef polygon_layer_type::iterator polygon_iterator_type;

  FlatRegion ();
  FlatRegion (const geo::Shapes &polygons, bool is_merged);
  FlatRegion (bool is_merged);

  FlatRegion (const FlatRegion &other);

  virtual ~FlatRegion ();

  RegionDelegate *clone () const
  {
    return new FlatRegion (*this);
  }

  virtual void reserve (size_t);

  virtual RegionIteratorDelegate *begin () const;
  virtual RegionIteratorDelegate *begin_merged () const;

  virtual std::pair<geo::RecursiveShapeIterator, geo::ICplxTrans> begin_iter () const;
  virtual std::pair<geo::RecursiveShapeIterator, geo::ICplxTrans> begin_merged_iter () const;

  virtual bool empty () const;
  virtual size_t count () const;
  virtual size_t hier_count () const;
  virtual bool is_merged () const;

  virtual void insert_into (Layout *layout, geo::cell_index_type into_cell, unsigned int into_layer) const;

  virtual RegionDelegate *merged_in_place ();
  virtual RegionDelegate *merged_in_place (bool min_coherence, unsigned int min_wc);
  virtual RegionDelegate *merged () const;
  virtual RegionDelegate *merged (bool min_coherence, unsigned int min_wc) const
  {
    return geo::AsIfFlatRegion::merged (min_coherence, min_wc);
  }

  virtual RegionDelegate *process_in_place (const PolygonProcessorBase &filter);
  virtual RegionDelegate *filter_in_place (const PolygonFilterBase &filter);

  virtual RegionDelegate *add_in_place (const Region &other);
  virtual RegionDelegate *add (const Region &other) const;

  virtual const geo::Polygon *nth (size_t n) const;
  virtual bool has_valid_polygons () const;
  virtual bool has_valid_merged_polygons () const;

  virtual const geo::RecursiveShapeIterator *iter () const;

  void do_insert (const geo::Polygon &polygon);

  void do_transform (const geo::Trans &t)
  {
    transform_generic (t);
  }

  void do_transform (const geo::ICplxTrans &t)
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

  void flatten () { }

  geo::Shapes &raw_polygons () { return *mp_polygons; }
  const geo::Shapes &raw_polygons () const { return *mp_polygons; }

protected:
  virtual void merged_semantics_changed ();
  virtual void min_coherence_changed ();
  virtual Box compute_bbox () const;
  void invalidate_cache ();
  void set_is_merged (bool m);

private:
  friend class AsIfFlatRegion;
  friend class Region;

  FlatRegion &operator= (const FlatRegion &other);

  bool m_is_merged;
  mutable geo::copy_on_write_ptr<geo::Shapes> mp_polygons;
  mutable geo::copy_on_write_ptr<geo::Shapes> mp_merged_polygons;
  mutable bool m_merged_polygons_valid;

  void init ();
  void ensure_merged_polygons_valid () const;

  template <class Trans>
  void transform_generic (const Trans &trans)
  {
    if (! trans.is_unity ()) {
      geo::Shapes &polygons = *mp_polygons;
      for (polygon_iterator_type p = polygons.get_layer<geo::Polygon, geo::unstable_layer_tag> ().begin (); p != polygons.get_layer<geo::Polygon, geo::unstable_layer_tag> ().end (); ++p) {
        polygons.get_layer<geo::Polygon, geo::unstable_layer_tag> ().replace (p, p->transformed (trans));
      }
      invalidate_cache ();
    }
  }
};

}

#endif

