#ifndef HDR_geoOASIS2
#define HDR_geoOASIS2

#include <stdint.h>
#include <stddef.h>

namespace geo
{

/**
 *  @brief The OASIS magic bytes (SEMI P39)
 *
 *  "%SEMI-OASIS" followed by CR LF.
 *
 *  Defined in the header (internal linkage) so that both the reader and the
 *  writer libraries can use it without depending on a shared object file.
 */
static const char oasis_magic [] = "%SEMI-OASIS\015\012";

/**
 *  @brief The length of the OASIS magic byte sequence (13 bytes)
 */
const size_t oasis_magic_length = 13;

// ------------------------------------------------------------------
//  Record IDs (SEMI P39, table "record types")
//
//  Note: the OASIS record IDs differ from the numbering that is
//  suggested by the record names in the specification. The numbering
//  below is the one actually used in the file format.
// ------------------------------------------------------------------

const unsigned char tPAD                = 0;
const unsigned char tSTART              = 1;
const unsigned char tEND                = 2;
const unsigned char tCELLNAME_IMPLICIT  = 3;
const unsigned char tCELLNAME_EXPLICIT  = 4;
const unsigned char tTEXTSTRING_IMPLICIT= 5;
const unsigned char tTEXTSTRING_EXPLICIT= 6;
const unsigned char tPROPNAME_IMPLICIT  = 7;
const unsigned char tPROPNAME_EXPLICIT  = 8;
const unsigned char tPROPSTRING_IMPLICIT= 9;
const unsigned char tPROPSTRING_EXPLICIT= 10;
const unsigned char tLAYERNAME_IMPLICIT = 11;
const unsigned char tLAYERNAME_EXPLICIT = 12;
const unsigned char tCELL_REFNUM        = 13;   //  CELL (by reference number)
const unsigned char tCELL_NAME          = 14;   //  CELL (by cell name)
const unsigned char tXYABSOLUTE         = 15;
const unsigned char tXYRELATIVE         = 16;
const unsigned char tPLACEMENT          = 17;   //  PLACEMENT (rotation/mirror in info byte)
const unsigned char tPLACEMENT_TRANSFORM= 18;   //  PLACEMENT (with magnification/angle)
const unsigned char tTEXT               = 19;
const unsigned char tRECTANGLE          = 20;
const unsigned char tPOLYGON            = 21;
const unsigned char tPATH               = 22;
const unsigned char tTRAPEZOID          = 23;   //  with both deltas
const unsigned char tTRAPEZOID_A        = 24;   //  with delta a only
const unsigned char tTRAPEZOID_B        = 25;   //  with delta b only
const unsigned char tCTRAPEZOID         = 26;
const unsigned char tCIRCLE             = 27;
const unsigned char tPROPERTY           = 28;
const unsigned char tPROPERTY_REPEAT    = 29;
const unsigned char tXNAME_IMPLICIT     = 30;
const unsigned char tXNAME_EXPLICIT     = 31;
const unsigned char tXELEMENT           = 32;
const unsigned char tXGEOMETRY          = 33;
const unsigned char tCBLOCK             = 34;

// ------------------------------------------------------------------
//  Info byte flags shared by the geometry records
// ------------------------------------------------------------------

const unsigned char iHAS_LAYER          = 0x01;
const unsigned char iHAS_DATATYPE       = 0x02;
const unsigned char iHAS_REPETITION     = 0x04;
const unsigned char iHAS_Y              = 0x08;
const unsigned char iHAS_X              = 0x10;
const unsigned char iHAS_HEIGHT         = 0x20;   //  also: point list, radius, height
const unsigned char iHAS_WIDTH          = 0x40;   //  also: text string, halfwidth
const unsigned char iHAS_VERTICAL       = 0x80;   //  also: square, extensions, ctraptype

//  placement specific flags
//  NOTE: the placement record uses a different bit assignment than the
//  geometry records!
const unsigned char iP_HAS_CELL         = 0x80;
const unsigned char iP_CELL_BY_REF      = 0x40;
const unsigned char iP_HAS_X            = 0x20;
const unsigned char iP_HAS_Y            = 0x10;
const unsigned char iP_HAS_REPETITION   = 0x08;
const unsigned char iP_HAS_MAGNIFICATION= 0x04;   //  record 18 only
const unsigned char iP_HAS_ANGLE        = 0x02;   //  record 18 only
const unsigned char iP_MIRROR           = 0x01;

//  text specific flags
const unsigned char iT_HAS_STRING       = 0x40;
const unsigned char iT_STRING_BY_REF    = 0x20;

// ------------------------------------------------------------------
//  Real number types (the first field of a "real" is the type)
// ------------------------------------------------------------------

const unsigned char rZERO               = 0;   //  unsigned integer value (non-negative)
const unsigned char rZERO_NEG           = 1;   //  unsigned integer value (negated)
const unsigned char rRECIPROCAL         = 2;   //  1/n
const unsigned char rRECIPROCAL_NEG     = 3;   //  -1/n
const unsigned char rRATIO              = 4;   //  n/m
const unsigned char rRATIO_NEG          = 5;   //  -n/m
const unsigned char rFLOAT              = 6;   //  4 byte IEEE real
const unsigned char rDOUBLE             = 7;   //  8 byte IEEE real

// ------------------------------------------------------------------
//  Point list types
// ------------------------------------------------------------------

const unsigned char plMANHATTAN_H       = 0;
const unsigned char plMANHATTAN_V       = 1;
const unsigned char plDOUBLE_DELTA      = 2;
const unsigned char plTRIPLE_DELTA      = 3;
const unsigned char plGENERIC           = 4;
const unsigned char plGENERIC_ACCUMULATE= 5;

// ------------------------------------------------------------------
//  Repetition types
// ------------------------------------------------------------------

const unsigned char rpREUSE             = 0;
const unsigned char rpGRID_2D           = 1;
const unsigned char rpGRID_X            = 2;
const unsigned char rpGRID_Y            = 3;
const unsigned char rpIRREGULAR_X       = 4;
const unsigned char rpIRREGULAR_X_GRID  = 5;
const unsigned char rpIRREGULAR_Y       = 6;
const unsigned char rpIRREGULAR_Y_GRID  = 7;
const unsigned char rpGENERIC_2D        = 8;
const unsigned char rpGENERIC_1D        = 9;
const unsigned char rpGENERIC_ITERATE   = 10;
const unsigned char rpGENERIC_ITER_GRID = 11;

}

#endif
