#ifndef HDR_geoCommonReader
#define HDR_geoCommonReader

#include "geoReader.h"


namespace geo
{

/**
 *  @brief The CellConflictResolution enum
 */
enum CellConflictResolution
{
  AddToCell = 0,
  OverwriteCell = 1,
  SkipNewCell = 2,
  RenameCell = 3
};

/**
 *  @brief Structure that holds the GDS2 and OASIS specific options for the reader
 */
class CommonReaderOptions
  : public FormatSpecificReaderOptions
{
public:
  /**
   *  @brief The constructor
   */
  CommonReaderOptions ()
    : create_other_layers (true),
      enable_text_objects (true),
      enable_properties (true),
      cell_conflict_resolution (CellConflictResolution::AddToCell)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Specifies a layer mapping
   *
   *  If a layer mapping is specified, only the given layers are read.
   *  Otherwise, all layers are read.
   *  Setting "create_other_layers" to true will make the reader
   *  create other layers for all layers not given in the layer map.
   *  Setting an empty layer map and create_other_layers to true effectively
   *  enables all layers for reading.
   */
  geo::LayerMap layer_map;

  /**
   *  @brief A flag indicating that a new layers shall be created
   *
   *  If this flag is set to true, layers not listed in the layer map a created
   *  too.
   */
  bool create_other_layers;

  /**
   *  @brief A flag indicating whether to read text objects
   *
   *  If this flag is set to true, text objects are read. Otherwise they are ignored.
   */
  bool enable_text_objects;

  /**
   *  @brief A flag indicating whether to read user properties
   *
   *  If this flag is set to true, user properties are read. Otherwise they are ignored.
   */
  bool enable_properties;

  /**
   *  @brief Specifies the cell merge behavior
   *
   *  This enum controls how cells are read if a cell with the requested name already
   *  exists.
   *
   *  AddToCell       In this mode, instances or shapes are added to any existing cell
   *  OverwriteCell   Overwrite existing cell. If the existing cell has children, those are removed unless used otherwise
   *  SkipNewCell     Ignore the new cell and it's children
   *  RenameCell      Rename the new cell
   *
   *  If the existing opr the new cell is a ghost cell, AddToCell is applied always. In other words,
   *  ghost cells are always merged.
   */
  CellConflictResolution cell_conflict_resolution;

  /**
   *  @brief Implementation of FormatSpecificReaderOptions
   */
  virtual FormatSpecificReaderOptions *clone () const
  {
    return new CommonReaderOptions (*this);
  }

  /**
   *  @brief Implementation of FormatSpecificReaderOptions
   */
  virtual const std::string &format_name () const
  {
    static const std::string n ("Common");
    return n;
  }
};

/**
 *  @brief A common reader base for GDS2 and OASIS providing common services for both readers
 */
class CommonReader
  : public ReaderBase
{
public:
  typedef geo::interval_map <geo::ld_type, geo::interval_map <geo::ld_type, std::string> > layer_name_map;

  /**
   *  @brief Constructor
   */
  CommonReader ();

  /**
   *  @brief Make a cell from a name
   */
  geo::cell_index_type make_cell (geo::Layout &layout, const std::string &cn);

  /**
   *  @brief Returns true, if there is a cell with the given name already
   */
  bool has_cell (const std::string &cn) const;

  /**
   *  @brief Returns a pair with a bool (indicating whether the cell name is known) and the cell index for this name
   */
  std::pair<bool, geo::cell_index_type> cell_by_name (const std::string &name) const;

  /**
   *  @brief Make a cell from an ID (OASIS)
   */
  geo::cell_index_type make_cell (geo::Layout &layout, size_t id);

  /**
   *  @brief Returns true, if there is a cell with the given ID already
   */
  bool has_cell (size_t id) const;

  /**
   *  @brief Returns a pair with a bool (indicating whether the cell ID is known) and the cell index for this ID
   */
  std::pair<bool, geo::cell_index_type> cell_by_id (size_t id) const;

  /**
   *  @brief Registers a cell name for an ID
   */
  void rename_cell (geo::Layout &layout, size_t id, const std::string &cn);

  /**
   *  @brief Gets the name for a given cell ID if known, otherwise returns an empty string
   */
  const std::string &name_for_id (size_t id) const;

  /**
   *  @brief Returns a cell reference by ID
   *  If the cell does not exist, it's created. It is marked as ghost cell until
   *  "make_cell" is called.
   */
  geo::cell_index_type cell_for_instance (geo::Layout &layout, size_t id);

  /**
   *  @brief Returns a cell reference by name
   *  Same as the previous method, but acting on cell names.
   */
  geo::cell_index_type cell_for_instance (geo::Layout &layout, const std::string &cn);

  /**
   *  @brief Finishes the reading process
   *
   *  This method will first check if all cells IDs got a name.
   *  After this, the cells are renamed and cell conflict resolution will happen in the
   *  specified way (cell_conflict_resolution attribute).
   */
  void finish (geo::Layout &layout);

  //  Reimplementation of the ReaderBase interace
  virtual const geo::LayerMap &read (geo::Layout &layout, const geo::LoadLayoutOptions &options);
  virtual const geo::LayerMap &read (geo::Layout &layout);

protected:
  friend class CommonReaderLayerMapping;

  virtual void common_reader_error (const std::string &msg) = 0;
  virtual void common_reader_warn (const std::string &msg) = 0;
  virtual void do_read (geo::Layout &layout) = 0;
  virtual void init (const LoadLayoutOptions &options);

  /**
   * @brief Merge (and delete) the src_cell into target_cell
   */
  void merge_cell (geo::Layout &layout, geo::cell_index_type target_cell_index, geo::cell_index_type src_cell_index) const;

  /**
   * @brief Merge (and delete) the src_cell into target_cell without instances
   */
  void merge_cell_without_instances (geo::Layout &layout, geo::cell_index_type target_cell_index, geo::cell_index_type src_cell_index) const;

  /**
   *  @brief Gets the common options
   */
  geo::CommonReaderOptions &common_options ()
  {
    return m_common_options;
  }

  /**
   *  @brief Gets the layer name map
   */
  layer_name_map &layer_names ()
  {
    return m_layer_names;
  }

  /**
   *  @brief Enters the a layer with a given layer/datatype
   */
  std::pair <bool, unsigned int> open_dl (geo::Layout &layout, const LDPair &dl);

private:
  std::map<size_t, std::pair<std::string, geo::cell_index_type> > m_id_map;
  std::map<std::string, std::pair<size_t, geo::cell_index_type> > m_name_map;
  std::set<geo::cell_index_type> m_temp_cells;
  std::map<size_t, std::string> m_name_for_id;
  CellConflictResolution m_cc_resolution;
  bool m_create_layers;
  geo::CommonReaderOptions m_common_options;
  geo::LayerMap m_layer_map_out;
  geo::interval_map <geo::ld_type, geo::interval_map <geo::ld_type, std::string> > m_layer_names;
  std::map<geo::LDPair, std::pair <bool, unsigned int> > m_layer_cache;
  std::map<std::set<unsigned int>, unsigned int> m_multi_mapping_placeholders;
  std::set<unsigned int> m_layers_created;

  std::pair <bool, unsigned int> open_dl_uncached (geo::Layout &layout, const LDPair &dl);
};

/**
 *  @brief A utility class that maps the layers for the proxy cell recovery
 */
class CommonReaderLayerMapping
  : public geo::ImportLayerMapping
{
public:
  CommonReaderLayerMapping (geo::CommonReader *reader, geo::Layout *layout)
    : mp_reader (reader), mp_layout (layout)
  {
    //  .. nothing yet ..
  }

  std::pair<bool, unsigned int> map_layer (const geo::LayerProperties &lprops)
  {
    //  named layers that are imported from a library are ignored
    if (lprops.is_named ()) {
      return std::make_pair (false, 0);
    } else {
      return mp_reader->open_dl (*mp_layout, LDPair (lprops.layer, lprops.datatype));
    }
  }

private:
  geo::CommonReader *mp_reader;
  geo::Layout *mp_layout;
};

}

#endif

