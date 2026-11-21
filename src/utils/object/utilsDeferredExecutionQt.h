#ifndef HDR_utilsDeferredExecutionQt
#define HDR_utilsDeferredExecutionQt

#include "utilsCommon.h"
#include "utilsDeferredExecution.h"

#include <QObject>
#include <QTimer>
#include <QMutex>

#include <list>

namespace geo
{

/**
 *  @brief The deferred method scheduler
 */
class TL_PUBLIC DeferredMethodSchedulerQt
  : public QObject, public DeferredMethodScheduler
{
Q_OBJECT
public:
  /**
   *  @brief Constructor
   */
  DeferredMethodSchedulerQt ();

  /**
   *  @brief Destructor
   */
  ~DeferredMethodSchedulerQt ();

protected:
  /**
   *  @brief Reimplementation of the interface: queue an event
   *  In effect, the event should later trigger a call to do_execute ().
   */
  void queue_event ();

private slots:
  void timer ();

private:
  QTimer m_timer, m_fallback_timer;
  int m_event_type;

  virtual bool event (QEvent *event);
};

}

#endif

