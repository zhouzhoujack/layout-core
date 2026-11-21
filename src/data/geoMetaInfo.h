#ifndef HDR_geoMetaInfo
#define HDR_geoMetaInfo

// #include "utilsMemStatistics.h"
#include <string>

namespace geo
{

/**
 *  @brief A structure describing the meta information from the reader
 *
 *  In the meta information block, the reader provides additional information
 *  about the file and content etc.
 *  "name" is a unique name that can be used to identify the information.
 *  "description" is a "speaking" description of the information.
 *  "value" is the value of the specific part of meta information.
 */
struct MetaInfo
{
  MetaInfo (const std::string &n, const std::string &d, const std::string &v)
    : name (n), description (d), value (v)
  {
    //  .. nothing else ..
  }

  MetaInfo ()
  {
    //  .. nothing else ..
  }

  std::string name;
  std::string description;
  std::string value;
};

// /**
//  *  @brief Collect memory statistics
//  */
// inline void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, const MetaInfo &v, bool no_self, void *parent)
// {
//   geo::mem_stat (stat, purpose, cat, v.name, no_self, parent);
//   geo::mem_stat (stat, purpose, cat, v.description, no_self, parent);
//   geo::mem_stat (stat, purpose, cat, v.value, no_self, parent);
// }

}

#endif

