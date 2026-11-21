#include "geoShapeProcessor.h"
#include "geoLayout.h"

#include <vector>
#include <deque>
#include <memory>

namespace geo
{

// -------------------------------------------------------------------------------
//  ShapeProcessor implementation

ShapeProcessor::ShapeProcessor (bool report_progress, const std::string &progress_desc)
  : m_processor (report_progress, progress_desc)
{
  // .. nothing yet ..
}

void 
ShapeProcessor::clear ()
{
  m_processor.clear ();
}

void 
ShapeProcessor::reserve (size_t n)
{
  m_processor.reserve (n);
}

void 
ShapeProcessor::process (geo::EdgeSink &es, EdgeEvaluatorBase &op)
{
  m_processor.process (es, op);
}

size_t
ShapeProcessor::count_edges (const geo::Shape &shape) const
{
  size_t n = 0;

  if (shape.is_polygon ()) {

    //  KLUDGE: Shape should have a point_count property (or similar)
    for (geo::Shape::polygon_edge_iterator e = shape.begin_edge (); ! e.at_end (); ++e) {
      ++n;
    }

  } else if (shape.is_path ()) {

    //  KLUDGE: This is rather inefficient ..
    geo::Polygon poly;
    shape.polygon (poly);
    for (geo::Polygon::polygon_edge_iterator e = poly.begin_edge (); ! e.at_end (); ++e) {
      ++n;
    }

  } else if (shape.is_box ()) {
    n += 4;
  }

  return n;
}

void
ShapeProcessor::merge (const std::vector<geo::Shape> &in, const std::vector<geo::CplxTrans> &trans, std::vector <geo::Edge> &out_edges, unsigned int min_wc)
{
  clear ();

  size_t e = 0;
  for (std::vector<geo::Shape>::const_iterator i = in.begin (); i != in.end (); ++i) {
    e += count_edges (*i);
  }
  reserve (e + e / 4); // heuristic reserve for crossing points

  size_t n = 0;
  for (std::vector<geo::Shape>::const_iterator i = in.begin (); i != in.end (); ++i, ++n) {
    if (n < trans.size ()) {
      insert (*i, geo::ICplxTrans (trans [n]), n);
    } else {
      insert (*i, n);
    }
  }

  geo::MergeOp op (min_wc);
  geo::EdgeContainer out (out_edges);
  process (out, op);
}

void
ShapeProcessor::merge (const std::vector<geo::Shape> &in, const std::vector<geo::CplxTrans> &trans, std::vector <geo::Polygon> &out_polygons, unsigned int min_wc, bool resolve_holes, bool min_coherence)
{
  clear ();

  size_t e = 0;
  for (std::vector<geo::Shape>::const_iterator i = in.begin (); i != in.end (); ++i) {
    e += count_edges (*i);
  }
  reserve (e + e / 4); // heuristic reserve for crossing points

  size_t n = 0;
  for (std::vector<geo::Shape>::const_iterator i = in.begin (); i != in.end (); ++i, ++n) {
    if (n < trans.size ()) {
      insert (*i, geo::ICplxTrans (trans [n]), n);
    } else {
      insert (*i, n);
    }
  }

  geo::MergeOp op (min_wc);
  geo::PolygonContainer pc (out_polygons);
  geo::PolygonGenerator out (pc, resolve_holes, min_coherence);
  process (out, op);
}

void
ShapeProcessor::boolean (const std::vector<geo::Shape> &in_a, const std::vector<geo::CplxTrans> &trans_a,
                         const std::vector<geo::Shape> &in_b, const std::vector<geo::CplxTrans> &trans_b,
                         int mode, std::vector <geo::Edge> &out_edges)
{
  clear ();

  size_t e = 0;
  for (std::vector<geo::Shape>::const_iterator i = in_a.begin (); i != in_a.end (); ++i) {
    e += count_edges (*i);
  }
  for (std::vector<geo::Shape>::const_iterator i = in_b.begin (); i != in_b.end (); ++i) {
    e += count_edges (*i);
  }
  reserve (e + e / 4); // heuristic reserve for crossing points

  size_t n;
  n = 0;
  for (std::vector<geo::Shape>::const_iterator i = in_a.begin (); i != in_a.end (); ++i, ++n) {
    if (n < trans_a.size ()) {
      insert (*i, geo::ICplxTrans (trans_a [n]), n * 2);
    } else {
      insert (*i, n * 2);
    }
  }
  n = 0;
  for (std::vector<geo::Shape>::const_iterator i = in_b.begin (); i != in_b.end (); ++i, ++n) {
    if (n < trans_b.size ()) {
      insert (*i, geo::ICplxTrans (trans_b [n]), n * 2 + 1);
    } else {
      insert (*i, n * 2 + 1);
    }
  }

  geo::BooleanOp op ((geo::BooleanOp::BoolOp) mode);
  geo::EdgeContainer out (out_edges);
  process (out, op);
}

void
ShapeProcessor::boolean (const std::vector<geo::Shape> &in_a, const std::vector<geo::CplxTrans> &trans_a,
                         const std::vector<geo::Shape> &in_b, const std::vector<geo::CplxTrans> &trans_b,
                         int mode, std::vector <geo::Polygon> &out_polygons, bool resolve_holes, bool min_coherence)
{
  clear ();

  size_t e = 0;
  for (std::vector<geo::Shape>::const_iterator i = in_a.begin (); i != in_a.end (); ++i) {
    e += count_edges (*i);
  }
  for (std::vector<geo::Shape>::const_iterator i = in_b.begin (); i != in_b.end (); ++i) {
    e += count_edges (*i);
  }
  reserve (e + e / 4); // heuristic reserve for crossing points

  size_t n;
  n = 0;
  for (std::vector<geo::Shape>::const_iterator i = in_a.begin (); i != in_a.end (); ++i, ++n) {
    if (n < trans_a.size ()) {
      insert (*i, geo::ICplxTrans (trans_a [n]), n * 2);
    } else {
      insert (*i, n * 2);
    }
  }
  n = 0;
  for (std::vector<geo::Shape>::const_iterator i = in_b.begin (); i != in_b.end (); ++i, ++n) {
    if (n < trans_b.size ()) {
      insert (*i, geo::ICplxTrans (trans_b [n]), n * 2 + 1);
    } else {
      insert (*i, n * 2 + 1);
    }
  }

  geo::BooleanOp op ((geo::BooleanOp::BoolOp) mode);
  geo::PolygonContainer pc (out_polygons);
  geo::PolygonGenerator out (pc, resolve_holes, min_coherence);
  process (out, op);
}

void 
ShapeProcessor::size (const std::vector<geo::Shape> &in, const std::vector<geo::CplxTrans> &trans,
                      geo::Coord dx, geo::Coord dy, std::vector <geo::Polygon> &out, unsigned int mode, bool resolve_holes, bool min_coherence)
{
  //  1st step: merge input
  clear ();

  size_t e = 0;
  for (std::vector<geo::Shape>::const_iterator i = in.begin (); i != in.end (); ++i) {
    e += count_edges (*i);
  }
  reserve (e + e / 4); // heuristic reserve for crossing points

  size_t n;
  n = 0;
  for (std::vector<geo::Shape>::const_iterator i = in.begin (); i != in.end (); ++i, ++n) {
    if (n < trans.size ()) {
      insert (*i, geo::ICplxTrans (trans [n]), n * 2);
    } else {
      insert (*i, n * 2);
    }
  }

  //  Merge the polygons and feed them into the sizing filter
#if ! defined(DEBUG_SIZE_INTERMEDIATE)
  geo::PolygonContainer pc (out);
  geo::PolygonGenerator pg2 (pc, resolve_holes, min_coherence);
  geo::SizingPolygonFilter siz (pg2, dx, dy, mode);
  geo::PolygonGenerator pg (siz, false /*don't resolve holes*/, false /*min. coherence*/);
  geo::BooleanOp op (geo::BooleanOp::Or);
  process (pg, op);
#else
  //  Intermediate output for debugging 
  geo::PolygonContainer pc (out);
  geo::PolygonGenerator pg2 (pc, false, false);
  geo::BooleanOp op (geo::BooleanOp::Or);
  process (pg2, op);
  for (std::vector <geo::Polygon>::iterator p = out.begin (); p != out.end (); ++p) {
    *p = p->sized (dx, dy, mode);
  }
#endif
}

void 
ShapeProcessor::size (const std::vector<geo::Shape> &in, const std::vector<geo::CplxTrans> &trans,
                      geo::Coord dx, geo::Coord dy, std::vector <geo::Edge> &out, unsigned int mode)
{
  //  1st step: merge input
  clear ();

  size_t e = 0;
  for (std::vector<geo::Shape>::const_iterator i = in.begin (); i != in.end (); ++i) {
    e += count_edges (*i);
  }
  reserve (e + e / 4); // heuristic reserve for crossing points

  size_t n;
  n = 0;
  for (std::vector<geo::Shape>::const_iterator i = in.begin (); i != in.end (); ++i, ++n) {
    if (n < trans.size ()) {
      insert (*i, geo::ICplxTrans (trans [n]), n * 2);
    } else {
      insert (*i, n * 2);
    }
  }

  //  Merge the polygons and feed them into the sizing filter
  geo::EdgeContainer ec (out);
  geo::SizingPolygonFilter siz (ec, dx, dy, mode);
  geo::PolygonGenerator pg (siz, false /*don't resolve holes*/, false /*min. coherence*/);
  geo::BooleanOp op (geo::BooleanOp::Or);
  process (pg, op);
}

void 
ShapeProcessor::collect_shapes_hier (const geo::CplxTrans &tr, const geo::Layout &layout, const geo::Cell &cell, unsigned int layer, int hier_levels, size_t &pn, size_t pdelta) 
{
  geo::ICplxTrans tri (tr);
  for (geo::Shapes::shape_iterator s = cell.shapes (layer).begin (geo::ShapeIterator::All); ! s.at_end (); ++s) {
    insert (*s, tri, pn);
    pn += pdelta;
  }

  if (hier_levels != 0) {

    for (geo::Cell::const_iterator i = cell.begin (); ! i.at_end (); ++i) {
      for (geo::CellInstArray::iterator a = i->begin (); ! a.at_end (); ++a) {
        collect_shapes_hier (tr * i->complex_trans (*a), layout, layout.cell (i->cell_index ()), layer, hier_levels > 0 ? hier_levels - 1 : hier_levels, pn, pdelta);
      }
    }

  }
}

size_t
ShapeProcessor::count_edges_hier (const geo::Layout &layout, const geo::Cell &cell, unsigned int layer, std::map<std::pair<geo::cell_index_type, int>, size_t> &cache, int hier_levels) const
{
  std::map<std::pair<geo::cell_index_type, int>, size_t>::iterator c = cache.find (std::make_pair (cell.cell_index (), hier_levels));

  if (c != cache.end ()) {
    return c->second;
  } else {

    size_t n = 0;
    for (geo::Shapes::shape_iterator s = cell.shapes (layer).begin (geo::ShapeIterator::All); ! s.at_end (); ++s) {
      n += count_edges (*s);
    }

    if (hier_levels != 0) {
      for (geo::Cell::const_iterator i = cell.begin (); ! i.at_end (); ++i) {
        n += count_edges_hier (layout, layout.cell (i->cell_index ()), layer, cache, hier_levels > 0 ? hier_levels - 1 : hier_levels) * i->size ();
      }
    }

    cache.insert (std::make_pair (std::make_pair (cell.cell_index (), hier_levels), n));
    return n;

  }
}

void 
ShapeProcessor::boolean (const geo::Layout &layout_in_a, const geo::Cell &cell_in_a, const std::vector<unsigned int> &layers_in_a, 
                         const geo::Layout &layout_in_b, const geo::Cell &cell_in_b, const std::vector<unsigned int> &layers_in_b, 
                         geo::Shapes &shapes, int mode, bool with_sub_hierarchy, bool resolve_holes, bool min_coherence)
{
  double fa = 1.0, fb = 1.0;
  if (shapes.cell () && shapes.cell ()->layout ()) {
    double target_dbu = shapes.cell ()->layout ()->dbu ();
    fa = layout_in_a.dbu () / target_dbu;
    fb = layout_in_b.dbu () / target_dbu;
  }

  //  count the edges so we know how much memory to reserve
  size_t ne = 0;
  std::map<std::pair<geo::cell_index_type, int>, size_t> cache;
  for (std::vector<unsigned int>::const_iterator l = layers_in_a.begin (); l != layers_in_a.end (); ++l) {
    ne += count_edges_hier (layout_in_a, cell_in_a, *l, cache, with_sub_hierarchy ? -1 : 0);
    cache.clear ();
  }
  for (std::vector<unsigned int>::const_iterator l = layers_in_b.begin (); l != layers_in_b.end (); ++l) {
    ne += count_edges_hier (layout_in_b, cell_in_b, *l, cache, with_sub_hierarchy ? -1 : 0);
    cache.clear ();
  }

  clear ();
  reserve (ne + ne / 4); // heuristic reserve for crossing points

  size_t pn;

  //  collect all shapes of layout a into property ID's 0, 2, 4, 6, ...
  pn = 0;
  for (std::vector<unsigned int>::const_iterator l = layers_in_a.begin (); l != layers_in_a.end (); ++l) {
    collect_shapes_hier (geo::CplxTrans (fa), layout_in_a, cell_in_a, *l, with_sub_hierarchy ? -1 : 0, pn, 2);
  }

  //  collect all shapes of layout a into property ID's 1, 3, 5, 7, ...
  pn = 1;
  for (std::vector<unsigned int>::const_iterator l = layers_in_b.begin (); l != layers_in_b.end (); ++l) {
    collect_shapes_hier (geo::CplxTrans (fb), layout_in_b, cell_in_b, *l, with_sub_hierarchy ? -1 : 0, pn, 2);
  }

  geo::BooleanOp op ((geo::BooleanOp::BoolOp) mode);
  geo::ShapeGenerator sg (shapes, true /*clear shapes*/);
  geo::PolygonGenerator out (sg, resolve_holes, min_coherence);
  process (out, op);
}

void 
ShapeProcessor::size (const geo::Layout &layout_in, const geo::Cell &cell_in, const std::vector<unsigned int> &layers_in, 
                      geo::Shapes &out, geo::Coord dx, geo::Coord dy, unsigned int mode, bool with_sub_hierarchy, bool resolve_holes, bool min_coherence)
{
  double f = 1.0;
  if (out.cell () && out.cell ()->layout ()) {
    double target_dbu = out.cell ()->layout ()->dbu ();
    f = layout_in.dbu () / target_dbu;
  }

  //  count the edges so we know how much memory to reserve
  size_t ne = 0;
  std::map<std::pair<geo::cell_index_type, int>, size_t> cache;
  for (std::vector<unsigned int>::const_iterator l = layers_in.begin (); l != layers_in.end (); ++l) {
    ne += count_edges_hier (layout_in, cell_in, *l, cache, with_sub_hierarchy ? -1 : 0);
    cache.clear ();
  }

  clear ();
  reserve (ne + ne / 4); // heuristic reserve for crossing points

  //  collect all shapes of layout a into property ID's 0, 2, 4, 6, ... so we can use boolean OR
  size_t pn = 0;
  for (std::vector<unsigned int>::const_iterator l = layers_in.begin (); l != layers_in.end (); ++l) {
    collect_shapes_hier (geo::CplxTrans (f), layout_in, cell_in, *l, with_sub_hierarchy ? -1 : 0, pn, 2);
  }

  //  1st step: merge input
  out.clear ();

  //  Merge the polygons and feed them into the sizing filter
#if ! defined(DEBUG_SIZE_INTERMEDIATE)
  geo::ShapeGenerator sg (out, true /*clear shapes*/);
  geo::PolygonGenerator pg2 (sg, resolve_holes, min_coherence);
  geo::SizingPolygonFilter siz (pg2, dx, dy, mode);
  geo::PolygonGenerator pg (siz, false /*don't resolve holes*/, false /*min. coherence*/);
  geo::BooleanOp op (geo::BooleanOp::Or);
  process (pg, op);
#else
  //  Intermediate output for debugging 
  geo::ShapeGenerator sg (out, true /*clear shapes*/);
  std::vector <geo::Polygon> out2;
  geo::PolygonContainer pc (out2);
  geo::PolygonGenerator pg2 (pc, false, false);
  geo::BooleanOp op (geo::BooleanOp::Or);
  process (pg2, op);
  sg.start ();
  for (std::vector <geo::Polygon>::iterator p = out2.begin (); p != out2.end (); ++p) {
    sg.put (p->sized (dx, dy, mode));
  }
#endif
}

void 
ShapeProcessor::merge (const geo::Layout &layout_in, const geo::Cell &cell_in, const std::vector<unsigned int> &layers_in, 
                       geo::Shapes &shapes, bool with_sub_hierarchy, unsigned int min_wc, bool resolve_holes, bool min_coherence)
{
  double f = 1.0;
  if (shapes.cell () && shapes.cell ()->layout ()) {
    double target_dbu = shapes.cell ()->layout ()->dbu ();
    f = layout_in.dbu () / target_dbu;
  }

  //  count the edges so we know how much memory to reserve
  size_t ne = 0;
  std::map<std::pair<geo::cell_index_type, int>, size_t> cache;
  for (std::vector<unsigned int>::const_iterator l = layers_in.begin (); l != layers_in.end (); ++l) {
    ne += count_edges_hier (layout_in, cell_in, *l, cache, with_sub_hierarchy ? -1 : 0);
    cache.clear ();
  }

  clear ();
  reserve (ne + ne / 4); // heuristic reserve for crossing points

  //  collect all shapes of layout a into property ID's 0, 1, 2, 3, ... 
  size_t pn = 0;
  for (std::vector<unsigned int>::const_iterator l = layers_in.begin (); l != layers_in.end (); ++l) {
    collect_shapes_hier (geo::CplxTrans (f), layout_in, cell_in, *l, with_sub_hierarchy ? -1 : 0, pn, 1);
  }

  geo::MergeOp op (min_wc);
  geo::ShapeGenerator sg (shapes, true /*clear shapes*/);
  geo::PolygonGenerator out (sg, resolve_holes, min_coherence);
  process (out, op);
}

} // namespace geo

