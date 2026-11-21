#ifndef HDR_utilsThreads
#define HDR_utilsThreads

#include "utilsCommon.h"

#include <limits>

#if defined(HAVE_QT) && !defined(HAVE_PTHREADS)
#  include <QMutex>
#  include <QWaitCondition>
#  include <QThread>
#  include <QThreadStorage>
#else
//  atomics taken from https://github.com/mbitsnbites/atomic
#  include "atomic/spinlock.h"
#endif

namespace geo
{

/**
 *  @brief A mutex implementation
 *  This class acts as an abstraction. Qt-based and other implementations are
 *  available.
 */

#if defined(HAVE_QT) && !defined(HAVE_PTHREADS)

class TL_PUBLIC Mutex
  : public QMutex
{
public:
  Mutex () : QMutex () { }
};

#else

//  The non-Qt version is a dummy implementation as threading is not supported (yet)
class TL_PUBLIC Mutex
{
public:
  Mutex () : m_spinlock () { }
  void lock() { m_spinlock.lock(); }
  void unlock() { m_spinlock.unlock(); }
private:
  atomic::spinlock m_spinlock;
};

#endif

/**
 *  @brief A wait condition implementation
 *  This class acts as an abstraction. Qt-based and other implementations are
 *  available.
 */

#if defined(HAVE_QT) && !defined(HAVE_PTHREADS)

class TL_PUBLIC WaitCondition
  : public QWaitCondition
{
public:
  WaitCondition () : QWaitCondition () { }

  bool wait (Mutex *mutex, unsigned long time = std::numeric_limits<unsigned long>::max ()) { return QWaitCondition::wait (mutex, time); }
};

#else

class WaitConditionPrivate;

//  The non-Qt version is a dummy implementation as threading is not supported (yet)
class TL_PUBLIC WaitCondition
{
public:
  WaitCondition ();
  ~WaitCondition ();
  bool wait (Mutex * /*mutex*/, unsigned long /*time*/ = std::numeric_limits<unsigned long>::max ());
  void wakeAll ();
  void wakeOne ();

private:
  WaitConditionPrivate *mp_data;
};

#endif

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


#if defined(HAVE_QT) && !defined(HAVE_PTHREADS)

class Thread
  : public QThread
{
public:
  Thread ()
    : QThread ()
  { }
};

#else

//  Non-Qt thread implementation (pthread based, see tlThreads.cc)
class ThreadPrivateData;

class TL_PUBLIC Thread
{
public:
  Thread ();
  virtual ~Thread ();

  void start ();
  void terminate ();
  bool wait (unsigned long time = std::numeric_limits<unsigned long>::max ());
  void quit ();
  void exit (int return_code = 0);
  bool isFinished () const;
  bool isRunning () const;

protected:
  //  Body of the thread - reimplement this in derived classes
  virtual void run () { }

  //  Entry point called by the pthread trampoline
  virtual void do_run ();
  friend void *start_thread (void *);

private:
  ThreadPrivateData *mp_data;
};

#endif


/**
 *  @brief A thread local storage implementation
 *  This class acts as an abstraction. Qt-based and other implementations are
 *  available.
 */

#if defined(HAVE_QT) && !defined(HAVE_PTHREADS)

template <class T>
class ThreadStorage
  : public QThreadStorage<T>
{
public:
  ThreadStorage () : QThreadStorage<T> () { }
};

#else

class TL_PUBLIC ThreadStorageHolderBase
{
public:
  ThreadStorageHolderBase (void *obj) : mp_obj (obj) { }
  virtual ~ThreadStorageHolderBase () { }

protected:
  void *obj () { return mp_obj; }
  void *mp_obj;
};

template <class T>
class ThreadStorageHolder
  : public ThreadStorageHolderBase
{
public:
  ThreadStorageHolder (T *t) : ThreadStorageHolderBase ((void *) t) { }
  ~ThreadStorageHolder () { delete data (); }
  T *data () { return (T *) obj (); }
};

class TL_PUBLIC ThreadStorageBase
{
public:
  ThreadStorageBase ();

protected:
  void add (ThreadStorageHolderBase *holder);
  ThreadStorageHolderBase *holder ();

  const ThreadStorageHolderBase *holder () const
  {
    return (const_cast<ThreadStorageBase *> (this)->holder ());
  }
};

//  TODO: this is the non-threaded dummy implementation
template <class T>
class ThreadStorage
  : public ThreadStorageBase
{
public:
  ThreadStorage () : ThreadStorageBase () { }

  bool hasLocalData () const
  {
    return dynamic_cast<const ThreadStorageHolder<T> *> (holder ()) != 0;
  }

  T &localData ()
  {
    return *(dynamic_cast<ThreadStorageHolder<T> *> (holder ())->data ());
  }

  T localData () const
  {
    return *(dynamic_cast<const ThreadStorageHolder<T> *> (holder ())->data ());
  }

  void setLocalData (const T &data)
  {
    add (new ThreadStorageHolder<T> (new T (data)));
  }
};

#endif

}

#endif
