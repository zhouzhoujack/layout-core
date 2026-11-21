#ifndef HDR_geoHierarchyBuilder
#define HDR_geoHierarchyBuilder

#include "geoCommon.h"

#include "geoRecursiveShapeIterator.h"
#include "geoLayout.h"

#include <map>
#include <vector>
#include <set>

namespace geo
{

/**
 *  @brief A helper function comparing two recursive shape iterators for compatibility with respect to hierarchy building
 *
 *  This function will return -1, 0 or 1 depending on whether the two iterators
 *  can be used with the same builder (0) or whether they are less (-1) or greater (1).
 */
int DB_PUBLIC compare_iterators_with_respect_to_target_hierarchy (const geo::RecursiveShapeIterator &iter1, const geo::RecursiveShapeIterator &iter2);

/**
 *  @brief A class to receive shapes from the hierarchy builder
 *
 *  This class can be reimplemented to implement clipping and/or
 *  simplification.
 */
class DB_PUBLIC HierarchyBuilderShapeReceiver
{
public:
  HierarchyBuilderShapeReceiver () { }
  virtual ~HierarchyBuilderShapeReceiver () { }

  virtual void push (const geo::Shape &shape, const geo::ICplxTrans &trans, const geo::Box &region, const geo::RecursiveShapeReceiver::box_tree_type *complex_region, geo::Shapes *target) = 0;
  virtual void push (const geo::Box &shape, const geo::ICplxTrans &trans, const geo::Box &region, const geo::RecursiveShapeReceiver::box_tree_type *complex_region, geo::Shapes *target) = 0;
  virtual void push (const geo::Polygon &shape, const geo::ICplxTrans &trans, const geo::Box &region, const geo::RecursiveShapeReceiver::box_tree_type *complex_region, geo::Shapes *target) = 0;
};

/**
 *  @brief A shape receiver that simply pushes into the target
 */
class DB_PUBLIC HierarchyBuilderShapeInserter
  : public HierarchyBuilderShapeReceiver
{
public:
  HierarchyBuilderShapeInserter () { }

  virtual void push (const geo::Shape &shape, const geo::ICplxTrans &trans, const geo::Box &, const geo::RecursiveShapeReceiver::box_tree_type *, geo::Shapes *target)
  {
    geo::ident_map<geo::Layout::properties_id_type> pm;
    target->insert (shape, trans, pm);
  }

  virtual void push (const geo::Box &shape, const geo::ICplxTrans &trans, const geo::Box &, const geo::RecursiveShapeReceiver::box_tree_type *, geo::Shapes *target)
  {
    if (trans.is_ortho ()) {
      target->insert (shape.transformed (trans));
    } else {
      geo::Polygon poly (shape);
      target->insert (poly.transformed (trans));
    }
  }

  virtual void push (const geo::Polygon &shape, const geo::ICplxTrans &trans, const geo::Box &, const geo::RecursiveShapeReceiver::box_tree_type *, geo::Shapes *target)
  {
    if (trans.is_unity ()) {
      target->insert (shape);
    } else {
      target->insert (shape.transformed (trans));
    }
  }
};

/**
 *  @brief A clipping shape receiver that forwards to another one
 */
class DB_PUBLIC ClippingHierarchyBuilderShapeReceiver
  : public HierarchyBuilderShapeReceiver
{
public:
  ClippingHierarchyBuilderShapeReceiver (HierarchyBuilderShapeReceiver *pipe = 0);

  virtual void push (const geo::Shape &shape, const geo::ICplxTrans &trans, const geo::Box &, const geo::RecursiveShapeReceiver::box_tree_type *, geo::Shapes *target);
  virtual void push (const geo::Box &shape, const geo::ICplxTrans &trans, const geo::Box &, const geo::RecursiveShapeReceiver::box_tree_type *, geo::Shapes *target);
  virtual void push (const geo::Polygon &shape, const geo::ICplxTrans &trans, const geo::Box &, const geo::RecursiveShapeReceiver::box_tree_type *, geo::Shapes *target);

private:
  void insert_clipped (const geo::Box &box, const geo::ICplxTrans &trans, const geo::Box &region, const geo::RecursiveShapeReceiver::box_tree_type *complex_region, geo::Shapes *target);
  void insert_clipped (const geo::Polygon &poly, const geo::ICplxTrans &trans, const geo::Box &region, const geo::RecursiveShapeReceiver::box_tree_type *complex_region, geo::Shapes *target);
  static bool is_inside (const geo::Box &box, const geo::Box &region, const geo::RecursiveShapeReceiver::box_tree_type *complex_region);
  static bool is_outside (const geo::Box &box, const geo::Box &region, const geo::RecursiveShapeReceiver::box_tree_type *complex_region);

  HierarchyBuilderShapeReceiver *mp_pipe;
};

/**
 *  @brief A polygon reducing shape receiver that forwards to another one
 */
class DB_PUBLIC ReducingHierarchyBuilderShapeReceiver
  : public HierarchyBuilderShapeReceiver
{
public:
  ReducingHierarchyBuilderShapeReceiver (HierarchyBuilderShapeReceiver *pipe = 0, double area_ratio = 3.0, size_t max_vertex_count = 16, bool reject_odd_polygons = false);

  virtual void push (const geo::Shape &shape, const geo::ICplxTrans &trans, const geo::Box &, const geo::RecursiveShapeReceiver::box_tree_type *, geo::Shapes *target);
  virtual void push (const geo::Box &shape, const geo::ICplxTrans &trans, const geo::Box &, const geo::RecursiveShapeReceiver::box_tree_type *, geo::Shapes *target);
  virtual void push (const geo::Polygon &shape, const geo::ICplxTrans &trans, const geo::Box &, const geo::RecursiveShapeReceiver::box_tree_type *, geo::Shapes *target);

private:
  void reduce (const geo::Polygon &poly, const geo::ICplxTrans &trans, const geo::Box &region, const geo::RecursiveShapeReceiver::box_tree_type *complex_region, geo::Shapes *target, bool check = true);

  HierarchyBuilderShapeReceiver *mp_pipe;
  double m_area_ratio;
  size_t m_max_vertex_count;
  bool m_reject_odd_polygons;
};

/**
 *  @brief A polygon reference generating shape receiver that feeds a shapes array after turning the shapes into PolygonRefs
 */
class DB_PUBLIC PolygonReferenceHierarchyBuilderShapeReceiver
  : public HierarchyBuilderShapeReceiver
{
public:
  PolygonReferenceHierarchyBuilderShapeReceiver (geo::Layout *layout, int text_enlargement = -1, const geo::Variant &text_prop_name = geo::Variant ());

  virtual void push (const geo::Shape &shape, const geo::ICplxTrans &trans, const geo::Box &, const geo::RecursiveShapeReceiver::box_tree_type *, geo::Shapes *target);
  virtual void push (const geo::Box &shape, const geo::ICplxTrans &trans, const geo::Box &, const geo::RecursiveShapeReceiver::box_tree_type *, geo::Shapes *target);
  virtual void push (const geo::Polygon &shape, const geo::ICplxTrans &trans, const geo::Box &, const geo::RecursiveShapeReceiver::box_tree_type *, geo::Shapes *target);

private:
  geo::Layout *mp_layout;
  int m_text_enlargement;
  bool m_make_text_prop;
  geo::property_names_id_type m_text_prop_id;
};

/**
 *  @brief An edge-generating shape receiver that feeds a shapes array after turning the shapes into edges
 */
class DB_PUBLIC EdgeBuildingHierarchyBuilderShapeReceiver
  : public HierarchyBuilderShapeReceiver
{
public:
  EdgeBuildingHierarchyBuilderShapeReceiver (bool as_edges);

  virtual void push (const geo::Shape &shape, const geo::ICplxTrans &trans, const geo::Box &, const geo::RecursiveShapeReceiver::box_tree_type *, geo::Shapes *target);
  virtual void push (const geo::Box &shape, const geo::ICplxTrans &trans, const geo::Box &, const geo::RecursiveShapeReceiver::box_tree_type *, geo::Shapes *target);
  virtual void push (const geo::Polygon &shape, const geo::ICplxTrans &trans, const geo::Box &, const geo::RecursiveShapeReceiver::box_tree_type *, geo::Shapes *target);

private:
  bool m_as_edges;
};

/**
 *  @brief An edge pair-generating shape receiver that feeds a shapes array after turning the shapes into edges
 */
class DB_PUBLIC EdgePairBuildingHierarchyBuilderShapeReceiver
  : public HierarchyBuilderShapeReceiver
{
public:
  EdgePairBuildingHierarchyBuilderShapeReceiver ();

  virtual void push (const geo::Shape &shape, const geo::ICplxTrans &trans, const geo::Box &, const geo::RecursiveShapeReceiver::box_tree_type *, geo::Shapes *target);
  virtual void push (const geo::Box &, const geo::ICplxTrans &, const geo::Box &, const geo::RecursiveShapeReceiver::box_tree_type *, geo::Shapes *) { }
  virtual void push (const geo::Polygon &, const geo::ICplxTrans &, const geo::Box &, const geo::RecursiveShapeReceiver::box_tree_type *, geo::Shapes *) { }
};

/**
 *  @brief An text-generating shape receiver that feeds a shapes array after turning the shapes into texts
 */
class DB_PUBLIC TextBuildingHierarchyBuilderShapeReceiver
  : public HierarchyBuilderShapeReceiver
{
public:
  TextBuildingHierarchyBuilderShapeReceiver (geo::Layout *layout);

  virtual void push (const geo::Shape &shape, const geo::ICplxTrans &trans, const geo::Box &, const geo::RecursiveShapeReceiver::box_tree_type *, geo::Shapes *target);
  virtual void push (const geo::Box &, const geo::ICplxTrans &, const geo::Box &, const geo::RecursiveShapeReceiver::box_tree_type *, geo::Shapes *) { }
  virtual void push (const geo::Polygon &, const geo::ICplxTrans &, const geo::Box &, const geo::RecursiveShapeReceiver::box_tree_type *, geo::Shapes *) { }

private:
  geo::Layout *mp_layout;
};

/**
 *  @brief A class building a hierarchy from a recursive shape iterator in push mode
 *
 *  This class is a RecursiveShapeReceiver which acts on the hierarchy events and
 *  uses them to rebuild a hierarchy in the target layout. In can be used multiple
 *  times and will reuse the hierarchy as far as possible.
 *
 *  The hierarchy builder can form clip variants for cells and clip the shapes
 *  according to the selected region.
 *
 *  NOTE: the hierarchy build should not be used in multiple passes with regions
 *  as the hierarchy is sampled in the first pass and the hierarchy builder will
 *  rely on precisely the same hierarchy arrangement. This is not given with
 *  region selections.
 */
class DB_PUBLIC HierarchyBuilder
  : public geo::RecursiveShapeReceiver
{
public:
  struct CellMapKey
  {
    CellMapKey ()
      : original_cell (0), inactive (false)
    { }

    CellMapKey (geo::cell_index_type _original_cell, bool _inactive, const std::set<geo::Box> &_clip_region)
      : original_cell (_original_cell), inactive (_inactive), clip_region (_clip_region)
    { }

    bool operator== (const CellMapKey &other) const
    {
      return original_cell == other.original_cell && inactive == other.inactive && clip_region == other.clip_region;
    }

    bool operator< (const CellMapKey &other) const
    {
      if (original_cell != other.original_cell) { return original_cell < other.original_cell; }
      if (inactive != other.inactive) { return inactive < other.inactive; }
      if (clip_region != other.clip_region) { return clip_region < other.clip_region; }
      return false;
    }

    geo::cell_index_type original_cell;
    bool inactive;
    std::set<geo::Box> clip_region;
  };


  typedef std::map<CellMapKey, geo::cell_index_type> cell_map_type;
  typedef std::map<geo::cell_index_type, std::vector<geo::cell_index_type> > original_target_to_variants_map_type;
  typedef std::map<geo::cell_index_type, geo::cell_index_type> variant_to_original_target_map_type;

  HierarchyBuilder (geo::Layout *target, unsigned int target_layer, const geo::ICplxTrans &trans = geo::ICplxTrans (), HierarchyBuilderShapeReceiver *pipe = 0);
  HierarchyBuilder (geo::Layout *target, const geo::ICplxTrans &trans = geo::ICplxTrans (), HierarchyBuilderShapeReceiver *pipe = 0);
  virtual ~HierarchyBuilder ();

  /**
   *  @brief Installs a custom shape receiver
   *  The hierarchy builder will *NOT* take ownership of this object.
   */
  void set_shape_receiver (HierarchyBuilderShapeReceiver *pipe);

  virtual void begin (const RecursiveShapeIterator *iter);
  virtual void end (const RecursiveShapeIterator *iter);
  virtual void enter_cell (const RecursiveShapeIterator *iter, const geo::Cell *cell, const geo::Box &region, const box_tree_type *complex_region);
  virtual void leave_cell (const RecursiveShapeIterator *iter, const geo::Cell *cell);
  virtual new_inst_mode new_inst (const RecursiveShapeIterator *iter, const geo::CellInstArray &inst, const ICplxTrans &always_apply, const geo::Box &region, const box_tree_type *complex_region, bool all);
  virtual bool new_inst_member (const RecursiveShapeIterator *iter, const geo::CellInstArray &inst, const ICplxTrans &always_apply, const geo::ICplxTrans &trans, const geo::Box &region, const box_tree_type *complex_region, bool all);
  virtual void shape (const RecursiveShapeIterator *iter, const geo::Shape &shape, const geo::ICplxTrans &always_apply, const geo::ICplxTrans &trans, const geo::Box &region, const box_tree_type *complex_region);

  /**
   *  @brief Sets the target layer - shapes will be put there
   */
  void set_target_layer (unsigned int target_layer)
  {
    m_target_layer = target_layer;
  }

  /**
   *  @brief Reset the builder - performs a new initial pass
   */
  void reset ();

  /**
   *  @brief Gets the initial cell the builder produced in the target layout
   */
  geo::Cell *initial_cell ()
  {
    return mp_initial_cell;
  }

  /**
   *  @brief Gets the target layout
   */
  geo::Layout *target ()
  {
    return mp_target.get ();
  }

  /**
   *  @brief Gets the recursive shape iterator the data was taken from
   */
  const geo::RecursiveShapeIterator &source () const
  {
    return m_source;
  }

  /**
   *  @brief Gets the iterator for the cell map
   */
  cell_map_type::const_iterator begin_cell_map () const
  {
    return m_cell_map.begin ();
  }

  /**
   *  @brief Gets the iterator for the cell map (end)
   */
  cell_map_type::const_iterator end_cell_map () const
  {
    return m_cell_map.end ();
  }

  /**
   *  @brief Unmaps an original cell/clip box version from the original-to-working copy cell map
   *
   *  An unmapped cell is never again considered.
   */
  void unmap (const cell_map_type::key_type &k)
  {
    m_cell_map.erase (k);
  }

  /**
   *  @brief Maps an original cell/clip box version to a original-to-working copy cell
   */
  void map (const cell_map_type::key_type &k, geo::cell_index_type ci)
  {
    m_cell_map [k] = ci;
  }

  /**
   *  @brief Marks a cell as a variant of another
   *
   *  The first cell is either the original, non-variant target cell or itself a variant.
   *  The second cell will be registered as a variant of the first one.
   */
  void register_variant (geo::cell_index_type non_var, geo::cell_index_type var);

  /**
   *  @brief Unregisters a cell as a variant
   */
  void unregister_variant (geo::cell_index_type var);

  /**
   *  @brief Gets a value indicating whether the given cell is a variant cell
   */
  bool is_variant (geo::cell_index_type ci) const
  {
    return m_variants_to_original_target_map.find (ci) != m_variants_to_original_target_map.end ();
  }

  /**
   *  @brief Gets the original target for a variant cell
   */
  geo::cell_index_type original_target_for_variant (geo::cell_index_type ci) const;

private:
  geo::cell_index_type make_cell_variant (const HierarchyBuilder::CellMapKey &key, const std::string &cell_name);

  geo::weak_ptr<geo::Layout> mp_target;
  HierarchyBuilderShapeReceiver *mp_pipe;
  bool m_initial_pass;
  geo::RecursiveShapeIterator m_source;
  cell_map_type m_cell_map;
  original_target_to_variants_map_type m_original_targets_to_variants_map;
  variant_to_original_target_map_type m_variants_to_original_target_map;

  std::set<cell_map_type::key_type> m_cells_seen;
  std::set<geo::cell_index_type> m_cells_to_be_filled;
  cell_map_type::const_iterator m_cm_entry;
  bool m_cm_new_entry;
  unsigned int m_target_layer;
  std::vector<std::pair<bool, std::vector<geo::Cell *> > > m_cell_stack;
  geo::Cell *mp_initial_cell;

  geo::ICplxTrans m_trans;
};

}

#endif
