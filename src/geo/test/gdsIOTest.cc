// ===========================================================================
// GDS2 写入 / 读回 / 按层部分读入（走发布出来的 include 头与 lib/*.so）
// ===========================================================================

#include <toolkit/lkGLog.h>
#include <gtest/gtest.h>

#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>

#include "geoBox.h"
#include "geoCell.h"
#include "geoCommonReader.h"
#include "geoLayerProperties.h"
#include "geoLayout.h"
#include "geoLoadLayoutOptions.h"
#include "geoReader.h"
#include "geoSaveLayoutOptions.h"
#include "geoShapes.h"
#include "geoStreamLayers.h"
#include "geoWriter.h"
#include "utilsStream.h"

namespace
{

TEST (ArtifactTest, GdsWriteThenRead)
{
    const std::string file = ::testing::TempDir () + "layoutio_artifact.gds";
    std::remove (file.c_str ());

    const double dbu = 0.001;
    const geo::Box box (0, 0, 100, 200);

    // 写入
    {
        geo::Layout layout;
        layout.dbu (dbu);

        geo::cell_index_type top = layout.add_cell ("TOP");
        unsigned int layer = layout.insert_layer (geo::LayerProperties (1, 0));
        layout.cell (top).shapes (layer).insert (box);

        geo::SaveLayoutOptions options;
        ASSERT_TRUE (options.set_format_from_filename (file));

        geo::OutputStream stream (file, geo::OutputStream::OM_Plain);
        geo::Writer writer (options);
        ASSERT_TRUE (writer.is_valid ());
        writer.write (layout, stream);
    }

    LOGI << "gds write/read: wrote " << file;

    // 读回
    {
        geo::InputStream stream (file);
        geo::Reader reader (stream);

        geo::Layout layout;
        geo::LoadLayoutOptions options;
        reader.read (layout, options);

        EXPECT_EQ (size_t (1), layout.cells ());
        EXPECT_DOUBLE_EQ (dbu, layout.dbu ());

        bool found = false;
        geo::cell_index_type top = 0;
        std::tie (found, top) = layout.cell_by_name ("TOP");
        ASSERT_TRUE (found);

        EXPECT_EQ (box, layout.cell (top).bbox ());
        EXPECT_EQ (size_t (1), layout.cell (top).shapes (0).size ());

        LOGI << "gds write/read: cells=" << layout.cells ()
                   << " layers=" << layout.layers ()
                   << " dbu=" << layout.dbu ()
                   << " shapes=" << layout.cell (top).shapes (0).size ();
    }

    std::remove (file.c_str ());
}

// ---------------------------------------------------------------------------
// 部分读入（partial load）
//
// 通过 CommonReaderOptions::layer_map 只读指定层（create_other_layers=false），
// 未映射的层不会进入目标 Layout —— 这是打开超大版图时最常用的省内存手段。
// ---------------------------------------------------------------------------
TEST (ArtifactTest, PartialLoadByLayerMap)
{
    const std::string file = ::testing::TempDir () + "layoutio_partial.gds";
    std::remove (file.c_str ());

    const geo::Box box_on_1 (0, 0, 100, 100);
    const geo::Box box_on_2 (1000, 1000, 1100, 1100);

    // 写入两层：1/0 与 2/0，两块方框相距很远，便于用 bbox 判断读入了哪些
    {
        geo::Layout layout;
        layout.dbu (0.001);

        geo::cell_index_type top = layout.add_cell ("TOP");
        unsigned int layer1 = layout.insert_layer (geo::LayerProperties (1, 0));
        unsigned int layer2 = layout.insert_layer (geo::LayerProperties (2, 0));
        layout.cell (top).shapes (layer1).insert (box_on_1);
        layout.cell (top).shapes (layer2).insert (box_on_2);

        geo::SaveLayoutOptions options;
        ASSERT_TRUE (options.set_format_from_filename (file));

        geo::OutputStream stream (file, geo::OutputStream::OM_Plain);
        geo::Writer writer (options);
        ASSERT_TRUE (writer.is_valid ());
        writer.write (layout, stream);
    }

    // 全量读入：两层都在
    {
        geo::InputStream stream (file);
        geo::Reader reader (stream);

        geo::Layout layout;
        geo::LoadLayoutOptions options;
        reader.read (layout, options);

        bool found = false;
        geo::cell_index_type top = 0;
        std::tie (found, top) = layout.cell_by_name ("TOP");
        ASSERT_TRUE (found);

        // 全量读入：两层都在（包围盒覆盖两块方框）
        EXPECT_EQ (geo::Box (0, 0, 1100, 1100), layout.cell (top).bbox ());
    }

    // 部分读入：只把流里的 1/0 映射到逻辑层 10，其余层不创建
    {
        geo::InputStream stream (file);
        geo::Reader reader (stream);

        geo::CommonReaderOptions reader_options;
        reader_options.create_other_layers = false;
        reader_options.layer_map.map (geo::LDPair (1, 0), 10);

        geo::LoadLayoutOptions options;
        options.set_options (reader_options);

        geo::Layout layout;
        const geo::LayerMap &layer_map = reader.read (layout, options);

        // 映射结果：1/0 有映射，2/0 没有
        EXPECT_TRUE (layer_map.is_mapped (geo::LDPair (1, 0)));
        EXPECT_FALSE (layer_map.is_mapped (geo::LDPair (2, 0)));

        bool found = false;
        geo::cell_index_type top = 0;
        std::tie (found, top) = layout.cell_by_name ("TOP");
        ASSERT_TRUE (found);

        const geo::Cell &top_cell = layout.cell (top);

        // 只有 1/0 的方框被读进来
        EXPECT_EQ (box_on_1, top_cell.bbox ());

        // 整个版图只读到 1 个图形对象，且落在 1/0 映射到的逻辑层上
        size_t total = 0;
        for (unsigned int l = 0; l < layout.layers (); ++l) {
            total += top_cell.shapes (l).size ();
        }
        EXPECT_EQ (size_t (1), total);

        std::pair<bool, unsigned int> logical = layer_map.first_logical (geo::LDPair (1, 0), layout);
        ASSERT_TRUE (logical.first);
        EXPECT_EQ (size_t (1), top_cell.shapes (logical.second).size ());

        LOGI << "partial load: mapped 1/0 -> logical " << logical.second
                   << ", total polygons=" << total
                   << ", layers=" << layout.layers ();
    }

    std::remove (file.c_str ());
}

// ---------------------------------------------------------------------------
// partial load 结果 dump
//
// 做完部分读入后，把读进来的多边形逐层打印出来，便于人工核对“到底读到了什么”。
// ---------------------------------------------------------------------------
TEST (ArtifactTest, PartialLoadDumpPolygons)
{
    const std::string file = ::testing::TempDir () + "layoutio_partial_dump.gds";
    std::remove (file.c_str ());

    // 1/0 放两个方框，2/0 放一个（部分读入时应当被丢弃）
    {
        geo::Layout layout;
        layout.dbu (0.001);

        geo::cell_index_type top = layout.add_cell ("TOP");
        unsigned int layer1 = layout.insert_layer (geo::LayerProperties (1, 0));
        unsigned int layer2 = layout.insert_layer (geo::LayerProperties (2, 0));
        layout.cell (top).shapes (layer1).insert (geo::Box (0, 0, 100, 100));
        layout.cell (top).shapes (layer1).insert (geo::Box (200, 0, 300, 100));
        layout.cell (top).shapes (layer2).insert (geo::Box (1000, 1000, 1100, 1100));

        geo::SaveLayoutOptions options;
        ASSERT_TRUE (options.set_format_from_filename (file));

        geo::OutputStream stream (file, geo::OutputStream::OM_Plain);
        geo::Writer writer (options);
        ASSERT_TRUE (writer.is_valid ());
        writer.write (layout, stream);
    }

    // 部分读入：只读 1/0
    geo::InputStream stream (file);
    geo::Reader reader (stream);

    geo::CommonReaderOptions reader_options;
    reader_options.create_other_layers = false;
    reader_options.layer_map.map (geo::LDPair (1, 0), 10);

    geo::LoadLayoutOptions options;
    options.set_options (reader_options);

    geo::Layout layout;
    reader.read (layout, options);

    bool found = false;
    geo::cell_index_type top = 0;
    std::tie (found, top) = layout.cell_by_name ("TOP");
    ASSERT_TRUE (found);

    const geo::Cell &top_cell = layout.cell (top);

    // ---- dump：逐层打印读进来的图形 ----
    std::ostringstream dump;
    size_t total = 0;

    for (unsigned int l = 0; l < layout.layers (); ++l) {
        const geo::Shapes &shapes = top_cell.shapes (l);
        if (shapes.size () == 0) {
            continue;
        }

        dump << "layer index " << l
             << " [" << layout.get_properties (l).to_string () << "] "
             << shapes.size () << " shape(s)" << std::endl;

        for (geo::ShapeIterator it (shapes); ! it.at_end (); ++it) {
            dump << "  " << (*it).to_string () << std::endl;
            ++total;
        }
    }

    std::cout << dump.str () << std::flush;

    // 只有 1/0 的两个方框：2/0 的 (1000,1000)-(1100,1100) 不应出现
    EXPECT_EQ (size_t (2), total);
    EXPECT_EQ (std::string::npos, dump.str ().find ("1000"));

    LOGI << "partial load dump: " << total << " polygon(s) on "
               << layout.layers () << " layer(s)";
}

}  // namespace
