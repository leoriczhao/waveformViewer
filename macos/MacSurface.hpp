#pragma once

#include "surface.hpp"
#include <CoreGraphics/CGContext.h>

namespace wv {

class MacSurface : public Surface {
public:
    void init(SurfaceID sid, i32 w, i32 h) override;
    void resize(i32 w, i32 h) override;
    void release() override;

    void beginFrame() override;
    void endFrame() override;

    void fillRect(Rect r, Color c) override;
    void strokeRect(Rect r, Color c, f32 width) override;
    void drawLine(Point p1, Point p2, Color c, f32 width) override;
    void drawPolyline(const Point* pts, i32 count, Color c, f32 width) override;
    void drawText(Point p, std::string_view text, Color c) override;

    void setClip(Rect r) override;
    void clearClip() override;

    void setContext(CGContextRef ctx) { ctx_ = ctx; }
    CGContextRef context() const { return ctx_; }

private:
    CGContextRef ctx_ = nullptr;
    i32 w_ = 0, h_ = 0;

    void setColor(Color c);
};

}
