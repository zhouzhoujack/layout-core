#include "geoRegionDelegate.h"

namespace geo
{

// -------------------------------------------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//  RegionDelegate implementation

RegionDelegate::RegionDelegate ()
{
  m_base_verbosity = 30;
  m_report_progress = false;
  m_merged_semantics = true;
  m_strict_handling = false;
  m_merge_min_coherence = false;
}

RegionDelegate::RegionDelegate (const RegionDelegate &other)
  : ShapeCollectionDelegateBase ()
{
  operator= (other);
}

RegionDelegate &
RegionDelegate::operator= (const RegionDelegate &other)
{
  if (this != &other) {
    m_base_verbosity = other.m_base_verbosity;
    m_report_progress = other.m_report_progress;
    m_merged_semantics = other.m_merged_semantics;
    m_strict_handling = other.m_strict_handling;
    m_merge_min_coherence = other.m_merge_min_coherence;
  }
  return *this;
}

RegionDelegate::~RegionDelegate ()
{
  //  .. nothing yet ..
}

void RegionDelegate::enable_progress (const std::string &progress_desc)
{
  m_report_progress = true;
  m_progress_desc = progress_desc;
}

void RegionDelegate::disable_progress ()
{
  m_report_progress = false;
}

void RegionDelegate::set_base_verbosity (int vb)
{
  m_base_verbosity = vb;
}

void RegionDelegate::set_min_coherence (bool f)
{
  if (f != m_merge_min_coherence) {
    m_merge_min_coherence = f;
    min_coherence_changed ();
  }
}

void RegionDelegate::set_merged_semantics (bool f)
{
  if (f != m_merged_semantics) {
    m_merged_semantics = f;
    merged_semantics_changed ();
  }
}

void RegionDelegate::set_strict_handling (bool f)
{
  m_strict_handling = f;
}

}

