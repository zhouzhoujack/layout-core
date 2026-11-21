#include "geoEdgePairsDelegate.h"

namespace geo
{

// -------------------------------------------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//  EdgePairsDelegate implementation

EdgePairsDelegate::EdgePairsDelegate ()
{
  m_report_progress = false;
}

EdgePairsDelegate::EdgePairsDelegate (const EdgePairsDelegate &other)
  : ShapeCollectionDelegateBase ()
{
  operator= (other);
}

EdgePairsDelegate &
EdgePairsDelegate::operator= (const EdgePairsDelegate &other)
{
  if (this != &other) {
    m_report_progress = other.m_report_progress;
  }
  return *this;
}

EdgePairsDelegate::~EdgePairsDelegate ()
{
  //  .. nothing yet ..
}

void EdgePairsDelegate::enable_progress (const std::string &progress_desc)
{
  m_report_progress = true;
  m_progress_desc = progress_desc;
}

void EdgePairsDelegate::disable_progress ()
{
  m_report_progress = false;
}

}

