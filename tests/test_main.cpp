#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "vcd_parser.hpp"
#include "waveform_viewer.hpp"
#include <fstream>

using namespace wv;

class VcdParserTest : public ::testing::Test {
protected:
    void writeVcd(const char* content) {
        std::ofstream f("/tmp/test.vcd");
        f << content;
    }
};

TEST_F(VcdParserTest, ParsesTimescale) {
    writeVcd(R"(
$timescale 1ns $end
$enddefinitions $end
)");
    
    VcdParser parser;
    ASSERT_TRUE(parser.parse("/tmp/test.vcd"));
    EXPECT_EQ(parser.data().timescale, 1000);
}

TEST_F(VcdParserTest, ParsesSignals) {
    writeVcd(R"(
$timescale 1ps $end
$scope module top $end
$var wire 1 ! clk $end
$var wire 8 # data [7:0] $end
$upscope $end
$enddefinitions $end
#0
0!
b00000000 #
#10
1!
b11110000 #
)");
    
    VcdParser parser;
    ASSERT_TRUE(parser.parse("/tmp/test.vcd"));
    
    auto& data = parser.data();
    ASSERT_EQ(data.signals.size(), 2);
    
    EXPECT_EQ(data.signals[0].name, "top.clk");
    EXPECT_EQ(data.signals[0].width, 1);
    ASSERT_EQ(data.signals[0].changes.size(), 2);
    EXPECT_EQ(data.signals[0].changes[0].value, 0);
    EXPECT_EQ(data.signals[0].changes[1].value, 1);
    
    EXPECT_EQ(data.signals[1].name, "top.data");
    EXPECT_EQ(data.signals[1].width, 8);
    EXPECT_EQ(data.signals[1].changes[1].value, 0xF0);
}

TEST_F(VcdParserTest, TracksEndTime) {
    writeVcd(R"(
$timescale 1ps $end
$scope module m $end
$var wire 1 ! x $end
$upscope $end
$enddefinitions $end
#0
0!
#100
1!
#200
0!
)");
    
    VcdParser parser;
    ASSERT_TRUE(parser.parse("/tmp/test.vcd"));
    EXPECT_EQ(parser.data().endTime, 200);
}

TEST_F(VcdParserTest, FailsOnMissingFile) {
    VcdParser parser;
    EXPECT_FALSE(parser.parse("/nonexistent/file.vcd"));
}

class WaveformViewerTest : public ::testing::Test {
protected:
    WaveformViewer viewer;
    WaveformData data;
    
    void SetUp() override {
        viewer.setSize(800, 600);
        data.endTime = 100;
        data.signals.push_back({"clk", "!", 1, {{0, 0}, {10, 1}, {20, 0}}});
        viewer.setData(&data);
    }
};

TEST_F(WaveformViewerTest, InitialState) {
    EXPECT_EQ(viewer.width(), 800);
    EXPECT_EQ(viewer.height(), 600);
    EXPECT_FALSE(viewer.needsRepaint());
}

TEST_F(WaveformViewerTest, MouseDragTriggersRepaint) {
    viewer.mouseDown(100, 100);
    viewer.mouseMove(150, 100);
    EXPECT_TRUE(viewer.needsRepaint());
}

TEST_F(WaveformViewerTest, ClearRepaintFlag) {
    viewer.mouseDown(100, 100);
    viewer.mouseMove(150, 100);
    viewer.clearRepaintFlag();
    EXPECT_FALSE(viewer.needsRepaint());
}

TEST_F(WaveformViewerTest, MouseMoveWithoutDownNoRepaint) {
    viewer.mouseMove(150, 100);
    EXPECT_FALSE(viewer.needsRepaint());
}

TEST_F(WaveformViewerTest, MouseUpStopsDrag) {
    viewer.mouseDown(100, 100);
    viewer.mouseUp();
    viewer.mouseMove(200, 100);
    EXPECT_FALSE(viewer.needsRepaint());
}

TEST_F(WaveformViewerTest, MouseWheelTriggersRepaint) {
    viewer.mouseWheel(400, 1);
    EXPECT_TRUE(viewer.needsRepaint());
}

TEST_F(WaveformViewerTest, MouseWheelInNameAreaNoRepaint) {
    viewer.mouseWheel(50, 1);
    EXPECT_FALSE(viewer.needsRepaint());
}

TEST_F(WaveformViewerTest, MouseDownInNameAreaNoDrag) {
    viewer.mouseDown(50, 100);
    viewer.mouseMove(100, 100);
    EXPECT_FALSE(viewer.needsRepaint());
}
