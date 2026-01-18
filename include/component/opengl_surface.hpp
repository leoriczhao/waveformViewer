#pragma once

#include "component/surface.hpp"
#include <memory>

namespace comp {

class OpenGLSurface : public Surface {
public:
    void init(SurfaceID sid) override;
    void release() override;
    
    i32 width() const override { return w_; }
    i32 height() const override { return h_; }
    void resize(i32 w, i32 h) { w_ = w; h_ = h; }
    
    void fillRect(Rect r, Color c) override;
    void strokeRect(Rect r, Color c, f32 width) override;
    void drawLine(Point p1, Point p2, Color c, f32 width) override;
    void drawPolyline(const Point* pts, i32 count, Color c, f32 width) override;
    void drawText(Point p, std::string_view text, Color c) override;
    
    void setClip(Rect r) override;
    void clearClip() override;
    
    void beginFrame();
    void endFrame();
    
private:
    i32 w_ = 0, h_ = 0;
};

std::unique_ptr<OpenGLSurface> createOpenGLSurface();

}
