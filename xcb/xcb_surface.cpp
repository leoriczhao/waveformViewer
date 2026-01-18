#include "xcb_surface.hpp"
#include <memory>

namespace wv {

void XcbSurface::init(SurfaceID sid, i32 w, i32 h) {
    conn_ = static_cast<xcb_connection_t*>(sid);
    w_ = w; h_ = h;
    
    auto setup = xcb_get_setup(conn_);
    auto screen = xcb_setup_roots_iterator(setup).data;
    
    pixmap_ = xcb_generate_id(conn_);
    xcb_create_pixmap(conn_, screen->root_depth, pixmap_, screen->root, w, h);
    
    gc_ = xcb_generate_id(conn_);
    u32 mask = XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES;
    u32 values[] = {screen->black_pixel, 0};
    xcb_create_gc(conn_, gc_, pixmap_, mask, values);
}

void XcbSurface::release() {
    if (conn_) {
        if (pixmap_) xcb_free_pixmap(conn_, pixmap_);
        if (gc_) xcb_free_gc(conn_, gc_);
        pixmap_ = 0;
        gc_ = 0;
    }
}

void XcbSurface::setColor(Color c) {
    u32 pixel = (c.r << 16) | (c.g << 8) | c.b;
    xcb_change_gc(conn_, gc_, XCB_GC_FOREGROUND, &pixel);
}

void XcbSurface::fillRect(Rect r, Color c) {
    setColor(c);
    xcb_rectangle_t rect = {int16_t(r.x), int16_t(r.y), uint16_t(r.w), uint16_t(r.h)};
    xcb_poly_fill_rectangle(conn_, pixmap_, gc_, 1, &rect);
}

void XcbSurface::strokeRect(Rect r, Color c, f32) {
    setColor(c);
    xcb_rectangle_t rect = {int16_t(r.x), int16_t(r.y), uint16_t(r.w), uint16_t(r.h)};
    xcb_poly_rectangle(conn_, pixmap_, gc_, 1, &rect);
}

void XcbSurface::drawLine(Point p1, Point p2, Color c, f32) {
    setColor(c);
    xcb_point_t pts[2] = {{int16_t(p1.x), int16_t(p1.y)}, {int16_t(p2.x), int16_t(p2.y)}};
    xcb_poly_line(conn_, XCB_COORD_MODE_ORIGIN, pixmap_, gc_, 2, pts);
}

void XcbSurface::drawPolyline(const Point* pts, i32 count, Color c, f32) {
    if (count < 2) return;
    setColor(c);
    
    auto xcb_pts = std::make_unique<xcb_point_t[]>(count);
    for (i32 i = 0; i < count; ++i) {
        xcb_pts[i].x = int16_t(pts[i].x);
        xcb_pts[i].y = int16_t(pts[i].y);
    }
    xcb_poly_line(conn_, XCB_COORD_MODE_ORIGIN, pixmap_, gc_, count, xcb_pts.get());
}

void XcbSurface::drawText(Point, std::string_view, Color) {}

void XcbSurface::setClip(Rect r) {
    xcb_rectangle_t rect = {int16_t(r.x), int16_t(r.y), uint16_t(r.w), uint16_t(r.h)};
    xcb_set_clip_rectangles(conn_, XCB_CLIP_ORDERING_UNSORTED, gc_, 0, 0, 1, &rect);
}

void XcbSurface::clearClip() {
    xcb_set_clip_rectangles(conn_, XCB_CLIP_ORDERING_UNSORTED, gc_, 0, 0, 0, nullptr);
}

void XcbSurface::copyToWindow(xcb_window_t win) {
    xcb_copy_area(conn_, pixmap_, win, gc_, 0, 0, 0, 0, w_, h_);
    xcb_flush(conn_);
}

}
