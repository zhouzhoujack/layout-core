#ifndef HDR_geoShapeRepository
#define HDR_geoShapeRepository


#include "dataObjectTag.h"
#include "geoTrans.h"
#include "geoBox.h"

#include "utilsMemStatistics.h"

#include <set>

namespace geo {

template <class C> class polygon;
template <class C> class simple_polygon;
template <class C> class path;
template <class C> class edge;
template <class C> class text;
template <class C> class user_object;

/**
 *  @brief A repository for a certain shape type
 *
 *  The repository is basically a set of shapes that
 *  can be used to store duplicates of shapes in an
 *  efficient way.
 */

template <class Sh>
class repository
{
public:
  typedef typename Sh::coord_type coord_type;
  typedef std::set<Sh> set_type;
  typedef typename set_type::const_iterator iterator;

  /** 
   *  @brief The standard constructor
   */
  repository ()
    : m_set ()
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Insert a shape into the repository
   *
   *  Inserts a shape into the repository.
   *
   *  @return A pointer to the instance of the identical shape
   */
  const Sh *insert (const Sh &shape)
  {
    typename std::set<Sh>::iterator f = m_set.insert (shape).first;
    return &(*f);
  }

  /**
   *  @brief Report the number of shapes in this repository
   */
  size_t size () const
  {
    return m_set.size ();
  }

  /**
   *  @brief begin iterator of the repository
   */
  iterator begin () const
  {
    return m_set.begin ();
  }

  /**
   *  @brief end iterator of the repository
   */
  iterator end () const
  {
    return m_set.end ();
  }

  void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, bool no_self, void *parent) const
  {
    geo::mem_stat (stat, purpose, cat, m_set, no_self, parent);
  }

private:
  set_type m_set;
};

/**
 *  @brief Collect memory statistics
 */
template <class Sh>
inline void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, const repository<Sh> &x, bool no_self = false, void *parent = 0)
{
  x.mem_stat (stat, purpose, cat, no_self, parent);
}

/**
 *  @brief A repository for many shape types
 *
 *  This generic repository is providing a repository for several
 *  shape types, even if these repositories are never used. 
 */

template <class C>
class generic_repository
{
public:
  typedef C coord_type;

  /**
   *  @brief Return the repository by tag 
   */
  geo::repository< geo::polygon<C> > &repository (geo::object_tag< geo::polygon<C> > /*tag*/)
  {
    return m_polygon_repository;
  }

  /**
   *  @brief Return the repository by tag 
   */
  geo::repository< geo::simple_polygon<C> > &repository (geo::object_tag< geo::simple_polygon<C> > /*tag*/)
  {
    return m_simple_polygon_repository;
  }

  /**
   *  @brief Return the repository by tag 
   */
  geo::repository< geo::path<C> > &repository (geo::object_tag< geo::path<C> > /*tag*/)
  {
    return m_path_repository;
  }

  /**
   *  @brief Return the repository by tag 
   */
  geo::repository< geo::text<C> > &repository (geo::object_tag< geo::text<C> > /*tag*/)
  {
    return m_text_repository;
  }

  /**
   *  @brief Return the repository by tag
   */
  template <class Sh>
  const geo::repository<Sh> &repository (geo::object_tag<Sh> tag) const
  {
    return const_cast<generic_repository<C> *> (this)->repository (tag);
  }

  void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, bool no_self, void *parent) const
  {
    geo::mem_stat (stat, purpose, cat, m_polygon_repository, no_self, parent);
    geo::mem_stat (stat, purpose, cat, m_simple_polygon_repository, no_self, parent);
    geo::mem_stat (stat, purpose, cat, m_path_repository, no_self, parent);
    geo::mem_stat (stat, purpose, cat, m_text_repository, no_self, parent);
  }

private:
  geo::repository< geo::polygon<C> > m_polygon_repository;
  geo::repository< geo::simple_polygon<C> > m_simple_polygon_repository;
  geo::repository< geo::path<C> > m_path_repository;
  geo::repository< geo::text<C> > m_text_repository;
};

/**
 *  @brief Collect memory statistics
 */
template <class C>
inline void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, const generic_repository<C> &x, bool no_self = false, void *parent = 0)
{
  x.mem_stat (stat, purpose, cat, no_self, parent);
}

/**
 *  @brief Standard repository typedef
 */
typedef geo::generic_repository<geo::Coord> GenericRepository;

/** 
 *  @brief A generic shape reference
 *
 *  A shape reference is basically a proxy to an actual shape  and
 *  is used to implement shape references with a repository.
 */

template <class Sh, class Trans>
struct shape_ref
{
  typedef Sh shape_type;
  typedef typename Sh::coord_type coord_type;
  typedef Trans trans_type;
  typedef geo::generic_repository<coord_type> repository_type;
  typedef geo::object_tag<shape_ref<Sh, Trans> > tag;

  /**
   *  @brief The default constructor.
   *  
   *  The default constructor creates a invalid polygon reference
   */
  shape_ref ()
    : m_ptr (0)
  {
    // .. nothing yet ..
  }

  /**
   *  @brief The constructor taking a shape pointer and a transformation.
   *  
   *  It is assumed that the shape is stored in a proper repository already
   */
  shape_ref (const Sh *ptr, const Trans &trans)
    : m_ptr (ptr), m_trans (trans)
  {
    // .. nothing yet ..
  }

  /**
   *  @brief The constructor creating a reference from an actual shape
   */
  shape_ref (const shape_type &p, repository_type &rep)
    : m_ptr (0)
  {
    shape_type p_red (p);
    p_red.reduce (m_trans);
    m_ptr = rep.repository (typename Sh::tag ()).insert (p_red);
  }

  /**
   *  @brief The translation constructor.
   *  
   *  This constructor allows one to copy a shape reference from one
   *  repository to another
   */
  shape_ref (const shape_ref<Sh, Trans> &ref, repository_type &rep)
    : m_ptr (0)
  {
    if (! ref.is_null ()) {
      m_trans = ref.trans ();
      m_ptr = rep.repository (typename Sh::tag ()).insert (ref.obj ());
    }
  }

  /**
   *  @brief The translation operator.
   *  
   *  This assignment allows assigning a reference in one repository
   *  to a reference in another repository
   */
  void translate (const shape_ref<Sh, Trans> &ref, repository_type &rep, geo::ArrayRepository &)
  {
    if (! ref.is_null ()) {
      m_trans = ref.trans ();
      m_ptr = rep.repository (typename Sh::tag ()).insert (ref.obj ());
    } else {
      m_ptr = 0;
    }
  }

  /**
   *  @brief The translation operator with transformation.
   *  
   *  This assignment allows assigning a reference in one repository
   *  to a reference in another repository
   */
  template <class T>
  void translate (const shape_ref<Sh, Trans> &ref, const T &t, repository_type &rep, geo::ArrayRepository &)
  {
    if (! ref.is_null ()) {

      m_trans = Trans ();
      shape_type p_red (ref.instantiate ().transformed (t));
      p_red.reduce (m_trans);
      m_ptr = rep.repository (typename Sh::tag ()).insert (p_red);

    } else {
      m_ptr = 0;
    }
  }

  /**
   *  @brief The translation operator.
   *  
   *  This operator allows changing a reference to another repository.
   */
  void translate (repository_type &rep)
  {
    if (! is_null ()) {
      m_ptr = rep.repository (typename Sh::tag ()).insert (obj ());
    } else {
      m_ptr = 0;
    }
  }

  /** 
   *  @brief Equality test
   *
   *  This test assumes that the source and target are from the same
   *  repository so it is sufficient to compare transformations and pointers.
   */
  bool operator== (const shape_ref<Sh, Trans> &b) const
  {
    return m_trans == b.m_trans && m_ptr == b.m_ptr;
  }

  /** 
   *  @brief Inequality test
   */
  bool operator!= (const shape_ref<Sh, Trans> &b) const
  {
    return !operator== (b);
  }

  /**
   *  @brief Transform the shape reference.
   *
   *  Transforms the shape with the given transformation.
   *  Modifies the shape with the transformed shape.
   *  
   *  @param t The transformation to apply.
   *
   *  @return The transformed shape reference.
   */
  shape_ref<Sh, Trans> &transform (const trans_type &t)
  {
    m_trans = t * m_trans;
    return *this;
  }

  /**
   *  @brief Returns the bounding box of the shape
   */
  geo::box<coord_type> box () const
  {
    tl_assert (m_ptr != 0);
    return m_trans * m_ptr->box ();
  }

  /**
   *  @brief Tell if the shape reference is an invalid reference
   */
  bool is_null () const
  {
    return m_ptr == 0;
  }

  /**
   *  @brief Return the reference the referenced shape
   */
  const shape_type &obj () const
  {
    tl_assert (m_ptr != 0);
    return *m_ptr;
  }

  /**
   *  @brief Return the Pointer the referenced shape
   *
   *  In contrast to obj(), this pointer can be 0 as well.
   */
  const shape_type *ptr () const
  {
    return m_ptr;
  }

  /**
   *  @brief Return the transformation to apply
   */
  const trans_type &trans () const
  {
    return m_trans;
  }

  /**
   *  @brief Instantiate the shape 
   */
  Sh instantiate () const
  {
    Sh shape = obj ();
    shape.transform (trans ());
    return shape;
  }

  /**
   *  @brief Instantiate the shape (other, faster version)
   */
  void instantiate (Sh &shape) const
  {
    shape = obj ();
    shape.transform (trans ());
  }

  /**
   *  @brief Some sorting criterion (geometrically)
   */
  bool operator< (const shape_ref &b) const
  {
    if (m_ptr == b.m_ptr || *m_ptr == *b.m_ptr) {
      return m_trans < b.m_trans;
    } else {
      return *m_ptr < *b.m_ptr;
    }
  }

  /**
   *  @brief The string conversion function
   */
  std::string to_string () const
  {
    return obj ().to_string () + "->" + m_trans.to_string ();
  }

  /**
   *  @brief Collect memory statistics
   */
  void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, bool no_self, void *parent) const
  {
    if (!no_self) {
      stat->add (typeid (shape_ref<Sh, Trans>), (void *) this, sizeof (shape_ref<Sh, Trans>), sizeof (shape_ref<Sh, Trans>), parent, purpose, cat);
    }
    if (m_ptr) {
      geo::mem_stat (stat, purpose, cat, *m_ptr, false, (void *) this);
    }
  }

private:
  const shape_type *m_ptr;
  trans_type m_trans;
};

/**
 *  @brief Collect memory statistics
 */
template <class Sh, class Tr>
inline void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, const shape_ref<Sh, Tr> &x, bool no_self = false, void *parent = 0)
{
  x.mem_stat (stat, purpose, cat, no_self, parent);
}

/**
 *  @brief Output stream insertion operator
 */
template <class Sh, class Tr>
inline std::ostream &
operator<< (std::ostream &os, const shape_ref<Sh, Tr> &p)
{
  return (os << p.to_string ());
}

} // namespace geo

#endif

