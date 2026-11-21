#include "geoWriter.h"

#include <map>
#include <string>

namespace geo
{

namespace
{

std::map<std::string, writer_factory_type> &
writer_factories ()
{
  static std::map<std::string, writer_factory_type> s_factories;
  return s_factories;
}

}

// -----------------------------------------------------------------------------
//  Writer factory registry

WriterBase *
create_writer_for_format (const std::string &format_name)
{
  std::map<std::string, writer_factory_type>::const_iterator f =
      writer_factories ().find (format_name);
  return (f == writer_factories ().end ()) ? 0 : f->second ();
}

WriterFactoryRegistration::WriterFactoryRegistration (const std::string &format_name,
                                                      writer_factory_type factory)
{
  writer_factories ()[format_name] = factory;
}

}
