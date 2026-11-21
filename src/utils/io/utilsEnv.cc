#include "utilsEnv.h"
#include "utilsString.h"

#include <string>

#ifdef _WIN32
#  include <windows.h>
#elif __APPLE__
#  include <libproc.h>
#  include <unistd.h>
#else
#  include <unistd.h>
#endif

namespace geo
{
// -----------------------------------------------------------------------------
//  Env implementation


std::string get_env (const std::string &name, const std::string &def_value)
{
#ifdef _WIN32
  std::wstring wname = geo::to_wstring (name);
  wchar_t *env = _wgetenv (wname.c_str ());
  if (env) {
    return geo::to_string (std::wstring (env));
  } else {
    return def_value;
  }
#else
  char *env = getenv (name.c_str ());
  if (env) {
    return geo::system_to_string (env);
  } else {
    return def_value;
  }
#endif
}

bool has_env (const std::string &name)
{
#ifdef _WIN32
  std::wstring wname = geo::to_wstring (name);
  wchar_t *env = _wgetenv (wname.c_str ());
  return env != 0;
#else
  char *env = getenv (name.c_str ());
  return env != 0;
#endif
}

bool app_flag (const std::string &name)
{
  std::string env_name = std::string ("KLAYOUT_") + geo::replaced (geo::to_upper_case (name), "-", "_");

  int v = 0;
  std::string vs = get_env (env_name);
  geo::Extractor ex (vs.c_str ());
  return ex.try_read (v) && v != 0;
}

} // namespace geo


