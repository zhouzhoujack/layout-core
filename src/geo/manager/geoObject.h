#ifndef HDR_geoObject
#define HDR_geoObject


#include "geoManager.h"

namespace geo
{

/**
 *  @brief The base class of a database object
 *
 *  The main purpose of the geo::Object class is to provide
 *  transaction and recall functionality. Basically this is 
 *  an implementation of the strategy pattern that is used
 *  to implement the recall functionality.
 */

class Object
{
public:
  /**
   *  @brief Default ctor
   *
   *  Attach the object to a manager if required
   */
  Object (geo::Manager *manager = 0);

  /**
   *  @brief Destructor
   *
   *  Detaches the object from the manager
   */
  virtual ~Object ();

  /** 
   *  @brief Copy constructor
   *
   *  The copy constructor copies the attachment to
   *  a manager object. 
   */
  Object (const Object &);

  /**
   *  @brief Manager object retrieval
   *
   *  Obtain the pointer to the management object this
   *  object is attached to. Returns 0 if not attached to a manager.
   */
  geo::Manager *manager () const
  {
    return mp_manager;
  }

  /**
   *  @brief Attach to a different manager or detach
   *
   *  Changes the attachment of the object to any
   *  manager. Pass 0 to detach it from any manager.
   */
  void manager (geo::Manager *p_manager);

  /** 
   *  @brief The undo 'strategy'
   *
   *  For a detailed description of this method see geo::Manager::queue.
   *  This method has been declared to be no-throwing since it is
   *  assumed that once the operation is successfully queued it can be undone
   *  in every case.
   */
  virtual void undo (geo::Op * /*op*/) 
  { }

  /** 
   *  @brief The redo 'strategy'
   *
   *  For a detailed description of this method see geo::Manager::queue.
   *  This method has been declared to be no-throwing since it is
   *  assumed that once the operation is successfully queued it can be redone
   *  in every case.
   */
  virtual void redo (geo::Op * /*op*/)
  { }

  /**
   *  @brief The id getter method
   */
  geo::Manager::ident_t id () const 
  {
    return m_id;
  }

  /**
   *  @brief A convenience function to determine if we are transacting
   */
  bool transacting () const
  {
    return manager () && manager ()->transacting ();
  }

  /**
   *  @brief A convenience function to determine if we are in an undo or redo replay operation
   */
  bool replaying () const
  {
    return manager () && manager ()->replaying ();
  }

  /**
   *  @brief Begins a transaction (same as calling manager ()->transaction (), but safe against null manager ())
   */
  void transaction (const std::string &description, geo::Manager::transaction_id_t join_with = 0)
  {
    if (manager ()) {
      manager ()->transaction (description, join_with);
    }
  }

  /**
   *  @brief Ends a transaction (same as calling manager ()->commit (), but safe against null manager ())
   */
  void commit ()
  {
    if (manager ()) {
      manager ()->commit ();
    }
  }

private:
  geo::Manager::ident_t m_id;
  geo::Manager *mp_manager;

  //  The assignment operator is private in order to
  //  force a specific treatment of the manager object attachment
  Object &operator= (const Object &);
};

} // namespace geo

#endif

