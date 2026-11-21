#ifndef HDR_utilsTypeTraits
#define HDR_utilsTypeTraits

#include "utilsCommon.h"

namespace geo
{

/**
 *  @brief A general "true" tag
 */
struct true_tag { };

/**
 *  @brief A general "false" tag
 */
struct false_tag { };

/**
 *  @brief Convert a boolean value to a type
 */
template <bool>
struct boolean_value;

template <>
struct boolean_value<true>
{
  typedef true_tag value;
};

template <>
struct boolean_value<false>
{
  typedef false_tag value;
};

/**
 *  @brief Convert a true_tag to a boolean value
 */
inline bool value_of (true_tag) { return true; }

/**
 *  @brief Convert a false_tag to a boolean value
 */
inline bool value_of (false_tag) { return false; }

/**
 *  @brief A tag class which defines a object to require complex relocation.
 */
struct complex_relocate_required { };

/**
 *  @brief A tag class which defines a object to allow trivial relocation.
 */
struct trivial_relocate_required { };

/**
 *  @brief The type traits struct that defines some requirements for the given type T
 *
 *  Specifically the following typedefs must be provided:
 *
 *  "relocate_requirements" specifies how the object needs to be relocated. 
 *  This typdef can be complex_relocate_required or trivial_relocate_required. 
 *  Complex relocation is implemented by a copy construction and destruction of the 
 *  source object. Trivial relocation is implemented by a memcpy.
 *  The default is complex relocation.
 *
 *  "has_copy_constructor" specifies if a class has a copy constructor.
 *  This typedef can be true_tag or false_tag. The default is "true_tag".
 *
 *  "has_default_constructor" specifies if a class has a default constructor.
 *  This typedef can be true_tag or false_tag. The default is "true_tag".
 *
 *  "has_efficient_swap" specifies that it is beneficial to use std::swap
 *  on those objects because it is implemented very efficiently. The default is "false_tag".
 *
 *  TODO: further requirements shall go here.
 */
template <class T>
struct type_traits
{
  typedef complex_relocate_required relocate_requirements;
  typedef true_tag has_copy_constructor;
  typedef true_tag has_default_constructor;
  typedef true_tag has_public_destructor;
  typedef false_tag has_efficient_swap;
  typedef false_tag supports_extractor;
  typedef false_tag supports_to_string;
  typedef false_tag has_less_operator;
  typedef false_tag has_equal_operator;
};

}

#endif


