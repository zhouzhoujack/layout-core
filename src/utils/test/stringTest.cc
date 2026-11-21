// ===========================================================================
// tl 字符串工具单元测试
// ===========================================================================

#include <gtest/gtest.h>

#include "utilsString.h"
#include "utilsVariant.h"

namespace
{

TEST (TlStringTest, SprintfFormatsVariants)
{
    EXPECT_EQ ("42", geo::sprintf ("%d", geo::Variant (42)));
    EXPECT_EQ ("hello world", geo::sprintf ("%s %s", geo::Variant ("hello"), geo::Variant ("world")));
    EXPECT_EQ ("x=1.50", geo::sprintf ("x=%.2f", geo::Variant (1.5)));
}

TEST (TlStringTest, SprintfWithoutArgumentsReturnsFormat)
{
    EXPECT_EQ ("no args", geo::sprintf ("no args"));
}

TEST (TlStringTest, ToStringHonoursPrecision)
{
    // 第二个参数是有效位数（%.*g）
    EXPECT_EQ (std::string ("3.1"), geo::to_string (3.14159, 2));
    EXPECT_EQ (std::string ("3.142"), geo::to_string (3.14159, 4));
}

TEST (TlStringTest, TrimAndToLowerCase)
{
    EXPECT_EQ ("abc", geo::trim ("  abc \t\n"));
    EXPECT_EQ ("abc", geo::to_lower_case ("AbC"));
    EXPECT_EQ ("ABC", geo::to_upper_case ("AbC"));
}

}  // namespace
