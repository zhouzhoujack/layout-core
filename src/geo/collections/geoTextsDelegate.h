#ifndef HDR_geoTextsDelegate
#define HDR_geoTextsDelegate

#include "geoShapeCollection.h"
#include "geoShapeCollectionUtils.h"
#include "geoGenericShapeIterator.h"
#include "geoText.h"

namespace geo {

class RecursiveShapeIterator;
class Texts;
class Region;
class TextFilterBase;
class RegionDelegate;
class EdgesDelegate;
class Layout;

typedef shape_collection_processor<geo::Text, geo::Polygon> TextToPolygonProcessorBase;

typedef geo::generic_shape_iterator_delegate_base <geo::Text> TextsIteratorDelegate;

/**
 *  @brief The delegate for the actual edge set implementation
 */
class TextsDelegate
  : public ShapeCollectionDelegateBase
{
public:
  typedef geo::Coord coord_type;
  typedef geo::coord_traits<geo::Coord> coord_traits;
  typedef geo::Text edge_pair_type;
  typedef geo::Vector vector_type;
  typedef geo::Point point_type;
  typedef geo::Box box_type;

  TextsDelegate ();
  virtual ~TextsDelegate ();

  TextsDelegate (const TextsDelegate &other);
  TextsDelegate &operator= (const TextsDelegate &other);

  virtual TextsDelegate *clone () const = 0;

  void enable_progress (const std::string &progress_desc);
  void disable_progress ();

  //  dummy features to harmonize the interface of region, edges and edge pair delegates
  void set_merged_semantics (bool) { }
  bool merged_semantics () const { return false; }
  void set_is_merged (bool) { }
  bool is_merged () const { return false; }

  virtual std::string to_string (size_t nmax) const = 0;

  virtual TextsIteratorDelegate *begin () const = 0;
  virtual std::pair<geo::RecursiveShapeIterator, geo::ICplxTrans> begin_iter () const = 0;

  virtual bool empty () const = 0;
  virtual size_t count () const = 0;
  virtual size_t hier_count () const = 0;

  virtual Box bbox () const = 0;

  virtual TextsDelegate *filter_in_place (const TextFilterBase &filter) = 0;
  virtual TextsDelegate *filtered (const TextFilterBase &filter) const = 0;
  virtual RegionDelegate *processed_to_polygons (const TextToPolygonProcessorBase &filter) const = 0;

  virtual RegionDelegate *polygons (geo::Coord e) const = 0;
  virtual EdgesDelegate *edges () const = 0;

  virtual TextsDelegate *add_in_place (const Texts &other) = 0;
  virtual TextsDelegate *add (const Texts &other) const = 0;

  virtual TextsDelegate *in (const Texts &other, bool invert) const = 0;

  virtual const geo::Text *nth (size_t n) const = 0;
  virtual bool has_valid_texts () const = 0;

  virtual const geo::RecursiveShapeIterator *iter () const = 0;

  virtual bool equals (const Texts &other) const = 0;
  virtual bool less (const Texts &other) const = 0;

  virtual void insert_into (Layout *layout, geo::cell_index_type into_cell, unsigned int into_layer) const = 0;
  virtual void insert_into_as_polygons (Layout *layout, geo::cell_index_type into_cell, unsigned int into_layer, geo::Coord enl) const = 0;

  virtual RegionDelegate *pull_interacting (const Region &) const = 0;
  virtual TextsDelegate *selected_interacting (const Region &other) const = 0;
  virtual TextsDelegate *selected_not_interacting (const Region &other) const = 0;

protected:
  const std::string &progress_desc () const
  {
    return m_progress_desc;
  }

  bool report_progress () const
  {
    return m_report_progress;
  }

private:
  bool m_report_progress;
  std::string m_progress_desc;
};

}

#endif

