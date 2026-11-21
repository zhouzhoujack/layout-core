#ifndef HDR_geoFlatTexts
#define HDR_geoFlatTexts

#include "geoCommon.h"

#include "geoMutableTexts.h"
#include "geoShapes.h"
#include "utilsCopyOnWrite.h"

namespace geo {

/**
 *  @brief An iterator delegate for the flat text set
 */
typedef generic_shapes_iterator_delegate<geo::Text> FlatTextsIterator;

/**
 *  @brief The delegate for the actual text set implementation
 */
class DB_PUBLIC FlatTexts
  : public MutableTexts
{
public:
  typedef geo::Text value_type;

  typedef geo::layer<geo::Text, geo::unstable_layer_tag> text_layer_type;
  typedef text_layer_type::iterator text_iterator_type;

  FlatTexts ();
  FlatTexts (const geo::Shapes &texts);

  FlatTexts (const FlatTexts &other);

  virtual ~FlatTexts ();

  TextsDelegate *clone () const
  {
    return new FlatTexts (*this);
  }

  void reserve (size_t);

  virtual TextsIteratorDelegate *begin () const;
  virtual std::pair<geo::RecursiveShapeIterator, geo::ICplxTrans> begin_iter () const;

  virtual bool empty () const;
  virtual size_t count () const;
  virtual size_t hier_count () const;

  virtual TextsDelegate *filter_in_place (const TextFilterBase &filter);

  virtual TextsDelegate *add_in_place (const Texts &other);
  virtual TextsDelegate *add (const Texts &other) const;

  virtual const geo::Text *nth (size_t n) const;
  virtual bool has_valid_texts () const;

  virtual const geo::RecursiveShapeIterator *iter () const;

  virtual void insert_into (Layout *layout, geo::cell_index_type into_cell, unsigned int into_layer) const;
  virtual void insert_into_as_polygons (Layout *layout, geo::cell_index_type into_cell, unsigned int into_layer, geo::Coord enl) const;

  virtual void flatten () { }

  void do_insert (const geo::Text &text);

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

  geo::Shapes &raw_texts () { return *mp_texts; }
  const geo::Shapes &raw_texts () const { return *mp_texts; }

protected:
  virtual Box compute_bbox () const;
  void invalidate_cache ();

private:
  friend class AsIfFlatTexts;

  FlatTexts &operator= (const FlatTexts &other);

  mutable geo::copy_on_write_ptr<geo::Shapes> mp_texts;

  template <class Trans>
  void transform_generic (const Trans &trans)
  {
    if (! trans.is_unity ()) {
      geo::Shapes &texts = *mp_texts;
      for (text_iterator_type p = texts.template get_layer<geo::Text, geo::unstable_layer_tag> ().begin (); p != texts.template get_layer<geo::Text, geo::unstable_layer_tag> ().end (); ++p) {
        texts.get_layer<geo::Text, geo::unstable_layer_tag> ().replace (p, p->transformed (trans));
      }
      invalidate_cache ();
    }
  }
};

}

#endif

