#ifndef HDR_geoEmptyTexts
#define HDR_geoEmptyTexts

#include "geoCommon.h"

#include "geoTextsDelegate.h"
#include "geoRecursiveShapeIterator.h"

namespace geo {

/**
 *  @brief The delegate for the actual edge set implementation
 */
class DB_PUBLIC EmptyTexts
  : public TextsDelegate
{
public:
  EmptyTexts ();
  EmptyTexts (const EmptyTexts &other);

  virtual TextsDelegate *clone () const;

  virtual std::string to_string (size_t) const { return std::string (); }

  virtual TextsIteratorDelegate *begin () const { return 0; }
  virtual std::pair<geo::RecursiveShapeIterator, geo::ICplxTrans> begin_iter () const { return std::make_pair (geo::RecursiveShapeIterator (), geo::ICplxTrans ()); }

  virtual bool empty () const { return true; }
  virtual size_t count () const { return 0; }
  virtual size_t hier_count () const { return 0; }

  virtual Box bbox () const { return Box (); }

  virtual TextsDelegate *filter_in_place (const TextFilterBase &) { return this; }
  virtual TextsDelegate *filtered (const TextFilterBase &) const { return new EmptyTexts (); }

  virtual RegionDelegate *processed_to_polygons (const TextToPolygonProcessorBase &) const;

  virtual RegionDelegate *polygons (geo::Coord e) const;
  virtual EdgesDelegate *edges () const;

  virtual TextsDelegate *add_in_place (const Texts &other);
  virtual TextsDelegate *add (const Texts &other) const;

  virtual TextsDelegate *in (const Texts &, bool) const { return new EmptyTexts (); }

  virtual const geo::Text *nth (size_t) const { tl_assert (false); }
  virtual bool has_valid_texts () const { return true; }

  virtual const geo::RecursiveShapeIterator *iter () const { return 0; }

  virtual bool equals (const Texts &other) const;
  virtual bool less (const Texts &other) const;

  virtual void insert_into (Layout *, geo::cell_index_type, unsigned int) const { }
  virtual void insert_into_as_polygons (Layout *, geo::cell_index_type, unsigned int, geo::Coord) const { }

  virtual RegionDelegate *pull_interacting (const Region &) const;
  virtual TextsDelegate *selected_interacting (const Region &) const;
  virtual TextsDelegate *selected_not_interacting (const Region &) const;

private:
  EmptyTexts &operator= (const EmptyTexts &other);
};

}

#endif

