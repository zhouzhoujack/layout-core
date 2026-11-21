#ifndef HDR_geoInit
#define HDR_geoInit


#include <string>
#include <list>
#include <vector>

namespace geo
{

/**
 *  @brief A tiny structure describing a db plugin
 */
struct PluginDescriptor
{
  std::string version;
  std::string path;
  std::string description;

  PluginDescriptor ()
  { }
};

/**
 *  @brief The main initialization function for the "db" module
 *  This function needs to be called initially by all code using the db
 *  module. It will load the plugins and perform initialization of all
 *  of them.
 */
void init (const std::vector<std::string> &paths = std::vector<std::string> ());

/**
 *  @brief Gets a list of all plugins registered
 */
const std::list<geo::PluginDescriptor> &plugins ();

}

#endif
