// ===========================================================================
// geo 单元测试入口
//
// 取代 gtest_main：日志走 toolkit 库
// （toolkit/tlGLog.h：LOGI/LOGW/LOGE）。
// ===========================================================================

#include <toolkit/tlGLog.h>
#include <gtest/gtest.h>

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);

    LOGI << "geo unit test: start";
    const int rc = ::RUN_ALL_TESTS();
    LOGI << "geo unit test: done, rc=" << rc;

    return rc;
}
