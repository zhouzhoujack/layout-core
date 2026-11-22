// ===========================================================================
// io 单元测试入口
//
// 与 ut_geo / ut_data 保持一致：glog 初始化与格式交给 toolkit 库
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

    LOGI << "io unit test: start";
    const int rc = ::RUN_ALL_TESTS();
    LOGI << "io unit test: done, rc=" << rc;

    google::ShutdownGoogleLogging();
    return rc;
}
