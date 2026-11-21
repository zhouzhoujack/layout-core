// ===========================================================================
// utils 单元测试入口
//
// 取代 gtest_main：glog 初始化与日志格式交给 toolkit 库
// （toolkit/tlGLog.h：LOGI/LOGW/LOGE + glogPrefixFormatter）。
// ===========================================================================

#include <toolkit/tlGLog.h>
#include <gtest/gtest.h>

int main(int argc, char** argv)
{
    google::InitGoogleLogging(argv[0]);
    google::InstallPrefixFormatter(glogPrefixFormatter);
    google::SetStderrLogging(google::GLOG_INFO);

    ::testing::InitGoogleTest(&argc, argv);

    LOGI << "utils unit test: start";
    const int rc = ::RUN_ALL_TESTS();
    LOGI << "utils unit test: done, rc=" << rc;

    google::ShutdownGoogleLogging();
    return rc;
}
