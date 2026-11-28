#pragma once

#if !defined(HDR_utilsCommon_h)
# define HDR_utilsCommon_h

//  符号可见性：layout-core 只构建 Linux（GCC/Clang）共享库，
//  库内/库外声明一致，不再区分 MAKE_*_LIBRARY。
# define TL_PUBLIC           __attribute__ ((visibility ("default")))
# define TL_PUBLIC_TEMPLATE  __attribute__ ((visibility ("default")))
# define TL_LOCAL            __attribute__ ((visibility ("hidden")))

#endif
