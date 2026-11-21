#include "geoShapes2.h"

namespace geo
{
// -----------------------------------------------------------------------------
//  Shapes2 implementation


// -------------------------------------------------------------------------------
//  dereference operator

/**
 *  @brief Dereferences the given shape into the given Shapes container
 */
struct deref_into_shapes
{
  deref_into_shapes (Shapes *shapes) 
    : mp_shapes (shapes)
  {
    //  .. nothing yet ..
  }

private:
  template <class Sh, class PropIdMap>
  void op (const Sh &sh, PropIdMap & /*pm*/)
  {
    mp_shapes->insert (sh);
  }

  template <class Sh, class Trans, class PropIdMap>
  void op (const geo::text_ref<Sh, Trans> &sh, PropIdMap & /*pm*/)
  {
    Sh inst;
    sh.instantiate (inst);
    mp_shapes->insert (inst);
  }

  template <class Sh, class Trans, class PropIdMap>
  void op (const geo::polygon_ref<Sh, Trans> &sh, PropIdMap & /*pm*/)
  {
    Sh inst;
    sh.instantiate (inst);
    mp_shapes->insert (inst);
  }

  template <class Sh, class Trans, class PropIdMap>
  void op (const geo::path_ref<Sh, Trans> &sh, PropIdMap & /*pm*/)
  {
    Sh inst;
    sh.instantiate (inst);
    mp_shapes->insert (inst);
  }

  template <class Sh, class ATrans, class PropIdMap>
  void op (const geo::array<Sh, ATrans> &sh, PropIdMap & /*pm*/)
  {
    for (typename geo::array<Sh, ATrans>::iterator a = sh.begin (); ! a.at_end (); ++a) {
      mp_shapes->insert (*a * sh.object ());
    }
  }

  template <class ATrans, class InnerSh, class Trans, class PropIdMap>
  void op (const geo::array<geo::text_ref<InnerSh, Trans>, ATrans> &sh, PropIdMap & /*pm*/)
  {
    InnerSh inst;
    for (typename geo::array<geo::text_ref<InnerSh, Trans>, ATrans>::iterator a = sh.begin (); ! a.at_end (); ++a) {
      (*a * sh.object ()).instantiate (inst);
      mp_shapes->insert (inst);
    }
  }

  template <class ATrans, class InnerSh, class Trans, class PropIdMap>
  void op (const geo::array<geo::polygon_ref<InnerSh, Trans>, ATrans> &sh, PropIdMap & /*pm*/)
  {
    InnerSh inst;
    for (typename geo::array<geo::polygon_ref<InnerSh, Trans>, ATrans>::iterator a = sh.begin (); ! a.at_end (); ++a) {
      (*a * sh.object ()).instantiate (inst);
      mp_shapes->insert (inst);
    }
  }

  template <class ATrans, class InnerSh, class Trans, class PropIdMap>
  void op (const geo::array<geo::path_ref<InnerSh, Trans>, ATrans> &sh, PropIdMap & /*pm*/)
  {
    InnerSh inst;
    for (typename geo::array<geo::path_ref<InnerSh, Trans>, ATrans>::iterator a = sh.begin (); ! a.at_end (); ++a) {
      (*a * sh.object ()).instantiate (inst);
      mp_shapes->insert (inst);
    }
  }

  //  special transformation for boxes - this must consider the non-ortho case where
  //  a box becomes a non-box.
  template <class C, class R, class ATrans, class PropIdMap>
  void op (const geo::array<geo::box<C, R>, ATrans> &sh, PropIdMap & /*pm*/)
  {
    geo::box<C> box (sh.object ()); // avoid problems with short boxes
    for (typename geo::array<geo::box<C, R>, ATrans>::iterator a = sh.begin (); ! a.at_end (); ++a) {
      mp_shapes->insert (box.transformed (*a));
    }
  }

  template <class Sh, class PropIdMap>
  void op (const geo::object_with_properties<Sh> &sh, PropIdMap &pm)
  {
    mp_shapes->insert (geo::object_with_properties<Sh> (sh, pm (sh.properties_id ())));
  }

  template <class InnerSh, class Trans, class PropIdMap>
  void op (const geo::object_with_properties<geo::text_ref<InnerSh, Trans> > &sh, PropIdMap &pm)
  {
    InnerSh inst;
    sh.instantiate (inst);
    mp_shapes->insert (geo::object_with_properties<InnerSh> (inst, pm (sh.properties_id ())));
  }

  template <class InnerSh, class Trans, class PropIdMap>
  void op (const geo::object_with_properties<geo::polygon_ref<InnerSh, Trans> > &sh, PropIdMap &pm)
  {
    InnerSh inst;
    sh.instantiate (inst);
    mp_shapes->insert (geo::object_with_properties<InnerSh> (inst, pm (sh.properties_id ())));
  }

  template <class InnerSh, class Trans, class PropIdMap>
  void op (const geo::object_with_properties<geo::path_ref<InnerSh, Trans> > &sh, PropIdMap &pm)
  {
    InnerSh inst;
    sh.instantiate (inst);
    mp_shapes->insert (geo::object_with_properties<InnerSh> (inst, pm (sh.properties_id ())));
  }

  template <class InnerSh, class ATrans, class PropIdMap>
  void op (const geo::object_with_properties<geo::array<InnerSh, ATrans> > &sh, PropIdMap &pm)
  {
    for (typename geo::array<InnerSh, ATrans>::iterator a = sh.begin (); ! a.at_end (); ++a) {
      mp_shapes->insert (geo::object_with_properties<InnerSh> (*a * sh.object (), pm (sh.properties_id ())));
    }
  }

  //  special transformation for boxes - this must consider the non-ortho case where
  //  a box becomes a non-box.
  template <class C, class R, class ATrans, class PropIdMap>
  void op (const geo::object_with_properties<geo::array<geo::box<C, R>, ATrans> > &sh, PropIdMap &pm)
  {
    geo::box<C> box (sh.object ()); // avoid problems with short boxes
    for (typename geo::array<geo::box<C, R>, ATrans>::iterator a = sh.begin (); ! a.at_end (); ++a) {
      mp_shapes->insert (geo::object_with_properties<geo::box<C> > (box.transformed (*a), pm (sh.properties_id ())));
    }
  }

  template <class ATrans, class InnerSh, class Trans, class PropIdMap>
  void op (const geo::object_with_properties<geo::array<geo::text_ref<InnerSh, Trans>, ATrans> > &sh, PropIdMap &pm)
  {
    InnerSh inst;
    for (typename geo::array<geo::text_ref<InnerSh, Trans>, ATrans>::iterator a = sh.begin (); ! a.at_end (); ++a) {
      (*a * sh.object ()).instantiate (inst);
      mp_shapes->insert (geo::object_with_properties<InnerSh> (inst, pm (sh.properties_id ())));
    }
  }

  template <class ATrans, class InnerSh, class Trans, class PropIdMap>
  void op (const geo::object_with_properties<geo::array<geo::polygon_ref<InnerSh, Trans>, ATrans> > &sh, PropIdMap &pm)
  {
    InnerSh inst;
    for (typename geo::array<geo::polygon_ref<InnerSh, Trans>, ATrans>::iterator a = sh.begin (); ! a.at_end (); ++a) {
      (*a * sh.object ()).instantiate (inst);
      mp_shapes->insert (geo::object_with_properties<InnerSh> (inst, pm (sh.properties_id ())));
    }
  }

  template <class ATrans, class InnerSh, class Trans, class PropIdMap>
  void op (const geo::object_with_properties<geo::array<geo::path_ref<InnerSh, Trans>, ATrans> > &sh, PropIdMap &pm)
  {
    InnerSh inst;
    for (typename geo::array<geo::path_ref<InnerSh, Trans>, ATrans>::iterator a = sh.begin (); ! a.at_end (); ++a) {
      (*a * sh.object ()).instantiate (inst);
      mp_shapes->insert (geo::object_with_properties<InnerSh> (inst, pm (sh.properties_id ())));
    }
  }

public:
  template <class Sh>
  void operator() (const Sh &shape)
  {
    geo::ident_map <geo::properties_id_type> pm;
    op (shape, pm);
  }

  template <class Sh, class PropIdMap>
  void operator() (const Sh &shape, PropIdMap &pm)
  {
    op (shape, pm);
  }

private:
  Shapes *mp_shapes;
};

// -------------------------------------------------------------------------------
//  dereference and transform operator

/**
 *  @brief Dereferences the given shape and transforms it into the given Shapes container
 */
struct deref_and_transform_into_shapes
{
  deref_and_transform_into_shapes (Shapes *shapes) 
    : mp_shapes (shapes)
  {
    //  .. nothing yet ..
  }

private:
  template <class Sh, class Trans, class PropIdMap>
  void op (const Sh &sh, const Trans &trans, PropIdMap & /*pm*/)
  {
    mp_shapes->insert (sh.transformed (trans));
  }

  template <class Sh, class Trans, class RTrans, class PropIdMap>
  void op (const geo::polygon_ref<Sh, RTrans> &sh, const Trans &trans, PropIdMap & /*pm*/)
  {
    Sh inst;
    sh.instantiate (inst);
    inst.transform (trans);
    mp_shapes->insert (inst);
  }

  template <class Sh, class Trans, class RTrans, class PropIdMap>
  void op (const geo::path_ref<Sh, RTrans> &sh, const Trans &trans, PropIdMap & /*pm*/)
  {
    Sh inst;
    sh.instantiate (inst);
    inst.transform (trans);
    mp_shapes->insert (inst);
  }

  template <class Sh, class Trans, class RTrans, class PropIdMap>
  void op (const geo::text_ref<Sh, RTrans> &sh, const Trans &trans, PropIdMap & /*pm*/)
  {
    Sh inst;
    sh.instantiate (inst);
    inst.transform (trans);
    mp_shapes->insert (inst);
  }

  //  special transformation for boxes - this must consider the non-ortho case where
  //  a box becomes a non-box.
  template <class C, class R, class T, class PropIdMap>
  void op (const geo::box<C, R> &sh, const T &trans, PropIdMap & /*pm*/)
  {
    if (trans.is_ortho ()) {
      geo::box<C> box (sh); // avoid problems with short boxes
      mp_shapes->insert (box.transformed (trans));
    } else {
      geo::polygon<C> poly = geo::polygon<C> (geo::box<C> (sh));
      mp_shapes->insert (poly.transformed (trans));
    }
  }

  //  special transformation for boxes - this must consider the non-ortho case where
  //  a box becomes a non-box.
  template <class C, class R, class ATrans, class Trans, class PropIdMap>
  void op (const geo::array<geo::box<C, R>, ATrans> &sh, const Trans &trans, PropIdMap & /*pm*/)
  {
    if (trans.is_ortho ()) {
      geo::box<C> box (sh.object ()); // avoid problems with short boxes
      for (typename geo::array<geo::box<C, R>, ATrans>::iterator a = sh.begin (); ! a.at_end (); ++a) {
        mp_shapes->insert (box.transformed (trans * Trans (*a)));
      }
    } else {
      geo::polygon<C> poly = geo::polygon<C> (geo::box<C> (sh.object ()));
      for (typename geo::array<geo::box<C, R>, ATrans>::iterator a = sh.begin (); ! a.at_end (); ++a) {
        mp_shapes->insert (poly.transformed (trans * Trans (*a)));
      }
    }
  }

  template <class Sh, class Trans, class ATrans, class PropIdMap>
  void op (const geo::array<Sh, ATrans> &sh, const Trans &trans, PropIdMap & /*pm*/)
  {
    for (typename geo::array<Sh, ATrans>::iterator a = sh.begin (); ! a.at_end (); ++a) {
      mp_shapes->insert (sh.object ().transformed (trans * Trans (*a)));
    }
  }

  template <class Trans, class ATrans, class InnerSh, class RTrans, class PropIdMap>
  void op (const geo::array<geo::text_ref<InnerSh, RTrans>, ATrans> &sh, const Trans &trans, PropIdMap & /*pm*/)
  {
    InnerSh inst;
    sh.object ().instantiate (inst);
    for (typename geo::array<geo::text_ref<InnerSh, RTrans>, ATrans >::iterator a = sh.begin (); ! a.at_end (); ++a) {
      mp_shapes->insert (inst.transformed (trans * Trans (*a)));
    }
  }

  template <class Trans, class ATrans, class InnerSh, class RTrans, class PropIdMap>
  void op (const geo::array<geo::polygon_ref<InnerSh, RTrans>, ATrans> &sh, const Trans &trans, PropIdMap & /*pm*/)
  {
    InnerSh inst;
    sh.object ().instantiate (inst);
    for (typename geo::array<geo::polygon_ref<InnerSh, RTrans>, ATrans >::iterator a = sh.begin (); ! a.at_end (); ++a) {
      mp_shapes->insert (inst.transformed (trans * Trans (*a)));
    }
  }

  template <class Trans, class ATrans, class InnerSh, class RTrans, class PropIdMap>
  void op (const geo::array<geo::path_ref<InnerSh, RTrans>, ATrans> &sh, const Trans &trans, PropIdMap & /*pm*/)
  {
    InnerSh inst;
    sh.object ().instantiate (inst);
    for (typename geo::array<geo::path_ref<InnerSh, RTrans>, ATrans >::iterator a = sh.begin (); ! a.at_end (); ++a) {
      mp_shapes->insert (inst.transformed (trans * Trans (*a)));
    }
  }

  template <class Sh, class Trans, class PropIdMap>
  void op (const geo::object_with_properties<Sh> &sh, const Trans &trans, PropIdMap &pm)
  {
    mp_shapes->insert (geo::object_with_properties<Sh> (sh.transformed (trans), pm (sh.properties_id ())));
  }

  //  special transformation for boxes - this must consider the non-ortho case where
  //  a box becomes a non-box.
  template <class C, class R, class Trans, class PropIdMap>
  void op (const geo::object_with_properties<geo::box<C, R> > &sh, const Trans &trans, PropIdMap &pm)
  {
    if (trans.is_ortho ()) {
      geo::box<C> box (sh); // avoid problems with short boxes
      mp_shapes->insert (geo::object_with_properties<geo::box<C> > (box.transformed (trans), pm (sh.properties_id ())));
    } else {
      geo::polygon<C> poly  = geo::polygon<C> (geo::box<C> (sh));
      mp_shapes->insert (geo::object_with_properties<geo::polygon<C> > (poly.transformed (trans), pm (sh.properties_id ())));
    }
  }

  template <class Trans, class InnerSh, class RTrans, class PropIdMap>
  void op (const geo::object_with_properties<geo::text_ref<InnerSh, RTrans> > &sh, const Trans &trans, PropIdMap &pm)
  {
    InnerSh inst;
    sh.instantiate (inst);
    inst.transform (trans);
    mp_shapes->insert (geo::object_with_properties<InnerSh> (inst, pm (sh.properties_id ())));
  }

  template <class Trans, class InnerSh, class RTrans, class PropIdMap>
  void op (const geo::object_with_properties<geo::polygon_ref<InnerSh, RTrans> > &sh, const Trans &trans, PropIdMap &pm)
  {
    InnerSh inst;
    sh.instantiate (inst);
    inst.transform (trans);
    mp_shapes->insert (geo::object_with_properties<InnerSh> (inst, pm (sh.properties_id ())));
  }

  template <class Trans, class InnerSh, class RTrans, class PropIdMap>
  void op (const geo::object_with_properties<geo::path_ref<InnerSh, RTrans> > &sh, const Trans &trans, PropIdMap &pm)
  {
    InnerSh inst;
    sh.instantiate (inst);
    inst.transform (trans);
    mp_shapes->insert (geo::object_with_properties<InnerSh> (inst, pm (sh.properties_id ())));
  }

  //  special transformation for boxes - this must consider the non-ortho case where
  //  a box becomes a non-box.
  template <class C, class R, class ATrans, class Trans, class PropIdMap>
  void op (const geo::object_with_properties<geo::array<geo::box<C, R>, ATrans> > &sh, const Trans &trans, PropIdMap &pm)
  {
    if (trans.is_ortho ()) {
      geo::box<C> box (sh.object ()); // avoid problems with short boxes
      for (typename geo::array<geo::box<C, R>, ATrans>::iterator a = sh.begin (); ! a.at_end (); ++a) {
        mp_shapes->insert (geo::object_with_properties<geo::box<C> > (box.transformed (trans * Trans (*a)), pm (sh.properties_id ())));
      }
    } else {
      geo::polygon<C> poly  = geo::polygon<C> (geo::box<C> (sh.object ()));
      for (typename geo::array<geo::box<C, R>, ATrans>::iterator a = sh.begin (); ! a.at_end (); ++a) {
        mp_shapes->insert (geo::object_with_properties<geo::polygon<C> > (poly.transformed (trans * Trans (*a)), pm (sh.properties_id ())));
      }
    }
  }

  template <class Trans, class AObject, class ATrans, class InnerSh, class PropIdMap>
  void op (const geo::object_with_properties<geo::array<AObject, ATrans> > &sh, const Trans &trans, PropIdMap &pm)
  {
    InnerSh inst (sh.object ());
    for (typename geo::array<AObject, ATrans>::iterator a = sh.begin (); ! a.at_end (); ++a) {
      mp_shapes->insert (geo::object_with_properties<InnerSh> (inst.transformed (trans * Trans (*a)), pm (sh.properties_id ())));
    }
  }

  template <class Trans, class ATrans, class InnerSh, class RTrans, class PropIdMap>
  void op (const geo::object_with_properties<geo::array<geo::text_ref<InnerSh, RTrans>, ATrans> > &sh, const Trans &trans, PropIdMap &pm)
  {
    InnerSh inst;
    sh.object ().instantiate (inst);
    for (typename geo::array<geo::text_ref<InnerSh, RTrans>, ATrans>::iterator a = sh.begin (); ! a.at_end (); ++a) {
      mp_shapes->insert (geo::object_with_properties<InnerSh> (inst.transformed (trans * Trans (*a)), pm (sh.properties_id ())));
    }
  }

  template <class Trans, class ATrans, class InnerSh, class RTrans, class PropIdMap>
  void op (const geo::object_with_properties<geo::array<geo::polygon_ref<InnerSh, RTrans>, ATrans> > &sh, const Trans &trans, PropIdMap &pm)
  {
    InnerSh inst;
    sh.object ().instantiate (inst);
    for (typename geo::array<geo::polygon_ref<InnerSh, RTrans>, ATrans>::iterator a = sh.begin (); ! a.at_end (); ++a) {
      mp_shapes->insert (geo::object_with_properties<InnerSh> (inst.transformed (trans * Trans (*a)), pm (sh.properties_id ())));
    }
  }

  template <class Trans, class ATrans, class InnerSh, class RTrans, class PropIdMap>
  void op (const geo::object_with_properties<geo::array<geo::path_ref<InnerSh, RTrans>, ATrans> > &sh, const Trans &trans, PropIdMap &pm)
  {
    InnerSh inst;
    sh.object ().instantiate (inst);
    for (typename geo::array<geo::path_ref<InnerSh, RTrans>, ATrans>::iterator a = sh.begin (); ! a.at_end (); ++a) {
      mp_shapes->insert (geo::object_with_properties<InnerSh> (inst.transformed (trans * Trans (*a)), pm (sh.properties_id ())));
    }
  }

public:
  template <class Sh, class T>
  void operator() (const Sh &shape, const T &trans)
  {
    geo::ident_map <geo::properties_id_type> pm;
    op (shape, trans, pm);
  }

  template <class Sh, class T, class PropIdMap>
  void operator() (const Sh &shape, const T &trans, PropIdMap &pm)
  {
    op (shape, trans, pm);
  }

private:
  Shapes *mp_shapes;
};

// -------------------------------------------------------------------------------
//  translate operator

/**
 *  @brief Translates the given shape into the given Shapes container using the given array repository 
 */
struct translate_into_shapes
{
  translate_into_shapes (Shapes *shapes, GenericRepository &rep, ArrayRepository &array_rep) 
    : mp_shapes (shapes), m_shape_rep (rep), m_array_rep (array_rep)
  {
    //  .. nothing yet ..
  }

  template <class Sh>
  void operator() (const Sh &sh)
  {
    Sh new_shape;
    new_shape.translate (sh, m_shape_rep, m_array_rep);
    mp_shapes->insert (new_shape);
  }

  template <class Sh, class PropIdMap>
  void operator() (const Sh &sh, PropIdMap & /*pm*/)
  {
    Sh new_shape;
    new_shape.translate (sh, m_shape_rep, m_array_rep);
    mp_shapes->insert (new_shape);
  }

  template <class Sh, class T>
  void operator() (const geo::object_with_properties<Sh> &sh)
  {
    Sh new_shape;
    new_shape.translate (sh, m_shape_rep, m_array_rep);
    mp_shapes->insert (geo::object_with_properties<Sh> (new_shape, sh.properties_id ()));
  }

  template <class Sh, class T, class PropIdMap>
  void operator() (const geo::object_with_properties<Sh> &sh, PropIdMap &pm)
  {
    Sh new_shape;
    new_shape.translate (sh, m_shape_rep, m_array_rep);
    mp_shapes->insert (geo::object_with_properties<Sh> (new_shape, pm (sh.properties_id ())));
  }

private:
  geo::Shapes *mp_shapes;
  GenericRepository &m_shape_rep;
  ArrayRepository &m_array_rep;
};

// -------------------------------------------------------------------------------
//  translate and transform operator

/**
 *  @brief Translates and transforms the given shape into the given Shapes container using the given array repository 
 */
struct translate_and_transform_into_shapes
{
  translate_and_transform_into_shapes (Shapes *shapes, GenericRepository &rep, ArrayRepository &array_rep) 
    : mp_shapes (shapes), m_shape_rep (rep), m_array_rep (array_rep)
  {
    //  .. nothing yet ..
  }

private:
  template <class Sh, class T, class PropIdMap>
  void op (const Sh &sh, const T &trans, PropIdMap & /*pm*/)
  {
    Sh new_shape;
    new_shape.translate (sh, trans, m_shape_rep, m_array_rep);
    mp_shapes->insert (new_shape);
  }

  template <class Sh, class T, class PropIdMap>
  void op (const geo::object_with_properties<Sh> &sh, const T &trans, PropIdMap &pm)
  {
    Sh new_shape;
    new_shape.translate (sh, trans, m_shape_rep, m_array_rep);
    mp_shapes->insert (geo::object_with_properties<Sh> (new_shape, pm (sh.properties_id ())));
  }

  //  special transformation for boxes - this must consider the non-ortho case where
  //  a box becomes a non-box.
  template <class C, class R, class T, class PropIdMap>
  void op (const box<C, R> &sh, const T &trans, PropIdMap & /*pm*/)
  {
    if (trans.is_ortho ()) {
      geo::box<C> box (sh); // avoid problems with short boxes
      mp_shapes->insert (box.transformed (trans));
    } else {
      geo::polygon<C> poly = geo::polygon<C> (geo::box<C> (sh));
      mp_shapes->insert (poly.transformed (trans));
    }
  }

  //  special transformation for boxes - this must consider the non-ortho case where
  //  a box becomes a non-box.
  template <class C, class R, class T, class PropIdMap>
  void op (const geo::object_with_properties<box<C, R> > &sh, const T &trans, PropIdMap &pm)
  {
    if (trans.is_ortho ()) {
      geo::box<C> box (sh); // avoid problems with short boxes
      mp_shapes->insert (geo::object_with_properties<geo::box<C> > (box.transformed (trans), pm (sh.properties_id ())));
    } else {
      geo::polygon<C> poly = geo::polygon<C> (geo::box<C> (sh));
      mp_shapes->insert (geo::object_with_properties<geo::polygon<C> > (poly.transformed (trans), pm (sh.properties_id ())));
    }
  }

  //  special transformation for boxes - this must consider the non-ortho case where
  //  a box becomes a non-box.
  template <class C, class R, class ATrans, class T, class PropIdMap>
  void op (const geo::array<geo::box<C, R>, ATrans> &sh, const T &trans, PropIdMap & /*pm*/)
  {
    if (trans.is_ortho ()) {
      geo::array<geo::box<C, R>, ATrans> new_array;
      new_array.translate (sh, trans, m_shape_rep, m_array_rep);
      mp_shapes->insert (new_array);
    } else {
      //  Convert the box to a polygon reference and translate ..
      geo::polygon_ref<geo::polygon<C>, geo::unit_trans<C> > poly_ref (geo::polygon<C> (geo::box<C> (sh.object ())), m_shape_rep);
      const geo::basic_array<C> *ba = dynamic_cast <const geo::basic_array<C> *> (sh.delegate ());
      geo::array<geo::polygon_ref<geo::polygon<C>, geo::unit_trans<C> >, geo::disp_trans<C> > poly_array (poly_ref, geo::disp_trans<C> (sh.front ()), ba ? ba->clone () : 0);
      geo::array<geo::polygon_ref<geo::polygon<C>, geo::unit_trans<C> >, geo::disp_trans<C> > new_array;
      new_array.translate (poly_array, trans, m_shape_rep, m_array_rep);
      mp_shapes->insert (new_array);
    }
  }

  //  special transformation for boxes - this must consider the non-ortho case where
  //  a box becomes a non-box.
  template <class C, class R, class ATrans, class T, class PropIdMap>
  void op (const geo::object_with_properties<geo::array<geo::box<C, R>, ATrans> > &sh, const T &trans, PropIdMap &pm)
  {
    if (trans.is_ortho ()) {
      geo::array<geo::box<C, R>, ATrans> new_array;
      new_array.translate (sh, trans, m_shape_rep, m_array_rep);
      mp_shapes->insert (geo::object_with_properties<geo::array<geo::box<C, R>, ATrans> > (new_array, pm (sh.properties_id ())));
    } else {
      //  Convert the box to a polygon reference and translate ..
      geo::polygon_ref<geo::polygon<C>, geo::unit_trans<C> > poly_ref (geo::polygon<C> (geo::box<C> (sh.object ())), m_shape_rep);
      const geo::basic_array<C> *ba = dynamic_cast <const geo::basic_array<C> *> (sh.delegate ());
      geo::array<geo::polygon_ref<geo::polygon<C>, geo::unit_trans<C> >, geo::disp_trans<C> > poly_array (poly_ref, geo::disp_trans<C> (sh.front ()), ba ? ba->clone () : 0);
      geo::array<geo::polygon_ref<geo::polygon<C>, geo::unit_trans<C> >, geo::disp_trans<C> > new_array;
      new_array.translate (poly_array, trans, m_shape_rep, m_array_rep);
      mp_shapes->insert (geo::object_with_properties<geo::array<geo::polygon_ref<geo::polygon<C>, geo::unit_trans<C> >, geo::disp_trans<C> > > (new_array, pm (sh.properties_id ())));
    }
  }

public:
  template <class Sh, class T, class PropIdMap>
  void operator() (const Sh &shape, const T &trans, PropIdMap &pm)
  {
    op (shape, trans, pm);
  }

  template <class Sh, class T>
  void operator() (const Sh &shape, const T &trans)
  {
    geo::ident_map <geo::properties_id_type> pm;
    op (shape, trans, pm);
  }

private:
  geo::Shapes *mp_shapes;
  GenericRepository &m_shape_rep;
  ArrayRepository &m_array_rep;
};

// -------------------------------------------------------------------------------
//  LayerBase implementations

/// @brief Internal: ShapeIterator masks per shape type
inline unsigned int iterator_type_mask (ShapeIterator::polygon_type::tag)
{
  return 1 << ShapeIterator::Polygon;
}

/// @brief Internal: ShapeIterator masks per shape type
inline unsigned int iterator_type_mask (ShapeIterator::polygon_ref_type::tag)
{
  return 1 << ShapeIterator::PolygonRef;
}

/// @brief Internal: ShapeIterator masks per shape type
inline unsigned int iterator_type_mask (ShapeIterator::polygon_ptr_array_type::tag)
{
  return 1 << ShapeIterator::PolygonPtrArray;
}

/// @brief Internal: ShapeIterator masks per shape type
inline unsigned int iterator_type_mask (ShapeIterator::simple_polygon_type::tag)
{
  return 1 << ShapeIterator::SimplePolygon;
}

/// @brief Internal: ShapeIterator masks per shape type
inline unsigned int iterator_type_mask (ShapeIterator::simple_polygon_ref_type::tag)
{
  return 1 << ShapeIterator::SimplePolygonRef;
}

/// @brief Internal: ShapeIterator masks per shape type
inline unsigned int iterator_type_mask (ShapeIterator::simple_polygon_ptr_array_type::tag)
{
  return 1 << ShapeIterator::SimplePolygonPtrArray;
}

/// @brief Internal: ShapeIterator masks per shape type
inline unsigned int iterator_type_mask (ShapeIterator::edge_type::tag)
{
  return 1 << ShapeIterator::Edge;
}

/// @brief Internal: ShapeIterator masks per shape type
inline unsigned int iterator_type_mask (ShapeIterator::edge_pair_type::tag)
{
  return 1 << ShapeIterator::EdgePair;
}

/// @brief Internal: ShapeIterator masks per shape type
inline unsigned int iterator_type_mask (ShapeIterator::path_type::tag)
{
  return 1 << ShapeIterator::Path;
}

/// @brief Internal: ShapeIterator masks per shape type
inline unsigned int iterator_type_mask (ShapeIterator::path_ref_type::tag)
{
  return 1 << ShapeIterator::PathRef;
}

/// @brief Internal: ShapeIterator masks per shape type
inline unsigned int iterator_type_mask (ShapeIterator::path_ptr_array_type::tag)
{
  return 1 << ShapeIterator::PathPtrArray;
}

/// @brief Internal: ShapeIterator masks per shape type
inline unsigned int iterator_type_mask (ShapeIterator::text_type::tag)
{
  return 1 << ShapeIterator::Text;
}

/// @brief Internal: ShapeIterator masks per shape type
inline unsigned int iterator_type_mask (ShapeIterator::text_ref_type::tag)
{
  return 1 << ShapeIterator::TextRef;
}

/// @brief Internal: ShapeIterator masks per shape type
inline unsigned int iterator_type_mask (ShapeIterator::text_ptr_array_type::tag)
{
  return 1 << ShapeIterator::TextPtrArray;
}

/// @brief Internal: ShapeIterator masks per shape type
inline unsigned int iterator_type_mask (ShapeIterator::box_type::tag)
{
  return 1 << ShapeIterator::Box;
}

/// @brief Internal: ShapeIterator masks per shape type
inline unsigned int iterator_type_mask (ShapeIterator::box_array_type::tag)
{
  return 1 << ShapeIterator::BoxArray;
}

/// @brief Internal: ShapeIterator masks per shape type
inline unsigned int iterator_type_mask (ShapeIterator::short_box_type::tag)
{
  return 1 << ShapeIterator::ShortBox;
}

/// @brief Internal: ShapeIterator masks per shape type
inline unsigned int iterator_type_mask (ShapeIterator::short_box_array_type::tag)
{
  return 1 << ShapeIterator::ShortBoxArray;
}

/// @brief Internal: ShapeIterator masks per shape type
inline unsigned int iterator_type_mask (ShapeIterator::user_object_type::tag)
{
  return 1 << ShapeIterator::UserObject;
}

/// @brief Internal: ShapeIterator masks per shape type
template <class Sh>
inline unsigned int iterator_type_mask (geo::object_tag< geo::object_with_properties<Sh> >)
{
  return iterator_type_mask (typename Sh::tag ());
}

template <class Sh, class StableTag>
LayerBase *
layer_class<Sh, StableTag>::clone () const
{
  layer_class<Sh, StableTag> *r = new layer_class<Sh, StableTag> ();
  r->m_layer = m_layer;
  return r;
}
  
template <class Sh, class StableTag>
void 
layer_class<Sh, StableTag>::translate_into (Shapes *target, GenericRepository &rep, ArrayRepository &array_rep) const 
{
  translate_into_shapes op (target, rep, array_rep);
  for (typename layer_type::iterator s = m_layer.begin (); s != m_layer.end (); ++s) {
    op (*s);
  }
}
  
template <class Sh, class StableTag>
void 
layer_class<Sh, StableTag>::translate_into (Shapes *target, GenericRepository &rep, ArrayRepository &array_rep, pm_delegate_type &pm) const 
{
  translate_into_shapes op (target, rep, array_rep);
  for (typename layer_type::iterator s = m_layer.begin (); s != m_layer.end (); ++s) {
    op (*s, pm);
  }
}
  
template <class Sh, class StableTag>
void 
layer_class<Sh, StableTag>::transform_into (Shapes *target, const Trans &trans, GenericRepository &rep, ArrayRepository &array_rep) const 
{
  translate_and_transform_into_shapes op (target, rep, array_rep);
  for (typename layer_type::iterator s = m_layer.begin (); s != m_layer.end (); ++s) {
    op (*s, trans);
  }
}

template <class Sh, class StableTag>
void 
layer_class<Sh, StableTag>::transform_into (Shapes *target, const Trans &trans, GenericRepository &rep, ArrayRepository &array_rep, pm_delegate_type &pm) const 
{
  translate_and_transform_into_shapes op (target, rep, array_rep);
  for (typename layer_type::iterator s = m_layer.begin (); s != m_layer.end (); ++s) {
    op (*s, trans, pm);
  }
}

template <class Sh, class StableTag>
void 
layer_class<Sh, StableTag>::transform_into (Shapes *target, const ICplxTrans &trans, GenericRepository &rep, ArrayRepository &array_rep) const 
{
  translate_and_transform_into_shapes op (target, rep, array_rep);
  for (typename layer_type::iterator s = m_layer.begin (); s != m_layer.end (); ++s) {
    op (*s, trans);
  }
}

template <class Sh, class StableTag>
void 
layer_class<Sh, StableTag>::transform_into (Shapes *target, const ICplxTrans &trans, GenericRepository &rep, ArrayRepository &array_rep, pm_delegate_type &pm) const 
{
  translate_and_transform_into_shapes op (target, rep, array_rep);
  for (typename layer_type::iterator s = m_layer.begin (); s != m_layer.end (); ++s) {
    op (*s, trans, pm);
  }
}

template <class Sh, class StableTag>
void
layer_class<Sh, StableTag>::insert_into (Shapes *target)
{
  target->insert (m_layer.begin (), m_layer.end ());
}

template <class Sh, class StableTag>
void 
layer_class<Sh, StableTag>::deref_into (Shapes *target) 
{
  deref_into_shapes op (target);
  for (typename layer_type::iterator s = m_layer.begin (); s != m_layer.end (); ++s) {
    op (*s);
  }
}

template <class Sh, class StableTag>
void 
layer_class<Sh, StableTag>::deref_into (Shapes *target, pm_delegate_type &pm) 
{
  deref_into_shapes op (target);
  for (typename layer_type::iterator s = m_layer.begin (); s != m_layer.end (); ++s) {
    op (*s, pm);
  }
}

template <class Sh, class StableTag>
void 
layer_class<Sh, StableTag>::deref_and_transform_into (Shapes *target, const Trans &trans) 
{
  deref_and_transform_into_shapes deref_op (target);
  for (typename layer_type::iterator s = m_layer.begin (); s != m_layer.end (); ++s) {
    deref_op (*s, trans);
  }
}

template <class Sh, class StableTag>
void 
layer_class<Sh, StableTag>::deref_and_transform_into (Shapes *target, const Trans &trans, pm_delegate_type &pm) 
{
  deref_and_transform_into_shapes deref_op (target);
  for (typename layer_type::iterator s = m_layer.begin (); s != m_layer.end (); ++s) {
    deref_op (*s, trans, pm);
  }
}

template <class Sh, class StableTag>
void 
layer_class<Sh, StableTag>::deref_and_transform_into (Shapes *target, const ICplxTrans &trans) 
{
  deref_and_transform_into_shapes op (target);
  for (typename layer_type::iterator s = m_layer.begin (); s != m_layer.end (); ++s) {
    op (*s, trans);
  }
}

template <class Sh, class StableTag>
void 
layer_class<Sh, StableTag>::deref_and_transform_into (Shapes *target, const ICplxTrans &trans, pm_delegate_type &pm) 
{
  deref_and_transform_into_shapes op (target);
  for (typename layer_type::iterator s = m_layer.begin (); s != m_layer.end (); ++s) {
    op (*s, trans, pm);
  }
}

template <class Sh, class StableTag>
unsigned int 
layer_class<Sh, StableTag>::type_mask () const 
{
  return iterator_type_mask (typename Sh::tag ());
}

//  explicit instantiations

template class layer_class<geo::Shape::polygon_type, geo::stable_layer_tag>;
template class layer_class<geo::object_with_properties<geo::Shape::polygon_type>, geo::stable_layer_tag>;
template class layer_class<geo::Shape::simple_polygon_type, geo::stable_layer_tag>;
template class layer_class<geo::object_with_properties<geo::Shape::simple_polygon_type>, geo::stable_layer_tag>;
template class layer_class<geo::Shape::polygon_ref_type, geo::stable_layer_tag>;
template class layer_class<geo::object_with_properties<geo::Shape::polygon_ref_type>, geo::stable_layer_tag>;
template class layer_class<geo::Shape::simple_polygon_ref_type, geo::stable_layer_tag>;
template class layer_class<geo::object_with_properties<geo::Shape::simple_polygon_ref_type>, geo::stable_layer_tag>;
template class layer_class<geo::Shape::polygon_ptr_array_type, geo::stable_layer_tag>;
template class layer_class<geo::object_with_properties<geo::Shape::polygon_ptr_array_type>, geo::stable_layer_tag>;
template class layer_class<geo::Shape::simple_polygon_ptr_array_type, geo::stable_layer_tag>;
template class layer_class<geo::object_with_properties<geo::Shape::simple_polygon_ptr_array_type>, geo::stable_layer_tag>;
template class layer_class<geo::Shape::path_type, geo::stable_layer_tag>;
template class layer_class<geo::object_with_properties<geo::Shape::path_type>, geo::stable_layer_tag>;
template class layer_class<geo::Shape::path_ref_type, geo::stable_layer_tag>;
template class layer_class<geo::object_with_properties<geo::Shape::path_ref_type>, geo::stable_layer_tag>;
template class layer_class<geo::Shape::path_ptr_array_type, geo::stable_layer_tag>;
template class layer_class<geo::object_with_properties<geo::Shape::path_ptr_array_type>, geo::stable_layer_tag>;
template class layer_class<geo::Shape::edge_type, geo::stable_layer_tag>;
template class layer_class<geo::object_with_properties<geo::Shape::edge_type>, geo::stable_layer_tag>;
template class layer_class<geo::Shape::edge_pair_type, geo::stable_layer_tag>;
template class layer_class<geo::object_with_properties<geo::Shape::edge_pair_type>, geo::stable_layer_tag>;
template class layer_class<geo::Shape::text_type, geo::stable_layer_tag>;
template class layer_class<geo::object_with_properties<geo::Shape::text_type>, geo::stable_layer_tag>;
template class layer_class<geo::Shape::text_ref_type, geo::stable_layer_tag>;
template class layer_class<geo::object_with_properties<geo::Shape::text_ref_type>, geo::stable_layer_tag>;
template class layer_class<geo::Shape::text_ptr_array_type, geo::stable_layer_tag>;
template class layer_class<geo::object_with_properties<geo::Shape::text_ptr_array_type>, geo::stable_layer_tag>;
template class layer_class<geo::Shape::box_type, geo::stable_layer_tag>;
template class layer_class<geo::object_with_properties<geo::Shape::box_type>, geo::stable_layer_tag>;
template class layer_class<geo::Shape::box_array_type, geo::stable_layer_tag>;
template class layer_class<geo::object_with_properties<geo::Shape::box_array_type>, geo::stable_layer_tag>;
template class layer_class<geo::Shape::short_box_type, geo::stable_layer_tag>;
template class layer_class<geo::object_with_properties<geo::Shape::short_box_type>, geo::stable_layer_tag>;
template class layer_class<geo::Shape::short_box_array_type, geo::stable_layer_tag>;
template class layer_class<geo::object_with_properties<geo::Shape::short_box_array_type>, geo::stable_layer_tag>;
template class layer_class<geo::Shape::user_object_type, geo::stable_layer_tag>;
template class layer_class<geo::object_with_properties<geo::Shape::user_object_type>, geo::stable_layer_tag>;
template class layer_class<geo::Shape::polygon_type, geo::unstable_layer_tag>;
template class layer_class<geo::object_with_properties<geo::Shape::polygon_type>, geo::unstable_layer_tag>;
template class layer_class<geo::Shape::simple_polygon_type, geo::unstable_layer_tag>;
template class layer_class<geo::object_with_properties<geo::Shape::simple_polygon_type>, geo::unstable_layer_tag>;
template class layer_class<geo::Shape::polygon_ref_type, geo::unstable_layer_tag>;
template class layer_class<geo::object_with_properties<geo::Shape::polygon_ref_type>, geo::unstable_layer_tag>;
template class layer_class<geo::Shape::simple_polygon_ref_type, geo::unstable_layer_tag>;
template class layer_class<geo::object_with_properties<geo::Shape::simple_polygon_ref_type>, geo::unstable_layer_tag>;
template class layer_class<geo::Shape::polygon_ptr_array_type, geo::unstable_layer_tag>;
template class layer_class<geo::object_with_properties<geo::Shape::polygon_ptr_array_type>, geo::unstable_layer_tag>;
template class layer_class<geo::Shape::simple_polygon_ptr_array_type, geo::unstable_layer_tag>;
template class layer_class<geo::object_with_properties<geo::Shape::simple_polygon_ptr_array_type>, geo::unstable_layer_tag>;
template class layer_class<geo::Shape::path_type, geo::unstable_layer_tag>;
template class layer_class<geo::object_with_properties<geo::Shape::path_type>, geo::unstable_layer_tag>;
template class layer_class<geo::Shape::path_ref_type, geo::unstable_layer_tag>;
template class layer_class<geo::object_with_properties<geo::Shape::path_ref_type>, geo::unstable_layer_tag>;
template class layer_class<geo::Shape::path_ptr_array_type, geo::unstable_layer_tag>;
template class layer_class<geo::object_with_properties<geo::Shape::path_ptr_array_type>, geo::unstable_layer_tag>;
template class layer_class<geo::Shape::edge_type, geo::unstable_layer_tag>;
template class layer_class<geo::object_with_properties<geo::Shape::edge_type>, geo::unstable_layer_tag>;
template class layer_class<geo::Shape::edge_pair_type, geo::unstable_layer_tag>;
template class layer_class<geo::object_with_properties<geo::Shape::edge_pair_type>, geo::unstable_layer_tag>;
template class layer_class<geo::Shape::text_type, geo::unstable_layer_tag>;
template class layer_class<geo::object_with_properties<geo::Shape::text_type>, geo::unstable_layer_tag>;
template class layer_class<geo::Shape::text_ref_type, geo::unstable_layer_tag>;
template class layer_class<geo::object_with_properties<geo::Shape::text_ref_type>, geo::unstable_layer_tag>;
template class layer_class<geo::Shape::text_ptr_array_type, geo::unstable_layer_tag>;
template class layer_class<geo::object_with_properties<geo::Shape::text_ptr_array_type>, geo::unstable_layer_tag>;
template class layer_class<geo::Shape::box_type, geo::unstable_layer_tag>;
template class layer_class<geo::object_with_properties<geo::Shape::box_type>, geo::unstable_layer_tag>;
template class layer_class<geo::Shape::box_array_type, geo::unstable_layer_tag>;
template class layer_class<geo::object_with_properties<geo::Shape::box_array_type>, geo::unstable_layer_tag>;
template class layer_class<geo::Shape::short_box_type, geo::unstable_layer_tag>;
template class layer_class<geo::object_with_properties<geo::Shape::short_box_type>, geo::unstable_layer_tag>;
template class layer_class<geo::Shape::short_box_array_type, geo::unstable_layer_tag>;
template class layer_class<geo::object_with_properties<geo::Shape::short_box_array_type>, geo::unstable_layer_tag>;
template class layer_class<geo::Shape::user_object_type, geo::unstable_layer_tag>;
template class layer_class<geo::object_with_properties<geo::Shape::user_object_type>, geo::unstable_layer_tag>;

}

