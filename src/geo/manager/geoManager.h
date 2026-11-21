#ifndef HDR_geoManager
#define HDR_geoManager

#include "geoCommon.h"

#include "utilsTypeTraits.h"

#include <vector>
#include <list>
#include <string>

namespace geo
{

class Object;


/**
 *  @brief An atomic operation.
 * 
 *  See Manager::queue for a detailed description.
 */

class DB_PUBLIC Op
{
private:
  friend class Manager;

  bool m_done;

  void set_done (bool d)
  {
    m_done = d;
  }

public:
  Op (bool done = true) : m_done (done)
  { }

  virtual ~Op () 
  { }

  bool is_done () const
  {
    return m_done;
  }
};

/**
 *  @brief The database object manager
 *
 *  The functionality of the database object manager is to
 *  manage absolute object references through object id's
 *  and to provide transaction management.
 */

class DB_PUBLIC Manager
{
public:
  typedef size_t ident_t;
  typedef size_t transaction_id_t;

  /**
   *  @brief Default constructor
   */
  Manager (bool enabled = true);
  
  /**
   *  @brief Destructor
   */
  ~Manager ();

  /**
   *  @brief Gets a value indicating whether the manager is enabled
   */
  bool is_enabled () const
  {
    return m_enabled;
  }

  /**
   *  @brief Release an object with the given id.
   *
   *  This will free this id and recycle it upon the
   *  next request. After this call, the id is no longer valid.
   */
  void release_object (ident_t id);

  /**
   *  @brief Request the next available id
   *
   *  Request the next id available and associate with an object
   */
  ident_t next_id (geo::Object *obj);

  /**
   *  @brief Retrieve the object pointer for a given id
   *
   *  For the given id, retrieve the object pointer.
   *  Returns 0 if the id is not a valid one.
   */  
  geo::Object *object_by_id (ident_t id);

  /** 
   *  @brief Begin a transaction
   *
   *  This call will open a new transaction. A transaction consists
   *  of a set of operations issued with the 'queue' method.
   *  A transaction is closed with the 'commit' method.
   *
   *  The transaction can be joined with a previous transaction. To do so, pass the
   *  previous transaction id to the "join_with" parameter. If the transaction specified
   *  with "join_with" is not the previous transaction, it is not joined.
   */
  transaction_id_t transaction (const std::string &description, transaction_id_t join_with = 0);

  /**
   *  @brief Returns the last transaction id
   *
   *  This method can be used to identify the current transaction by id.
   */
  transaction_id_t last_transaction_id () const;

  /**
   *  @brief Close a transaction successfully.
   */
  void commit ();

  /**
   *  @brief Cancels a transaction
   *
   *  If called instead of commit, this method will undo all operations of the pending
   *  transaction.
   */
  void cancel ();

  /**
   *  @brief Undo the current transaction
   *
   *  The current transaction is undone with this method.
   *  The 'available_undo' method can be used to determine whether
   *  there are transactions to undo.
   */
  void undo ();

  /**
   *  @brief Redo the next available transaction
   *
   *  The next transaction is redone with this method.
   *  The 'available_redo' method can be used to determine whether
   *  there are transactions to undo.
   */
  void redo ();

  /**
   *  @brief Determine available 'undo' transactions
   *
   *  @return The first parameter is true, if there is a transaction
   *          to undo. The second is the description of the transaction
   *          if there is one available.
   */
  std::pair<bool, std::string> available_undo () const;

  /**
   *  @brief Determine available 'redo' transactions
   *
   *  @return The first parameter is true, if there is a transaction
   *          to redo. The second is the description of the transaction
   *          if there is one available.
   */
  std::pair<bool, std::string> available_redo () const;

  /**
   *  @brief Queue a operation for undo
   *
   *  With this method a atomic undoable operation can be registered.
   *  The operation is an object derived from geo::Op. 
   *  This object's pointer is passed to the 'undo' method of the
   *  object in charge once a undo operation is requested.
   *  The same object is passed also to the 'redo' method to redo
   *  to operation. 
   *  The operation also holds the state: initially the operation
   *  signals "done" which means that the operation defined by the
   *  geo::Op object was performed. Upon 'undo' the state changes to
   *  'undone' which signals that the operation was undone. Upon 'redo'
   *  then the state again changes to 'done'.
   *  If the 'op' object is passed in 'undone' state to the queue method,
   *  it will be brought into done state by issueing a 'redo'. This 
   *  way the operation can be implemented fully implicitly through
   *  geo::Object's undo and redo methods.
   *  The geo::Op object will be owned by the manager and there will
   *  be no copying for the object. Therefore the geo::Op object is
   *  a good place for storing pointers to objects created in the
   *  process of the operation for example.
   */ 
  void queue (geo::Object *object, geo::Op *op);

  /**
   *  @brief Get the last queued geo::Op object
   *
   *  This method allows one to fetch and modify the last queued operation for the given object in order
   *  to allow some optimisation, i.e. joining two ops. It can be modified but must not
   *  be deleted. The returned object is guaranteed to be inside same transaction.
   *
   *  @param object The object for which to look for a queued operation.
   *  @return See above. 0 if no operation is queued for this transaction.
   */
  geo::Op *last_queued (geo::Object *object);

  /** 
   *  @brief Clear all transactions 
   */
  void clear ();

  /**
   *  @brief Query if we are within a transaction
   */
  bool transacting () const
  {
    return m_opened;
  }

  /**
   *  @brief Query if we are within a undo/redo operation
   */
  bool replaying () const
  {
    return m_replay;
  }

private:
  std::vector<geo::Object *> m_id_table;
  std::vector<ident_t> m_unused_ids;

  typedef std::pair<geo::Manager::ident_t, geo::Op *> operation_t;
  typedef std::list<operation_t> operations_t;
  typedef std::pair<operations_t, std::string> transaction_t;
  typedef std::list<transaction_t> transactions_t;

  transactions_t m_transactions;
  transactions_t::iterator m_current;
  bool m_opened;
  bool m_replay;
  bool m_enabled;

  void erase_transactions (transactions_t::iterator from, transactions_t::iterator to);
};

/**
 *  @brief A transaction controller utility class
 *
 *  This object controls a transaction through it's lifetime. On construction, the 
 *  transaction is started, on destruction, the transaction is committed.
 *
 *  "cancel" can be used to cancel the operation. This will undo all operations collected
 *  so far and delete the transaction.
 *
 *  "close" temporarily disable the collection of operations.
 *  "open" will enable operation collection again and continue
 *  collection at the point when it was stopped with "close".
 */

class DB_PUBLIC Transaction
{
public:
  Transaction (geo::Manager *manager, const std::string &desc)
    : mp_manager (manager), m_transaction_id (0), m_description (desc)
  {
    if (mp_manager) {
      m_transaction_id = mp_manager->transaction (desc);
    }
  }

  Transaction (geo::Manager *manager, const std::string &desc, geo::Manager::transaction_id_t join_with)
    : mp_manager (manager), m_transaction_id (0), m_description (desc)
  {
    if (mp_manager) {
      m_transaction_id = mp_manager->transaction (desc, join_with);
    }
  }

  ~Transaction ()
  {
    if (mp_manager) {
      if (mp_manager->transacting ()) {
        mp_manager->commit ();
      }
      mp_manager = 0;
    }
  }

  void cancel ()
  {
    if (mp_manager) {
      open ();
      mp_manager->cancel ();
      mp_manager = 0;
    }
  }

  void close ()
  {
    if (mp_manager->transacting ()) {
      mp_manager->commit ();
    }
  }

  void open ()
  {
    if (! mp_manager->transacting ()) {
      mp_manager->transaction (m_description, m_transaction_id);
    }
  }

  geo::Manager::transaction_id_t id () const
  {
    return m_transaction_id;
  }

private:
  geo::Manager *mp_manager;
  geo::Manager::transaction_id_t m_transaction_id;
  std::string m_description;

  //  no copying.
  Transaction (const Transaction &);
  Transaction &operator= (const Transaction &);
};

} // namespace geo

namespace geo
{
  template <>
  struct type_traits <geo::Manager> : public type_traits<void> 
  {
    typedef geo::false_tag has_copy_constructor;
    typedef geo::true_tag has_default_constructor;
  };
}

#endif

