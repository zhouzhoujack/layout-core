#ifndef HDR_utilsVector
#define HDR_utilsVector

#include <vector>
#include "utilsTypeTraits.h"

namespace geo 
{

/**
 *  @brief a specialized vector class that is (once) supposed to support garbage collection mechanisms
 *
 *  The general idea to use vectors as containers as far as possible. 
 *  This special incarnation of a vector is supposed to: 
 *  1.) provide fast allocation schemes using thread-local storage
 *      and per-object size freelists.
 *  2.) by use of a special allocator provide a garbage collection 
 *      mechanism that may move and compact the blocks allocated by
 *      the vectors.
 *  3.) Avoid memory fragmentation by using blocks with a maximum size
 */

template <class T>
class uvector 
  : public std::vector<T>
{
public:
  typedef std::vector<T> base;

  /**
   *  @brief Default constructor: creates an empty vector
   */
  uvector () : std::vector<T> () { }

  /**
   *  @brief Copy constructor
   */
  explicit uvector (const geo::uvector<T> &d) : base (d) { }

  /**
   *  @brief Move constructor
   */
  explicit uvector (const geo::uvector<T> &&d) : base (d) { }

  /**
   *  @brief Assignment
   */
  uvector &operator= (const geo::uvector<T> &d)
  {
    if (&d != this) {
      base::operator= (d);
    }
    return *this;
  }

  /**
   *  @brief Assignment (Move)
   */
  uvector &operator= (const geo::uvector<T> &&d)
  {
    if (&d != this) {
      base::operator= (d);
    }
    return *this;
  }

  /**
   *  @brief Initialization with value and length
   */
  uvector (const T &v, int s) : base (v, s) { }
};

/**
 *  @brief The type traits for the vector type
 */
template <class C>
struct type_traits <geo::uvector<C> > : public type_traits<void>
{
#if defined(_ITERATOR_DEBUG_LEVEL) && _ITERATOR_DEBUG_LEVEL != 0
  //  With iterator debugging on, the vector carries additional
  //  information which cannot be copied trivially
  typedef complex_relocate_required relocate_requirements;
#else
  typedef trivial_relocate_required relocate_requirements;
#endif
  typedef true_tag has_efficient_swap;
  typedef false_tag supports_extractor;
  typedef false_tag supports_to_string;
  typedef true_tag has_less_operator;
  typedef true_tag has_equal_operator;
};

} // namespace geo

#endif

