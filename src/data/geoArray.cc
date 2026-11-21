#include "geoArray.h"

namespace geo
{

// -----------------------------------------------------------------------------
//  Array implementation

ArrayRepository::ArrayRepository ()
{
  //  .. nothing yet ..
}

ArrayRepository::ArrayRepository (const ArrayRepository &d)
{
  operator= (d);
}

ArrayRepository::~ArrayRepository ()
{
  clear ();
}

void
ArrayRepository::clear ()
{
  for (repositories::iterator r = m_reps.begin (); r != m_reps.end (); ++r) {
    for (basic_repository::iterator rr = r->begin (); rr != r->end (); ++rr) {
      delete *rr;
    }
  }
  m_reps.clear ();
}

ArrayRepository &
ArrayRepository::operator= (const ArrayRepository &d)
{
  clear ();

  for (repositories::const_iterator r = d.m_reps.begin (); r != d.m_reps.end (); ++r) {
    m_reps.push_back (basic_repository ());
    for (basic_repository::const_iterator rr = r->begin (); rr != r->end (); ++rr) {
      m_reps.back ().insert ((*rr)->basic_clone ());
    }
  }
  return *this;
}

void
ArrayRepository::mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, bool no_self, void *parent) const
{
  geo::mem_stat (stat, purpose, cat, m_reps, no_self, parent);
  for (repositories::const_iterator r = m_reps.begin (); r != m_reps.end (); ++r) {
    for (basic_repository::const_iterator rr = r->begin (); rr != r->end (); ++rr) {
      geo::mem_stat (stat, purpose, cat, *rr, false, (void *) this);
    }
  }
}

}

