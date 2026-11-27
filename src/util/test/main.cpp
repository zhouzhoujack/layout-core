// ===========================================================================
// utils 单元测试入口
//
// 取代 gtest_main：日志走 toolkit 库
// （toolkit/lkGLog.h：LOGI/LOGW/LOGE）。
// ===========================================================================

#include <toolkit/lkGLog.h>
#include <gtest/gtest.h>

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);

    LOGI << "utils unit test: start";
    const int rc = ::RUN_ALL_TESTS();
    LOGI << "utils unit test: done, rc=" << rc;

    return rc;
}
