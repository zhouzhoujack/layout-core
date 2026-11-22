// ===========================================================================
// io 层测试：格式注册 / 格式探测 / 读写往返
//
// 覆盖点：
//   - GDS2 / OASIS 两种格式在 io 库中完成静态注册（Writer 与 Reader 都能用）
//   - 按文件名后缀选择写入格式
//   - 按文件内容自动探测读取格式
//   - GDS2 / OASIS 写入后由自家 Reader 读回，几何信息一致
//   - 未知格式给出明确异常
// ===========================================================================

#include <toolkit/tlGLog.h>
#include <gtest/gtest.h>

#include <cstdio>
#include <string>
#include <tuple>

#include "geoBox.h"
#include "geoCell.h"
#include "geoCommonReader.h"
#include "geoGDS2.h"
#include "geoLayerProperties.h"
#include "geoLayout.h"
#include "geoLoadLayoutOptions.h"
#include "geoOASIS2.h"
#include "geoReader.h"
#include "geoSaveLayoutOptions.h"
#include "geoShapes.h"
#include "geoWriter.h"
#include "utilsStream.h"

namespace
{

//  writes a layout with a single box to the given file
static void write_box_layout (const std::string &file, const geo::Box &box)
{
    geo::Layout layout;
    layout.dbu (0.001);

    geo::cell_index_type top = layout.add_cell ("TOP");
    unsigned int layer = layout.insert_layer (geo::LayerProperties (1, 0));
    layout.cell (top).shapes (layer).insert (box);

    geo::SaveLayoutOptions options;
    ASSERT_TRUE (options.set_format_from_filename (file)) << file;

    geo::OutputStream stream (file, geo::OutputStream::OM_Plain);
    geo::Writer writer (options);
    ASSERT_TRUE (writer.is_valid ()) << options.format ();
    writer.write (layout, stream);
}

//  reads a layout back and checks the bounding box of the top cell
static void check_box_layout (const std::string &file, const geo::Box &box)
{
    geo::InputStream stream (file);
    geo::Reader reader (stream);

    geo::Layout layout;
    geo::LoadLayoutOptions options;
    reader.read (layout, options);

    bool found = false;
    geo::cell_index_type top = 0;
    std::tie (found, top) = layout.cell_by_name ("TOP");
    ASSERT_TRUE (found) << file;

    ASSERT_EQ (size_t (1), layout.cell (top).shapes (0).size ()) << file;
    EXPECT_EQ (box, layout.cell (top).bbox ()) << file;
}

// ---------------------------------------------------------------
//  格式注册

TEST (IoFormatTest, FormatsRegistered)
{
    //  两种格式都必须能被 Writer 找到
    geo::SaveLayoutOptions gds;
    ASSERT_TRUE (gds.set_format_from_filename ("out.gds"));
    EXPECT_EQ (std::string ("GDS2"), gds.format ());

    geo::SaveLayoutOptions oas;
    ASSERT_TRUE (oas.set_format_from_filename ("out.oas"));
    EXPECT_EQ (std::string ("OASIS"), oas.format ());

    //  构造 Writer 成功即说明格式声明已注册（否则会抛 "Unknown stream format"）
    {
        geo::Writer writer (gds);
        EXPECT_TRUE (writer.is_valid ());
    }
    {
        geo::Writer writer (oas);
        EXPECT_TRUE (writer.is_valid ());
    }
}

TEST (IoFormatTest, UnknownFormatThrows)
{
    geo::SaveLayoutOptions options;
    options.set_format ("NO-SUCH-FORMAT");
    EXPECT_THROW (geo::Writer writer (options), geo::Exception);
}

// ---------------------------------------------------------------
//  格式常量

TEST (IoFormatTest, FormatConstants)
{
    //  OASIS magic（头内定义，reader / writer 共用）
    EXPECT_EQ (size_t (13), geo::oasis_magic_length);
    EXPECT_EQ (std::string ("%SEMI-OASIS\015\012", 13),
               std::string (geo::oasis_magic, geo::oasis_magic_length));

    //  GDS2 头部记录
    EXPECT_EQ (short (0x0002), geo::sHEADER);
    EXPECT_EQ (short (0x0400), geo::sENDLIB);
}

// ---------------------------------------------------------------
//  写 → 读回（按内容自动探测格式）

TEST (IoRoundTripTest, Gds2)
{
    const std::string file = ::testing::TempDir () + "layoutio_io_test.gds";
    std::remove (file.c_str ());

    const geo::Box box (-100, -200, 300, 400);
    write_box_layout (file, box);
    LOGI << "gds2 round trip: " << file;

    check_box_layout (file, box);
}

TEST (IoRoundTripTest, Oasis)
{
    const std::string file = ::testing::TempDir () + "layoutio_io_test.oas";
    std::remove (file.c_str ());

    const geo::Box box (10, 20, 30, 40);
    write_box_layout (file, box);
    LOGI << "oasis round trip: " << file;

    check_box_layout (file, box);
}

// ---------------------------------------------------------------
//  Reader 对未知内容的处理

TEST (IoReaderTest, UnknownFormatThrows)
{
    const std::string file = ::testing::TempDir () + "layoutio_io_test.unknown";
    std::remove (file.c_str ());

    {
        std::FILE *fp = std::fopen (file.c_str (), "wb");
        ASSERT_NE (nullptr, fp);
        const char garbage [] = "this is definitely not a layout file";
        std::fwrite (garbage, 1, sizeof (garbage) - 1, fp);
        std::fclose (fp);
    }

    geo::InputStream stream (file);
    EXPECT_THROW (geo::Reader reader (stream), geo::ReaderException);

    std::remove (file.c_str ());
}

}
