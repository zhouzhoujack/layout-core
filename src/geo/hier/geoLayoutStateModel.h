#ifndef HDR_geoLayoutStateModel
#define HDR_geoLayoutStateModel

#include "geoCommon.h"

#include "utilsEvents.h"

namespace geo 
{

/**
 *  @brief The layout state model
 *
 *  The layout state model is supposed to track the state of a layout object.
 *  The layout object to be tracked should be derived from geo::LayoutStateModel
 *  and implement the do_update method. 
 *  The state model can track the state of any layout (or similar) object in two
 *  ways: once, if the bounding boxes become invalid and in another way, if the
 *  cell tree becomes invalid. These events are issued by containers used within
 *  the layout object (i.e. geo::Shapes) and "collected" in the layout state model.
 *  Calling "update" will reset this state and call "do_update" to bring the 
 *  layout object into a consistent state.
 *  In addition, observers using the geo::Observer interface can attach to this
 *  state model to track if the layout changes it's state.
 */

class DB_PUBLIC LayoutStateModel
{
public:
  /**
   *  @brief Constructor
   *  If the "busy" flag is set, "bboxes_changed" and "hier_changed"
   *  events will be emitted on every change, not just once after an update.
   */
  LayoutStateModel (bool busy = false);

  /**
   *  @brief Copy constructor
   *
   *  This constructor does not copy the attachments to the geo::Observed members!
   */
  LayoutStateModel (const LayoutStateModel &d);

  /**
   *  @brief Destructor
   */
  virtual ~LayoutStateModel ();

  /**
   *  @brief Assignment constructor
   *
   *  This operator does not copy the attachments to the geo::Observed members!
   */
  LayoutStateModel &operator= (const LayoutStateModel &d);

  /**
   *  @brief Invalidate the hierarchy information
   * 
   *  This method is supposed to be called if something on the
   *  hierarchy has been changed - i.e. cells have been inserted
   *  or cell instances have been inserted.
   */
  void invalidate_hier ()
  {
    if (! m_hier_dirty || m_busy) {
      do_invalidate_hier ();  //  must be called before the hierarchy is invalidated (stopping of redraw thread requires this)
      m_hier_dirty = true;
    }
  }

  /**
   *  @brief Invalidate the bounding boxes
   *
   *  This method is supposed to be called by shape containers for example if 
   *  some event has occurred that changed the bounding boxes.
   *
   *  If the index is std::numeric_limits<unsigned int>::max, this method
   *  applies to all layers.
   */
  void invalidate_bboxes (unsigned int index);

  /**
   *  @brief Signal that the database unit has changed
   */
  void dbu_changed ()
  {
    dbu_changed_event ();
  }

  /**
   *  @brief This method resets the layout's state back to valid hierarchy and bounding boxes
   *
   *  This method will call do_update if necessary and reset the invalid flags. 
   */
  void update ();

  /**
   *  @brief The "dirty hierarchy" attribute
   *
   *  This attribute is true, if the hierarchy has changed since the last "update" call
   */
  bool hier_dirty () const
  {
    return m_hier_dirty;
  }

  /**
   *  @brief The "dirty bounding box" attribute
   *
   *  This attribute is true, if the bounding boxes have changed since the last "update" call
   */
  bool bboxes_dirty () const;

  /**
   *  @brief Sets or resets busy mode
   *
   *  See the constructor for details about busy mode.
   */
  void set_busy (bool b)
  {
    m_busy = b;
  }

  /**
   *  @brief Gets a flag indicating busy mode
   */
  bool busy () const
  {
    return m_busy;
  }

protected:
  friend class PropertiesRepository;

  /**
   *  @brief Reimplement this method to update anything related to the hierarchy or bounding boxes.
   */
  virtual void do_update () { }

  /**
   *  @brief Issue a "prop id's changed event"
   */
  void prop_ids_changed ()
  {
    prop_ids_changed_event ();
  }

  /**
   *  @brief Issue a "prop id's changed event"
   */
  void cell_name_changed ()
  {
    cell_name_changed_event ();
  }

  /**
   *  @brief Issue a "layer properties changed event"
   */
  void layer_properties_changed ()
  {
    layer_properties_changed_event ();
  }

public:
  geo::Event hier_changed_event;
  geo::event<unsigned int> bboxes_changed_event;
  geo::Event bboxes_changed_any_event;
  geo::Event dbu_changed_event;
  geo::Event cell_name_changed_event;
  geo::Event prop_ids_changed_event;
  geo::Event layer_properties_changed_event;

private:
  bool m_hier_dirty;
  std::vector<bool> m_bboxes_dirty;
  bool m_all_bboxes_dirty;
  bool m_busy;

  void do_invalidate_hier ();
  void do_invalidate_bboxes (unsigned int index);
};

}

#endif

