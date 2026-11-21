#ifndef HDR_geoShapeProcessor
#define HDR_geoShapeProcessor


#include "geoTypes.h"
#include "geoEdge.h"
#include "geoPolygonGenerators.h"
#include "geoShapes.h"

#include <vector>

namespace geo
{

class Layout;
class Cell;

/**
 *  @brief A polygon receiver creating shapes from the polygons inside a geo::Shapes container
 *
 *  This class implements the PolygonSink interface.
 */
class ShapeGenerator
  : public PolygonSink
{
public:
  /**
   *  @brief Constructor associating the receiver with a shapes container
   *
   *  See the description of the EdgeSink and PolygonSink interface for a explanation when
   *  the start method is called and when the shape container is cleared if "clear_shapes"
   *  is set.
   *
   *  @param clear_shapes If true, the shapes container is cleared on the start event.
   */
  ShapeGenerator (geo::Shapes &shapes, bool clear_shapes = false)
    : PolygonSink (), mp_shapes (&shapes), m_clear_shapes (clear_shapes)
  { }

  /**
   *  @brief Implementation of the PolygonSink interface
   */
  virtual void put (const geo::Polygon &polygon) 
  {
    mp_shapes->insert (polygon);
  }

  /**
   *  @brief Implementation of the PolygonSink interface
   */
  virtual void start () 
  { 
    if (m_clear_shapes) {
      mp_shapes->clear ();
      //  The single-shot scheme is a easy way to overcome problems with multiple start/flush brackets (i.e. on size filter)
      m_clear_shapes = false;
    }
  }

private:
  geo::Shapes *mp_shapes;
  bool m_clear_shapes;
};

/**
 *  @brief An edge receiver creating shapes from the edges inside a geo::Shapes container
 *
 *  This class implements the EdgeSink interface.
 */
class EdgeShapeGenerator
  : public EdgeSink
{
public:
  /**
   *  @brief Constructor associating the receiver with a shapes container
   *
   *  See the description of the EdgeSink and PolygonSink interface for a explanation when
   *  the start method is called and when the shape container is cleared if "clear_shapes"
   *  is set.
   *
   *  @param clear_shapes If true, the shapes container is cleared on the start event.
   */
  EdgeShapeGenerator (geo::Shapes &shapes, bool clear_shapes = false)
    : EdgeSink (), mp_shapes (&shapes), m_clear_shapes (clear_shapes)
  { }

  /**
   *  @brief Implementation of the EdgeSink interface
   */
  virtual void put (const geo::Edge &edge) 
  {
    mp_shapes->insert (edge);
  }

  /**
   *  @brief Implementation of the PolygonSink interface
   */
  virtual void start () 
  { 
    if (m_clear_shapes) {
      mp_shapes->clear ();
      //  The single-shot scheme is a easy way to overcome problems with multiple start/flush brackets (i.e. on size filter)
      m_clear_shapes = false;
    }
  }

private:
  geo::Shapes *mp_shapes;
  bool m_clear_shapes;
};

/**
 *  @brief A processor for shape objects
 *
 *  Similar to the edge processor, this class deals with shape objects and shape containers 
 *  instead of polygons.
 */
class ShapeProcessor 
{
public:
  /**
   *  @brief Constructor
   *
   *  @param report_progress If true, a geo::Progress object will be created to report any progress (warning: this will impose a performance penalty)
   */
  ShapeProcessor (bool report_progress = false, const std::string &progress_desc = std::string ());

  /**
   *  @brief Clear the shapes stored currently
   */
  void clear ();

  /**
   *  @brief Reserve the number of edges
   */
  void reserve (size_t n);

  /**
   *  @brief Sets the base verbosity of the processor (see EdgeProcessor::set_base_verbosity for details)
   */
  void set_base_verbosity (int bv)
  {
    m_processor.set_base_verbosity (bv);
  }

  /**
   *  @brief Enable progress
   *
   *  @param progress_text The description text of the progress object
   */
  void enable_progress (const std::string &progress_desc = std::string ())
  {
    m_processor.enable_progress (progress_desc);
  }

  /**
   *  @brief Disable progress
   */
  void disable_progress ()
  {
    m_processor.disable_progress ();
  }

  /**
   *  @brief Insert a shape without transformation
   */
  void insert (const geo::Shape &shape, geo::EdgeProcessor::property_type p)
  {
    insert (shape, geo::UnitTrans (), p);
  }

  /**
   *  @brief Insert a native shape
   */
  template <class S>
  void insert_native (const S &shape, geo::EdgeProcessor::property_type p)
  {
    m_processor.insert (shape, p);
  }

  /**
   *  @brief Insert a shape
   */
  template <class T>
  void insert (const geo::Shape &shape, const T &trans, geo::EdgeProcessor::property_type p)
  {
    if (shape.is_polygon ()) {

      for (geo::Shape::polygon_edge_iterator e = shape.begin_edge (); ! e.at_end (); ++e) {
        m_processor.insert ((*e).transform (trans), p);
      }

    } else if (shape.is_path ()) {

      geo::Polygon poly;
      shape.polygon (poly);
      for (geo::Polygon::polygon_edge_iterator e = poly.begin_edge (); ! e.at_end (); ++e) {
        m_processor.insert ((*e).transform (trans), p);
      }

    } else if (shape.is_box ()) {

      geo::Box b (shape.box ());
      m_processor.insert (geo::Edge (b.lower_left (), b.upper_left ()).transform (trans), p);
      m_processor.insert (geo::Edge (b.upper_left (), b.upper_right ()).transform (trans), p);
      m_processor.insert (geo::Edge (b.upper_right (), b.lower_right ()).transform (trans), p);
      m_processor.insert (geo::Edge (b.lower_right (), b.lower_left ()).transform (trans), p);

    }
  }

  /**
   *  @brief Count the edges for a shape
   */
  size_t count_edges (const geo::Shape &shape) const;

  /**
   *  @brief Insert a sequence of shapes
   *
   *  This method does not reserve for the number of elements required. This must
   *  be done explicitly for performance benefits.
   */
  template <class Iter>
  void insert_sequence (Iter from, Iter to, geo::EdgeProcessor::property_type p = 0)
  {
    for (Iter i = from; i != to; ++i) {
      insert (*i, p);
    }
  }

  /**
   *  @brief Insert a sequence of shapes (iterator with at_end semantics)
   *
   *  This method does not reserve for the number of elements required. This must
   *  be done explicitly for performance benefits.
   */
  template <class Iter>
  void insert_sequence (Iter i, geo::EdgeProcessor::property_type p = 0)
  {
    for ( ; !i.at_end (); ++i) {
      insert (*i, p);
    }
  }

  /**
   *  @brief Process the given edges 
   *
   *  This method uses the given sink as target and the given evaluator for defining
   *  the method to use
   */
  void process (geo::EdgeSink &es, EdgeEvaluatorBase &op);

  /**
   *  @brief Merge the given shapes
   *
   *  See the EdgeProcessor for a description of the merge method. This implementation takes shapes
   *  rather than polygons for input and produces a polygon set.
   *
   *  @param in The set of shapes to merge
   *  @param trans A corresponding set of transformations to apply on the shapes
   *  @param out The result (a polygon vector)
   *  @param min_wc The minimum wrap count for output (0: all polygons, 1: at least two overlapping)
   *  @param resolve_holes true, if holes should be resolved into the hull
   *  @param min_coherence true, if minimum polygons should be created for touching corners
   */
  void merge (const std::vector<geo::Shape> &in, const std::vector<geo::CplxTrans> &trans,
              std::vector <geo::Polygon> &out, unsigned int min_wc = 0, bool resolve_holes = true, bool min_coherence = true);

  /**
   *  @brief Merge the given shapes
   *
   *  This is equivalent to the previous method except that no transformations can be specified.
   *
   *  @param in The set of shapes to merge
   *  @param out The result (a polygon vector)
   *  @param min_wc The minimum wrap count for output (0: all polygons, 1: at least two overlapping)
   *  @param resolve_holes true, if holes should be resolved into the hull
   *  @param min_coherence true, if minimum polygons should be created for touching corners
   */
  void merge (const std::vector<geo::Shape> &in, std::vector <geo::Polygon> &out, unsigned int min_wc = 0, bool resolve_holes = true, bool min_coherence = true)
  {
    merge (in, std::vector<geo::CplxTrans> (), out, min_wc, resolve_holes, min_coherence);
  }

  /**
   *  @brief Merge the given shapes into an edge set
   *
   *  This is basically a "or" operation on a single layer creating edges.
   *
   *  @param in The set of shapes to merge
   *  @param trans A corresponding set of transformations to apply on the shapes
   *  @param out The result (an edge vector)
   *  @param min_wc The minimum wrap count for output (0: all polygons, 1: at least two overlapping)
   */
  void merge (const std::vector<geo::Shape> &in, const std::vector<geo::CplxTrans> &trans,
              std::vector <geo::Edge> &out, unsigned int min_wc = 0);

  /**
   *  @brief Merge the given shapes into an edge set
   *
   *  This is basically a "or" operation on a single layer creating edges.
   *  This is equivalent to the previous method except that no transformations can be specified.
   *
   *  @param in The set of shapes to merge
   *  @param out The result (an edge vector)
   *  @param min_wc The minimum wrap count for output (0: all polygons, 1: at least two overlapping)
   */
  void merge (const std::vector<geo::Shape> &in, std::vector<geo::Edge> &out, unsigned int min_wc = 0)
  {
    merge (in, std::vector<geo::CplxTrans> (), out, min_wc);
  }

  /**
   *  @brief Merge the given shapes from a layout to a shape container
   *
   *  This is basically a "or" operation on a single layer.
   *  The input can be taken from a layout and the result is delivered to a 
   *  shape container.
   *
   *  @param layout_in The layout from which to take the input
   *  @param cell_in The cell from which to take the input
   *  @param layer_in The layer from which to take the input
   *  @param out Where to store the results 
   *  @param with_sub_hierarchy Take shapes from the cell and subcells
   *  @param min_wc The minimum wrap count for output (0: all polygons, 1: at least two overlapping)
   *  @param resolve_holes true, if holes should be resolved into the hull
   *  @param min_coherence true, if minimum polygons should be created for touching corners
   */
  void merge (const geo::Layout &layout_in, const geo::Cell &cell_in, unsigned int layer_in, 
              geo::Shapes &out, bool with_sub_hierarchy, unsigned int min_wc = 0, bool resolve_holes = true, bool min_coherence = true)
  {
    std::vector<unsigned int> layers_in;
    layers_in.push_back (layer_in);
    merge (layout_in, cell_in, layers_in, out, with_sub_hierarchy, min_wc, resolve_holes, min_coherence);
  }

  /**
   *  @brief Merge the given shapes from a layout to a shape container
   *
   *  This is basically a "or" operation on a single layer.
   *  The input can be taken from a layout and the result is delivered to a 
   *  shape container.
   *
   *  @param layout_in The layout from which to take the input
   *  @param cell_in The cell from which to take the input
   *  @param layers_in The layers from which to take the input
   *  @param out Where to store the results 
   *  @param with_sub_hierarchy Take shapes from the cell and subcells
   *  @param min_wc The minimum wrap count for output (0: all polygons, 1: at least two overlapping)
   *  @param resolve_holes true, if holes should be resolved into the hull
   *  @param min_coherence true, if minimum polygons should be created for touching corners
   */
  void merge (const geo::Layout &layout_in, const geo::Cell &cell_in, const std::vector<unsigned int> &layers_in, 
              geo::Shapes &out, bool with_sub_hierarchy, unsigned int min_wc = 0, bool resolve_holes = true, bool min_coherence = true);

  /**
   *  @brief Boolean operation on two given shape sets into a polygon set
   *
   *  @param in_a The set of shapes to use for input A
   *  @param trans_a A set of transformations to apply before the shapes are used
   *  @param in_b The set of shapes to use for input A
   *  @param trans_b A set of transformations to apply before the shapes are used
   *  @param mode The boolean operation
   *  @param out The result (an polygon vector)
   *  @param resolve_holes true, if holes should be resolved into the hull
   *  @param min_coherence true, if minimum polygons should be created for touching corners
   */
  void boolean (const std::vector<geo::Shape> &in_a, const std::vector<geo::CplxTrans> &trans_a,
                const std::vector<geo::Shape> &in_b, const std::vector<geo::CplxTrans> &trans_b,
                int mode, std::vector <geo::Polygon> &out, bool resolve_holes = true, bool min_coherence = true);

  /**
   *  @brief Boolean operation on two given shape sets into a polygon set
   *
   *  @param in_a The set of shapes to use for input A
   *  @param in_b The set of shapes to use for input A
   *  @param mode The boolean operation
   *  @param out The result (an polygon vector)
   *  @param resolve_holes true, if holes should be resolved into the hull
   *  @param min_coherence true, if minimum polygons should be created for touching corners
   */
  void boolean (const std::vector<geo::Shape> &in_a, const std::vector<geo::Shape> &in_b, 
                int mode, std::vector <geo::Polygon> &out, bool resolve_holes = true, bool min_coherence = true)
  {
    boolean (in_a, std::vector<geo::CplxTrans> (), in_b, std::vector<geo::CplxTrans> (), mode, out, resolve_holes, min_coherence);
  }

  /**
   *  @brief Boolean operation on two given shape sets into an edge set
   *
   *  @param in_a The set of shapes to use for input A
   *  @param trans_a A set of transformations to apply before the shapes are used
   *  @param in_b The set of shapes to use for input A
   *  @param trans_b A set of transformations to apply before the shapes are used
   *  @param mode The boolean operation
   *  @param out The result (an edge vector)
   */
  void boolean (const std::vector<geo::Shape> &in_a, const std::vector<geo::CplxTrans> &trans_a,
                const std::vector<geo::Shape> &in_b, const std::vector<geo::CplxTrans> &trans_b,
                int mode, std::vector <geo::Edge> &out);

  /**
   *  @brief Boolean operation on two given shape sets into an edge set
   *
   *  @param in_a The set of shapes to use for input A
   *  @param in_b The set of shapes to use for input A
   *  @param mode The boolean operation
   *  @param out The result (an edge vector)
   */
  void boolean (const std::vector<geo::Shape> &in_a, const std::vector<geo::Shape> &in_b, 
                int mode, std::vector <geo::Edge> &out)
  {
    boolean (in_a, std::vector<geo::CplxTrans> (), in_b, std::vector<geo::CplxTrans> (), mode, out);
  }

  /**
   *  @brief Boolean operation on the given shapes from a layout to a shape container
   *
   *  The input can be taken from separate layouts and the result is delivered to a 
   *  shape container.
   *
   *  @param layout_in_a The layout from which to take the input for source A
   *  @param cell_in_a The cell from which to take the input for source A
   *  @param layer_in_a The layer from which to take the input for source A
   *  @param layout_in_b The layout from which to take the input for source B
   *  @param cell_in_b The cell from which to take the input for source B
   *  @param layer_in_b The layer from which to take the input for source B
   *  @param out Where to store the results 
   *  @param mode The boolean operation to apply
   *  @param with_sub_hierarchy Take shapes from the cell and subcells
   *  @param resolve_holes true, if holes should be resolved into the hull
   *  @param min_coherence true, if minimum polygons should be created for touching corners
   */
  void boolean (const geo::Layout &layout_in_a, const geo::Cell &cell_in_a, unsigned int layer_in_a, 
                const geo::Layout &layout_in_b, const geo::Cell &cell_in_b, unsigned int layer_in_b, 
                geo::Shapes &out, int mode, bool with_sub_hierarchy = false, bool resolve_holes = true, bool min_coherence = true)
  {
    std::vector<unsigned int> layers_in_a;
    layers_in_a.push_back (layer_in_a);
    std::vector<unsigned int> layers_in_b;
    layers_in_b.push_back (layer_in_b);
    boolean (layout_in_a, cell_in_a, layers_in_a, layout_in_b, cell_in_b, layers_in_b, out, mode, with_sub_hierarchy, resolve_holes, min_coherence);
  }

  /**
   *  @brief Boolean operation on the given shapes from a layout to a shape container
   *
   *  The input can be taken from separate layouts and the result is delivered to a 
   *  shape container.
   *
   *  @param layout_in_a The layout from which to take the input for source A
   *  @param cell_in_a The cell from which to take the input for source A
   *  @param layers_in_a The layer from which to take the input for source A
   *  @param layout_in_b The layout from which to take the input for source B
   *  @param cell_in_b The cell from which to take the input for source B
   *  @param layers_in_b The layer from which to take the input for source B
   *  @param out Where to store the results 
   *  @param mode The boolean operation to apply
   *  @param with_sub_hierarchy Take shapes from the cell and subcells
   *  @param resolve_holes true, if holes should be resolved into the hull
   *  @param min_coherence true, if minimum polygons should be created for touching corners
   */
  void boolean (const geo::Layout &layout_in_a, const geo::Cell &cell_in_a, const std::vector<unsigned int> &layers_in_a, 
                const geo::Layout &layout_in_b, const geo::Cell &cell_in_b, const std::vector<unsigned int> &layers_in_b, 
                geo::Shapes &out, int mode, bool with_sub_hierarchy = false, bool resolve_holes = true, bool min_coherence = true);

  /**
   *  @brief Size the given shapes into an polygon set
   *
   *  This is equivalent to the previous method except that no transformations can be specified.
   *
   *  @param in The set of shapes to size
   *  @param trans A set of transformations to apply before the shapes are used
   *  @param dx The sizing to apply (x-direction)
   *  @param dy The sizing to apply (y-direction)
   *  @param out The result (an edge vector)
   *  @param mode The sizing mode (see geo::Polygon for a description)
   *  @param resolve_holes true, if holes should be resolved into the hull
   *  @param min_coherence true, if minimum polygons should be created for touching corners
   */
  void size (const std::vector<geo::Shape> &in, const std::vector<geo::CplxTrans> &trans,
             geo::Coord dx, geo::Coord dy, std::vector <geo::Polygon> &out, unsigned int mode = 2, bool resolve_holes = true, bool min_coherence = true);

  /**
   *  @brief Size the given shapes into an polygon set
   *
   *  This is equivalent to the previous method except that no transformations can be specified.
   *
   *  @param in The set of shapes to size
   *  @param trans A set of transformations to apply before the shapes are used
   *  @param d The sizing to apply 
   *  @param out The result (an edge vector)
   *  @param mode The sizing mode (see geo::Polygon for a description)
   *  @param resolve_holes true, if holes should be resolved into the hull
   *  @param min_coherence true, if minimum polygons should be created for touching corners
   */
  void size (const std::vector<geo::Shape> &in, const std::vector<geo::CplxTrans> &trans,
             geo::Coord d, std::vector <geo::Polygon> &out, unsigned int mode = 2, bool resolve_holes = true, bool min_coherence = true)
  {
    size (in, trans, d, d, out, mode, resolve_holes, min_coherence);
  }

  /**
   *  @brief Size the given shapes into an polygon set
   *
   *  This is equivalent to the previous method except that no transformations can be specified.
   *
   *  @param in The set of shapes to size
   *  @param d The sizing to apply 
   *  @param out The result (an edge vector)
   *  @param mode The sizing mode (see geo::Polygon for a description)
   *  @param resolve_holes true, if holes should be resolved into the hull
   *  @param min_coherence true, if minimum polygons should be created for touching corners
   */
  void size (const std::vector<geo::Shape> &in, 
             geo::Coord d, std::vector <geo::Polygon> &out, unsigned int mode = 2, bool resolve_holes = true, bool min_coherence = true)
  {
    size (in, std::vector<geo::CplxTrans> (), d, out, mode, resolve_holes, min_coherence);
  }

  /**
   *  @brief Size the given shapes into an polygon set
   *
   *  This is equivalent to the previous method except that no transformations can be specified.
   *
   *  @param in The set of shapes to size
   *  @param dx The sizing to apply (x-direction)
   *  @param dy The sizing to apply (y-direction)
   *  @param out The result (an edge vector)
   *  @param mode The sizing mode (see geo::Polygon for a description)
   *  @param resolve_holes true, if holes should be resolved into the hull
   *  @param min_coherence true, if minimum polygons should be created for touching corners
   */
  void size (const std::vector<geo::Shape> &in, 
             geo::Coord dx, geo::Coord dy, std::vector <geo::Polygon> &out, unsigned int mode = 2, bool resolve_holes = true, bool min_coherence = true)
  {
    size (in, std::vector<geo::CplxTrans> (), dx, dy, out, mode, resolve_holes, min_coherence);
  }

  /**
   *  @brief Size the given shapes into an edge set
   *
   *  This is equivalent to the previous method except that no transformations can be specified.
   *
   *  @param in The set of shapes to size
   *  @param trans A set of transformations to apply before the shapes are used
   *  @param dx The sizing to apply (x-direction)
   *  @param dy The sizing to apply (y-direction)
   *  @param out The result (an edge vector)
   *  @param mode The sizing mode (see geo::Polygon for a description)
   */
  void size (const std::vector<geo::Shape> &in, const std::vector<geo::CplxTrans> &trans,
             geo::Coord dx, geo::Coord dy, std::vector <geo::Edge> &out, unsigned int mode = 2);

  /**
   *  @brief Size the given shapes into an edge set
   *
   *  This is equivalent to the previous method except that no transformations can be specified.
   *
   *  @param in The set of shapes to size
   *  @param trans A set of transformations to apply before the shapes are used
   *  @param d The sizing to apply 
   *  @param out The result (an edge vector)
   *  @param mode The sizing mode (see geo::Polygon for a description)
   */
  void size (const std::vector<geo::Shape> &in, const std::vector<geo::CplxTrans> &trans,
             geo::Coord d, std::vector <geo::Edge> &out, unsigned int mode = 2)
  {
    size (in, trans, d, d, out, mode);
  }

  /**
   *  @brief Size the given shapes into an edge set
   *
   *  This is equivalent to the previous method except that no transformations can be specified.
   *
   *  @param in The set of shapes to size
   *  @param d The sizing to apply 
   *  @param out The result (an edge vector)
   *  @param mode The sizing mode (see geo::Polygon for a description)
   */
  void size (const std::vector<geo::Shape> &in, 
             geo::Coord d, std::vector <geo::Edge> &out, unsigned int mode = 2)
  {
    size (in, std::vector<geo::CplxTrans> (), d, out, mode);
  }

  /**
   *  @brief Size the given shapes into an edge set
   *
   *  This is equivalent to the previous method except that no transformations can be specified.
   *
   *  @param in The set of shapes to size
   *  @param dx The sizing to apply (x-direction)
   *  @param dy The sizing to apply (y-direction)
   *  @param out The result (an edge vector)
   *  @param mode The sizing mode (see geo::Polygon for a description)
   */
  void size (const std::vector<geo::Shape> &in, 
             geo::Coord dx, geo::Coord dy, std::vector <geo::Edge> &out, unsigned int mode = 2)
  {
    size (in, std::vector<geo::CplxTrans> (), dx, dy, out, mode);
  }

  /**
   *  @brief Size operation on the given shapes from a layout to a shape container
   *
   *  The input can be taken from a layout and the result is delivered to a 
   *  shape container.
   *
   *  @param layout_in The layout from which to take the input for source A
   *  @param cell_in The cell from which to take the input for source A
   *  @param layer_in The layer from which to take the input for source A
   *  @param out Where to store the results 
   *  @param d The sizing to apply
   *  @param mode The sizing mode (see geo::Polygon for a description)
   *  @param with_sub_hierarchy Take shapes from the cell and subcells
   *  @param resolve_holes true, if holes should be resolved into the hull
   *  @param min_coherence true, if minimum polygons should be created for touching corners
   */
  void size (const geo::Layout &layout_in, const geo::Cell &cell_in, unsigned int layer_in, 
             geo::Shapes &out, geo::Coord d, unsigned int mode = 2, bool with_sub_hierarchy = false, bool resolve_holes = true, bool min_coherence = true)
  {
    size (layout_in, cell_in, layer_in, out, d, d, mode, with_sub_hierarchy, resolve_holes, min_coherence);
  }
             
  /**
   *  @brief Size operation on the given shapes from a layout to a shape container
   *
   *  The input can be taken from a layout and the result is delivered to a 
   *  shape container.
   *
   *  @param layout_in The layout from which to take the input for source A
   *  @param cell_in The cell from which to take the input for source A
   *  @param layers_in The layers from which to take the input for source A
   *  @param out Where to store the results 
   *  @param d The sizing to apply
   *  @param mode The sizing mode (see geo::Polygon for a description)
   *  @param with_sub_hierarchy Take shapes from the cell and subcells
   *  @param resolve_holes true, if holes should be resolved into the hull
   *  @param min_coherence true, if minimum polygons should be created for touching corners
   */
  void size (const geo::Layout &layout_in, const geo::Cell &cell_in, const std::vector<unsigned int> &layers_in, 
             geo::Shapes &out, geo::Coord d, unsigned int mode = 2, bool with_sub_hierarchy = false, bool resolve_holes = true, bool min_coherence = true)
  {
    size (layout_in, cell_in, layers_in, out, d, d, mode, with_sub_hierarchy, resolve_holes, min_coherence);
  }
             
  /**
   *  @brief Size operation on the given shapes from a layout to a shape container
   *
   *  The input can be taken from a layout and the result is delivered to a 
   *  shape container.
   *
   *  @param layout_in The layout from which to take the input for source A
   *  @param cell_in The cell from which to take the input for source A
   *  @param layer_in The layer from which to take the input for source A
   *  @param out Where to store the results 
   *  @param dx The sizing to apply (x-direction)
   *  @param dy The sizing to apply (y-direction)
   *  @param mode The sizing mode (see geo::Polygon for a description)
   *  @param with_sub_hierarchy Take shapes from the cell and subcells
   *  @param resolve_holes true, if holes should be resolved into the hull
   *  @param min_coherence true, if minimum polygons should be created for touching corners
   */
  void size (const geo::Layout &layout_in, const geo::Cell &cell_in, unsigned int layer_in, 
             geo::Shapes &out, geo::Coord dx, geo::Coord dy, unsigned int mode = 2, bool with_sub_hierarchy = false, bool resolve_holes = true, bool min_coherence = true)
  {
    std::vector<unsigned int> layers_in;
    layers_in.push_back (layer_in);
    size (layout_in, cell_in, layers_in, out, dx, dy, mode, with_sub_hierarchy, resolve_holes, min_coherence);
  }

  /**
   *  @brief Size operation on the given shapes from a layout to a shape container
   *
   *  The input can be taken from a layout and the result is delivered to a 
   *  shape container.
   *
   *  @param layout_in The layout from which to take the input for source A
   *  @param cell_in The cell from which to take the input for source A
   *  @param layers_in The layers from which to take the input for source A
   *  @param out Where to store the results 
   *  @param dx The sizing to apply (x-direction)
   *  @param dy The sizing to apply (y-direction)
   *  @param mode The sizing mode (see geo::Polygon for a description)
   *  @param with_sub_hierarchy Take shapes from the cell and subcells
   *  @param resolve_holes true, if holes should be resolved into the hull
   *  @param min_coherence true, if minimum polygons should be created for touching corners
   */
  void size (const geo::Layout &layout_in, const geo::Cell &cell_in, const std::vector<unsigned int> &layers_in, 
             geo::Shapes &out, geo::Coord dx, geo::Coord dy, unsigned int mode = 2, bool with_sub_hierarchy = false, bool resolve_holes = true, bool min_coherence = true);

private:
  EdgeProcessor m_processor;

  void collect_shapes_hier (const geo::CplxTrans &tr, const geo::Layout &layout, const geo::Cell &cell, unsigned int layer, int hier_levels, size_t &pn, size_t pdelta);
  size_t count_edges_hier (const geo::Layout &layout, const geo::Cell &cell, unsigned int layer, std::map<std::pair<geo::cell_index_type, int>, size_t> &cache, int hier_levels) const;
};

}

#endif


