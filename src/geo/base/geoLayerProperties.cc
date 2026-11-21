#include "geoLayerProperties.h"
#include "geoStreamLayers.h"

namespace geo
{

// -----------------------------------------------------------------
//  Implementation of the LayerProperties class

LayerProperties::LayerProperties () 
  : name (), layer (geo::any_ld ()), datatype (geo::any_ld ())
{ }

LayerProperties::LayerProperties (int l, int d) 
  : name (), layer (l), datatype (d)
{ }

LayerProperties::LayerProperties (const std::string &n) 
  : name (n), layer (geo::any_ld ()), datatype (geo::any_ld ())
{ }

LayerProperties::LayerProperties (int l, int d, const std::string &n) 
  : name (n), layer (l), datatype (d)
{ }

bool 
LayerProperties::is_named () const
{
  return geo::is_any_ld (layer) && geo::is_any_ld (datatype) && ! name.empty ();
}

bool
LayerProperties::is_null () const
{
  return geo::is_any_ld (layer) && geo::is_any_ld (datatype) && name.empty ();
}

bool 
LayerProperties::log_equal (const LayerProperties &b) const
{
  if (is_null () != b.is_null ()) {
    return false;
  }
  if (is_named () != b.is_named ()) {
    return false;
  }
  if (is_named ()) {
    return name == b.name;
  } else {
    return layer == b.layer && datatype == b.datatype;
  }
}

bool 
LayerProperties::log_less (const LayerProperties &b) const
{
  if (is_null () != b.is_null ()) {
    return is_null () < b.is_null ();
  }
  if (is_named () != b.is_named ()) {
    return is_named () < b.is_named ();
  }
  if (is_named ()) {
    return name < b.name;
  } else {
    if (layer != b.layer) { return layer < b.layer; }
    return datatype < b.datatype;
  }
}

bool 
LayerProperties::operator== (const LayerProperties &b) const
{
  if (is_null () != b.is_null ()) {
    return false;
  }
  if (is_named () != b.is_named ()) {
    return false;
  }
  if (is_named ()) {
    return name == b.name;
  } else {
    return layer == b.layer && datatype == b.datatype && name == b.name;
  }
}

bool 
LayerProperties::operator!= (const LayerProperties &b) const
{
  return ! operator== (b);
}

bool 
LayerProperties::operator< (const LayerProperties &b) const
{
  if (is_null () != b.is_null ()) {
    return is_null () < b.is_null ();
  }
  if (is_named () != b.is_named ()) {
    return is_named () < b.is_named ();
  }
  if (! is_named ()) {
    if (layer != b.layer) { return layer < b.layer; }
    if (datatype != b.datatype) { return datatype < b.datatype; }
  }
  return name < b.name;
}

static std::string format_ld (geo::ld_type ld)
{
  if (geo::is_static_ld (ld)) {
    return geo::to_string (ld);
  } else if (geo::is_any_ld (ld)) {
    return "*";
  } else if (geo::is_relative_ld (ld)) {
    geo::ld_type offset = geo::ld_offset (ld);
    if (offset < 0) {
      return "*-" + geo::to_string (-offset);
    } else {
      return "*+" + geo::to_string (offset);
    }
  } else {
    return geo::to_string (ld);
  }
}

std::string 
LayerProperties::to_string (bool as_target) const
{
  std::string r;
  if (! name.empty ()) {
    if (is_named ()) {
      r = geo::to_word_or_quoted_string (name);
    } else {
      r = geo::to_word_or_quoted_string (name) + " (" + format_ld (layer) + "/" + format_ld (datatype) + ")";
    }
  } else if (! is_null () || as_target) {
    r = format_ld (layer) + "/" + format_ld (datatype);
  }
  return r;
}

static bool read_ld (geo::Extractor &ex, ld_type &l, bool with_relative)
{
  if (ex.test ("*")) {

    int offset = 0;

    geo::Extractor eex = ex;
    if (with_relative && eex.test ("+") && eex.try_read (offset)) {
      l = geo::relative_ld (offset);
      ex = eex;
    } else {
      eex = ex;
      if (with_relative && eex.test ("-") && eex.try_read (offset)) {
        l = geo::relative_ld (-offset);
        ex = eex;
      } else {
        l = geo::any_ld ();
      }
    }

    return true;

  } else {
    return ex.try_read (l);
  }
}

void
LayerProperties::read (geo::Extractor &ex, bool as_target)
{
  layer = geo::any_ld ();
  datatype = geo::any_ld ();
  name.clear ();

  int l = 0, d = 0;
  if (read_ld (ex, l, as_target)) {

    if (ex.test ("/")) {
      read_ld (ex, d, as_target);
    }

    layer = l;
    datatype = d;

  } else if (ex.try_read_word_or_quoted (name)) {

    if (ex.test ("(")) {

      read_ld (ex, l, as_target);
      if (ex.test ("/")) {
        read_ld (ex, d, as_target);
      }

      ex.expect (")");

      layer = l;
      datatype = d;

    }

  }
}

// -----------------------------------------------------------------
//  Implementation of the LayerOffset class

LayerOffset::LayerOffset () 
  : name (), layer (-1), datatype (-1)
{ }

LayerOffset::LayerOffset (int l, int d) 
  : name (), layer (l), datatype (d)
{ }

LayerOffset::LayerOffset (const std::string &n) 
  : name (n), layer (-1), datatype (-1)
{ }

LayerOffset::LayerOffset (int l, int d, const std::string &n) 
  : name (n), layer (l), datatype (d)
{ }

bool 
LayerOffset::operator== (const LayerOffset &b) const
{
  if (is_named () != b.is_named ()) {
    return false;
  }
  if (is_named ()) {
    return name == b.name;
  } else {
    return layer == b.layer && datatype == b.datatype && name == b.name;
  }
}

bool 
LayerOffset::operator!= (const LayerOffset &b) const
{
  return ! operator== (b);
}

bool 
LayerOffset::operator< (const LayerOffset &b) const
{
  if (is_named () != b.is_named ()) {
    return is_named () < b.is_named ();
  }
  if (! is_named ()) {
    if (layer != b.layer) { return layer < b.layer; }
    if (datatype != b.datatype) { return datatype < b.datatype; }
  } 
  return name < b.name;
}

bool 
LayerOffset::is_named () const
{
  return layer < 0 || datatype < 0;
}

std::string 
LayerOffset::to_string () const
{
  std::string r;
  if (! name.empty ()) {
    if (is_named ()) {
      r = geo::to_word_or_quoted_string (name, "_.$\\*");
    } else {
      r = geo::to_word_or_quoted_string (name, "_.$\\*") + geo::sprintf (" (%d/%d)", layer, datatype);
    }
  } else if (! is_named ()) {
    r = geo::sprintf ("%d/%d", layer, datatype);
  }
  return r;
}

void
LayerOffset::read (geo::Extractor &ex)
{
  layer = -1;
  datatype = -1;
  name.clear ();

  int l = 0, d = 0;
  if (ex.try_read (l)) {

    if (ex.test ("/")) {
      ex.read (d);
    }

    layer = l;
    datatype = d;

  } else if (ex.try_read_word_or_quoted (name, "_.$\\*")) {

    if (ex.test ("(")) {

      ex.read (l);
      if (ex.test ("/")) {
        ex.read (d);
      }
      ex.expect (")");

      layer = l;
      datatype = d;

    }

  }
}

LayerProperties 
LayerOffset::apply (const LayerProperties &props) const
{
  LayerProperties p (props);
  if (layer > 0 && p.layer >= 0) {
    p.layer += layer;
  }
  if (datatype > 0 && p.datatype >= 0) {
    p.datatype += datatype;
  }

  if (is_named () && p.is_named ()) {
    std::string new_name;
    for (const char *cp = name.c_str (); *cp; ++cp) {
      if (*cp == '\\' && cp[1]) {
        new_name += cp[1];
        ++cp;
      } else if (*cp == '*') {
        new_name += p.name;
      } 
    }
    p.name = new_name;
  }

  return p;
}

}

namespace geo
{

template<> bool test_extractor_impl (geo::Extractor &ex, geo::LayerProperties &e)
{
  e.read (ex);
  return true; // TODO: never returns false!
}

template<> bool test_extractor_impl (geo::Extractor &ex, geo::LayerOffset &e)
{
  e.read (ex);
  return true; // TODO: never returns false!
}

template<> DB_PUBLIC void extractor_impl (geo::Extractor &ex, geo::LayerProperties &e)
{
  if (! test_extractor_impl (ex, e)) {
    ex.error (geo::to_string (tr ("Expected a layer specification")));
  }
}

template<> DB_PUBLIC void extractor_impl (geo::Extractor &ex, geo::LayerOffset &e)
{
  if (! test_extractor_impl (ex, e)) {
    ex.error (geo::to_string (tr ("Expected a layer offset specification")));
  }
}

}

