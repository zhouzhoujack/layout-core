#include "utilsEnv.h"
#include "utilsString.h"

#include <string>

#include <unistd.h>

namespace geo
{
// -----------------------------------------------------------------------------
//  Env implementation


std::string get_env (const std::string &name, const std::string &def_value)
{
  char *env = getenv (name.c_str ());
  if (env) {
    return geo::system_to_string (env);
  } else {
    return def_value;
  }
}

bool has_env (const std::string &name)
{
  char *env = getenv (name.c_str ());
  return env != 0;
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


