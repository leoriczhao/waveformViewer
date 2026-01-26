#include "surface.hpp"
#include "xcb_raster_device.hpp"

namespace wv {

std::unique_ptr<Surface> Surface::MakeRaster(SurfaceID sid, WindowID win, i32 w, i32 h) {
    auto* conn = static_cast<xcb_connection_t*>(sid);
    auto window = static_cast<xcb_window_t>(win);
    auto device = std::make_unique<XcbRasterDevice>(conn, window, w, h);
    return std::unique_ptr<Surface>(new Surface(std::move(device), nullptr));
}

}
