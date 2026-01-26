#pragma once

#include "types.hpp"
#include <string>
#include <string_view>
#include <vector>
#include <memory>

namespace wv {

struct DrawOp {
    enum class Type {
        FillRect,
        StrokeRect,
        Line,
        Polyline,
        Text,
        SetClip,
        ClearClip
    };

    Type type;
    Rect rect = {};
    Point p1 = {};
    Point p2 = {};
    std::vector<Point> points;
    std::string text;
    Color color = {};
    f32 width = 1.0f;
};

class Recording {
public:
    explicit Recording(std::vector<DrawOp> ops);

    const std::vector<DrawOp>& ops() const { return ops_; }

private:
    std::vector<DrawOp> ops_;
};

class Recorder {
public:
    void reset();

    void fillRect(Rect r, Color c);
    void strokeRect(Rect r, Color c, f32 width);
    void drawLine(Point p1, Point p2, Color c, f32 width);
    void drawPolyline(const Point* pts, i32 count, Color c, f32 width);
    void drawText(Point p, std::string_view text, Color c);
    void setClip(Rect r);
    void clearClip();

    std::unique_ptr<Recording> finish();

private:
    std::vector<DrawOp> ops_;
};

}
