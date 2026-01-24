#include "xcb_raster_device.hpp"
#include <memory>

namespace wv {

XcbRasterDevice::XcbRasterDevice(xcb_connection_t* conn, xcb_window_t win, i32 w, i32 h)
    : conn_(conn), win_(win), w_(w), h_(h) {
    createPixmap();
}

void XcbRasterDevice::createPixmap() {
    auto setup = xcb_get_setup(conn_);
    auto screen = xcb_setup_roots_iterator(setup).data;
    
    xcb_drawable_t drawable = win_ ? win_ : screen->root;
    depth_ = screen->root_depth;
    
    pixmap_ = xcb_generate_id(conn_);
    xcb_create_pixmap(conn_, depth_, pixmap_, drawable, w_, h_);
    
    gc_ = xcb_generate_id(conn_);
    u32 mask = XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES;
    u32 values[] = {screen->black_pixel, 0};
    xcb_create_gc(conn_, gc_, pixmap_, mask, values);
    
    loadFont();
}

void XcbRasterDevice::loadFont() {
    if (font_) return;
    font_ = xcb_generate_id(conn_);
    xcb_open_font(conn_, font_, 5, "fixed");
    xcb_change_gc(conn_, gc_, XCB_GC_FONT, &font_);
}

XcbRasterDevice::~XcbRasterDevice() {
    if (conn_) {
        if (font_) xcb_close_font(conn_, font_);
        if (pixmap_) xcb_free_pixmap(conn_, pixmap_);
        if (gc_) xcb_free_gc(conn_, gc_);
        font_ = 0;
        pixmap_ = 0;
        gc_ = 0;
    }
}

void XcbRasterDevice::resize(i32 w, i32 h) {
    if (w == w_ && h == h_) return;
    w_ = w;
    h_ = h;
    if (pixmap_) xcb_free_pixmap(conn_, pixmap_);
    if (gc_) xcb_free_gc(conn_, gc_);
    createPixmap();
}

void XcbRasterDevice::setColor(Color c) {
    u32 pixel = (c.r << 16) | (c.g << 8) | c.b;
    xcb_change_gc(conn_, gc_, XCB_GC_FOREGROUND, &pixel);
}

void XcbRasterDevice::fillRect(Rect r, Color c) {
    setColor(c);
    xcb_rectangle_t rect = {int16_t(r.x), int16_t(r.y), uint16_t(r.w), uint16_t(r.h)};
    xcb_poly_fill_rectangle(conn_, pixmap_, gc_, 1, &rect);
}

void XcbRasterDevice::strokeRect(Rect r, Color c, f32) {
    setColor(c);
    xcb_rectangle_t rect = {int16_t(r.x), int16_t(r.y), uint16_t(r.w), uint16_t(r.h)};
    xcb_poly_rectangle(conn_, pixmap_, gc_, 1, &rect);
}

void XcbRasterDevice::drawLine(Point p1, Point p2, Color c, f32) {
    setColor(c);
    xcb_point_t pts[2] = {{int16_t(p1.x), int16_t(p1.y)}, {int16_t(p2.x), int16_t(p2.y)}};
    xcb_poly_line(conn_, XCB_COORD_MODE_ORIGIN, pixmap_, gc_, 2, pts);
}

void XcbRasterDevice::drawPolyline(const Point* pts, i32 count, Color c, f32) {
    if (count < 2) return;
    setColor(c);
    
    auto xcb_pts = std::make_unique<xcb_point_t[]>(count);
    for (i32 i = 0; i < count; ++i) {
        xcb_pts[i].x = int16_t(pts[i].x);
        xcb_pts[i].y = int16_t(pts[i].y);
    }
    xcb_poly_line(conn_, XCB_COORD_MODE_ORIGIN, pixmap_, gc_, count, xcb_pts.get());
}

void XcbRasterDevice::drawText(Point p, std::string_view text, Color c) {
    setColor(c);
    xcb_image_text_8(conn_, text.size(), pixmap_, gc_, 
                     int16_t(p.x), int16_t(p.y), text.data());
}

void XcbRasterDevice::setClipRect(Rect r) {
    xcb_rectangle_t rect = {int16_t(r.x), int16_t(r.y), uint16_t(r.w), uint16_t(r.h)};
    u32 mask = XCB_GC_CLIP_ORIGIN_X | XCB_GC_CLIP_ORIGIN_Y;
    u32 values[] = {0, 0};
    xcb_change_gc(conn_, gc_, mask, values);
    xcb_set_clip_rectangles(conn_, XCB_CLIP_ORDERING_UNSORTED, gc_, 0, 0, 1, &rect);
}

void XcbRasterDevice::resetClip() {
    u32 mask = XCB_GC_CLIP_MASK;
    u32 values[] = {XCB_NONE};
    xcb_change_gc(conn_, gc_, mask, values);
}

void XcbRasterDevice::beginFrame() {
}

void XcbRasterDevice::endFrame() {
}

void XcbRasterDevice::present() {
    if (win_) {
        xcb_copy_area(conn_, pixmap_, win_, gc_, 0, 0, 0, 0, w_, h_);
        xcb_flush(conn_);
    }
}
}
