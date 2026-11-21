// ===========================================================================
// layout-io demo case —— 五个演示步骤的实现（与入口 main.cpp 分开，便于按需引用）
//
//   1) 生成示例版图并写成 GDS2
//   2) 多边形布尔运算（并 / 交 / 差 / 异或）
//   3) 读回整个版图
//   4) 按层部分读入（partial load）并打印读到的图形
//   5) 读真实 GDS，用 [0,0,100,100] µm 窗口裁剪后 dump 成新文件
// ===========================================================================

#include <toolkit/tlGLog.h>   // LOGI / LOGW / LOGE

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

#include "geoBox.h"
#include "geoCell.h"
#include "geoCommonReader.h"
#include "geoLayerProperties.h"
#include "geoLayout.h"
#include "geoLoadLayoutOptions.h"
#include "geoPolygon.h"
#include "geoReader.h"
#include "geoRegion.h"
#include "geoSaveLayoutOptions.h"
#include "geoShapes.h"
#include "geoStreamLayers.h"
#include "geoWriter.h"
#include "utilsStream.h"

#include "demoCase.h"

namespace
{

const std::string kFile = "/tmp/layoutio_demo.gds";

// ---------------------------------------------------------------------------
// 1) 生成示例版图：1/0 放两个方框，2/0 放一个（相距很远，便于后面区分）
// ---------------------------------------------------------------------------
void write_sample_layout ()
{
    geo::Layout layout;
    layout.dbu (0.001);

    geo::cell_index_type top = layout.add_cell ("TOP");

    const unsigned int layer1 = layout.insert_layer (geo::LayerProperties (1, 0));
    const unsigned int layer2 = layout.insert_layer (geo::LayerProperties (2, 0));

    layout.cell (top).shapes (layer1).insert (geo::Box (0, 0, 100, 100));
    layout.cell (top).shapes (layer1).insert (geo::Box (200, 0, 300, 100));
    layout.cell (top).shapes (layer2).insert (geo::Box (1000, 1000, 1100, 1100));

    geo::SaveLayoutOptions options;
    options.set_format_from_filename (kFile);

    geo::OutputStream stream (kFile, geo::OutputStream::OM_Plain);
    geo::Writer writer (options);
    writer.write (layout, stream);

    LOGI << "[1] 已写出 " << kFile
         << "，cells=" << layout.cells () << " layers=" << layout.layers ();
}

// ---------------------------------------------------------------------------
// 2) 布尔运算：Region 是「多边形集合」，支持 & | - ^ 与面积统计
// ---------------------------------------------------------------------------
void boolean_operations ()
{
    const geo::Region a (geo::Box (0, 0, 100, 100));
    const geo::Region b (geo::Box (50, 50, 150, 150));

    LOGI << "[2] 布尔运算（面积单位 dbu²，area() 返回 64 位整数）";
    std::cout << "    a.area            = " << a.area () << std::endl
              << "    (a | b).area 并   = " << (a | b).area () << std::endl
              << "    (a & b).area 交   = " << (a & b).area () << std::endl
              << "    (a - b).area 差   = " << (a - b).area () << std::endl
              << "    (a ^ b).area 异或 = " << (a ^ b).area () << std::endl;

    // 非矩形多边形也一样：从点集构造三角形的 hull
    geo::Polygon triangle;
    const geo::Point pts[3] = { geo::Point (0, 0), geo::Point (100, 0), geo::Point (0, 100) };
    triangle.assign_hull (pts, pts + 3);

    const geo::Region tri (triangle);
    std::cout << "    triangle.area     = " << tri.area ()
              << "（与 a 的交集 " << (tri & a).area () << "）" << std::endl;
}

// ---------------------------------------------------------------------------
// 3) 读回整个版图
// ---------------------------------------------------------------------------
void read_whole_layout ()
{
    geo::InputStream stream (kFile);
    geo::Reader reader (stream);

    geo::Layout layout;
    geo::LoadLayoutOptions options;
    reader.read (layout, options);

    LOGI << "[3] 全量读入：cells=" << layout.cells ()
         << " layers=" << layout.layers ()
         << " dbu=" << layout.dbu ();
}

// ---------------------------------------------------------------------------
// 4) 部分读入：只读 1/0，并打印读到的每个图形
// ---------------------------------------------------------------------------
void partial_load ()
{
    geo::InputStream stream (kFile);
    geo::Reader reader (stream);

    geo::CommonReaderOptions reader_options;
    reader_options.create_other_layers = false;            // 未映射的层不创建
    reader_options.layer_map.map (geo::LDPair (1, 0), 10); // 1/0 -> 逻辑层

    geo::LoadLayoutOptions options;
    options.set_options (reader_options);

    geo::Layout layout;
    const geo::LayerMap &layer_map = reader.read (layout, options);

    bool found = false;
    geo::cell_index_type top = 0;
    std::tie (found, top) = layout.cell_by_name ("TOP");

    LOGI << "[4] 部分读入（只读 1/0）：layers=" << layout.layers ()
         << "，1/0 是否被映射=" << (layer_map.is_mapped (geo::LDPair (1, 0)) ? "是" : "否");

    if (!found) {
        LOGW << "    没有找到 TOP 单元";
        return;
    }

    const geo::Cell &top_cell = layout.cell (top);
    for (unsigned int l = 0; l < layout.layers (); ++l) {
        const geo::Shapes &shapes = top_cell.shapes (l);
        if (shapes.size () == 0) {
            continue;
        }

        std::cout << "    layer index " << l
                  << " [" << layout.get_properties (l).to_string () << "] "
                  << shapes.size () << " shape(s)" << std::endl;

        for (geo::ShapeIterator it (shapes); ! it.at_end (); ++it) {
            std::cout << "      " << (*it).to_string () << std::endl;
        }
    }
}

// ---------------------------------------------------------------------------
// 5) 读真实 GDS → 按 [0,0,100,100] µm 窗口裁剪 → dump 成新文件 → 读回校验
//
// 库没有 bbox 级的流式部分读入，所以做法是：读全量后把每个单元/层的图形
// 收集成 Region，再与窗口 Region 求交（= 裁剪）。
// 输出优先用 .oas（OASIS），格式不可用时回退 .gds。
// ---------------------------------------------------------------------------
void clip_and_dump (const std::string &src_file)
{
    if (! std::ifstream (src_file).good ()) {
        LOGW << "[5] 跳过：找不到示例文件 " << src_file;
        return;
    }

    const double window_um = 100.0;

    geo::Layout src;
    {
        geo::InputStream stream (src_file);
        geo::Reader reader (stream);
        geo::LoadLayoutOptions options;
        reader.read (src, options);
    }

    const double dbu = src.dbu () > 0.0 ? src.dbu () : 0.001;
    const geo::Coord limit = static_cast<geo::Coord> (std::lround (window_um / dbu));
    const geo::Box window (0, 0, limit, limit);

    LOGI << "[5] 读入 " << src_file
         << "：cells=" << src.cells () << " layers=" << src.layers ()
         << " dbu=" << dbu << "，裁剪窗口 0.." << window_um << "um";

    geo::Layout dst;
    dst.dbu (dbu);
    const geo::cell_index_type dst_top = dst.add_cell ("TOP");

    size_t kept = 0;
    for (unsigned int l = 0; l < src.layers (); ++l) {
        for (geo::cell_index_type c = 0; c < src.cells (); ++c) {
            geo::Region region;
            const geo::Shapes &shapes = src.cell (c).shapes (l);
            for (geo::ShapeIterator it (shapes); ! it.at_end (); ++it) {
                region.insert (*it);
            }

            const geo::Region clipped = region & geo::Region (window);
            if (clipped.empty ()) {
                continue;
            }

            const unsigned int dst_layer = dst.insert_layer (src.get_properties (l));
            for (geo::Region::const_iterator p = clipped.begin (); ! p.at_end (); ++p) {
                dst.cell (dst_top).shapes (dst_layer).insert (*p);
                ++kept;
            }
        }
    }

    if (kept == 0) {
        LOGW << "[5] 窗口内没有图形，跳过写出";
        return;
    }

    // 优先写 .oas（OASIS），格式不可用时回退 .gds
    const std::string out_oas = "/tmp/layoutio_demo_clip.oas";
    const std::string out_gds = "/tmp/layoutio_demo_clip.gds";
    std::remove (out_oas.c_str ());
    std::remove (out_gds.c_str ());

    geo::SaveLayoutOptions options;
    std::string out_file = out_oas;
    if (! options.set_format_from_filename (out_oas)) {
        LOGW << "[5] OASIS 格式不可用，改写成 " << out_gds;
        out_file = out_gds;
        geo::SaveLayoutOptions gds_options;
        gds_options.set_format_from_filename (out_gds);
        options = gds_options;
    }

    {
        geo::OutputStream stream (out_file, geo::OutputStream::OM_Plain);
        geo::Writer writer (options);
        if (! writer.is_valid ()) {
            LOGW << "[5] 没有可用的写出格式";
            return;
        }
        writer.write (dst, stream);
    }

    std::cout << "    保留图形 " << kept << " 个，已 dump 到 " << out_file << std::endl;

    // 读回刚写出的文件，验证 OASIS 读写闭环（GDS2 同样适用）
    {
        geo::InputStream stream (out_file);
        geo::Reader reader (stream);

        geo::Layout check;
        geo::LoadLayoutOptions load_options;
        reader.read (check, load_options);

        size_t total = 0;
        for (unsigned int l = 0; l < check.layers (); ++l) {
            for (geo::cell_index_type c = 0; c < check.cells (); ++c) {
                total += check.cell (c).shapes (l).size ();
            }
        }

        LOGI << "[5] 读回 " << out_file << "：格式=" << reader.format ()
             << " cells=" << check.cells () << " layers=" << check.layers ()
             << " dbu=" << check.dbu () << " shapes=" << total;
    }
}

}  // namespace

namespace demo
{

void run ()
{
    std::remove (kFile.c_str ());

    write_sample_layout ();
    boolean_operations ();
    read_whole_layout ();
    partial_load ();
    clip_and_dump ("/home/hz/test-files/polygons.gds");

    std::remove (kFile.c_str ());
}

}  // namespace demo
