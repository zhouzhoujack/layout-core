#if !defined(HDR_geoCommon_h)
# define HDR_geoCommon_h

# if defined _WIN32 || defined __CYGWIN__

#   ifdef MAKE_DB_LIBRARY
#     define DB_PUBLIC __declspec(dllexport)
#   else
#     define DB_PUBLIC __declspec(dllimport)
#   endif
#   define DB_LOCAL
#   define DB_PUBLIC_TEMPLATE

# else

#   if __GNUC__ >= 4 || defined(__clang__)
#     define DB_PUBLIC __attribute__ ((visibility ("default")))
#     define DB_PUBLIC_TEMPLATE __attribute__ ((visibility ("default")))
#     define DB_LOCAL  __attribute__ ((visibility ("hidden")))
#   else
#     define DB_PUBLIC
#     define DB_PUBLIC_TEMPLATE
#     define DB_LOCAL
#   endif

# endif

#endif
