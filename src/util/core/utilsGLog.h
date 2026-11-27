#pragma once

// 日志统一走 toolchain 的 toolkit 库：
//   toolkit/lkGLog.h 提供 LOGI / LOGW / LOGE / LOGF（由 toolkit::logkit 通道输出）
#include "toolkit/lkGLog.h"

#include <QString>

// toolkit 的通道用 to_string () 把参数转成文本；这里给 QString 补一个特化，
// 因此 LOGI << someQString 可以直接输出（不用先 toStdString ()）。
namespace toolkit { namespace logkit
{

template <> inline std::string to_string (const QString &s)
{
  return s.toStdString ();
}

}}
