// ===========================================================================
// geo 布尔运算（polygon boolean）测试
//
// 用非矩形多边形（三角形）与矩形组合，覆盖 并/交/差/异或、自反性、
// 空操作数、链式运算以及“差集挖洞”这几种布尔运算的典型情形。
// 面积单位是 dbu²。
// ===========================================================================

#include <toolkit/tlGLog.h>
#include <gtest/gtest.h>

#include "geoBox.h"
#include "geoPoint.h"
#include "geoPolygon.h"
#include "geoRegion.h"

namespace
{

// 直角三角形 (0,0)-(100,0)-(0,100)，面积 5000
geo::Polygon triangle ()
{
    geo::Polygon p;
    const geo::Point pts[3] = {
        geo::Point (0, 0),
        geo::Point (100, 0),
        geo::Point (0, 100)
    };
    p.assign_hull (pts, pts + 3);
    return p;
}

TEST (BooleanTest, TriangleArea)
{
    const geo::Region r (triangle ());

    EXPECT_EQ (size_t (1), r.count ());
    EXPECT_DOUBLE_EQ (5000.0, r.area ());
}


TEST (BooleanTest, UnionSquaresMergesIntoOnePolygon)
{
    const geo::Region a (geo::Box (0, 0, 100, 100));
    const geo::Region b (geo::Box (50, 0, 150, 100));   // 与 a 重叠

    const geo::Region u = a | b;

    EXPECT_DOUBLE_EQ (15000.0, u.area ());
    EXPECT_EQ (size_t (1), u.count ());
    EXPECT_EQ (geo::Box (0, 0, 150, 100), u.bbox ());

    LOGI << "boolean or: area=" << u.area () << " polygons=" << u.count ();
}

// `+` 是拼接（join），不会做布尔合并；`|` 才是真正的布尔并。
TEST (BooleanTest, JoinVersusOr)
{
    const geo::Region a (geo::Box (0, 0, 100, 100));
    const geo::Region b (geo::Box (50, 0, 150, 100));

    EXPECT_EQ (size_t (2), (a + b).count ());
    EXPECT_EQ (size_t (1), (a | b).count ());
    EXPECT_DOUBLE_EQ ((a + b).area (), (a | b).area ());

    LOGI << "join: polygons=" << (a + b).count ()
               << " or: polygons=" << (a | b).count () << " (area 一致)";
}

TEST (BooleanTest, IntersectionOfTriangleAndBox)
{
    const geo::Region tri (triangle ());            // (0,0)(100,0)(0,100)
    const geo::Region box (geo::Box (0, 0, 100, 100));

    // 三角形完全落在方框内
    EXPECT_DOUBLE_EQ (5000.0, (tri & box).area ());
    EXPECT_TRUE ((tri - box).empty ());

    // 反过来：方框减三角形 = 5000
    EXPECT_DOUBLE_EQ (5000.0, (box - tri).area ());
}

TEST (BooleanTest, IntersectionOfTwoHalfOverlappingTriangles)
{
    geo::Polygon shifted;
    const geo::Point pts[3] = {
        geo::Point (100, 0),
        geo::Point (200, 0),
        geo::Point (100, 100)
    };
    shifted.assign_hull (pts, pts + 3);

    const geo::Region a (triangle ());
    const geo::Region b (shifted);

    // 两个 5000 的三角形只在 (100,0) 处接触 → 交集为空、并集 10000
    EXPECT_TRUE ((a & b).empty ());
    EXPECT_DOUBLE_EQ (10000.0, (a + b).area ());
}

TEST (BooleanTest, DifferenceCreatesHole)
{
    const geo::Region outer (geo::Box (0, 0, 100, 100));
    const geo::Region inner (geo::Box (25, 25, 75, 75));

    const geo::Region ring = outer - inner;

    // 10000 - 2500 = 7500，单个多边形（带一个洞）
    EXPECT_DOUBLE_EQ (7500.0, ring.area ());
    EXPECT_EQ (size_t (1), ring.count ());
    EXPECT_EQ (geo::Box (0, 0, 100, 100), ring.bbox ());

    LOGI << "boolean difference: area=" << ring.area ()
               << " polygons=" << ring.count () << " (带洞)";
}

TEST (BooleanTest, XorEqualsUnionMinusIntersection)
{
    const geo::Region a (geo::Box (0, 0, 100, 100));
    const geo::Region b (geo::Box (50, 50, 150, 150));

    const double xor_area = (a ^ b).area ();
    const double expect = (a + b).area () - (a & b).area ();

    EXPECT_DOUBLE_EQ (expect, xor_area);
    EXPECT_DOUBLE_EQ (15000.0, xor_area);

    LOGI << "boolean xor: area=" << xor_area
               << " union-intersection=" << expect;
}

TEST (BooleanTest, BooleanWithEmptyOperand)
{
    const geo::Region a (geo::Box (0, 0, 100, 100));
    const geo::Region empty;

    EXPECT_TRUE (empty.empty ());
    EXPECT_DOUBLE_EQ (10000.0, (a + empty).area ());
    EXPECT_DOUBLE_EQ (10000.0, (a - empty).area ());
    EXPECT_TRUE ((a & empty).empty ());
    EXPECT_DOUBLE_EQ (10000.0, (a ^ empty).area ());
}

TEST (BooleanTest, SelfOperationsAreIdempotent)
{
    geo::Region a;
    a.insert (geo::Box (0, 0, 100, 100));
    a.insert (geo::Box (50, 0, 150, 100));

    EXPECT_DOUBLE_EQ (a.area (), (a & a).area ());
    EXPECT_DOUBLE_EQ (a.area (), (a + a).area ());
    EXPECT_TRUE ((a - a).empty ());
    EXPECT_TRUE ((a ^ a).empty ());
}

TEST (BooleanTest, ChainedOperations)
{
    const geo::Region a (geo::Box (0, 0, 100, 100));
    const geo::Region b (geo::Box (50, 0, 150, 100));
    const geo::Region c (geo::Box (0, 0, 75, 100));

    // (a ∪ b) ∩ c = c（c 完全在 a 内）
    EXPECT_DOUBLE_EQ (7500.0, ((a + b) & c).area ());

    // 原地运算与返回新对象等价
    geo::Region d = a;
    d |= b;
    d &= c;
    EXPECT_DOUBLE_EQ (7500.0, d.area ());
}

TEST (BooleanTest, MergedRegionOfOverlappingBoxes)
{
    geo::Region r;
    r.insert (geo::Box (0, 0, 100, 100));
    r.insert (geo::Box (50, 0, 150, 100));   // 与第一个重叠

    EXPECT_EQ (size_t (2), r.count ());

    const geo::Region merged = r.merged ();
    EXPECT_EQ (size_t (1), merged.count ());
    EXPECT_DOUBLE_EQ (15000.0, merged.area ());
}

}  // namespace
