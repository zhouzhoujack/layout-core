#ifndef HDR_geoDeepTexts
#define HDR_geoDeepTexts


#include "geoMutableTexts.h"
#include "geoDeepShapeStore.h"
#include "geoTexts.h"

namespace geo {

/**
 *  @brief Provides hierarchical edges implementation
 */
class DeepTexts
  : public geo::MutableTexts, public geo::DeepShapeCollectionDelegateBase
{
public:
  DeepTexts ();
  DeepTexts (const geo::Texts &other, DeepShapeStore &dss);
  DeepTexts (const RecursiveShapeIterator &si, DeepShapeStore &dss);
  DeepTexts (const RecursiveShapeIterator &si, DeepShapeStore &dss, const geo::ICplxTrans &trans);

  DeepTexts (const DeepTexts &other);
  DeepTexts (const DeepLayer &dl);

  virtual ~DeepTexts ();

  TextsDelegate *clone () const;

  virtual void do_insert (const geo::Text &text);

  virtual void do_transform (const geo::Trans &t);
  virtual void do_transform (const geo::ICplxTrans &t);
  virtual void do_transform (const geo::IMatrix2d &t);
  virtual void do_transform (const geo::IMatrix3d &t);

  virtual void flatten ();

  virtual void reserve (size_t n);

  virtual TextsIteratorDelegate *begin () const;
  virtual std::pair<geo::RecursiveShapeIterator, geo::ICplxTrans> begin_iter () const;

  virtual size_t count () const;
  virtual size_t hier_count () const;
  virtual std::string to_string (size_t) const;
  virtual Box bbox () const;
  virtual bool empty () const;
  virtual const geo::Text *nth (size_t n) const;
  virtual bool has_valid_texts () const;
  virtual const geo::RecursiveShapeIterator *iter () const;

  virtual TextsDelegate *filter_in_place (const TextFilterBase &filter);
  virtual TextsDelegate *filtered (const TextFilterBase &) const;

  virtual RegionDelegate *processed_to_polygons (const TextToPolygonProcessorBase &filter) const;

  virtual TextsDelegate *add_in_place (const Texts &other);
  virtual TextsDelegate *add (const Texts &other) const;

  virtual RegionDelegate *polygons (geo::Coord e) const;
  virtual EdgesDelegate *edges () const;

  virtual TextsDelegate *in (const Texts &, bool) const;

  virtual bool equals (const Texts &other) const;
  virtual bool less (const Texts &other) const;

  virtual void insert_into (Layout *layout, geo::cell_index_type into_cell, unsigned int into_layer) const;
  virtual void insert_into_as_polygons (Layout *layout, geo::cell_index_type into_cell, unsigned int into_layer, geo::Coord enl) const;

  virtual DeepShapeCollectionDelegateBase *deep ()
  {
    return this;
  }

private:
  DeepTexts &operator= (const DeepTexts &other);

  void init ();
  DeepTexts *apply_filter (const TextFilterBase &filter) const;

  virtual TextsDelegate *selected_interacting_generic (const Region &other, bool inverse) const;
  virtual RegionDelegate *pull_generic (const Region &other) const;
};

}

#endif

