#include "geoFlatTexts.h"
#include "geoEmptyTexts.h"
#include "geoTexts.h"

namespace geo
{

// -------------------------------------------------------------------------------------------------------------
//  FlatTexts implementation

FlatTexts::FlatTexts ()
  : MutableTexts (), mp_texts (new geo::Shapes (false))
{
  //  .. nothing yet ..
}

FlatTexts::~FlatTexts ()
{
  //  .. nothing yet ..
}

FlatTexts::FlatTexts (const FlatTexts &other)
  : MutableTexts (other), mp_texts (other.mp_texts)
{
  //  .. nothing yet ..
}

FlatTexts::FlatTexts (const geo::Shapes &texts)
  : MutableTexts (), mp_texts (new geo::Shapes (texts))
{
  //  .. nothing yet ..
}

void FlatTexts::invalidate_cache ()
{
  invalidate_bbox ();
}

void FlatTexts::reserve (size_t n)
{
  mp_texts->reserve (geo::Text::tag (), n);
}

TextsIteratorDelegate *FlatTexts::begin () const
{
  return new FlatTextsIterator (mp_texts.get_non_const ());
}

std::pair<geo::RecursiveShapeIterator, geo::ICplxTrans> FlatTexts::begin_iter () const
{
  return std::make_pair (geo::RecursiveShapeIterator (*mp_texts), geo::ICplxTrans ());
}

bool FlatTexts::empty () const
{
  return mp_texts->empty ();
}

size_t FlatTexts::count () const
{
  return mp_texts->size ();
}

size_t FlatTexts::hier_count () const
{
  return mp_texts->size ();
}

Box FlatTexts::compute_bbox () const
{
  mp_texts->update_bbox ();
  return mp_texts->bbox ();
}

TextsDelegate *
FlatTexts::filter_in_place (const TextFilterBase &filter)
{
  geo::Shapes &texts = *mp_texts;

  text_iterator_type pw = texts.get_layer<geo::Text, geo::unstable_layer_tag> ().begin ();
  for (TextsIterator p (begin ()); ! p.at_end (); ++p) {
    if (filter.selected (*p)) {
      if (pw == texts.get_layer<geo::Text, geo::unstable_layer_tag> ().end ()) {
        texts.get_layer<geo::Text, geo::unstable_layer_tag> ().insert (*p);
        pw = texts.get_layer<geo::Text, geo::unstable_layer_tag> ().end ();
      } else {
        texts.get_layer<geo::Text, geo::unstable_layer_tag> ().replace (pw++, *p);
      }
    }
  }

  texts.get_layer<geo::Text, geo::unstable_layer_tag> ().erase (pw, texts.get_layer<geo::Text, geo::unstable_layer_tag> ().end ());

  return this;
}

TextsDelegate *FlatTexts::add (const Texts &other) const
{
  std::unique_ptr<FlatTexts> new_texts (new FlatTexts (*this));
  new_texts->invalidate_cache ();

  FlatTexts *other_flat = dynamic_cast<FlatTexts *> (other.delegate ());
  if (other_flat) {

    new_texts->raw_texts ().insert (other_flat->raw_texts ().get_layer<geo::Text, geo::unstable_layer_tag> ().begin (), other_flat->raw_texts ().get_layer<geo::Text, geo::unstable_layer_tag> ().end ());

  } else {

    size_t n = new_texts->raw_texts ().size ();
    for (TextsIterator p (other.begin ()); ! p.at_end (); ++p) {
      ++n;
    }

    new_texts->raw_texts ().reserve (geo::Text::tag (), n);

    for (TextsIterator p (other.begin ()); ! p.at_end (); ++p) {
      new_texts->raw_texts ().insert (*p);
    }

  }

  return new_texts.release ();
}

TextsDelegate *FlatTexts::add_in_place (const Texts &other)
{
  invalidate_cache ();

  geo::Shapes &texts = *mp_texts;

  FlatTexts *other_flat = dynamic_cast<FlatTexts *> (other.delegate ());
  if (other_flat) {

    texts.insert (other_flat->raw_texts ().get_layer<geo::Text, geo::unstable_layer_tag> ().begin (), other_flat->raw_texts ().get_layer<geo::Text, geo::unstable_layer_tag> ().end ());

  } else {

    size_t n = texts.size ();
    for (TextsIterator p (other.begin ()); ! p.at_end (); ++p) {
      ++n;
    }

    texts.reserve (geo::Text::tag (), n);

    for (TextsIterator p (other.begin ()); ! p.at_end (); ++p) {
      texts.insert (*p);
    }

  }

  return this;
}

const geo::Text *FlatTexts::nth (size_t n) const
{
  return n < mp_texts->size () ? &mp_texts->get_layer<geo::Text, geo::unstable_layer_tag> ().begin () [n] : 0;
}

bool FlatTexts::has_valid_texts () const
{
  return true;
}

const geo::RecursiveShapeIterator *FlatTexts::iter () const
{
  return 0;
}

void
FlatTexts::insert_into_as_polygons (Layout *layout, geo::cell_index_type into_cell, unsigned int into_layer, geo::Coord enl) const
{
  geo::Shapes &out = layout->cell (into_cell).shapes (into_layer);
  for (TextsIterator p (begin ()); ! p.at_end (); ++p) {
    geo::Box box = p->box ();
    box.enlarge (geo::Vector (enl, enl));
    out.insert (geo::SimplePolygon (box));
  }
}

void
FlatTexts::insert_into (Layout *layout, geo::cell_index_type into_cell, unsigned int into_layer) const
{
  layout->cell (into_cell).shapes (into_layer).insert (*mp_texts);
}

void
FlatTexts::do_insert (const geo::Text &t)
{
  mp_texts->insert (t);
  invalidate_cache ();
}

}

