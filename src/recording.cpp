#include "recording.hpp"

namespace wv {

Recording::Recording(std::vector<DrawOp> ops) : ops_(std::move(ops)) {}

void Recorder::reset() {
    ops_.clear();
}

void Recorder::fillRect(Rect r, Color c) {
    DrawOp op;
    op.type = DrawOp::Type::FillRect;
    op.rect = r;
    op.color = c;
    ops_.push_back(std::move(op));
}

void Recorder::strokeRect(Rect r, Color c, f32 width) {
    DrawOp op;
    op.type = DrawOp::Type::StrokeRect;
    op.rect = r;
    op.color = c;
    op.width = width;
    ops_.push_back(std::move(op));
}

void Recorder::drawLine(Point p1, Point p2, Color c, f32 width) {
    DrawOp op;
    op.type = DrawOp::Type::Line;
    op.p1 = p1;
    op.p2 = p2;
    op.color = c;
    op.width = width;
    ops_.push_back(std::move(op));
}

void Recorder::drawPolyline(const Point* pts, i32 count, Color c, f32 width) {
    if (count < 2) return;
    DrawOp op;
    op.type = DrawOp::Type::Polyline;
    op.color = c;
    op.width = width;
    op.points.assign(pts, pts + count);
    ops_.push_back(std::move(op));
}

void Recorder::drawText(Point p, std::string_view text, Color c) {
    DrawOp op;
    op.type = DrawOp::Type::Text;
    op.p1 = p;
    op.text = std::string(text);
    op.color = c;
    ops_.push_back(std::move(op));
}

void Recorder::setClip(Rect r) {
    DrawOp op;
    op.type = DrawOp::Type::SetClip;
    op.rect = r;
    ops_.push_back(std::move(op));
}

void Recorder::clearClip() {
    DrawOp op;
    op.type = DrawOp::Type::ClearClip;
    ops_.push_back(std::move(op));
}

std::unique_ptr<Recording> Recorder::finish() {
    auto recording = std::make_unique<Recording>(std::move(ops_));
    ops_.clear();
    return recording;
}

}
