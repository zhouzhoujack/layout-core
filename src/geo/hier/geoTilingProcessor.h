#ifndef HDR_geoTilingProcessor
#define HDR_geoTilingProcessor

#include "geoLayout.h"
#include "geoEdgePairs.h"
#include "geoRegion.h"
#include "geoEdges.h"
#include "geoLayoutUtils.h"
#include "geoPolygonTools.h"
#include "geoClip.h"

#include "gsiObject.h"

#include "utilsExpression.h"
#include "utilsTypeTraits.h"
#include "utilsThreads.h"

namespace geo
{

class TilingProcessor;

/**
 *  @brief A receiver for the output data 
 */
class TileOutputReceiver
  : public gsi::ObjectBase, public geo::RefCounted
{
public:
  /**
   *  @brief Constructor
   */
  TileOutputReceiver ()
    : mp_proc (0)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Destructor
   */
  virtual ~TileOutputReceiver ()
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Initiate the deliver 
   *
   *  This method is called before the first tile delivers it's data.
   *
   *  @param nx The number of tiles in x direction
   *  @param ny The number of tiles in y direction
   *  @param p0 The initial point
   *  @param dx The tile's x dimension
   *  @param dy The tile's y dimension
   *  @param frame The overall frame covered
   *
   *  The tile's coordinates will be p0+(ix*dx,iy*dy),p0+((ix+1)*dx,(iy+1)*dy) 
   *  where ix=0..nx-1, iy=0..ny-1.
   *
   *  All coordinates are given in micron. If tiles are not used, nx and ny are 0.
   */
  virtual void begin (size_t /*nx*/, size_t /*ny*/, const geo::DPoint &/*p0*/, double /*dx*/, double /*dy*/, const geo::DBox & /*frame*/) { }

  /**
   *  @brief Deliver an object for one tile
   *
   *  Delivery is protected by a mutex - only one thread will access the 
   *  receiver at one time.
   *
   *  The interpretation of the object remains subject to the implementation.
   *
   *  @param ix The x index of the tile
   *  @param iy The y index of the tile
   *  @param tile The tile's box
   *  @param id The output id specified in the "output" method
   *  @param obj The object which is delivered
   *  @param dbu The database unit
   *  @param trans The transformation
   *  @param clip If true, clipping at the tile box is requested
   */
  virtual void put (size_t /*ix*/, size_t /*iy*/, const geo::Box &/*tile*/, size_t /*id*/, const geo::Variant & /*obj*/, double /*dbu*/, const geo::ICplxTrans & /*trans*/, bool /*clip*/) { }

  /**
   *  @brief Indicate the end of the execution
   *  @param success Will be true if all tiles executed successfully
   */
  virtual void finish (bool /*success*/) { }

  /**
   *  @brief Gets the tiling processor the receiver is attached to
   *
   *  The returned value is a null pointer if no processor is attached.
   */
  TilingProcessor *processor () const
  {
    return mp_proc;
  }

private:
  friend class TilingProcessor;

  /**
   *  @brief Sets the tiling processor
   */
  void set_processor (TilingProcessor *proc)
  {
    mp_proc = proc;
  }

  TilingProcessor *mp_proc;
};

/**
 *  @brief Delivery of tiling processor output
 *  This utility is put between the container and the receiver.
 *  The inserter is an object having an operator() that takes the object.
 *  This function is responsible for preparing (i.e. clipping) and delivering the output.
 */
template <class X>
void insert (X &inserter, const geo::Polygon &o, const geo::Box &tile, bool clip)
{
  if (clip && ! o.box ().inside (tile)) {
    //  apply clipping
    if (o.box ().touches (tile)) {
      std::vector <geo::Polygon> clipped_poly;
      geo::clip_poly (o, tile, clipped_poly);
      for (std::vector <geo::Polygon>::const_iterator cp = clipped_poly.begin (); cp != clipped_poly.end (); ++cp) {
        inserter (*cp);
      }
    }
  } else {
    //  no clipping
    inserter (o);
  }
}

/**
 *  @brief Delivery of tiling processor output
 *  This utility is put between the container and the receiver.
 *  The inserter is an object having an operator() that takes the object.
 *  This function is responsible for preparing (i.e. clipping) and delivering the output.
 */
template <class X>
void insert (X &inserter, const geo::SimplePolygon &o, const geo::Box &tile, bool clip)
{
  if (clip && ! o.box ().inside (tile)) {
    //  apply clipping
    if (o.box ().touches (tile)) {
      std::vector <geo::SimplePolygon> clipped_poly;
      geo::clip_poly (o, tile, clipped_poly);
      for (std::vector <geo::SimplePolygon>::const_iterator cp = clipped_poly.begin (); cp != clipped_poly.end (); ++cp) {
        inserter (*cp);
      }
    }
  } else {
    //  no clipping
    inserter (o);
  }
}

/**
 *  @brief Delivery of tiling processor output
 *  This utility is put between the container and the receiver.
 *  The inserter is an object having an operator() that takes the object.
 *  This function is responsible for preparing (i.e. clipping) and delivering the output.
 */
template <class X>
void insert (X &inserter, const geo::Path &o, const geo::Box &tile, bool clip)
{
  if (clip && ! o.box ().inside (tile)) {
    //  apply clipping
    if (o.box ().touches (tile)) {
      insert (inserter, o.polygon (), tile, clip);
    }
  } else {
    //  no clipping
    inserter (o);
  }
}

/**
 *  @brief Delivery of tiling processor output
 *  This utility is put between the container and the receiver.
 *  The inserter is an object having an operator() that takes the object.
 *  This function is responsible for preparing (i.e. clipping) and delivering the output.
 */
template <class X>
void insert (X &inserter, const geo::EdgePair &o, const geo::Box &tile, bool clip)
{
  if (clip) {
    //  no real clipping for edge pairs - just select the ones that touch the clip rectangle
    if (o.first ().clipped (tile).first || o.second ().clipped (tile).first) {
      inserter (o);
    }
  } else {
    inserter (o);
  }
}

/**
 *  @brief Delivery of tiling processor output
 *  This utility is put between the container and the receiver.
 *  The inserter is an object having an operator() that takes the object.
 *  This function is responsible for preparing (i.e. clipping) and delivering the output.
 */
template <class X>
void insert (X &inserter, const geo::Text &o, const geo::Box &tile, bool clip)
{
  if (clip) {
    //  clipping
    if (o.box ().inside (tile)) {
      inserter (o);
    }
  } else {
    //  no clipping
    inserter (o);
  }
}

/**
 *  @brief Delivery of tiling processor output
 *  This utility is put between the container and the receiver.
 *  The inserter is an object having an operator() that takes the object.
 *  This function is responsible for preparing (i.e. clipping) and delivering the output.
 */
template <class X>
void insert (X &inserter, const geo::Box &o, const geo::Box &tile, bool clip)
{
  if (clip) {
    //  clipping
    geo::Box oc = o & tile;
    if (! oc.empty ()) {
      inserter (oc);
    }
  } else {
    //  no clipping
    inserter (o);
  }
}

/**
 *  @brief Delivery of tiling processor output
 *  This utility is put between the container and the receiver.
 *  The inserter is an object having an operator() that takes the object.
 *  This function is responsible for preparing (i.e. clipping) and delivering the output.
 */
template <class X>
void insert (X &inserter, const geo::Edge &o, const geo::Box &tile, bool clip)
{
  if (clip) {
    //  apply clipping. To ensure an edge is just added once, clipping is applied in a
    //  somewhat special way: edges coincident with the clip box are only taken if their
    //  inside side is towards the clip box (see Region::perimeter implementation).
    if (o.bbox ().touches (tile)) {

      std::pair<bool, geo::Edge> ce = o.clipped (tile);
      if (ce.first) {

        geo::Coord dx = ce.second.dx ();
        geo::Coord dy = ce.second.dy ();
        geo::Coord x = ce.second.p1 ().x ();
        geo::Coord y = ce.second.p1 ().y ();
        if ((dx == 0 && x == tile.left ()   && dy < 0) ||
            (dx == 0 && x == tile.right ()  && dy > 0) ||
            (dy == 0 && y == tile.top ()    && dx < 0) ||
            (dy == 0 && y == tile.bottom () && dx > 0)) {
          //  not counted -> box is at outside side of the edge
        } else {
          inserter (ce.second);
        }

      }

    }

  } else {
    //  no clipping
    inserter (o);
  }
}

/**
 *  @brief Delivery of tiling processor output
 *  This utility is put between the container and the receiver.
 *  The inserter is an object having an operator() that takes the object.
 *  This function is responsible for preparing (i.e. clipping) and delivering the output.
 */
template <class X>
void insert (X &inserter, const geo::Region &data, const geo::Box &tile, bool clip)
{
  for (geo::Region::const_iterator o = data.begin (); ! o.at_end (); ++o) {
    insert (inserter, *o, tile, clip);
  }
}

/**
 *  @brief Delivery of tiling processor output
 *  This utility is put between the container and the receiver.
 *  The inserter is an object having an operator() that takes the object.
 *  This function is responsible for preparing (i.e. clipping) and delivering the output.
 */
template <class X>
void insert (X &inserter, const geo::Edges &data, const geo::Box &tile, bool clip)
{
  for (geo::Edges::const_iterator o = data.begin (); ! o.at_end (); ++o) {
    insert (inserter, *o, tile, clip);
  }
}

/**
 *  @brief Delivery of tiling processor output
 *  This utility is put between the container and the receiver.
 *  The inserter is an object having an operator() that takes the object.
 *  This function is responsible for preparing (i.e. clipping) and delivering the output.
 */
template <class X>
void insert (X &inserter, const geo::EdgePairs &data, const geo::Box &tile, bool clip)
{
  for (geo::EdgePairs::const_iterator o = data.begin (); ! o.at_end (); ++o) {
    insert (inserter, *o, tile, clip);
  }
}

/**
 *  @brief Delivery of tiling processor output
 *  This utility is put between the container and the receiver.
 *  The inserter is an object having an operator() that takes the object.
 *  This function is responsible for preparing (i.e. clipping) and delivering the output.
 */
template <class X>
void insert (X &inserter, const geo::Texts &data, const geo::Box &tile, bool clip)
{
  for (geo::Texts::const_iterator o = data.begin (); ! o.at_end (); ++o) {
    insert (inserter, *o, tile, clip);
  }
}

/**
 *  @brief Delivery of tiling processor output
 *  This utility is put between the container and the receiver.
 *  The inserter is an object having an operator() that takes the object.
 *  This function is responsible for preparing (i.e. clipping) and delivering the output.
 */
template <class X>
bool insert_var (X &inserter, const geo::Variant &obj, const geo::Box &tile, bool clip)
{
  if (obj.is_user<geo::Region> ()) {
    insert (inserter, obj.to_user<geo::Region> (), tile, clip);
    return true;
  } else if (obj.is_user<geo::EdgePairs> ()) {
    insert (inserter, obj.to_user<geo::EdgePairs> (), tile, clip);
    return true;
  } else if (obj.is_user<geo::Edges> ()) {
    insert (inserter, obj.to_user<geo::Edges> (), tile, clip);
    return true;
  } else if (obj.is_user<geo::Texts> ()) {
    insert (inserter, obj.to_user<geo::Texts> (), tile, clip);
    return true;
  } else if (obj.is_user<geo::Box> ()) {
    insert (inserter, obj.to_user<geo::Box> (), tile, clip);
    return true;
  } else if (obj.is_user<geo::Path> ()) {
    insert (inserter, obj.to_user<geo::Path> (), tile, clip);
    return true;
  } else if (obj.is_user<geo::Text> ()) {
    insert (inserter, obj.to_user<geo::Text> (), tile, clip);
    return true;
  } else if (obj.is_user<geo::Polygon> ()) {
    insert (inserter, obj.to_user<geo::Polygon> (), tile, clip);
    return true;
  } else if (obj.is_user<geo::SimplePolygon> ()) {
    insert (inserter, obj.to_user<geo::SimplePolygon> (), tile, clip);
    return true;
  } else if (obj.is_user<geo::EdgePair> ()) {
    insert (inserter, obj.to_user<geo::EdgePair> (), tile, clip);
    return true;
  } else if (obj.is_user<geo::Edge> ()) {
    insert (inserter, obj.to_user<geo::Edge> (), tile, clip);
    return true;
  } else if (obj.is_nil ()) {
    //  .. ignore nil
    return true;
  } else {
    return false;
  }
}

/**
 *  @brief A processor for executing scripts on tiles of a layout
 *
 *  The idea of the tiling processor is to offer a way to execute scripts
 *  (written in geo::Expressions language) on tiles of a layout. Multiple
 *  scripts can be registered per tile.
 *
 *  Multiple inputs can be specified. Custom functions can be registered to 
 *  implement additional functionality.
 *  Multiple outputs can also be specified. Each output channel is assigned a name
 *  which will be available to the script through a variable. To output an object 
 *  use _output(name, object).
 *
 *  Inside the scripts provided, the following variables and functions are
 *  available:
 *   _tile       a function, delivers the region describing the current tile or nil if tiling is not enabled
 *   _dbu        the database unit used for the data
 *   _output     output the data: _output(name, object)
 *   _count      report a count for a specific output for display during execution
 *   <name>      fetch the input <name> as specified in the "input" method
 */
class TilingProcessor
{
public:
  enum Type { TypeRegion, TypeEdges, TypeEdgePairs, TypeTexts };

  /**
   *  @brief Constructor
   */
  TilingProcessor ();

  /**
   *  @brief Destructor
   */ 
  ~TilingProcessor ();

  /**
   *  @brief Specifies a variable
   *
   *  The name is the variable name which will receive the given value in the script.
   */
  void var (const std::string &name, const geo::Variant &var);

  /**
   *  @brief Specifies an input
   *
   *  The name is the variable name which will receive the shapes for the individual tile.
   *  The transformation can be used to convert between database units.  
   *  If "as_region" is false, the input is taken as edge input.
   */
  void input (const std::string &name, const geo::RecursiveShapeIterator &iter, const geo::ICplxTrans &trans = geo::ICplxTrans (), Type type = TypeRegion, bool merged_semantics = true);

  /**
   *  @brief Specifies the output 
   *
   *  The name is the variable which carries the context information.
   *  In the script, the output given by this specification can be referred to as 
   *  _output(name, object).
   *
   *  This version will employ the output receiver given by "rec". "id" will be passed
   *  to the "id" parameter of the receiver's "put" implementation.
   *
   *  The ownership of the receiver object will be passed over to the processor.
   */
  void output (const std::string &name, size_t id, TileOutputReceiver *rec, const geo::ICplxTrans &trans);

  /**
   *  @brief Specifies output to a layout
   *
   *  This version will specify output to a layout object to the specified cell and layer. 
   *  The ep_ext parameter specifies what extension to apply when converting edge pairs to polygons.
   */
  void output (const std::string &name, geo::Layout &layout, geo::cell_index_type cell, const geo::LayerProperties &lp, geo::Coord ep_ext = 1);

  /**
   *  @brief Specifies output to a layout
   *
   *  This version will specify output to a layout object to the specified cell and layer. 
   *  The ep_ext parameter specifies what extension to apply when converting edge pairs to polygons.
   */
  void output (const std::string &name, geo::Layout &layout, geo::cell_index_type cell, unsigned int layer, geo::Coord ep_ext = 1);

  /**
   *  @brief Specifies output to a region
   *
   *  This version will specify output to a region object.
   *  The ep_ext parameter specifies what extension to apply when converting edge pairs to polygons.
   */
  void output (const std::string &name, geo::Region &region, geo::Coord ep_ext = 1);

  /**
   *  @brief Specifies output to an edge pair collection
   *
   *  This version will specify output to a EdgePairs object.
   *  Only edge pairs will be stored.
   */
  void output (const std::string &name, geo::EdgePairs &edge_pairs);

  /**
   *  @brief Specifies output to an text collection
   *
   *  This version will specify output to a Texts object.
   *  Only texts will be stored.
   */
  void output (const std::string &name, geo::Texts &texts);

  /**
   *  @brief Specifies output to an edge collection
   *
   *  This version will specify output to a Edges object.
   */
  void output (const std::string &name, geo::Edges &edges);

  /**
   *  @brief Gets the database unit under which the computation will be done
   */
  double dbu () const
  {
    return m_dbu_specific_set ? m_dbu_specific : m_dbu;
  }

  /**
   *  @brief Sets the database unit under which the computation will be done
   *
   *  Using this method will establish the given database unit as the target unit. 
   *  Otherwise, the first input's database unit will be used.
   */
  void set_dbu (double dbu) 
  {
    m_dbu_specific_set = true;
    m_dbu_specific = dbu;
  }

  /**
   *  @brief Disables or enables automatic scaling to database unit
   *
   *  If enabled, all inputs are automatically scaled to the database unit.
   *  The default is "enabled".
   */
  void set_scale_to_dbu (bool sdbu)
  {
    m_scale_to_dbu = sdbu;
  }

  /**
   *  @brief Returns a value indicating whether automatic scaling to database unit is enabled
   */
  bool scale_to_dbu () const
  {
    return m_scale_to_dbu;
  }

  /**
   *  @brief Sets the layout frame
   *
   *  The layout frame is the box (in micron units) taken into account for computing
   *  the tiles if the tile counts are not given. If the layout frame is not set or
   *  set to an empty box, the processor will try to derive the frame from the given
   *  inputs.
   */
  void set_frame (const geo::DBox &frame);

  /**
   *  @brief Gets the layout frame
   *
   *  This is the value set by "set_frame".
   */
  const geo::DBox &frame () const
  {
    return m_frame;
  }

  /**
   *  @brief Sets the tile size 
   *
   *  The tile size is given in micron units.
   *  Alternatively, the tiling can be specified in terms of tile number (tiles method).
   *  The tiles method will override the tile size specified in tile_size.
   */
  void tile_size (double w, double h);

  /**
   *  @brief Sets the tile number 
   *
   *  Specifies how many tiles to use.
   *  Alternatively, the tiling can be specified in terms of tile size (tile_size method).
   *  The tile_size method will override the tile count specified in tiles.
   */
  void tiles (size_t nx, size_t ny);

  /**
   *  @brief Sets the tile border 
   *
   *  The tile border specifies the amount by which the tiles overlap.
   *  The tile border is given in micron units.
   */
  void tile_border (double bx, double by);

  /**
   *  @brief Sets the tile origin
   *
   *  If the tile origin is specified, the lower left corner of the 
   *  lower left tile is fixed and will not be moved. 
   */
  void tile_origin (double xo, double yo);

  /**
   *  @brief Specifies the number of threads to use
   */
  void set_threads (size_t n);

  /**
   *  @brief Gets the number of threads used
   */
  size_t threads () const
  {
    return m_threads;
  }

  /**
   *  @brief Queue a script for execution with "execute"
   *
   *  Multiple scripts can be installed. They will be executed in parallel (if the number 
   *  of threads allows).
   */
  void queue (const std::string &script);

  /**
   *  @brief Execute the job
   *
   *  The description (desc) is shown in the progress bar.
   */
  void execute (const std::string &desc);

  /**
   *  @brief Gets the output mutex for operations not using the output method
   */
  static geo::Mutex &output_lock ()
  {
    return s_output_lock;
  }

private:
  friend class TilingProcessorWorker;
  friend class TilingProcessorOutputFunction;
  friend class TilingProcessorReceiverFunction;

  struct InputSpec
  {
    InputSpec () : type (TilingProcessor::TypeRegion), merged_semantics (false) { }
    std::string name;
    geo::RecursiveShapeIterator iter;
    geo::ICplxTrans trans;
    TilingProcessor::Type type;
    bool merged_semantics;
  };

  struct OutputSpec
  {
    OutputSpec () : id (0) { }
    std::string name;
    size_t id;
    geo::shared_ptr<geo::TileOutputReceiver> receiver;
    geo::ICplxTrans trans;
  };

  std::vector<InputSpec>::const_iterator begin_inputs () const { return m_inputs.begin (); }
  std::vector<InputSpec>::const_iterator end_inputs () const { return m_inputs.end (); }

  void put (size_t ix, size_t iy, const geo::Box &tile, const std::vector<geo::Variant> &args);
  geo::Variant receiver (const std::vector<geo::Variant> &args);
  geo::Eval &top_eval () { return m_top_eval; }

  std::vector<InputSpec> m_inputs;
  std::vector<OutputSpec> m_outputs;
  geo::DBox m_frame;
  double m_tile_width, m_tile_height;
  size_t m_ntiles_w, m_ntiles_h;
  bool m_tile_size_given, m_tile_count_given;
  double m_tile_origin_x, m_tile_origin_y;
  bool m_tile_origin_given;
  double m_tile_bx, m_tile_by;
  size_t m_threads;
  double m_dbu, m_dbu_specific;
  bool m_dbu_specific_set;
  bool m_scale_to_dbu;
  std::vector<std::string> m_scripts;
  geo::Eval m_top_eval;
  static geo::Mutex s_output_lock;
};

}

namespace geo
{
  template <>
  struct type_traits<geo::TilingProcessor> : public type_traits<void>
  {
    typedef geo::true_tag has_default_constructor;
    typedef geo::false_tag has_copy_constructor;
  };
}

#endif

