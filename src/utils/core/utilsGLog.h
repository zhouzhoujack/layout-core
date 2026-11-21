#pragma once

// 日志统一走 toolchain 的 toolkit 库：
//   toolkit/tlGLog.h 提供 LOGI / LOGW / LOGE / LOGF 以及 glogPrefixFormatter
// 这里只补一个 QString 的输出支持（layout-io 编译时启用了 HAVE_QT）。
#include "toolkit/tlGLog.h"

#include <QString>
#include <ostream>

inline std::ostream& operator<<(std::ostream& os, const QString& s)
{
    return os << s.toStdString();
}
