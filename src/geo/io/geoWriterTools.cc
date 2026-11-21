#include "geoWriterTools.h" 
#include "geoLayout.h" 
#include "utilsString.h" 
#include "utilsAssert.h" 

#include <map>
#include <set>
#include <string>
#include <limits>
#include <string.h>

namespace geo
{

// -----------------------------------------------------------------------------
//  WriterTools implementation

WriterCellNameMap::WriterCellNameMap ()
  : m_max_cellname_length (std::numeric_limits<size_t>::max ())
{
  for (unsigned int i = 0; i < sizeof (m_character_trans) / sizeof (m_character_trans [0]); ++i) {
    m_character_trans [i] = 0;
  }
  allow_standard (true, true, true);
  m_default_char = '$';
}

WriterCellNameMap::WriterCellNameMap (size_t max_cellname_length)
  : m_max_cellname_length (max_cellname_length)
{
  for (unsigned int i = 0; i < sizeof (m_character_trans) / sizeof (m_character_trans [0]); ++i) {
    m_character_trans [i] = 0;
  }
  allow_standard (true, true, true);
  m_default_char = '$';
}

void 
WriterCellNameMap::replacement (char c)
{
  m_default_char = c;
}

void 
WriterCellNameMap::transform (const char *what, const char *with)
{
  size_t n = std::min (strlen (what), strlen (with));
  for (size_t i = 0; i < n; ++i) {
    m_character_trans [((unsigned int)what [i] & 0xff)] = with [i];
  }
}

void 
WriterCellNameMap::disallow_all ()
{
  for (unsigned int i = 0; i < sizeof (m_character_trans) / sizeof (m_character_trans [0]); ++i) {
    m_character_trans [i] = 0;
  }
}

void 
WriterCellNameMap::allow_all_printing ()
{
  for (unsigned char i = 0x21; i <= 0x7f; ++i) {
    m_character_trans [i] = i;
  } 
}

void 
WriterCellNameMap::allow_standard (bool upper_case, bool lower_case, bool digits)
{
  for (unsigned char i = 'A'; i <= 'Z'; ++i) {
    m_character_trans [i] = upper_case ? i : 0;
  } 
  for (unsigned char i = 'a'; i <= 'z'; ++i) {
    m_character_trans [i] = lower_case ? i : 0;
  }
  for (unsigned char i = '0'; i <= '9'; ++i) {
    m_character_trans [i] = digits ? i : 0;
  }
}

void 
WriterCellNameMap::insert (geo::cell_index_type id, const std::string &cell_name)
{
  const char *hex_format = "%c%02X";
  const char *num_format = "%c%lu";

  std::string cn_mapped;
  cn_mapped.reserve (cell_name.size ());

  for (const char *p = cell_name.c_str (); *p; ++p) {
    char c = m_character_trans [(unsigned int)*p & 0xff];
    if (c == '\0') {
      cn_mapped += m_default_char;
    } else if (c == '\t') {
      cn_mapped += geo::sprintf(hex_format, m_default_char, ((unsigned int) *p) & 0xff);
    } else {
      cn_mapped += c;
    }
  }

  if (cn_mapped.size () > m_max_cellname_length) {
    cn_mapped.erase (cn_mapped.begin () + m_max_cellname_length, cn_mapped.end ());
  }

  if (m_cell_names.find (cn_mapped) != m_cell_names.end ()) {

    std::string cn_mapped_mod;

    size_t n = 0;
    size_t m = 1;

    while (true) {

      std::string pf = geo::sprintf(num_format, m_default_char, m);
      if (pf.size () < m_max_cellname_length) {

        cn_mapped_mod.assign (cn_mapped.begin (), cn_mapped.begin () + std::min (cn_mapped.size (), m_max_cellname_length - pf.size ()));
        cn_mapped_mod += pf;
        if (m_cell_names.find (cn_mapped_mod) == m_cell_names.end ()) {
          break;
        }

      } else {
        break;
      }

      m *= 2;

    } 

    while (m > 0) {

      n += m;

      std::string pf = geo::sprintf(num_format, m_default_char, n);
      tl_assert (pf.size () < m_max_cellname_length);

      cn_mapped_mod.assign (cn_mapped.begin (), cn_mapped.begin () + std::min (cn_mapped.size (), m_max_cellname_length - pf.size ()));
      cn_mapped_mod += pf;
      if (m_cell_names.find (cn_mapped_mod) == m_cell_names.end ()) {
        n -= m;
      }

      m /= 2;

    } 

    ++n;

    std::string pf = geo::sprintf(num_format, m_default_char, n);
    tl_assert (pf.size () < m_max_cellname_length);

    cn_mapped.erase (cn_mapped.begin () + std::min (cn_mapped.size (), m_max_cellname_length - pf.size ()), cn_mapped.end ());
    cn_mapped += pf;

    tl_assert (m_cell_names.find (cn_mapped) == m_cell_names.end ());

  }

  m_map.insert (std::make_pair (id, cn_mapped));
  m_cell_names.insert (cn_mapped);
}

void 
WriterCellNameMap::insert (const geo::Layout &layout)
{
  for (geo::Layout::const_iterator c = layout.begin (); c != layout.end (); ++c) {
    insert (c->cell_index (), layout.cell_name (c->cell_index ()));
  }
}

const std::string &
WriterCellNameMap::cell_name (geo::cell_index_type id) const
{
  std::map <geo::cell_index_type, std::string>::const_iterator c = m_map.find (id);
  tl_assert (c != m_map.end ());
  return c->second;
}

}


