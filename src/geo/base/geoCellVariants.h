#ifndef HDR_geoCellVariants
#define HDR_geoCellVariants


#include "geoTrans.h"
#include "geoLayout.h"

#include <memory>

namespace geo
{

/**
 *  @brief The reducer interface
 *
 *  The transformation reducer is used by the variant builder to provide a
 *  reduced version of the transformation. Variants are built based on this
 *  reduced transformation.
 *
 *  Reduction must satisfy the modulo condition:
 *
 *   reduce(A*B) = reduce(reduce(A)*reduce(B))
 */
class TransformationReducer
{
public:
  TransformationReducer () { }
  virtual ~TransformationReducer () { }

  virtual geo::Trans reduce_trans (const geo::Trans &trans) const { return reduce (trans); }
  virtual geo::ICplxTrans reduce_trans (const geo::ICplxTrans &trans) const { return reduce (trans); }
  virtual geo::Trans reduce (const geo::Trans &trans) const = 0;
  virtual geo::ICplxTrans reduce (const geo::ICplxTrans &trans) const = 0;
  virtual bool is_translation_invariant () const { return true; }
};

/**
 *  @brief An orientation reducer
 *
 *  This reducer incarnation reduces the transformation to it's rotation/mirror part.
 */
struct OrientationReducer
  : public TransformationReducer
{
  geo::ICplxTrans reduce (const geo::ICplxTrans &trans) const;
  geo::Trans reduce (const geo::Trans &trans) const;
};

/**
 *  @brief A magnification reducer
 *
 *  This reducer incarnation reduces the transformation to it's scaling part.
 */
struct MagnificationReducer
  : public TransformationReducer
{
  geo::ICplxTrans reduce (const geo::ICplxTrans &trans) const;
  geo::Trans reduce (const geo::Trans &) const;
};

/**
 *  @brief A reducer for magnification and XYAnisotropy
 *
 *  This reducer is used for cases where an x and y-value is given, e.g. anisotropic size.
 */
struct XYAnisotropyAndMagnificationReducer
  : public TransformationReducer
{
  geo::ICplxTrans reduce (const geo::ICplxTrans &trans) const;
  geo::Trans reduce (const geo::Trans &trans) const;
};

/**
 *  @brief A magnification and orientation reducer
 *
 *  This reducer incarnation reduces the transformation to it's rotation/mirror/magnification part (2d matrix)
 */
struct MagnificationAndOrientationReducer
  : public TransformationReducer
{
  geo::ICplxTrans reduce (const geo::ICplxTrans &trans) const;
  geo::Trans reduce (const geo::Trans &trans) const;
};

/**
 *  @brief A grid reducer
 *
 *  This reducer incarnation reduces the transformation to it's displacement modulo a grid
 */
struct GridReducer
  : public TransformationReducer
{
  GridReducer (geo::Coord grid);

  geo::ICplxTrans reduce (const geo::ICplxTrans &trans) const;
  geo::Trans reduce (const geo::Trans &trans) const;

  bool is_translation_invariant () const { return false; }

private:
  geo::Coord m_grid;
};

/**
 *  @brief A scale+grid reducer
 *
 *  This reducer incarnation reduces the transformation to it's displacement modulo a grid
 *  after a specified scaling has been applied.
 *  The scaling is given by a divider and multiplier and is mult / div.
 */
struct ScaleAndGridReducer
  : public TransformationReducer
{
  ScaleAndGridReducer (geo::Coord grid, geo::Coord mult, geo::Coord div);

  virtual geo::ICplxTrans reduce_trans (const geo::ICplxTrans &trans) const;
  virtual geo::Trans reduce_trans (const geo::Trans &trans) const;
  virtual geo::ICplxTrans reduce (const geo::ICplxTrans &trans) const;
  virtual geo::Trans reduce (const geo::Trans &trans) const;

  bool is_translation_invariant () const { return false; }

private:
  int64_t m_mult;
  int64_t m_grid;
};

/**
 *  @brief A class computing variants for cells according to a given criterion
 *
 *  The cell variants are build from the cell instances and are accumulated over
 *  the hierarchy path.
 */
class VariantsCollectorBase
{
public:
  /**
   *  @brief Creates a variant collector without a transformation reducer
   */
  VariantsCollectorBase ();

  /**
   *  @brief Creates a variant collector with the given reducer
   */
  VariantsCollectorBase (const TransformationReducer *red);

  /**
   *  @brief Collects cell variants for the given layout starting from the top cell
   */
  void collect (const geo::Layout &layout, const geo::Cell &top_cell);

  /**
   *  @brief Creates cell variants for singularization of the different variants
   *
   *  After this method can been used, all cells with more than one variant are separated and
   *  the corresponding instances are updated.
   *
   *  If given, *var_table will be filled with a map giving the new cell and variant against
   *  the old cell for all cells with more than one variant.
   */
  void separate_variants (geo::Layout &layout, geo::Cell &top_cell, std::map<geo::cell_index_type, std::map<geo::ICplxTrans, geo::cell_index_type> > *var_table = 0);

  /**
   *  @brief Commits the shapes for different variants to the current cell hierarchy
   *
   *  This is an alternative approach and will push the variant shapes into the parent hierarchy.
   *  "to_commit" initially is a set of shapes to commit for the given cell and variant.
   *  This map is modified during the algorithm and should be discarded later.
   */
  void commit_shapes (geo::Layout &layout, geo::Cell &top_cell, unsigned int layer, std::map<geo::cell_index_type, std::map<geo::ICplxTrans, geo::Shapes> > &to_commit);

  /**
   *  @brief Gets the variants for a given cell
   *
   *  The keys of the map are the variants, the values is the instance count of the variant
   *  (as seen from the top cell).
   */
  const std::map<geo::ICplxTrans, size_t> &variants (geo::cell_index_type ci) const;

  /**
   *  @brief Returns true, if variants have been built
   */
  bool has_variants () const;

  /**
   *  @brief Utility: copy all shapes from one cell to another
   */
  static void copy_shapes (geo::Layout &layout, geo::cell_index_type ci_to, geo::cell_index_type ci_from);

private:
  std::map<geo::cell_index_type, std::map<geo::ICplxTrans, size_t> > m_variants;
  const TransformationReducer *mp_red;

  void add_variant (std::map<geo::ICplxTrans, size_t> &variants, const geo::CellInstArray &inst, bool tl_invariant) const;
  void add_variant_non_tl_invariant (std::map<geo::ICplxTrans, size_t> &variants, const geo::CellInstArray &inst) const;
  void add_variant_tl_invariant (std::map<geo::ICplxTrans, size_t> &variants, const geo::CellInstArray &inst) const;
  void product (const std::map<geo::ICplxTrans, size_t> &v1, const std::map<geo::ICplxTrans, size_t> &v2, std::map<geo::ICplxTrans, size_t> &prod) const;
  void create_var_instances (geo::Cell &in_cell, std::vector<geo::CellInstArrayWithProperties> &inst, const geo::ICplxTrans &for_var, const std::map<geo::cell_index_type, std::map<geo::ICplxTrans, geo::cell_index_type> > &var_table, bool tl_invariant) const;
  void create_var_instances_non_tl_invariant (geo::Cell &in_cell, std::vector<geo::CellInstArrayWithProperties> &inst, const geo::ICplxTrans &for_var, const std::map<geo::cell_index_type, std::map<geo::ICplxTrans, geo::cell_index_type> > &var_table) const;
  void create_var_instances_tl_invariant (geo::Cell &in_cell, std::vector<geo::CellInstArrayWithProperties> &inst, const geo::ICplxTrans &for_var, const std::map<geo::cell_index_type, std::map<geo::ICplxTrans, geo::cell_index_type> > &var_table) const;
};

/**
 *  @brief A template using a specific transformation reducer
 */
template <class RED>
class cell_variants_collector
  : public VariantsCollectorBase
{
public:
  /**
   *  @brief Creates a variant collector without a transformation reducer
   */
  cell_variants_collector ()
    : VariantsCollectorBase (&m_red)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Creates a variant collector with the given reducer
   *
   *  The collector will take ownership over the reducer
   */
  cell_variants_collector (const RED &red)
    : VariantsCollectorBase (&m_red), m_red (red)
  {
    //  .. nothing yet ..
  }

private:
  RED m_red;
};

}  // namespace geo

#endif

