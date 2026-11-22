// ===========================================================================
// geo::Region / geo::Polygon 单元测试
//
// 这是 layout-core 的核心能力：多边形区域的布尔运算。
// 面积单位是 dbu²（此处坐标即 dbu），故 100x100 的方框面积为 10000。
// ===========================================================================

#include <gtest/gtest.h>

#include "geoBox.h"
#include "geoPolygon.h"
#include "geoRegion.h"

namespace
{

TEST (RegionTest, SingleBox)
{
    geo::Region r (geo::Box (0, 0, 100, 100));

    EXPECT_EQ (size_t (1), r.count ());
    EXPECT_EQ (geo::Box (0, 0, 100, 100), r.bbox ());
    EXPECT_DOUBLE_EQ (10000.0, r.area ());
}

TEST (RegionTest, UnionAndIntersection)
{
    geo::Region a (geo::Box (0, 0, 100, 100));
    geo::Region b (geo::Box (50, 50, 150, 150));

    // 两个 100x100 的方框错位 50，重叠部分为 50x50
    EXPECT_DOUBLE_EQ (17500.0, (a + b).area ());
    EXPECT_DOUBLE_EQ (2500.0, (a & b).area ());
}

TEST (RegionTest, DifferenceAndXor)
{
    geo::Region a (geo::Box (0, 0, 100, 100));
    geo::Region b (geo::Box (50, 50, 150, 150));

    EXPECT_DOUBLE_EQ (7500.0, (a - b).area ());
    EXPECT_DOUBLE_EQ (15000.0, (a ^ b).area ());
    EXPECT_EQ (size_t (1), (a - b).count ());
}

TEST (RegionTest, DisjointRegionsDoNotIntersect)
{
    geo::Region a (geo::Box (0, 0, 10, 10));
    geo::Region b (geo::Box (100, 100, 110, 110));

    EXPECT_TRUE ((a & b).empty ());
    EXPECT_EQ (size_t (2), (a + b).count ());
}

TEST (RegionTest, MultipleShapesAndMerge)
{
    geo::Region r;
    r.insert (geo::Box (0, 0, 10, 10));
    r.insert (geo::Box (90, 90, 100, 100));

    EXPECT_EQ (size_t (2), r.count ());
    EXPECT_EQ (geo::Box (0, 0, 100, 100), r.bbox ());
    EXPECT_DOUBLE_EQ (200.0, r.area ());

    // 两块互不相连，合并后仍是两个多边形
    geo::Region merged = r.merged ();
    EXPECT_EQ (size_t (2), merged.count ());
    EXPECT_DOUBLE_EQ (200.0, merged.area ());
}

TEST (RegionTest, MergeTouchingBoxes)
{
    geo::Region r;
    r.insert (geo::Box (0, 0, 10, 10));
    r.insert (geo::Box (10, 0, 20, 10));

    // 共边相邻，合并成一个矩形
    geo::Region merged = r.merged ();
    EXPECT_EQ (size_t (1), merged.count ());
    EXPECT_DOUBLE_EQ (200.0, merged.area ());
    EXPECT_EQ (geo::Box (0, 0, 20, 10), merged.bbox ());
}

TEST (PolygonTest, AreaFromBox)
{
    geo::Polygon p (geo::Box (0, 0, 100, 200));

    EXPECT_EQ (size_t (1), p.holes () + 1);
    EXPECT_EQ (geo::Box (0, 0, 100, 200), p.box ());
    EXPECT_DOUBLE_EQ (20000.0, p.area ());
}

TEST (PolygonTest, RegionFromPolygon)
{
    geo::Polygon p (geo::Box (0, 0, 30, 40));
    geo::Region r (p);

    EXPECT_EQ (size_t (1), r.count ());
    EXPECT_DOUBLE_EQ (1200.0, r.area ());
}

}  // namespace
