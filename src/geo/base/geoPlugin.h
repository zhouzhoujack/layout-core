#ifndef HDR_geoPlugin
#define HDR_geoPlugin

/**
 *  @brief A struct to hold the data of the plugin
 *
 *  Use it like this:
 *
 *  @code
 *  static DbPlugin plugin_desc = {
 *    "1.0", //  (const char *) version information - should be given at least
 *    0      //  (const char *) description or 0/empty if no description is given
 *  };
 *  DECLARE_DB_PLUGIN (plugin_desc);
 *  @endcode
 */
struct DbPlugin
{
  const char *version;
  const char *description;
};

/**
 *  @brief A typedef for the initialization function a plugin is supposed to expose.
 */
typedef void (*dbp_init_func_t) (const char **version, const char **description);

# if defined _WIN32 || defined __CYGWIN__
#   define DBP_PUBLIC __declspec(dllexport)
# else
#   if __GNUC__ >= 4 || defined(__clang__)
#     define DBP_PUBLIC __attribute__ ((visibility ("default")))
#   else
#     define DBP_PUBLIC
#   endif

# endif

#define DECLARE_DB_PLUGIN(desc) \
  extern "C" { \
    DBP_PUBLIC void dbp_init (const char **version, const char **description) { \
      *version = desc.version; \
      *description = desc.description; \
    } \
  }

#endif
