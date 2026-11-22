// ===========================================================================
// geo::point 单元测试
//
// 只依赖 data 模块自己的头（geoTypeDef.h / dataTypes.h / geoPoint.h），
// 刻意不碰 geo::vector —— 那属于 geo 层，data 不应该反向依赖。
// ===========================================================================

#include "geoPoint.h"

#include <toolkit/tlGLog.h>
#include <gtest/gtest.h>

TEST(PointTest, DefaultConstruction)
{
    const geo::Point p;
    EXPECT_EQ(0, p.x());
    EXPECT_EQ(0, p.y());
}

TEST(PointTest, ConstructionAndAccessors)
{
    const geo::Point p(123, -456);
    EXPECT_EQ(123, p.x());
    EXPECT_EQ(-456, p.y());

    const geo::DPoint dp(1.25, -2.5);
    EXPECT_DOUBLE_EQ(1.25, dp.x());
    EXPECT_DOUBLE_EQ(-2.5, dp.y());

    LOGI << "point: " << p.to_string() << " / " << dp.to_string();
}

TEST(PointTest, ConvertingConstructorRounds)
{
    // double -> int32_t 走 point_traits<int32_t>::rounded（四舍五入，远离零）
    const geo::Point a(1.4, 1.6);
    EXPECT_EQ(1, a.x());
    EXPECT_EQ(2, a.y());

    const geo::Point b(-1.5, -2.5);
    EXPECT_EQ(-2, b.x());
    EXPECT_EQ(-3, b.y());

    // int -> double 不丢精度
    const geo::DPoint d(geo::Point(7, -8));
    EXPECT_DOUBLE_EQ(7.0, d.x());
    EXPECT_DOUBLE_EQ(-8.0, d.y());

    LOGI << "converting ctor: " << b.to_string() << " / " << d.to_string();
}

TEST(PointTest, Setters)
{
    geo::Point p;
    p.set_x(11);
    p.set_y(-22);
    EXPECT_EQ(geo::Point(11, -22), p);

    geo::DPoint dp;
    dp.set_x(0.5);
    dp.set_y(-0.5);
    EXPECT_EQ(geo::DPoint(0.5, -0.5), dp);
}

TEST(PointTest, EqualityOperators)
{
    const geo::Point a(3, 4);
    const geo::Point b(3, 4);
    const geo::Point c(4, 3);

    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a != b);
    EXPECT_FALSE(a == c);
    EXPECT_TRUE(a != c);

    // equal() 走 traits：整数坐标是精确比较
    EXPECT_TRUE(a.equal(b));
    EXPECT_FALSE(a.equal(c));
    EXPECT_TRUE(a.not_equal(c));
}

TEST(PointTest, LessOperatorSortsByYThenX)
{
    const geo::Point a(2, 1);
    const geo::Point b(1, 2);

    EXPECT_TRUE(a < b);   // y 小的在前
    EXPECT_FALSE(b < a);

    const geo::Point c(1, 1);
    EXPECT_TRUE(c < a);   // y 相同则比较 x
    EXPECT_TRUE(a.less(b));
    EXPECT_TRUE(c.less(a));
    EXPECT_FALSE(a.less(a));
}

TEST(PointTest, FuzzyComparisonForDoubleCoordinates)
{
    // point_traits<double>::equal 的容差是 1e-5
    const geo::DPoint a(1.0, 2.0);
    const geo::DPoint b(1.0 + 1e-7, 2.0 - 1e-7);
    const geo::DPoint c(1.0 + 1e-3, 2.0);

    EXPECT_TRUE(a.equal(b));
    EXPECT_FALSE(a.equal(c));

    // operator== 仍是精确比较
    EXPECT_FALSE(a == b);
    EXPECT_TRUE(a == geo::DPoint(1.0, 2.0));

    LOGI << "fuzzy: a=" << a.to_string() << " b=" << b.to_string();
}

TEST(PointTest, ScalarArithmetic)
{
    const geo::Point p(10, -20);

    // point<C> * double -> point<double>（不取整）
    const geo::DPoint d = p * 0.5;
    EXPECT_DOUBLE_EQ(5.0, d.x());
    EXPECT_DOUBLE_EQ(-10.0, d.y());

    // point<C> * int -> point<C>
    EXPECT_EQ(geo::Point(20, -40), p * 2);

    // 除法回来仍按坐标类型取整（远离零：3.5->4，-6.5->-7）
    EXPECT_EQ(geo::Point(4, -7), geo::Point(7, -13) / 2);

    geo::Point q(101, -101);
    q *= 0.5;
    EXPECT_EQ(geo::Point(51, -51), q);

    q /= 2L;
    EXPECT_EQ(geo::Point(26, -26), q);

    LOGI << "arithmetic: p*0.5=" << d.to_string() << " q=" << q.to_string();
}

TEST(PointTest, Distances)
{
    const geo::Point p(3, 4);   // 3-4-5

    EXPECT_EQ(geo::Point::distance_type(5), p.distance());
    EXPECT_DOUBLE_EQ(5.0, p.double_distance());
    EXPECT_EQ(geo::Point::area_type(25), p.sq_distance());
    EXPECT_DOUBLE_EQ(25.0, p.sq_double_distance());

    const geo::Point o(0, 0);
    EXPECT_EQ(geo::Point::distance_type(5), p.distance(o));
    EXPECT_EQ(geo::Point::area_type(25), p.sq_distance(o));

    LOGI << "distance=" << p.distance() << " sq_distance=" << p.sq_distance();
}

TEST(PointTest, ToString)
{
    const geo::Point p(1000, -2000);
    EXPECT_FALSE(p.to_string().empty());
    EXPECT_NE(std::string::npos, p.to_string().find(','));
    EXPECT_EQ(p.to_string(0.0), p.to_string());

    // dbu = 1.0 走 db_to_string
    EXPECT_EQ(p.to_string(0.0), p.to_string(1.0));

    const geo::DPoint dp(1.5, -2.25);
    EXPECT_NE(std::string::npos, dp.to_string().find(','));

    LOGI << "to_string: " << p.to_string() << " | " << p.to_string(0.001);
}
