#include "utilsEvents.h"
#include "utilsLog.h"
#include "utilsCommon.h"

namespace geo
{
// -----------------------------------------------------------------------------
//  Events implementation


void handle_event_exception (geo::Exception &ex)
{
  //  Exceptions have unpredictable side effects for events so we just log
  //  them here.
  geo::warn << ex.msg ();
}

void handle_event_exception (std::exception &ex)
{
  //  Exceptions have unpredictable side effects for events so we just log
  //  them here.
  geo::warn << ex.what ();
}

}
