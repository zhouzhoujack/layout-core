# layout-io

独立的 GDS2 / OASIS 读写 + 多边形布尔运算库，从 layout-viewer 的 `tl` / `db` 中拆出，
以 **`.so` + 头文件** 的形式供其它工程（如 layout-viewer）使用。

| 目录 | 内容 |
| --- | --- |
| `lib/` | 构建产物：`libutils.so`、`libdata.so`、`libdb-io.so`、`libreader.so`、`libwriter.so`（自动生成） |
| `include/` | 构建产物：**全部头文件平铺**（geo 公共头 + utils + gsi_shim） |
| `src/CMakeLists.txt` | **工程根**：project / 工具链 / 构建开关 / 引入模块与测试 |
| `src/utils/` | 基础设施：字符串 / 变体 / 容器 / 异常 / 流 / XML / 线程 / 内存统计（命名空间 `geo::`，文件前缀 `utils*`）→ `libutils.so` |
| `src/utils/test/` | utils 的 gtest 单元测试（`ut_utils`） |
| `src/geo/` | 几何与 IO：`geo::` 命名空间下的 Polygon / Region / Edges / Layout / Reader / Writer / 布尔引擎 → `libdb-io.so`；GDS2 / OASIS 的格式常量与选项声明（`geoGDS2.h` / `geoGDS2Format.h` / `geoOASIS2.h` / `geoOASIS2Format.h` / `geoOASIS2Magic.cc`）位于 `src/geo/base/` |
| `src/geo/test/` | geo 的 gtest 单元测试（`ut_geo`） |
| `src/reader/` | **读取器**：GDS2 / OASIS 的 Reader 实现与格式注册（`geo::GDS2Reader` / `geo::OASIS2Reader` / `geo::CommonReader`，含 `GDS2FormatDeclaration` / `OASIS2FormatDeclaration`）→ `libreader.so`（依赖 `libdb-io.so` 与 `libwriter.so`；使用方需连同它一起链接，否则格式注册缺失） |
| `src/data/` | layout data 基础类型（`geo::Coord` / `coord_traits` / `geo::point` / `ShortPoint` / `Point` / `DPoint` / `geo::vector` / `geo::matrix` / `geo::Trans` 等变换 / `geo::box` / `geo::edge` / `geo::edge_pair` / `geo::polygon` / `geo::path` / `geo::text` / `geo::array` / `geo::PropertiesRepository` / `geo::LayoutStateModel` / `geo::box_tree` 空间索引 / `geo::Font` 等文本对齐枚举 / `geo::MetaInfo` / 全局静态参数 `geoStatic`）→ `libdata.so`（geo 依赖它，data 不反向依赖 geo） |
| `demo/` | **独立工程**：纯可执行 demo（无 gtest），演示如何用 `lib/` 与 `include/` |
| `src/writer/` | **写入器**：GDS2 / OASIS 的 Writer 实现（`geo::GDS2Writer` / `geo::OASIS2Writer`）→ `libwriter.so`（依赖 `libdb-io.so`；`libreader.so` 已通过 DT_NEEDED 拖入它，通常只链 reader 即可） |

（源码目录是 `src/utils` / `src/data` / `src/geo` / `src/reader` / `src/writer`，CMake 目标名分别是 `utils` / `data` / `db-io` / `reader` / `writer`。
构建完成后，`.so` 输出到 `lib/`，所有头文件（geo / utils / gsi_shim）平铺到
`include/`，使用方只需一个 `-I include`。）

## 命名空间

三个库（`libutils.so` 基础设施 + `libdata.so` 基础类型 + `libdb-io.so` 几何/IO）统一使用 **`geo`** 一个命名空间，
使用方不需要区分前缀：

```cpp
geo::Layout layout;                        // 几何模型
geo::InputStream stream ("in.oas");        // 基础设施（原 utils::）
geo::Reader reader (stream);
geo::Variant v = geo::Variant (1);         // 基础设施（原 utils::）
```

> 源码目录与文件名前缀（`src/utils/`、`utils*.h`）和 CMake 目标名（`utils` / `db-io`）保持
> 不变，只有命名空间被统一。

合并时有两个名字与几何库冲突，已改用新名字（语义不变）：

| 原名字（utils） | 现名字（geo） | 冲突原因 |
| --- | --- | --- |
| `utils::vector<T>` | `geo::uvector<T>` | `geo::vector<T>` 是几何向量模板 |
| `utils::Object` | `geo::RefCounted` | `geo::Object` 是数据库对象（事务/recall）基类 |

> 注意：`geo` 命名空间里有 `log` / `info` / `warn` / `error` 四个**日志对象**
> （`geo::log << ...`），在命名空间内调用 `<cmath>` 的同名函数时需写成 `::log (...)`
> （`geoGDS2Writer.cc` 里就有这样一处）。

## 编译

```bash
./compile.sh                 # Debug，构建到 build/（内部执行 cmake -S src -B build）
./compile.sh --release       # Release
```

或者手动：

```bash
cmake -S src -B build && cmake --build build -j3
```

并行度固定 `-j3`。

## 单元测试（gtest）

```bash
ctest --test-dir build --output-on-failure                 # 跑全部
ctest --test-dir build -R ut_geo                          # 只跑几何层
./build/utils/test/ut_utils --gtest_filter='Tl*'          # 细粒度过滤
```

| 可执行文件 | 覆盖内容 |
| --- | --- |
| `src/utils/test/ut_utils` | `geo::` 字符串格式化、`geo::Variant` 取值/比较 |
| `src/geo/test/ut_geo` | `geo::Region` / `geo::Polygon` 基础、polygon 布尔（并/交/差/异或、挖洞、链式）、GDS2 写入→读回、**OASIS 写入→读回** |

测试在 `src/utils/test` 与 `src/geo/test` 里各自声明，由顶层统一登记到 ctest。

新增用例：直接在所属模块的 `test/` 目录下加 `*Test.cc`，并登记到该目录的
`CMakeLists.txt`；测试入口用同目录的 `main.cpp`（内部初始化 glog + gtest）。

### 日志（toolkit 库）

日志统一走 toolchain 的 **toolkit** 库（`toolkit::toolkit`，由 `utils` 以 PUBLIC 方式依赖），
使用它提供的宏与格式器：

```cpp
#include <toolkit/tlGLog.h>                    // LOGI / LOGW / LOGE / LOGF

google::InitGoogleLogging (argv[0]);
google::InstallPrefixFormatter (glogPrefixFormatter);  // "[HH:MM:SS file:line]"
google::SetStderrLogging (google::GLOG_INFO);
```

两个测试入口（`src/utils/test/main.cpp`、`src/geo/test/main.cpp`）与 demo（`demo/main.cpp`）都按上面
方式初始化，因此：

- 库内部的 `LOGI`（如 `geoGDS2.cc` 的初始化日志）会直接出现在测试输出里；
- 用例内用 `LOGI` 打印关键数值（面积、多边形数、读写路径、映射结果等）。

输出形如：

```
[01:14:56 gdsIOTest.cc:55] gds write/read: wrote /tmp/layoutio_artifact.gds
[01:14:56 booleanTest.cc:53] boolean or: area=15000 polygons=1
```

查看方式：`ctest --test-dir build -V`，或直接执行测试可执行文件；日志级别可用
`GLOG_minloglevel=0/1/2` 过滤。

## OASIS 读写（`src/reader` + `src/writer`）

除了 GDS2，库还提供 **OASIS（SEMI P39，`%SEMI-OASIS` 1.0）** 的读写能力。使用方式与 GDS2
完全一致：格式按**文件名后缀**自动选择，`Reader` / `Writer` 不需要改动。

```cpp
// 写 OASIS（.oas 后缀自动选中 OASIS 格式）
geo::SaveLayoutOptions options;
options.set_format_from_filename ("out.oas");     // 失败则说明格式未注册
geo::OutputStream stream ("out.oas", geo::OutputStream::OM_Plain);
geo::Writer writer (options);
writer.write (layout, stream);

// 读 OASIS（自动探测格式，与读 GDS 的代码相同）
geo::InputStream stream ("out.oas");
geo::Reader reader (stream);
geo::Layout layout;
geo::LoadLayoutOptions load_options;
reader.read (layout, load_options);               // reader.format () == "OASIS"
```

### 支持的记录（写入）

| 图形 | OASIS 记录 |
| --- | --- |
| 矩形 / Box | `RECTANGLE`（方形用 square 标志） |
| 多边形 | `POLYGON`（通用 point list 类型 4） |
| 路径 | `PATH`（半宽 + 端点扩展方案） |
| 文本 | `TEXT`（内联字符串） |
| 单元引用 | `PLACEMENT`（旋转/镜像在 info 字节中） |
| 一般变换引用（放大 + 任意角度） | `PLACEMENT`（record 18，带 mag/angle） |
| 单元数组 | 重复记录（1D/2D 网格，或一般 g-delta 网格） |
| 单元名 / 单元体 | `CELLNAME`（显式 id）+ `CELL` |

写出的文件为 **非 strict 模式**：表写在文件开头，偏移表留空，末尾按 256 字节对齐补 `PAD`。
所有坐标以绝对值（`g-delta` 有符号编码）写出，不使用 modal 变量压缩——文件仍完全合规，
KLayout 可直接读取。

### 支持的记录（读取）

`START` / `END` / `CELLNAME` / `CELL` / `TEXTSTRING` / `PROPNAME` / `PROPSTRING` /
`LAYERNAME` / `PROPERTY` / `PLACEMENT` / `TEXT` / `RECTANGLE` / `POLYGON` / `PATH` /
`TRAPEZOID`（3 种）/ `CIRCLE` / `XELEMENT` / `XGEOMETRY` / `CBLOCK`。

要点：

- **modal 变量**：层号/数据类型、几何原点、point list、路径宽度/扩展、重复、放置位置都可被
  省略并由前一条记录继承（KLayout 就是这么写的）。
- **坐标模式**：`XYABSOLUTE` / `XYRELATIVE` 都支持。
- **point list**：类型 0/1（曼哈顿）、2（2-delta）、3（3-delta）、4/5（g-delta）全部支持。
- **重复（repetition）**：类型 0–11 全部支持，读取时统一**展开**成独立的图形/引用。
- **CBLOCK（DEFLATE 压缩块）**：通过 `geo::InputStream::inflate ()` 透明解压。
- **strict 模式的偏移表**：会读取并按顺序跳过（表在文件末尾时不影响顺序扫描）。

### 与 KLayout 的互操作性（已验证）

- 本库写出的 `.oas` 可被 **KLayout 0.30** 正确读入（单元、层、dbu、bbox、图形数量、实例/数组一致）；
- KLayout 写出的 `.oas`（strict / 非 strict、压缩 / 未压缩）可被本库正确读入，
  包括 KLayout 合并出的 iterated repetition（同一图形多次出现被压缩成一条重复记录）。

### 当前限制

- 写出**不支持** CBLOCK 压缩、strict 模式、`S_BOUNDING_BOX` 等标准属性
  （对应选项保留但被忽略，`strict_mode` 会打一条 `LOGW`）；
- 读取**不支持** `CTRAPEZOID`（会抛异常），`PROPERTY` 会被完整解析但**不挂到图形上**
  （几何模型暂无 OASIS 属性支持）；
- 带洞的多边形无法写成 OASIS 多边形（OASIS 多边形无洞），写入时会抛异常。


## 编辑器（IntelliSense）

layout-io 是独立的 CMake 工程，`build/` 与 `build-demo/` 都会生成 `compile_commands.json`
（`CMAKE_EXPORT_COMPILE_COMMANDS ON`）。如果头文件在 VS Code 里变红（`cannot open source
file "geoBox.h"` / `"toolkit/tlGLog.h"`），检查 `.vscode/c_cpp_properties.json` 是否包含：

- `${workspaceFolder}/layout-io/include`（平铺的全部 layout-io 头文件）
- `${workspaceFolder}/layout-io/src/**`（模块内的裸文件名 include）
- toolchain 第三方：`toolkit_0_1_0/include`、`glog_0_7_1/include`、`googletest/include`、`zlib_1_3/include`
- Qt5：`/usr/include/x86_64-linux-gnu/qt5` 及 `QtCore`/`QtGui`/`QtXml`
- 宏：`HAVE_QT`、`QT_NO_VERSION_TAGGING`、`GLOG_USE_GLOG_EXPORT`

## Demo（layout-io/demo）

`demo/` 是一个**独立工程**，不使用 gtest，只把发布出来的 `lib/` 与 `include/` 当外部库用，
是一份可以直接抄走的用法示例：

```bash
./compile.sh --demo              # 构建 layout-io → 构建并运行 demo 一步到位

# 或者手动：
./compile.sh                     # 先产出 lib/ 与 include/
cmake -S demo -B build-demo && cmake --build build-demo -j3
./build-demo/layoutio_demo
```

`demo/` 的文件划分：

| 文件 | 内容 |
| --- | --- |
| `demo/main.cpp` | 入口：初始化 glog（toolkit 格式器）→ 调用 `demo::run ()` |
| `demo/demoCase.h` | `namespace demo { void run (); }` |
| `demo/demoCase.cpp` | 五个演示步骤的实现 |

`demoCase.cpp` 依次演示五件事：

| 步骤 | 内容 |
| --- | --- |
| 1 | 生成示例版图（两个图层）并写成 GDS2 |
| 2 | 多边形布尔运算：`\|` 并、`&` 交、`-` 差、`^` 异或，以及三角形等非矩形多边形 |
| 3 | 读回整个版图（cells / layers / dbu） |
| 4 | 按层部分读入（`layer_map` + `create_other_layers=false`）并打印读到的每个图形 |
| 5 | 读 `/home/hz/test-files/polygons.gds`，用 `[0,0,100,100]` µm 窗口裁剪后 dump 成新文件（优先 `.oas`，不可用时回退 `.gds`），再读回校验 |

运行输出示例：

```
[1] 已写出 /tmp/layoutio_demo.gds，cells=1 layers=2
[2] 布尔运算（面积单位 dbu²，area() 返回 64 位整数）
    a.area            = 10000
    (a | b).area 并   = 17500
    (a & b).area 交   = 2500
    (a - b).area 差   = 7500
    (a ^ b).area 异或 = 15000
    triangle.area     = 5000（与 a 的交集 5000）
[3] 全量读入：cells=1 layers=2 dbu=0.001
[4] 部分读入（只读 1/0）：layers=1，1/0 是否被映射=是
    layer index 0 [1/0] 2 shape(s)
      box (0,0;100,100)
      box (200,0;300,100)
[5] 读入 /home/hz/test-files/polygons.gds：cells=1 layers=1 dbu=0.001，裁剪窗口 0..100um
    保留图形 5 个，已 dump 到 /tmp/layoutio_demo_clip.oas
[5] 读回 /tmp/layoutio_demo_clip.oas：格式=OASIS cells=1 layers=1 dbu=0.001 shapes=5
```

> 第 5 步的示例文件路径是写死的；文件不存在时该步骤只打印一条 `LOGW` 后跳过，不影响其余步骤。

`demo/CMakeLists.txt` 的写法就是推荐的外部接入方式：把 `lib/<name>.so` 声明成
`IMPORTED` 目标并带上 include 路径/`HAVE_QT`/依赖，然后 `target_link_libraries(demo PRIVATE layoutio::db-io)`。

## 产物与接入方式

layout-io 只**编译**，不做安装 / 打包。构建完成后，产物直接落在 `src` 的上级目录：

```
lib/libutils.so         # utils 编译产物
lib/libdata.so          # data 编译产物
lib/libdb-io.so         # geo 编译产物
include/*.h             # 187 个头文件平铺（geo 公共头 + utils + gsi_shim）
```

所有头都在同一个目录里、用裸文件名互相 include（`#include "geoRegion.h"`、
`#include "tlVector.h"`），因此使用方**只需要一个 `-I include`**。

其中 `utils*.h`（utils）与 `gsi*.h`（脚本绑定替身）不是 geo 的公共 API，但被 geo 的
公共头直接引用（`geoLayout.h` → `gsiObject.h`、`utilsVariant.h` …），所以一并平铺发布。

### 方式一：subdirectory 引入（推荐）

```cmake
add_subdirectory(<path>/layout-io/src layoutio)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE db-io)   # 传递 utils、Qt5、glog、toolkit、ZLIB
```

### 方式二：直接用 lib/ 与 include/

```bash
GDS=<path>/layout-io
g++ -std=c++14 -fPIC -DHAVE_QT -I$GDS/include \
    main.cpp -L$GDS/lib -ldb-io -ltl ...
```

> `geoReader` / `geoWriter` 的构造参数是 `geo::InputStream` / `geo::OutputStream`，
> 需要流对象时直接 `#include "utilsStream.h"` 即可（`demo/main.cpp` 就是这么做）。

```cpp
#include "geoBox.h"
#include "geoRegion.h"

geo::Region a (geo::Box (0, 0, 100, 100));
geo::Region b (geo::Box (50, 50, 150, 150));
geo::Region u = a + b;   // 并
geo::Region i = a & b;   // 交
```

运行期需要 `libutils.so`、`libdata.so`、`libdb-io.so` 与 glog、Qt5 的库路径。

## 依赖

| 类型 | 依赖 |
| --- | --- |
| 构建/链接 | Qt5（Core / Gui / Xml）、glog、toolkit、zlib |
| 运行 | `libutils.so`、`libdata.so`、`libdb-io.so`、`libtoolkit.so`、Qt5、glog |

toolkit 库（toolchain 提供）在 layout-io 中的两处用途：

- **日志**：`toolkit/tlGLog.h` 的 `LOGI/LOGW/LOGE` 与 `glogPrefixFormatter`（见上一节）；
- **字符串工具**：`geo::trim` 直接复用 `toolkit::trim` 的实现，`libutils.so` 因此对
  `libtoolkit.so` 有真实的运行时依赖。

第三方依赖由 `src/CMakeLists.txt` 里的 `/home/hz/toolchain/cmake/toolchain.cmake` 定位。

## 约束

- 两个 `.so` 均以 `-Wl,--no-undefined` 链接：构建期若残留未定义符号会直接失败，
  避免产出"能链接但运行时报符号缺失"的库。
- 内部未使用的脚本/网络相关模块（如 `geoTilingProcessor`、`geoLayoutQuery`）不纳入构建。
