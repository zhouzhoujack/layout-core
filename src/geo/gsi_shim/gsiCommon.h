#pragma once

// ---------------------------------------------------------------------------
// Minimal stand-in for KLayout's "gsi" (generic scripting interface).
//
// The standalone layout-io library does not export objects to a scripting
// language, so the geometry classes only need an empty base class to keep
// their definitions unchanged. The real gsi layer depends on Qt and is
// deliberately not part of layout-io.
// ---------------------------------------------------------------------------

#ifndef GSI_PUBLIC
#  define GSI_PUBLIC
#endif
