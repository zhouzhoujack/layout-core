#include "geoOriginalLayerTexts.h"
#include "geoTexts.h"
#include "utilsInternational.h"

namespace geo
{

// -------------------------------------------------------------------------------------------------------------
//  OriginalLayerTexts implementation

namespace
{

  class OriginalLayerTextsIterator
    : public TextsIteratorDelegate
  {
  public:
    typedef geo::Text value_type;

    OriginalLayerTextsIterator (const geo::RecursiveShapeIterator &iter, const geo::ICplxTrans &trans)
      : m_rec_iter (iter), m_iter_trans (trans)
    {
      set ();
    }

    virtual bool is_addressable() const
    {
      return false;
    }

    virtual bool at_end () const
    {
      return m_rec_iter.at_end ();
    }

    virtual void increment ()
    {
      do_increment ();
      set ();
    }

    virtual const value_type *get () const
    {
      return &m_shape;
    }

    virtual OriginalLayerTextsIterator *clone () const
    {
      return new OriginalLayerTextsIterator (*this);
    }

    virtual bool equals (const generic_shape_iterator_delegate_base<value_type> *other) const
    {
      const OriginalLayerTextsIterator *o = dynamic_cast<const OriginalLayerTextsIterator *> (other);
      return o && o->m_rec_iter == m_rec_iter && o->m_iter_trans.equal (m_iter_trans);
    }

    virtual void do_reset (const geo::Box &region, bool overlapping)
    {
      if (region == geo::Box::world ()) {
        m_rec_iter.set_region (region);
      } else {
        m_rec_iter.set_region (m_iter_trans.inverted () * region);
      }
      m_rec_iter.set_overlapping (overlapping);
      set ();
    }

    virtual geo::Box bbox () const
    {
      return m_iter_trans * m_rec_iter.bbox ();
    }

  private:
    friend class Texts;

    geo::RecursiveShapeIterator m_rec_iter;
    geo::ICplxTrans m_iter_trans;
    value_type m_shape;

    void set ()
    {
      while (! m_rec_iter.at_end () && !m_rec_iter.shape ().is_text ()) {
        ++m_rec_iter;
      }
      if (! m_rec_iter.at_end ()) {
        m_rec_iter.shape ().text (m_shape);
        m_shape.transform (m_iter_trans * m_rec_iter.trans ());
      }
    }

    void do_increment ()
    {
      if (! m_rec_iter.at_end ()) {
        ++m_rec_iter;
      }
    }
  };

}

OriginalLayerTexts::OriginalLayerTexts ()
  : AsIfFlatTexts ()
{
  init ();
}

OriginalLayerTexts::OriginalLayerTexts (const OriginalLayerTexts &other)
  : AsIfFlatTexts (other),
    m_iter (other.m_iter),
    m_iter_trans (other.m_iter_trans)
{
  //  .. nothing yet ..
}

OriginalLayerTexts::OriginalLayerTexts (const RecursiveShapeIterator &si)
  : AsIfFlatTexts (), m_iter (si)
{
  init ();
}

OriginalLayerTexts::OriginalLayerTexts (const RecursiveShapeIterator &si, const geo::ICplxTrans &trans)
  : AsIfFlatTexts (), m_iter (si), m_iter_trans (trans)
{
  init ();
}

OriginalLayerTexts::~OriginalLayerTexts ()
{
  //  .. nothing yet ..
}

TextsDelegate *
OriginalLayerTexts::clone () const
{
  return new OriginalLayerTexts (*this);
}

TextsIteratorDelegate *
OriginalLayerTexts::begin () const
{
  return new OriginalLayerTextsIterator (m_iter, m_iter_trans);
}

std::pair<geo::RecursiveShapeIterator, geo::ICplxTrans>
OriginalLayerTexts::begin_iter () const
{
  return std::make_pair (m_iter, m_iter_trans);
}

bool
OriginalLayerTexts::empty () const
{
  return m_iter.at_end ();
}

const geo::Text *
OriginalLayerTexts::nth (size_t) const
{
  throw geo::Exception (geo::to_string (tr ("Random access to texts is available only for flat collections")));
}

bool
OriginalLayerTexts::has_valid_texts () const
{
  return false;
}

const geo::RecursiveShapeIterator *
OriginalLayerTexts::iter () const
{
  return &m_iter;
}

bool
OriginalLayerTexts::equals (const Texts &other) const
{
  const OriginalLayerTexts *other_delegate = dynamic_cast<const OriginalLayerTexts *> (other.delegate ());
  if (other_delegate && other_delegate->m_iter == m_iter && other_delegate->m_iter_trans == m_iter_trans) {
    return true;
  } else {
    return AsIfFlatTexts::equals (other);
  }
}

bool
OriginalLayerTexts::less (const Texts &other) const
{
  const OriginalLayerTexts *other_delegate = dynamic_cast<const OriginalLayerTexts *> (other.delegate ());
  if (other_delegate && other_delegate->m_iter == m_iter && other_delegate->m_iter_trans == m_iter_trans) {
    return false;
  } else {
    return AsIfFlatTexts::less (other);
  }
}

void
OriginalLayerTexts::init ()
{
  //  .. nothing yet ..
}

}
