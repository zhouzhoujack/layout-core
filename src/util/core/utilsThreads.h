#ifndef HDR_utilsThreads
#define HDR_utilsThreads

#include "utilsCommon.h"

#include <limits>

#include <QMutex>
#include <QWaitCondition>
#include <QThread>
#include <QThreadStorage>

namespace geo
{

/**
 *  @brief A mutex implementation
 *  This class acts as an abstraction. Qt-based and other implementations are
 *  available.
 */

class TL_PUBLIC Mutex
  : public QMutex
{
public:
  Mutex () : QMutex () { }
};

/**
 *  @brief A wait condition implementation
 *  This class acts as an abstraction. Qt-based and other implementations are
 *  available.
 */

class TL_PUBLIC WaitCondition
  : public QWaitCondition
{
public:
  WaitCondition () : QWaitCondition () { }

  bool wait (Mutex *mutex, unsigned long time = std::numeric_limits<unsigned long>::max ()) { 
    return QWaitCondition::wait (mutex, time);
   }
};

/**
 *  @brief A RAII-based Mutex locker
 */
class TL_PUBLIC MutexLocker
{
public:
  MutexLocker (Mutex *mutex)
    : mp_mutex (mutex)
  {
    mp_mutex->lock ();
  }

  ~MutexLocker ()
  {
    mp_mutex->unlock ();
  }

private:
  Mutex *mp_mutex;
};


class Thread
  : public QThread
{
public:
  Thread ()
    : QThread ()
  { }
};


/**
 *  @brief A thread local storage implementation
 *  This class acts as an abstraction. Qt-based and other implementations are
 *  available.
 */

template <class T>
class ThreadStorage
  : public QThreadStorage<T>
{
public:
  ThreadStorage () : QThreadStorage<T> () {
    
   }
};

}

#endif
