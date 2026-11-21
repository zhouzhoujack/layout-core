#include "geoClip.h"
#include "geoLayout.h"
#include "geoPolygonGenerators.h"

namespace geo
{

// ------------------------------------------------------------------------------
//  clip_poly implementation

struct EdgeCompareP1Func 
{
  bool operator () (const geo::Edge &e1, const geo::Edge &e2) const
  {
    return e1.p1 () < e2.p1 ();
  }
};

struct CoordSignPairCompareFunc
{
  CoordSignPairCompareFunc (int s)
    : m_s (s)
  { }

  bool operator() (const std::pair <geo::Coord, int> &a, const std::pair <geo::Coord, int> &b) const
  {
    if (a.first != b.first) {
      return a.first < b.first;
    } 
    return m_s > 0 ? (a.second < b.second) : (a.second > b.second);
  }

private:
  int m_s;
};

//  TODO: check, if the clip can be implemented by subsequent "cut" operations using all four borders
//  Is that more efficient?
template <class P, class PC> static void 
clip_poly (const P &poly, const geo::Box &box, std::vector <P> &clipped_poly, bool resolve_holes)
{
  geo::Box pbox = poly.box ();

  //  Polygon completely inside the clip box -> return the polygon
  if (pbox.inside (box)) {
    clipped_poly.push_back (poly);
    return;
  }

  //  Polygon completely outside the clip box -> return nothing.
  if (! pbox.overlaps (box)) {
    return;
  }

  //  first, extract and sort all edges
  std::vector <geo::Edge> edges;
  edges.reserve (poly.hull ().size ());

  //  create a set of edges to consider
  for (typename P::polygon_edge_iterator e = poly.begin_edge (); ! e.at_end (); ++e) {

    geo::Edge edge = *e;

    geo::Coord y1 = std::min (edge.p1 ().y (), edge.p2 ().y ());
    geo::Coord y2 = std::max (edge.p1 ().y (), edge.p2 ().y ());

    if (y1 < box.p2 ().y () && y2 > box.p1 ().y ()) {

      std::pair <bool, geo::Edge> ce = edge.clipped (box);
      if (ce.first) {

        edges.push_back (ce.second);

        geo::Edge e1 (geo::Point (ce.second.p1 ().x (), edge.p1 ().y ()), ce.second.p1 ());
        geo::Edge e2 (ce.second.p2 (), geo::Point (ce.second.p2 ().x (), edge.p2 ().y ()));

        if (! e1.is_degenerate () && (ce = e1.clipped (box)).first && ! ce.second.is_degenerate ()) {
          edges.push_back (ce.second);
        }
        if (! e2.is_degenerate () && (ce = e2.clipped (box)).first && ! ce.second.is_degenerate ()) {
          edges.push_back (ce.second);
        }

      } else {

        //  determine whether the edge is "to the left" of the box
        bool left = false;
        if (edge.p1 ().y () <= box.top () && edge.p1 ().y () >= box.bottom ()) {
          left = (edge.p1 ().x () < box.left ());
        } else if (edge.p2 ().y () <= box.top () && edge.p2 ().y () >= box.bottom ()) {
          left = (edge.p2 ().x () < box.left ());
        } else {
          tl_assert ((edge.p1 ().y () < box.bottom () && edge.p2 ().y () > box.top ()) || (edge.p2 ().y () < box.bottom () && edge.p1 ().y () > box.top ()));
          double cx = double (edge.p1 ().x ()) + (double (box.center ().y ()) - double (edge.p1 ().y ())) * double (edge.dx ()) / double (edge.dy ());
          left = (cx < box.center ().x ());
        }

        //  compute a projection on the box'es walls
        if (left) {
          edge = geo::Edge (geo::Point (box.p1 ().x (), edge.p1 ().y ()), geo::Point (box.p1 ().x (), edge.p2 ().y ()));
        } else {
          edge = geo::Edge (geo::Point (box.p2 ().x (), edge.p1 ().y ()), geo::Point (box.p2 ().x (), edge.p2 ().y ()));
        }

        ce = edge.clipped (box);
        if (ce.first) {
          edges.push_back (ce.second);
        }

      }

    }

  }

  //  synthesize horizontal edges at upper and lower boundary of the clip rectangle
  std::vector <std::pair <geo::Coord, int> > coord_values;
  std::vector <geo::Point> p1_stack;
  std::vector <geo::Point> p2_stack;

  for (unsigned int l = 0; l <= 1; ++l) {

    geo::Coord y = (l > 0 ? box.p2 ().y () : box.p1 ().y ());

    coord_values.clear ();
    p1_stack.clear ();
    p2_stack.clear ();

    for (std::vector <geo::Edge>::const_iterator e = edges.begin (); e != edges.end (); ++e) {
      if (e->p1 ().y () == y) {
        coord_values.push_back (std::make_pair (e->p1 ().x (), -1));
      }
      if (e->p2 ().y () == y) {
        coord_values.push_back (std::make_pair (e->p2 ().x (), 1));
      }
    }

    std::sort (coord_values.begin (), coord_values.end (), CoordSignPairCompareFunc (l > 0 ? 1 : -1));

    for (std::vector <std::pair <geo::Coord, int> >::const_iterator xv = coord_values.begin (); xv != coord_values.end (); ++xv) {
      geo::Point p = geo::Point (xv->first, y);
      if (xv->second < 0) {
        if (! p1_stack.empty ()) {
          if (p != p1_stack.back () && p != p1_stack.back ()) {
            edges.push_back (geo::Edge (p1_stack.back (), p));
          }
          p1_stack.pop_back ();
        } else {
          p2_stack.push_back (p);
        }
      } else {
        if (! p2_stack.empty ()) {
          if (p != p2_stack.back () && p != p2_stack.back ()) {
            edges.push_back (geo::Edge (p, p2_stack.back ()));
          }
          p2_stack.pop_back ();
        } else {
          p1_stack.push_back (p);
        }
      }
    }

    tl_assert (p1_stack.empty ());
    tl_assert (p2_stack.empty ());
    coord_values.clear ();

  }

  //  remove all edges being vertical and coincident with the clip box ..
  std::vector <geo::Edge>::iterator e_write = edges.begin ();
  for (std::vector <geo::Edge>::const_iterator e = edges.begin (); e != edges.end (); ++e) {
    if (e->dx () != 0 || (e->p1 ().x () > box.p1 ().x () && e->p1 ().x () < box.p2 ().x ())) {
      *e_write = *e;
      ++e_write;
    }
  }
  edges.erase (e_write, edges.end ());

  //  and synthesize them again thus removing coincident edges ..
  
  for (unsigned int l = 0; l <= 1; ++l) {

    geo::Coord x = (l > 0 ? box.p2 ().x () : box.p1 ().x ());

    coord_values.clear ();
    p1_stack.clear ();
    p2_stack.clear ();

    for (std::vector <geo::Edge>::const_iterator e = edges.begin (); e != edges.end (); ++e) {
      if (e->p1 ().x () == x) {
        coord_values.push_back (std::make_pair (e->p1 ().y (), -1));
      }
      if (e->p2 ().x () == x) {
        coord_values.push_back (std::make_pair (e->p2 ().y (), 1));
      }
    }

    std::sort (coord_values.begin (), coord_values.end (), CoordSignPairCompareFunc (l > 0 ? -1 : 1));

    for (std::vector <std::pair <geo::Coord, int> >::const_iterator xv = coord_values.begin (); xv != coord_values.end (); ++xv) {
      geo::Point p = geo::Point (x, xv->first);
      if (xv->second < 0) {
        if (! p1_stack.empty ()) {
          if (p != p1_stack.back () && p != p1_stack.back ()) {
            edges.push_back (geo::Edge (p1_stack.back (), p));
          }
          p1_stack.pop_back ();
        } else {
          p2_stack.push_back (p);
        }
      } else {
        if (! p2_stack.empty ()) {
          if (p != p2_stack.back () && p != p2_stack.back ()) {
            edges.push_back (geo::Edge (p, p2_stack.back ()));
          }
          p2_stack.pop_back ();
        } else {
          p1_stack.push_back (p);
        }
      }
    }

    tl_assert (p1_stack.empty ());
    tl_assert (p2_stack.empty ());
    coord_values.clear ();

  }

  //  Use the edge processor to merge and create the output polygons.
  //  This is slow, but there is no good alternative for producing the holes and some situations are not well caught by the 
  //  previous algorithm.
  //  Anyway it is faster that a pure AND.

  geo::EdgeProcessor ep;
  ep.reserve (edges.size ());
  ep.insert_sequence (edges.begin (), edges.end ());
  edges.clear ();

  PC poly_cont (clipped_poly);

  geo::PolygonGenerator poly_gen (poly_cont);
  poly_gen.min_coherence (false);
  poly_gen.resolve_holes (resolve_holes);

  SimpleMerge sm; 
  ep.process (poly_gen, sm);

}

void 
clip_poly (const geo::Polygon &poly, const geo::Box &box, std::vector <geo::Polygon> &clipped_poly, bool resolve_holes)
{
  clip_poly<geo::Polygon, geo::PolygonContainer> (poly, box, clipped_poly, resolve_holes);
}

void 
clip_poly (const geo::SimplePolygon &poly, const geo::Box &box, std::vector <geo::SimplePolygon> &clipped_poly, bool resolve_holes)
{
  clip_poly<geo::SimplePolygon, geo::SimplePolygonContainer> (poly, box, clipped_poly, resolve_holes);
}

// ------------------------------------------------------------------------------
//  helper method: clip a cell

static void 
clip_cell (const geo::Layout &layout, 
           geo::cell_index_type cell_index, 
           geo::Layout &target_layout, 
           geo::cell_index_type target_cell_index, 
           const geo::Box &clip_box,
           std::map <std::pair <geo::cell_index_type, geo::Box>, geo::cell_index_type> &variants)
{
  const geo::Cell &cell = layout.cell (cell_index);
  geo::Cell &target_cell = target_layout.cell (target_cell_index);

  if (cell.bbox ().inside (clip_box)) {

    if (&target_layout != &layout || cell_index != target_cell_index) {

      //  simplification of shape copy op in case of no clipping ..
      for (unsigned int l = 0; l < layout.layers (); ++l) {
        if (layout.is_valid_layer (l)) {
          target_cell.shapes (l) = cell.shapes (l);
        }
      }

      for (geo::Cell::const_iterator inst = cell.begin (); ! inst.at_end (); ++inst) {

        //  instance is completely inside: nevertheless we must look up the target cell for the different layout case.
        geo::CellInstArray new_inst = inst->cell_inst ();

        const geo::Cell &inst_cell = layout.cell (inst->cell_index ());
        if (! inst_cell.bbox ().empty ()) {

          std::map <std::pair <geo::cell_index_type, geo::Box>, geo::cell_index_type>::const_iterator vmp = variants.find (std::make_pair (inst->cell_index (), inst_cell.bbox ()));
          tl_assert (vmp != variants.end ());

          new_inst.object () = geo::CellInst (vmp->second);

          //  TODO: keep properties 
          target_cell.insert (new_inst);

        }

      }

    }

  } else {

    tl_assert (&layout != &target_layout || target_cell_index != cell_index);

    for (unsigned int l = 0; l < layout.layers (); ++l) {
      
      if (layout.is_valid_layer (l)) {

        for (geo::ShapeIterator sh = cell.shapes (l).begin_touching (clip_box, geo::ShapeIterator::All); ! sh.at_end (); ++sh) {

          if (sh->is_box ()) {

            geo::Box new_box = sh->box () & clip_box;
            if (! new_box.empty () && new_box.width () > 0 && new_box.height () > 0) {
              if (sh->has_prop_id ()) {
                target_cell.shapes (l).insert (geo::BoxWithProperties (new_box, sh->prop_id ()));
              } else {
                target_cell.shapes (l).insert (new_box);
              }
            }

          } else if (sh->is_path () && sh->bbox ().inside (clip_box)) {

            geo::Path path;
            sh->path (path);

            if (sh->has_prop_id ()) {
              target_cell.shapes (l).insert (geo::PathRefWithProperties (geo::PathRef (path, target_layout.shape_repository ()), sh->prop_id ()));
            } else {
              target_cell.shapes (l).insert (geo::PathRef (path, target_layout.shape_repository ()));
            }

          } else if (sh->is_polygon () || sh->is_simple_polygon () || sh->is_path ()) {

            geo::SimplePolygon poly;

            if (sh->is_path ()) {
              geo::Path path;
              sh->path (path);
              poly = path.simple_polygon ();
            } else if (sh->is_polygon ()) {
              geo::Polygon ppoly;
              sh->polygon (ppoly);
              poly = geo::SimplePolygon (ppoly);
            } else {
              sh->simple_polygon (poly);
            }

            std::vector <geo::SimplePolygon> clipped_polygons;

            if (! poly.box ().inside (clip_box)) {
              clip_poly (poly, clip_box, clipped_polygons);
              for (std::vector <geo::SimplePolygon>::const_iterator cp = clipped_polygons.begin (); cp != clipped_polygons.end (); ++cp) {
                if (sh->has_prop_id ()) {
                  target_cell.shapes (l).insert (geo::SimplePolygonRefWithProperties (geo::SimplePolygonRef (*cp, target_layout.shape_repository ()), sh->prop_id ()));
                } else {
                  target_cell.shapes (l).insert (geo::SimplePolygonRef (*cp, target_layout.shape_repository ()));
                }
              }
            } else {
              if (sh->has_prop_id ()) {
                target_cell.shapes (l).insert (geo::SimplePolygonRefWithProperties (geo::SimplePolygonRef (poly, target_layout.shape_repository ()), sh->prop_id ()));
              } else {
                target_cell.shapes (l).insert (geo::SimplePolygonRef (poly, target_layout.shape_repository ()));
              }
            }

          } else if (sh->is_text ()) {

            if (sh->bbox ().inside (clip_box)) {
              geo::Text text;
              sh->text (text);
              if (sh->has_prop_id ()) {
                target_cell.shapes (l).insert (geo::TextRefWithProperties (geo::TextRef (text, target_layout.shape_repository ()), sh->prop_id ()));
              } else {
                target_cell.shapes (l).insert (geo::TextRef (text, target_layout.shape_repository ()));
              }
            }

          } else {
            tl_assert (false); // invalid shape type encountered
          }

        }

      }

    }

    geo::box_convert <geo::CellInst> bc (layout);

    for (geo::Cell::touching_iterator inst = cell.begin_touching (clip_box); ! inst.at_end (); ++inst) {

      if (inst->cell_inst ().bbox (bc).inside (clip_box)) {

        //  instance is completely inside
        geo::CellInstArray new_inst = inst->cell_inst ();

        const geo::Cell &inst_cell = layout.cell (inst->cell_index ());

        std::map <std::pair <geo::cell_index_type, geo::Box>, geo::cell_index_type>::const_iterator vmp = variants.find (std::make_pair (inst->cell_index (), inst_cell.bbox ()));
        tl_assert (vmp != variants.end ());

        new_inst.object () = geo::CellInst (vmp->second);

        //  TODO: keep properties 
        target_cell.insert (new_inst);

      } else {

        for (geo::CellInstArray::iterator a = inst->cell_inst ().begin_touching (clip_box, bc); ! a.at_end (); ++a) {

          geo::Box inst_clip_box = geo::Box (clip_box.transformed (inst->cell_inst ().complex_trans (*a).inverted ()));
          const geo::Cell &inst_cell = layout.cell (inst->cell_index ());

          inst_clip_box &= inst_cell.bbox ();

          if (! inst_clip_box.empty ()) {

            std::map <std::pair <geo::cell_index_type, geo::Box>, geo::cell_index_type>::const_iterator vmp = variants.find (std::make_pair (inst->cell_index (), inst_clip_box));
            tl_assert (vmp != variants.end ());

            geo::CellInstArray new_inst;

            if (inst->is_complex ()) {
              new_inst = geo::CellInstArray (geo::CellInst (vmp->second), inst->cell_inst ().complex_trans (*a));
            } else {
              new_inst = geo::CellInstArray (geo::CellInst (vmp->second), *a);
            }

            target_cell.insert (new_inst);

          }

        }

      }

    }

  }

}

// ------------------------------------------------------------------------------
//  collect_clip_boxes implementation

static void 
collect_clip_boxes (const geo::Layout &layout, 
                    geo::cell_index_type cell_index, 
                    unsigned int layer, 
                    const geo::CplxTrans &trans, 
                    std::vector <geo::Box> &boxes)
{
  const geo::Cell &cell = layout.cell (cell_index);
  if (! cell.bbox (layer).empty ()) {

    //  any shapes to consider ..
    for (geo::ShapeIterator sh = cell.shapes (layer).begin (geo::ShapeIterator::All); ! sh.at_end (); ++sh) {
      boxes.push_back (geo::Box (sh->bbox ().transformed (trans)));
    }

    for (geo::Cell::const_iterator inst = cell.begin (); ! inst.at_end (); ++inst) {
      for (geo::CellInstArray::iterator a = inst->cell_inst ().begin (); ! a.at_end (); ++a) {
        collect_clip_boxes (layout, inst->cell_index (), layer, trans * inst->cell_inst ().complex_trans (*a), boxes);
      }
    }

  }
}

void 
collect_clip_boxes (const geo::Layout &layout, 
                    geo::cell_index_type cell_index, 
                    unsigned int layer, 
                    std::vector <geo::Box> &boxes)
{
  collect_clip_boxes (layout, cell_index, layer, geo::CplxTrans (), boxes);
}

// ------------------------------------------------------------------------------
//  Helper function for the layout clipper

static void 
collect_clip_variants (const geo::Layout &layout,
                       geo::Layout &target_layout,
                       geo::cell_index_type cell_index,
                       const geo::Box &clip_box,
                       std::map <std::pair <geo::cell_index_type, geo::Box>, geo::cell_index_type> &variants,
                       bool stable)
{
  const geo::Cell &cell = layout.cell (cell_index);
  geo::box_convert <geo::CellInst> bc (layout);

  geo::Box cell_box;
  if (stable) {
    cell_box = clip_box;
  } else {
    cell_box = cell.bbox () & clip_box;
    if (cell_box.empty ()) {
      return;
    }
  }

  std::pair <std::map <std::pair <geo::cell_index_type, geo::Box>, geo::cell_index_type>::iterator, bool> vmp = variants.insert (std::make_pair (std::make_pair (cell_index, cell_box), 0));
  if (vmp.second) {

    for (geo::Cell::touching_iterator inst = cell.begin_touching (cell_box); ! inst.at_end (); ++inst) {
      for (geo::CellInstArray::iterator a = inst->cell_inst ().begin_touching (cell_box, bc); ! a.at_end (); ++a) {
        geo::Box inst_clip_box = geo::Box (cell_box.transformed (inst->cell_inst ().complex_trans (*a).inverted ()));
        collect_clip_variants (layout, target_layout, inst->cell_index (), inst_clip_box, variants, false);
      }
    }

  }
}

static void 
make_clip_variants (const geo::Layout &layout,
                    geo::Layout &target_layout,
                    std::map <std::pair <geo::cell_index_type, geo::Box>, geo::cell_index_type> &variants)
{
  for (std::map <std::pair <geo::cell_index_type, geo::Box>, geo::cell_index_type>::iterator v = variants.begin (); v != variants.end (); ++v) {
    if (v->first.second != layout.cell (v->first.first).bbox () || &layout != &target_layout) {
      //  need for a new cell
      v->second = target_layout.add_cell (layout.cell_name (v->first.first));
    } else {
      v->second = v->first.first;
    }
  }
}

// ------------------------------------------------------------------------------
//  clip_layout implementation

std::vector<geo::cell_index_type> 
clip_layout (const Layout &layout, 
             Layout &target_layout, 
             geo::cell_index_type cell_index, 
             const std::vector <geo::Box> &clip_boxes,
             bool stable)
{
  std::vector<geo::cell_index_type> result;

  //  since we know that we are not changing something on the cells we need as input, 
  //  we can disable updates for the target layout after doing an explicit update. 
  //  Otherwise this would cause recursion when target_layout == layout:
  layout.update();
  target_layout.start_changes ();

  try {

    //  create clip variants of cells
    std::map <std::pair <geo::cell_index_type, geo::Box>, geo::cell_index_type> variants;

    for (std::vector <geo::Box>::const_iterator cbx = clip_boxes.begin (); cbx != clip_boxes.end (); ++cbx) {
      collect_clip_variants (layout, target_layout, cell_index, *cbx, variants, stable);
    }

    make_clip_variants (layout, target_layout, variants);

    //  actually do the clipping by filling the variants
    for (std::map <std::pair <geo::cell_index_type, geo::Box>, geo::cell_index_type>::const_iterator var = variants.begin (); var != variants.end (); ++var) {
      clip_cell (layout, var->first.first, target_layout, var->second, var->first.second, variants);
    }

    //  prepare the result vector ..
    if (! stable) {

      for (std::map <std::pair <geo::cell_index_type, geo::Box>, geo::cell_index_type>::const_iterator var = variants.begin (); var != variants.end (); ++var) {
        if (var->first.first == cell_index) {
          result.push_back (var->second);
        }
      }

    } else {

      //  We have made sure before that there is a top-level entry for each clip box that was input
      for (std::vector <geo::Box>::const_iterator cbx = clip_boxes.begin (); cbx != clip_boxes.end (); ++cbx) {
        std::map <std::pair <geo::cell_index_type, geo::Box>, geo::cell_index_type>::const_iterator var = variants.find (std::make_pair (cell_index, *cbx));
        tl_assert (var != variants.end ());
        result.push_back (var->second);
      }

    }

    //  release the under construction state
    target_layout.end_changes ();

  } catch (...) {
    //  ensure we have a end_changes call corresponding to the start_changes call.
    target_layout.end_changes ();
    throw;
  } 

  return result;

}

} // namespace geo


