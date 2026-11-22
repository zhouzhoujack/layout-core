// ===========================================================================
// layout-core demo —— 入口
//
// 演示代码在 demoCase.cpp（demo::DemoCase），这里只负责初始化日志并调用。
// 编译：见 demo/CMakeLists.txt（或直接 ./compile.sh --demo）
// ===========================================================================

#include <toolkit/tlGLog.h>   // LOGI + glogPrefixFormatter

#include "demoCase.h"

int main (int argc, char **argv)
{
    google::InitGoogleLogging (argv[0]);
    google::InstallPrefixFormatter (glogPrefixFormatter);
    google::SetStderrLogging (google::GLOG_INFO);

    LOGI << "layout-core demo 开始";
    demo::DemoCase ().run ();
    LOGI << "layout-core demo 结束";

    google::ShutdownGoogleLogging ();
    return 0;
}
