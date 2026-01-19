#include "waveform_viewer.hpp"
#include "vcd_parser.hpp"
#include "xcb_surface.hpp"
#include <xcb/xcb.h>

using namespace wv;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        return 1;
    }
    
    VcdParser parser;
    if (!parser.parse(argv[1])) {
        return 1;
    }
    
    xcb_connection_t* conn = xcb_connect(nullptr, nullptr);
    auto setup = xcb_get_setup(conn);
    auto screen = xcb_setup_roots_iterator(setup).data;
    
    xcb_window_t win = xcb_generate_id(conn);
    u32 mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    u32 values[] = {screen->black_pixel, 
        XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_STRUCTURE_NOTIFY |
        XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_POINTER_MOTION};
    xcb_create_window(conn, XCB_COPY_FROM_PARENT, win, screen->root, 0, 0, 800, 600, 0,
        XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, mask, values);
    xcb_map_window(conn, win);
    xcb_flush(conn);
    
    XcbSurface surface;
    surface.init(conn, 800, 600);
    surface.setWindow(win);
    
    WaveformViewer viewer;
    viewer.setSize(800, 600);
    viewer.setData(&parser.data());
    
    bool running = true;
    while (running) {
        xcb_generic_event_t* ev = xcb_wait_for_event(conn);
        if (!ev) break;
        
        switch (ev->response_type & ~0x80) {
            case XCB_EXPOSE:
                surface.beginFrame();
                viewer.paint(&surface);
                surface.endFrame();
                break;
            case XCB_CONFIGURE_NOTIFY: {
                auto* cfg = reinterpret_cast<xcb_configure_notify_event_t*>(ev);
                viewer.setSize(cfg->width, cfg->height);
                surface.resize(cfg->width, cfg->height);
                viewer.setData(&parser.data());
                surface.beginFrame();
                viewer.paint(&surface);
                surface.endFrame();
                break;
            }
            case XCB_BUTTON_PRESS: {
                auto* btn = reinterpret_cast<xcb_button_press_event_t*>(ev);
                if (btn->detail == 1)
                    viewer.mouseDown(btn->event_x, btn->event_y);
                else if (btn->detail == 4)
                    viewer.mouseWheel(btn->event_x, 1);
                else if (btn->detail == 5)
                    viewer.mouseWheel(btn->event_x, -1);
                break;
            }
            case XCB_BUTTON_RELEASE: {
                auto* btn = reinterpret_cast<xcb_button_release_event_t*>(ev);
                if (btn->detail == 1)
                    viewer.mouseUp();
                break;
            }
            case XCB_MOTION_NOTIFY: {
                auto* motion = reinterpret_cast<xcb_motion_notify_event_t*>(ev);
                viewer.mouseMove(motion->event_x, motion->event_y);
                break;
            }
        }
        free(ev);
        
        if (viewer.needsRepaint()) {
            surface.beginFrame();
            viewer.paint(&surface);
            surface.endFrame();
            viewer.clearRepaintFlag();
        }
    }
    
    surface.release();
    xcb_destroy_window(conn, win);
    xcb_disconnect(conn);
    return 0;
}
