#pragma once

#include "device.hpp"
#include <xcb/xcb.h>

namespace wv {

class XcbRasterDevice : public RasterDevice {
public:
    XcbRasterDevice(xcb_connection_t* conn, xcb_window_t win, i32 w, i32 h);
    ~XcbRasterDevice() override;

    void resize(i32 w, i32 h) override;
    void beginFrame() override;
    void endFrame() override;
    void present() override;

    void fillRect(Rect r, Color c) override;
    void strokeRect(Rect r, Color c, f32 width) override;
    void drawLine(Point p1, Point p2, Color c, f32 width) override;
    void drawPolyline(const Point* pts, i32 count, Color c, f32 width) override;
    void drawText(Point p, std::string_view text, Color c) override;

    void setClipRect(Rect r) override;
    void resetClip() override;

private:
    xcb_connection_t* conn_ = nullptr;
    xcb_window_t win_ = 0;
    xcb_pixmap_t pixmap_ = 0;
    xcb_gcontext_t gc_ = 0;
    xcb_font_t font_ = 0;
    i32 w_ = 0, h_ = 0;
    u8 depth_ = 24;

    void setColor(Color c);
    void createPixmap();
    void loadFont();
};

}
