#if !defined(HDR_utilsDefs_h)
# define HDR_utilsDefs_h

//  templates provided for building the external symbol
//  declarations per library

# define DEF_INSIDE_PUBLIC __attribute__ ((visibility ("default")))
# define DEF_INSIDE_PUBLIC_TEMPLATE __attribute__ ((visibility ("default")))
# define DEF_INSIDE_LOCAL  __attribute__ ((visibility ("hidden")))

# define DEF_OUTSIDE_PUBLIC DEF_INSIDE_PUBLIC
# define DEF_OUTSIDE_PUBLIC_TEMPLATE DEF_INSIDE_PUBLIC_TEMPLATE
# define DEF_OUTSIDE_LOCAL DEF_INSIDE_LOCAL

#endif
