#include "geoOriginalLayerEdgePairs.h"
#include "geoEdgePairs.h"
#include "utilsInternational.h"

namespace geo
{

// -------------------------------------------------------------------------------------------------------------
//  OriginalLayerEdgePairs implementation

namespace
{

  class OriginalLayerEdgePairsIterator
    : public EdgePairsIteratorDelegate
  {
  public:
    typedef geo::EdgePair value_type;

    OriginalLayerEdgePairsIterator (const geo::RecursiveShapeIterator &iter, const geo::ICplxTrans &trans)
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

    virtual EdgePairsIteratorDelegate *clone () const
    {
      return new OriginalLayerEdgePairsIterator (*this);
    }

    virtual bool equals (const generic_shape_iterator_delegate_base<value_type> *other) const
    {
      const OriginalLayerEdgePairsIterator *o = dynamic_cast<const OriginalLayerEdgePairsIterator *> (other);
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
    friend class EdgePairs;

    geo::RecursiveShapeIterator m_rec_iter;
    geo::ICplxTrans m_iter_trans;
    value_type m_shape;

    void set ()
    {
      while (! m_rec_iter.at_end () && !m_rec_iter.shape ().is_edge_pair ()) {
        ++m_rec_iter;
      }
      if (! m_rec_iter.at_end ()) {
        m_rec_iter.shape ().edge_pair (m_shape);
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

OriginalLayerEdgePairs::OriginalLayerEdgePairs ()
  : AsIfFlatEdgePairs ()
{
  init ();
}

OriginalLayerEdgePairs::OriginalLayerEdgePairs (const OriginalLayerEdgePairs &other)
  : AsIfFlatEdgePairs (other),
    m_iter (other.m_iter),
    m_iter_trans (other.m_iter_trans)
{
  //  .. nothing yet ..
}

OriginalLayerEdgePairs::OriginalLayerEdgePairs (const RecursiveShapeIterator &si)
  : AsIfFlatEdgePairs (), m_iter (si)
{
  init ();
}

OriginalLayerEdgePairs::OriginalLayerEdgePairs (const RecursiveShapeIterator &si, const geo::ICplxTrans &trans)
  : AsIfFlatEdgePairs (), m_iter (si), m_iter_trans (trans)
{
  init ();
}

OriginalLayerEdgePairs::~OriginalLayerEdgePairs ()
{
  //  .. nothing yet ..
}

EdgePairsDelegate *
OriginalLayerEdgePairs::clone () const
{
  return new OriginalLayerEdgePairs (*this);
}

EdgePairsIteratorDelegate *
OriginalLayerEdgePairs::begin () const
{
  return new OriginalLayerEdgePairsIterator (m_iter, m_iter_trans);
}

std::pair<geo::RecursiveShapeIterator, geo::ICplxTrans>
OriginalLayerEdgePairs::begin_iter () const
{
  return std::make_pair (m_iter, m_iter_trans);
}

bool
OriginalLayerEdgePairs::empty () const
{
  return m_iter.at_end ();
}

const geo::EdgePair *
OriginalLayerEdgePairs::nth (size_t) const
{
  throw geo::Exception (geo::to_string (tr ("Random access to edge pairs is available only for flat collections")));
}

bool
OriginalLayerEdgePairs::has_valid_edge_pairs () const
{
  return false;
}

const geo::RecursiveShapeIterator *
OriginalLayerEdgePairs::iter () const
{
  return &m_iter;
}

bool
OriginalLayerEdgePairs::equals (const EdgePairs &other) const
{
  const OriginalLayerEdgePairs *other_delegate = dynamic_cast<const OriginalLayerEdgePairs *> (other.delegate ());
  if (other_delegate && other_delegate->m_iter == m_iter && other_delegate->m_iter_trans == m_iter_trans) {
    return true;
  } else {
    return AsIfFlatEdgePairs::equals (other);
  }
}

bool
OriginalLayerEdgePairs::less (const EdgePairs &other) const
{
  const OriginalLayerEdgePairs *other_delegate = dynamic_cast<const OriginalLayerEdgePairs *> (other.delegate ());
  if (other_delegate && other_delegate->m_iter == m_iter && other_delegate->m_iter_trans == m_iter_trans) {
    return false;
  } else {
    return AsIfFlatEdgePairs::less (other);
  }
}

void
OriginalLayerEdgePairs::init ()
{
  //  .. nothing yet ..
}

}
