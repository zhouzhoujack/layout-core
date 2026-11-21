#include "geoEdgesDelegate.h"

namespace geo
{

// -------------------------------------------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//  EdgesDelegate implementation

EdgesDelegate::EdgesDelegate ()
{
  m_base_verbosity = 30;
  m_report_progress = false;
  m_merged_semantics = true;
  m_strict_handling = false;
}

EdgesDelegate::EdgesDelegate (const EdgesDelegate &other)
  : ShapeCollectionDelegateBase ()
{
  operator= (other);
}

EdgesDelegate &
EdgesDelegate::operator= (const EdgesDelegate &other)
{
  if (this != &other) {
    m_base_verbosity = other.m_base_verbosity;
    m_report_progress = other.m_report_progress;
    m_merged_semantics = other.m_merged_semantics;
    m_strict_handling = other.m_strict_handling;
  }
  return *this;
}

EdgesDelegate::~EdgesDelegate ()
{
  //  .. nothing yet ..
}

void EdgesDelegate::set_base_verbosity (int vb)
{
  m_base_verbosity = vb;
}

void EdgesDelegate::enable_progress (const std::string &progress_desc)
{
  m_report_progress = true;
  m_progress_desc = progress_desc;
}

void EdgesDelegate::disable_progress ()
{
  m_report_progress = false;
}

void EdgesDelegate::set_merged_semantics (bool f)
{
  if (f != m_merged_semantics) {
    m_merged_semantics = f;
    merged_semantics_changed ();
  }
}

void EdgesDelegate::set_strict_handling (bool f)
{
  m_strict_handling = f;
}

}

