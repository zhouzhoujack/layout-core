// ===========================================================================
// layout-io demo case 接口
// ===========================================================================
#pragma once

#include <string>

namespace demo
{

/**
 *  @brief layout-io 演示用例
 *
 *  五个演示步骤（写盘 / 布尔运算 / 读回 / 部分读入 / 裁剪 dump）
 *  均为私有实现，外部只需要调用 run ()。
 */
class DemoCase
{
public:
    /**
     *  @brief 依次运行 demo 的五个步骤
     */
    void run ();

private:
    void write_sample_layout ();
    void boolean_operations ();
    void read_whole_layout ();
    void partial_load ();
    void clip_and_dump (const std::string &src_file);
};

}  // namespace demo
