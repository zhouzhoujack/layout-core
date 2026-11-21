#ifndef HDR_geoOriginalLayerRegion
#define HDR_geoOriginalLayerRegion

#include "geoCommon.h"

#include "geoAsIfFlatRegion.h"

namespace geo {

class EdgesDelegate;
class RegionDelegate;
class DeepShapeStore;

/**
 *  @brief An original layerregion based on a RecursiveShapeIterator
 */
class DB_PUBLIC OriginalLayerRegion
  : public AsIfFlatRegion
{
public:
  OriginalLayerRegion ();
  OriginalLayerRegion (const OriginalLayerRegion &other);
  OriginalLayerRegion (const RecursiveShapeIterator &si, bool is_merged = false);
  OriginalLayerRegion (const RecursiveShapeIterator &si, const geo::ICplxTrans &trans, bool merged_semantics, bool is_merged = false);
  virtual ~OriginalLayerRegion ();

  RegionDelegate *clone () const;

  virtual RegionIteratorDelegate *begin () const;
  virtual RegionIteratorDelegate *begin_merged () const;

  virtual std::pair<geo::RecursiveShapeIterator, geo::ICplxTrans> begin_iter () const;
  virtual std::pair<geo::RecursiveShapeIterator, geo::ICplxTrans> begin_merged_iter () const;

  virtual bool empty () const;

  virtual bool is_merged () const;
  virtual size_t count () const;
  virtual size_t hier_count () const;

  virtual const geo::Polygon *nth (size_t n) const;
  virtual bool has_valid_polygons () const;
  virtual bool has_valid_merged_polygons () const;

  virtual const geo::RecursiveShapeIterator *iter () const;

  virtual bool equals (const Region &other) const;
  virtual bool less (const Region &other) const;

  virtual void insert_into (Layout *layout, geo::cell_index_type into_cell, unsigned int into_layer) const;

protected:
  virtual void merged_semantics_changed ();
  virtual void min_coherence_changed ();

private:
  OriginalLayerRegion &operator= (const OriginalLayerRegion &other);

  bool m_is_merged;
  mutable geo::Shapes m_merged_polygons;
  mutable bool m_merged_polygons_valid;
  mutable geo::RecursiveShapeIterator m_iter;
  geo::ICplxTrans m_iter_trans;

  void init ();
  void ensure_merged_polygons_valid () const;
};

}

#endif

