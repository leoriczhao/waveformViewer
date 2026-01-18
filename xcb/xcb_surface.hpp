#pragma once

#include "surface.hpp"
#include <xcb/xcb.h>

namespace wv {

class XcbSurface : public Surface {
public:
    void init(SurfaceID sid, i32 w, i32 h) override;
    void release() override;
    
    void fillRect(Rect r, Color c) override;
    void strokeRect(Rect r, Color c, f32 width) override;
    void drawLine(Point p1, Point p2, Color c, f32 width) override;
    void drawPolyline(const Point* pts, i32 count, Color c, f32 width) override;
    void drawText(Point p, std::string_view text, Color c) override;
    
    void setClip(Rect r) override;
    void clearClip() override;
    
    void copyToWindow(xcb_window_t win);
    xcb_pixmap_t pixmap() const { return pixmap_; }
    
private:
    xcb_connection_t* conn_ = nullptr;
    xcb_pixmap_t pixmap_ = 0;
    xcb_gcontext_t gc_ = 0;
    i32 w_ = 0, h_ = 0;
    
    void setColor(Color c);
};

}
