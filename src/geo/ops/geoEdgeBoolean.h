#ifndef HDR_geoEdgeBoolean
#define HDR_geoEdgeBoolean

#include "geoEdge.h"
#include "geoHash.h"
#include "geoBoxScanner.h"
#include "geoShapes.h"

#include "utilsIntervalMap.h"

namespace geo
{

/**
 *  @brief A common definition for the boolean operations available on edges
 */
enum EdgeBoolOp { EdgeOr, EdgeNot, EdgeXor, EdgeAnd, EdgeIntersections };

struct OrJoinOp
{
  void operator() (int &v, int n)
  {
    v += n;
  }
};

struct AndJoinOp
{
  void operator() (int &v, int n)
  {
    if (n == 0) {
      v = 0;
    }
  }
};

struct NotJoinOp
{
  void operator() (int &v, int n)
  {
    if (n != 0) {
      v = 0;
    }
  }
};

struct XorJoinOp
{
  void operator() (int &v, int n)
  {
    if (n != 0) {
      if (v == 0) {
        v = (n > 0 ? 1 : -1);
      } else {
        v = 0;
      }
    }
  }
};

template <class OutputContainer>
struct EdgeBooleanCluster 
  : public geo::cluster<geo::Edge, size_t>
{
  typedef geo::Edge::coord_type coord_type;

  EdgeBooleanCluster (OutputContainer *output, EdgeBoolOp op)
    : mp_output (output), m_op (op)
  {
    //  .. nothing yet ..
  }

  void finish ()
  {
    //  determine base edge (longest overall edge)

    //  shortcut for single edge
    if (begin () + 1 == end ()) {
      if (begin ()->second == 0) {
        if (m_op != EdgeAnd) {
          mp_output->insert (*(begin ()->first));
        }
      } else {
        if (m_op != EdgeAnd && m_op != EdgeNot) {
          mp_output->insert (*(begin ()->first));
        }
      }
      return;
    }

    geo::Edge r = *begin ()->first;
    double l1 = 0.0, l2 = r.double_length ();
    double n = 1.0 / l2;
    geo::Point p1 = r.p1 (), p2 = r.p2 ();

    for (iterator o = begin () + 1; o != end (); ++o) {
      double ll1 = geo::sprod (geo::Vector (o->first->p1 () - r.p1 ()), r.d ()) * n;
      double ll2 = geo::sprod (geo::Vector (o->first->p2 () - r.p1 ()), r.d ()) * n;
      if (ll1 < l1) {
        p1 = o->first->p1 ();
        l1 = ll1;
      }
      if (ll2 < l1) {
        p1 = o->first->p2 ();
        l1 = ll2;
      }
      if (ll1 > l2) {
        p2 = o->first->p1 ();
        l2 = ll1;
      }
      if (ll2 > l2) {
        p2 = o->first->p2 ();
        l2 = ll2;
      }
    }

    geo::Vector d = geo::Vector (p2 - p1);
    n = 1.0 / d.double_length ();

    OrJoinOp or_jop;
    AndJoinOp and_jop;
    NotJoinOp not_jop;
    XorJoinOp xor_jop;

    geo::interval_map<geo::Coord, int> a, b;
    a.add (0, geo::coord_traits<geo::Coord>::rounded (d.double_length ()), 0, or_jop);
    b.add (0, geo::coord_traits<geo::Coord>::rounded (d.double_length ()), 0, or_jop);

    for (iterator o = begin (); o != end (); ++o) {
      geo::Coord l1 = geo::coord_traits<geo::Coord>::rounded (geo::sprod (geo::Vector (o->first->p1 () - p1), d) * n);
      geo::Coord l2 = geo::coord_traits<geo::Coord>::rounded (geo::sprod (geo::Vector (o->first->p2 () - p1), d) * n);
      if (o->second == 0 || m_op == EdgeOr) {
        if (l1 < l2) {
          a.add (l1, l2, 1, or_jop);
        } else if (l1 > l2) {
          a.add (l2, l1, -1, or_jop);
        }
      } else {
        if (l1 < l2) {
          b.add (l1, l2, 1, or_jop);
        } else {
          b.add (l2, l1, -1, or_jop);
        }
      }
    }

    geo::interval_map<geo::Coord, int> q;
    for (geo::interval_map<geo::Coord, int>::const_iterator ia = a.begin (); ia != a.end (); ++ia) {
      q.add (ia->first.first, ia->first.second, ia->second > 0 ? 1 : (ia->second < 0 ? -1 : 0), or_jop);
    }

    if (b.begin () == b.end ()) {

      //  optimize for empty b
      if (m_op != EdgeAnd) {
        for (geo::interval_map<geo::Coord, int>::const_iterator ib = b.begin (); ib != b.end (); ++ib) {
          if (ib->second > 0) {
            mp_output->insert (geo::Edge (p1 + geo::Vector (d * (ib->first.first * n)), p1 + geo::Vector (d * (ib->first.second * n))));
          } else if (ib->second < 0) {
            mp_output->insert (geo::Edge (p1 + geo::Vector (d * (ib->first.second * n)), p1 + geo::Vector (d * (ib->first.first * n))));
          }
        }
      }

    } else {

      if (m_op == EdgeAnd) {
        for (geo::interval_map<geo::Coord, int>::const_iterator ib = b.begin (); ib != b.end (); ++ib) {
          q.add (ib->first.first, ib->first.second, ib->second, and_jop);
        }
      } else if (m_op == EdgeNot) {
        for (geo::interval_map<geo::Coord, int>::const_iterator ib = b.begin (); ib != b.end (); ++ib) {
          q.add (ib->first.first, ib->first.second, ib->second, not_jop);
        }
      } else if (m_op == EdgeXor) {
        for (geo::interval_map<geo::Coord, int>::const_iterator ib = b.begin (); ib != b.end (); ++ib) {
          q.add (ib->first.first, ib->first.second, ib->second, xor_jop);
        }
      }

      for (geo::interval_map<geo::Coord, int>::const_iterator iq = q.begin (); iq != q.end (); ++iq) {
        if (iq->second > 0) {
          mp_output->insert (geo::Edge (p1 + geo::Vector (d * (iq->first.first * n)), p1 + geo::Vector (d * (iq->first.second * n))));
        } else if (iq->second < 0) {
          mp_output->insert (geo::Edge (p1 + geo::Vector (d * (iq->first.second * n)), p1 + geo::Vector (d * (iq->first.first * n))));
        }
      }

    }

  }

private:
  OutputContainer *mp_output;
  geo::EdgeBoolOp m_op;
};

template <class OutputContainer>
struct EdgeBooleanClusterCollector
  : public geo::cluster_collector<geo::Edge, size_t, EdgeBooleanCluster<OutputContainer> >
{
  EdgeBooleanClusterCollector (OutputContainer *output, EdgeBoolOp op)
    : geo::cluster_collector<geo::Edge, size_t, EdgeBooleanCluster<OutputContainer> > (EdgeBooleanCluster<OutputContainer> (output, op == EdgeIntersections ? EdgeAnd : op), op != EdgeAnd && op != EdgeIntersections /*report single*/),
      mp_output (output), mp_intersections (op == EdgeIntersections ? output : 0)
  {
    //  .. nothing yet ..
  }

  EdgeBooleanClusterCollector (OutputContainer *output, OutputContainer *intersections, EdgeBoolOp op)
    : geo::cluster_collector<geo::Edge, size_t, EdgeBooleanCluster<OutputContainer> > (EdgeBooleanCluster<OutputContainer> (output, op), op != EdgeAnd /*report single*/),
      mp_output (output), mp_intersections (intersections)
  {
    //  .. nothing yet ..
  }

  void add (const geo::Edge *o1, size_t p1, const geo::Edge *o2, size_t p2)
  {
    //  Select edges which are:
    //  1.) not degenerate
    //  2.) parallel with some tolerance of roughly 1 dbu
    //  3.) connected
    //  In intersection-detection mode, identify intersection points otherwise
    //  and insert into the intersections container as degenerated edges.

    if (! o1->is_degenerate () && ! o2->is_degenerate () 
        && fabs ((double) geo::vprod (*o1, *o2)) < geo::coord_traits<geo::Coord>::prec_distance () * std::min (o1->double_length (), o2->double_length ())
        && (o1->p1 () == o2->p1 () || o1->p1 () == o2->p2 () || o1->p2 () == o2->p1 () || o1->p2 () == o2->p2 () || o1->coincident (*o2))) {

      geo::cluster_collector<geo::Edge, size_t, EdgeBooleanCluster<OutputContainer> >::add (o1, p1, o2, p2);

    } else if (mp_intersections && p1 != p2) {

      std::pair<bool, geo::Point> ip = o1->intersect_point (*o2);
      if (ip.first) {
        m_intersections.insert (ip.second);
      }

    }
  }

  /**
   *  @brief A receiver for the reducer which removes points that are on the edges
   */
  struct RemovePointsOnEdges
    : public geo::box_scanner_receiver2<geo::Edge, size_t, geo::Point, size_t>
  {
  public:
    RemovePointsOnEdges (std::set<geo::Point> &points_to_remove)
      : mp_points_to_remove (&points_to_remove)
    { }

    void add (const geo::Edge *e, const size_t &, const geo::Point *pt, const size_t &)
    {
      if (e->contains (*pt)) {
        mp_points_to_remove->insert (*pt);
      }
    }

  private:
    std::set<geo::Point> *mp_points_to_remove;
  };

  /**
   *  @brief An inserter to produce degenerated edges from points
   */
  struct PointInserter
    : public std::iterator<std::output_iterator_tag, void, void, void, void>
  {
    typedef geo::Point value_type;

    PointInserter (OutputContainer *output)
      : mp_output (output)
    { }

    PointInserter &operator= (const geo::Point &pt)
    {
      mp_output->insert (geo::Edge (pt, pt));
      return *this;
    }

    PointInserter &operator* () { return *this; }
    PointInserter &operator++ () { return *this; }
    PointInserter &operator++ (int) { return *this; }

  private:
    OutputContainer *mp_output;
  };

  /**
   *  @brief Finalizes the implementation for "EdgeIntersections"
   *  This method pushes those points which don't interact with the edges to the output container
   *  as degenerate edges. It needs to be called after the pass has been made.
   */
  void finalize (bool)
  {
    if (m_intersections.empty ()) {
      return;
    }

    geo::box_scanner2<geo::Edge, size_t, geo::Point, size_t> intersections_to_edge_scanner;
    for (typename OutputContainer::const_iterator e = mp_output->begin (); e != mp_output->end (); ++e) {
      intersections_to_edge_scanner.insert1 (e.operator-> (), 0);
    }
    for (std::set<geo::Point>::const_iterator p = m_intersections.begin (); p != m_intersections.end (); ++p) {
      intersections_to_edge_scanner.insert2 (p.operator-> (), 0);
    }

    std::set<geo::Point> points_to_remove;
    RemovePointsOnEdges rpoe (points_to_remove);
    intersections_to_edge_scanner.process (rpoe, 1, geo::box_convert<geo::Edge> (), geo::box_convert<geo::Point> ());

    std::set_difference (m_intersections.begin (), m_intersections.end (), points_to_remove.begin (), points_to_remove.end (), PointInserter (mp_intersections));
  }

private:
  OutputContainer *mp_output;
  OutputContainer *mp_intersections;
  std::set<geo::Point> m_intersections;
};

/**
 *  @brief A helper class to use geo::Shapes as container for EdgeBooleanClusterCollector
 */
struct DB_PUBLIC ShapesToOutputContainerAdaptor
{
public:
  struct Iterator
    : public geo::Shapes::shape_iterator
  {
    Iterator (const geo::Shapes::shape_iterator &iter)
      : geo::Shapes::shape_iterator (iter)
    { }

    Iterator ()
      : geo::Shapes::shape_iterator ()
    { }

    const geo::Edge *operator-> () const
    {
      return (geo::Shapes::shape_iterator::operator* ()).basic_ptr (geo::Edge::tag ());
    }

    bool operator!= (const Iterator &other) const
    {
      //  only for testing whether at end:
      return at_end () != other.at_end ();
    }

    bool operator== (const Iterator &other) const
    {
      //  only for testing whether at end:
      return at_end () == other.at_end ();
    }
  };

  typedef Iterator const_iterator;

  ShapesToOutputContainerAdaptor (geo::Shapes &shapes)
    : mp_shapes (&shapes)
  {
    //  .. nothing yet ..
  }

  const_iterator begin ()
  {
    return Iterator (mp_shapes->begin (geo::ShapeIterator::Edges));
  }

  const_iterator end ()
  {
    return Iterator ();
  }

  void insert (const geo::Edge &edge)
  {
    mp_shapes->insert (edge);
  }

private:
  geo::Shapes *mp_shapes;
};

/**
 *  @brief A specialization of the EdgeBooleanClusterCollector for a Shapes output container
 */
struct DB_PUBLIC EdgeBooleanClusterCollectorToShapes
  : EdgeBooleanClusterCollector<ShapesToOutputContainerAdaptor>
{
  EdgeBooleanClusterCollectorToShapes (geo::Shapes *output, EdgeBoolOp op)
    : EdgeBooleanClusterCollector<ShapesToOutputContainerAdaptor> (&m_adaptor, op), m_adaptor (*output)
  {
  }

private:
  ShapesToOutputContainerAdaptor m_adaptor;
};

}

#endif

