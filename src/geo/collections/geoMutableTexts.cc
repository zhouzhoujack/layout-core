#include "geoMutableTexts.h"

namespace geo
{

// -------------------------------------------------------------------------------------------------------------
//  MutableTexts implementation

MutableTexts::MutableTexts ()
  : AsIfFlatTexts ()
{
  //  .. nothing yet ..
}

MutableTexts::MutableTexts (const MutableTexts &other)
  : AsIfFlatTexts (other)
{
  //  .. nothing yet ..
}

MutableTexts::~MutableTexts ()
{
  //  .. nothing yet ..
}

void
MutableTexts::insert (const geo::Shape &shape)
{
  if (shape.is_text ()) {
    geo::Text text;
    shape.text (text);
    insert (text);
  }
}

}

