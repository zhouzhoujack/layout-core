#ifndef HDR_geoWriterTools
#define HDR_geoWriterTools

#include "geoCommon.h"

#include "geoTypes.h" 

#include <map>
#include <set>
#include <string>

namespace geo
{

class Layout;

/**
 *  @brief A class for cell name transformations
 *
 *  The default settings are: allow upper and lower case characters and digits.
 *  Replace all other characters with a '$'.
 */
class DB_PUBLIC WriterCellNameMap
{
public:
  /**
   *  @brief Create a cell name map with unlimited cell name length
   */
  WriterCellNameMap ();

  /**
   *  @brief Create a cell name map with the specified maximum cell name length
   */
  WriterCellNameMap (size_t max_cellname_length);

  /**
   *  @brief Specify the replacement characters.
   *
   *  This method specifies the replacement character which will replace all 
   *  non-specified characters.
   *  If the replacement character is 0, characters not allowed are ignored.
   *  If the replacement character is \t, a hex sequence will be inserted for
   *  the original characters.
   */
  void replacement (char c);

  /**
   *  @brief Specify character transformation
   *
   *  Replaces all of the characters in the first string by the ones in the second 
   *  string. Both strings must have the same length.
   *  By specifying a replacement character of \t, a hex sequence will be inserted for
   *  the original characters.
   */
  void transform (const char *what, const char *with);

  /**
   *  @brief Disallows all characters
   */
  void disallow_all ();

  /**
   *  @brief Allows the specified characters
   *
   *  @param what The characters to allow
   */
  void allow (const char *what)
  {
    transform (what, what);
  }

  /**
   *  @brief Allows all printing characters (ASCII 0x21..0x7f)
   */
  void allow_all_printing ();

  /**
   *  @brief Allow standard characters
   *
   *  Allows upper case, lower case, digits, the underscore and the dollar character
   *  or disallow them. By default, all these characters are allowed.
   */
  void allow_standard (bool upper_case, bool lower_case, bool digits);

  /**
   *  @brief Insert all cells from the given layout
   */
  void insert (const geo::Layout &layout);

  /**
   *  @brief Insert the given cell name for the given cell id
   *
   *  The name is checked for length and compliance with the character map.
   *  If the name does not comply, it is adjusted accordingly.
   */
  void insert (geo::cell_index_type id, const std::string &cell_name);

  /**
   *  @brief Obtain the output cell name for a given cell id
   *
   *  The output cell name is guaranteed to be compliant with the max cell name length
   *  and the character transformation rules.
   */
  const std::string &cell_name (geo::cell_index_type id) const;

private:
  std::map <geo::cell_index_type, std::string> m_map;
  std::set <std::string> m_cell_names;
  char m_character_trans [256];
  char m_default_char;
  size_t m_max_cellname_length;
};

}

#endif


