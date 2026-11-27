#pragma once

// 日志统一走 toolchain 的 toolkit 库：
//   toolkit/lkGLog.h 提供 LOGI / LOGW / LOGE / LOGF（由 toolkit::logkit 通道输出）
// 这里只补一个 QString 的输出支持。
#include "toolkit/lkGLog.h"

#include <QString>
#include <ostream>

inline std::ostream& operator<<(std::ostream& os, const QString& s)
{
    return os << s.toStdString();
}
