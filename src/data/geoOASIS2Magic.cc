#include <stddef.h>

#include "geoOASIS2.h"

namespace geo
{

//  OASIS magic bytes — shared by the writer (db-io) and the reader (reader).
//  Kept here so both libraries use a single definition of this constant.
const char oasis_magic [] = "%SEMI-OASIS\015\012";

}
