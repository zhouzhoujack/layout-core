#ifndef HDR_geoHersheyFont
#define HDR_geoHersheyFont

namespace geo {

enum Font { NoFont = -1, DefaultFont = 0, GothicFont, SansSerifFont, StickFont, TimesItalicFont, TimesThinFont, TimesFont, NFonts };
enum HAlign { NoHAlign = -1, HAlignCenter = 1, HAlignLeft = 0, HAlignRight = 2 };
enum VAlign { NoVAlign = -1, VAlignCenter = 1, VAlignTop = 0, VAlignBottom = 2 };

}

#endif

