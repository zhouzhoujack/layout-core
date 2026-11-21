#if !defined(HDR_utilsDefs_h)
# define HDR_utilsDefs_h

//  templates provided for building the external symbol
//  declarations per library

# if defined _WIN32 || defined __CYGWIN__

#   define DEF_INSIDE_PUBLIC __declspec(dllexport)
#   define DEF_INSIDE_LOCAL
#   define DEF_INSIDE_PUBLIC_TEMPLATE

#   define DEF_OUTSIDE_PUBLIC __declspec(dllimport)
#   define DEF_OUTSIDE_LOCAL
#   define DEF_OUTSIDE_PUBLIC_TEMPLATE

# else

#   if __GNUC__ >= 4 || defined(__clang__)
#     define DEF_INSIDE_PUBLIC __attribute__ ((visibility ("default")))
#     define DEF_INSIDE_PUBLIC_TEMPLATE __attribute__ ((visibility ("default")))
#     define DEF_INSIDE_LOCAL  __attribute__ ((visibility ("hidden")))
#   else
#     define DEF_INSIDE_PUBLIC
#     define DEF_INSIDE_PUBLIC_TEMPLATE
#     define DEF_INSIDE_LOCAL
#   endif

#   define DEF_OUTSIDE_PUBLIC DEF_INSIDE_PUBLIC
#   define DEF_OUTSIDE_PUBLIC_TEMPLATE DEF_INSIDE_PUBLIC_TEMPLATE
#   define DEF_OUTSIDE_LOCAL DEF_INSIDE_LOCAL

# endif

#endif
