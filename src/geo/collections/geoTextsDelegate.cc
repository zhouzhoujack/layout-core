#include "geoTextsDelegate.h"

namespace geo
{

// -------------------------------------------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//  TextsDelegate implementation

TextsDelegate::TextsDelegate ()
{
  m_report_progress = false;
}

TextsDelegate::TextsDelegate (const TextsDelegate &other)
  : geo::ShapeCollectionDelegateBase ()
{
  operator= (other);
}

TextsDelegate &
TextsDelegate::operator= (const TextsDelegate &other)
{
  if (this != &other) {
    m_report_progress = other.m_report_progress;
  }
  return *this;
}

TextsDelegate::~TextsDelegate ()
{
  //  .. nothing yet ..
}

void TextsDelegate::enable_progress (const std::string &progress_desc)
{
  m_report_progress = true;
  m_progress_desc = progress_desc;
}

void TextsDelegate::disable_progress ()
{
  m_report_progress = false;
}

}

