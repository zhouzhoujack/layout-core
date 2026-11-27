// ===========================================================================
// OASIS 写入 / 读回
//
// 覆盖：矩形、多边形、路径、文本、单元引用（单个 / 数组）、
//       以及写入后用自家 reader 读回的往返一致性。
// ===========================================================================

#include <toolkit/lkGLog.h>
#include <gtest/gtest.h>

#include <cstdio>
#include <string>

#include "geoBox.h"
#include "geoCell.h"
#include "geoCellInst.h"
#include "geoLayerProperties.h"
#include "geoLayout.h"
#include "geoLoadLayoutOptions.h"
#include "geoPath.h"
#include "geoPoint.h"
#include "geoPolygon.h"
#include "geoReader.h"
#include "geoSaveLayoutOptions.h"
#include "geoShapes.h"
#include "geoText.h"
#include "geoTrans.h"
#include "geoWriter.h"
#include "utilsStream.h"

namespace
{

//  builds the reference layout used for the round trip test
static void build_layout (geo::Layout &layout)
{
    layout.dbu (0.001);

    geo::cell_index_type sub = layout.add_cell ("SUB");
    geo::cell_index_type top = layout.add_cell ("TOP");

    unsigned int l1 = layout.insert_layer (geo::LayerProperties (1, 0));
    unsigned int l2 = layout.insert_layer (geo::LayerProperties (2, 1));

    //  SUB: a single rectangle
    layout.cell (sub).shapes (l1).insert (geo::Box (0, 0, 100, 200));

    //  TOP: rectangle
    layout.cell (top).shapes (l1).insert (geo::Box (0, 0, 1000, 500));

    //  TOP: triangle (non-rectangular polygon)
    geo::Polygon poly;
    geo::Point hull [3] = { geo::Point (2000, 0), geo::Point (2100, 0), geo::Point (2000, 100) };
    poly.assign_hull (hull, hull + 3);
    layout.cell (top).shapes (l2).insert (poly);

    //  TOP: path
    geo::Path path;
    geo::Point pts [3] = { geo::Point (0, 2000), geo::Point (1000, 2000), geo::Point (1000, 3000) };
    path.width (100);
    path.assign (pts, pts + 3);
    layout.cell (top).shapes (l1).insert (path);

    //  TOP: text
    layout.cell (top).shapes (l2).insert (geo::Text ("hello OASIS", geo::Trans (0, false, geo::Vector (50, 60))));

    //  TOP: a plain instance and a 3x2 array
    layout.cell (top).insert (geo::CellInstArray (geo::CellInst (sub), geo::Trans (1, false, geo::Vector (5000, 0))));
    layout.cell (top).insert (geo::CellInstArray (geo::CellInst (sub), geo::Trans (0, true, geo::Vector (0, 5000)),
                                                  geo::Vector (0, 300), geo::Vector (200, 0), 3, 2));
}

static geo::Layout read_layout (const std::string &file)
{
    geo::InputStream stream (file);
    geo::Reader reader (stream);

    geo::Layout layout;
    geo::LoadLayoutOptions options;
    reader.read (layout, options);

    return layout;
}

TEST (OasisTest, WriteThenRead)
{
    const std::string file = ::testing::TempDir () + "layoutio_artifact.oas";
    std::remove (file.c_str ());

    geo::Layout layout;
    build_layout (layout);

    //  the OASIS format must be registered so the file name can be mapped
    geo::SaveLayoutOptions options;
    ASSERT_TRUE (options.set_format_from_filename (file));

    {
        geo::OutputStream stream (file, geo::OutputStream::OM_Plain);
        geo::Writer writer (options);
        ASSERT_TRUE (writer.is_valid ());
        writer.write (layout, stream);
    }

    LOGI << "oasis write/read: wrote " << file;

    geo::Layout read = read_layout (file);

    LOGI << "oasis write/read: cells=" << read.cells ()
         << " layers=" << read.layers ()
         << " dbu=" << read.dbu ();

    EXPECT_EQ (layout.cells (), read.cells ());
    EXPECT_EQ (layout.layers (), read.layers ());
    EXPECT_DOUBLE_EQ (layout.dbu (), read.dbu ());

    bool found = false;
    geo::cell_index_type top_org = 0, top_read = 0;
    std::tie (found, top_org) = layout.cell_by_name ("TOP");
    ASSERT_TRUE (found);
    std::tie (found, top_read) = read.cell_by_name ("TOP");
    ASSERT_TRUE (found);
    EXPECT_EQ (top_org == top_read, true);   //  both are cell 1

    //  the geometry must be identical (arrays are expanded by the reader, so
    //  the bounding box is the robust criterion)
    geo::Box bbox_org = layout.cell (top_org).bbox ();
    geo::Box bbox_read = read.cell (top_read).bbox ();

    LOGI << "oasis write/read: bbox org=" << bbox_org.to_string ()
         << " read=" << bbox_read.to_string ();

    EXPECT_EQ (bbox_org, bbox_read);

    //  same number of shapes per layer in the top cell
    for (unsigned int l = 0; l < layout.layers (); ++l) {
        EXPECT_EQ (layout.cell (top_org).shapes (l).size (),
                   read.cell (top_read).shapes (l).size ())
            << "layer index " << l << " [" << layout.get_properties (l).to_string () << "]";
    }
}

TEST (OasisTest, DetectFormat)
{
    //  a file written by the OASIS writer must be detected as OASIS
    const std::string file = ::testing::TempDir () + "layoutio_detect.oas";
    std::remove (file.c_str ());

    geo::Layout layout;
    layout.dbu (0.001);
    layout.add_cell ("A");

    geo::SaveLayoutOptions options;
    ASSERT_TRUE (options.set_format_from_filename (file));

    {
        geo::OutputStream stream (file, geo::OutputStream::OM_Plain);
        geo::Writer writer (options);
        ASSERT_TRUE (writer.is_valid ());
        writer.write (layout, stream);
    }

    geo::InputStream stream (file);
    geo::Reader reader (stream);
    EXPECT_EQ (std::string ("OASIS"), std::string (reader.format ()));
}

}
