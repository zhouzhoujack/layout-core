#include "utilsDeferredExecutionQt.h"
#include "utilsException.h"
#include "utilsLog.h"

#include <QCoreApplication>

namespace geo
{

// -----------------------------------------------------------------------------
//  DeferredExecutionQt implementation

DeferredMethodSchedulerQt::DeferredMethodSchedulerQt ()
  : QObject (qApp), DeferredMethodScheduler ()
{
  m_event_type = QEvent::registerEventType ();

  connect (&m_timer, SIGNAL (timeout ()), this, SLOT (timer ()));

  m_timer.setInterval (0); // immediately
  m_timer.setSingleShot (true); //  just once

  //  set up a fallback timer that cleans up pending execute jobs if something goes wrong
  connect (&m_fallback_timer, SIGNAL (timeout ()), this, SLOT (timer ()));
  m_fallback_timer.setInterval (500);
  m_fallback_timer.setSingleShot (false);
}

DeferredMethodSchedulerQt::~DeferredMethodSchedulerQt ()
{
  //  .. nothing yet ..
}

void 
DeferredMethodSchedulerQt::queue_event ()
{
  qApp->postEvent (this, new QEvent (QEvent::Type (m_event_type)));
}

bool
DeferredMethodSchedulerQt::event (QEvent *event)
{
  if (event->type () == m_event_type) {
    timer ();
    return true;
  } else {
    return QObject::event (event);
  }
}

void 
DeferredMethodSchedulerQt::timer ()
{
  if (is_disabled ()) {
    //  start again if disabled
    m_timer.start ();
  } else {
    try {
      do_execute ();
    } catch (geo::Exception &ex) {
      geo::error << geo::to_string (QObject::tr ("Exception caught: ")) << ex.msg ();
    } catch (std::exception &ex) {
      geo::error << geo::to_string (QObject::tr ("Exception caught: ")) << ex.what ();
    } catch (...) {
      geo::error << geo::to_string (QObject::tr ("Unspecific exception caught"));
    }
  }
}

}
