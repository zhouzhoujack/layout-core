#pragma once

#include "utilsDefs.h"

#if !defined(HDR_utilsCommon_h)
# define HDR_utilsCommon_h

# ifdef MAKE_TL_LIBRARY
#   define TL_PUBLIC           DEF_INSIDE_PUBLIC
#   define TL_PUBLIC_TEMPLATE  DEF_INSIDE_PUBLIC_TEMPLATE
#   define TL_LOCAL            DEF_INSIDE_LOCAL
# else
#   define TL_PUBLIC           DEF_OUTSIDE_PUBLIC
#   define TL_PUBLIC_TEMPLATE  DEF_OUTSIDE_PUBLIC_TEMPLATE
#   define TL_LOCAL            DEF_OUTSIDE_LOCAL
# endif

//  NOTE: this is required because we have some forward declarations to
//  gsi::Class and gsi::ClassBase in tlVariant.h.
//  TODO: there should not be any dependency of tl on gsi.
# ifdef MAKE_GSI_LIBRARY
#   define GSI_PUBLIC           DEF_INSIDE_PUBLIC
#   define GSI_PUBLIC_TEMPLATE  DEF_INSIDE_PUBLIC_TEMPLATE
#   define GSI_LOCAL            DEF_INSIDE_LOCAL
# else
#   define GSI_PUBLIC           DEF_OUTSIDE_PUBLIC
#   define GSI_PUBLIC_TEMPLATE  DEF_OUTSIDE_PUBLIC_TEMPLATE
#   define GSI_LOCAL            DEF_OUTSIDE_LOCAL
# endif

#endif
