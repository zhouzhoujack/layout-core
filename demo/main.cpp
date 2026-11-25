// ===========================================================================
// layout-core demo —— 入口
//
// 演示代码在 demoCase.cpp（demo::DemoCase），这里只负责初始化日志并调用。
// 编译：见 demo/CMakeLists.txt（或直接 ./compile.sh --demo）
// ===========================================================================

#include <toolkit/tlGLog.h>   // LOGI（toolkit::logkit 通道）

#include "demoCase.h"

int main (int argc, char **argv)
{
    (void) argc;
    (void) argv;

    LOGI << "layout-core demo 开始";
    demo::DemoCase ().run ();
    LOGI << "layout-core demo 结束";

    return 0;
}
