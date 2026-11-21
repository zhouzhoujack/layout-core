// ===========================================================================
// tl Variant 单元测试
//
// Variant 是 layout-io 与外部交互的通用值类型（读取属性、参数传递等），
// 这里只覆盖最常用的存取与类型判定路径。
// ===========================================================================

#include <gtest/gtest.h>

#include "utilsVariant.h"

namespace
{

TEST (TlVariantTest, DefaultIsNil)
{
    geo::Variant v;
    EXPECT_TRUE (v.is_nil ());
}

TEST (TlVariantTest, HoldsInteger)
{
    geo::Variant v (42);
    EXPECT_FALSE (v.is_nil ());
    EXPECT_TRUE (v.is_long ());
    EXPECT_EQ (42, v.to_int ());
    EXPECT_EQ (std::string ("42"), v.to_string ());
}

TEST (TlVariantTest, HoldsString)
{
    geo::Variant v ("layout");
    EXPECT_TRUE (v.is_a_string ());
    EXPECT_EQ (std::string ("layout"), v.to_string ());
}

TEST (TlVariantTest, HoldsDouble)
{
    geo::Variant v (1.5);
    EXPECT_TRUE (v.is_double ());
    EXPECT_DOUBLE_EQ (1.5, v.to_double ());
}

TEST (TlVariantTest, ComparisonAndAssignment)
{
    geo::Variant a (1);
    geo::Variant b (2);

    EXPECT_TRUE (a < b);
    EXPECT_TRUE (a == geo::Variant (1));

    a = b;
    EXPECT_EQ (2, a.to_int ());
}

}  // namespace
