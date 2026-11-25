// ===========================================================================
// io 单元测试入口
//
// 与 ut_geo / ut_data 保持一致：日志走 toolkit 库
// （toolkit/tlGLog.h：LOGI/LOGW/LOGE）。
// ===========================================================================

#include <toolkit/tlGLog.h>
#include <gtest/gtest.h>

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);

    LOGI << "io unit test: start";
    const int rc = ::RUN_ALL_TESTS();
    LOGI << "io unit test: done, rc=" << rc;

    return rc;
}
