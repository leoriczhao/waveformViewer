#include "waveform_viewer.hpp"
#include "vcd_parser.hpp"
#include "surface.hpp"
#include "glyph_cache.hpp"
#include <xcb/xcb.h>
#include <cstdio>
#include <cstdint>
#include <cstring>

#if WAVEFORM_HAS_GL
#include "gl_context.hpp"
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <GL/glx.h>
#endif

using namespace wv;

static bool parseArgs(int argc, char* argv[], bool& useGpu, const char*& path) {
    useGpu = false;
    path = nullptr;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--gpu") == 0) {
            useGpu = true;
        } else {
            path = argv[i];
        }
    }
    return path != nullptr;
}

static int runXcb(const char* path, GlyphCache& glyphCache) {
    VcdParser parser;
    if (!parser.parse(path)) return 1;

    xcb_connection_t* conn = xcb_connect(nullptr, nullptr);
    auto setup = xcb_get_setup(conn);
    auto screen = xcb_setup_roots_iterator(setup).data;

    xcb_window_t win = xcb_generate_id(conn);
    u32 mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    u32 values[] = {screen->black_pixel,
        XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_STRUCTURE_NOTIFY |
        XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE |
        XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_KEY_PRESS};
    xcb_create_window(conn, XCB_COPY_FROM_PARENT, win, screen->root, 0, 0, 800, 600, 0,
        XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, mask, values);
    xcb_map_window(conn, win);
    xcb_flush(conn);

    auto surface = Surface::MakeRaster(conn, static_cast<WindowID>(win), 800, 600);
    if (!surface) {
        xcb_destroy_window(conn, win);
        xcb_disconnect(conn);
        return 1;
    }
    surface->setGlyphCache(&glyphCache);

    WaveformViewer viewer;
    viewer.setSize(800, 600);
    viewer.setData(&parser.data());

    bool running = true;
    while (running) {
        xcb_generic_event_t* ev = xcb_wait_for_event(conn);
        if (!ev) break;

        switch (ev->response_type & ~0x80) {
            case XCB_EXPOSE:
                surface->beginFrame();
                viewer.paint(surface.get());
                surface->endFrame();
                surface->present();
                break;
            case XCB_CONFIGURE_NOTIFY: {
                auto* cfg = reinterpret_cast<xcb_configure_notify_event_t*>(ev);
                viewer.setSize(cfg->width, cfg->height);
                surface->resize(cfg->width, cfg->height);
                viewer.setData(&parser.data());
                surface->beginFrame();
                viewer.paint(surface.get());
                surface->endFrame();
                surface->present();
                break;
            }
            case XCB_BUTTON_PRESS: {
                auto* btn = reinterpret_cast<xcb_button_press_event_t*>(ev);
                if (btn->detail == 1)
                    viewer.mouseDown(btn->event_x, btn->event_y);
                else if (btn->detail == 3)
                    viewer.setCursorFromX(btn->event_x);
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
            case XCB_KEY_PRESS: {
                auto* key = reinterpret_cast<xcb_key_press_event_t*>(ev);
                viewer.keyPress(key->detail);
                break;
            }
        }
        free(ev);

        if (viewer.needsRepaint()) {
            surface->beginFrame();
            viewer.paint(surface.get());
            surface->endFrame();
            surface->present();
            viewer.clearRepaintFlag();
        }
    }

    xcb_destroy_window(conn, win);
    xcb_disconnect(conn);
    return 0;
}

#if WAVEFORM_HAS_GL
static int runGl(const char* path, GlyphCache& glyphCache) {
    VcdParser parser;
    if (!parser.parse(path)) return 1;

    Display* dpy = XOpenDisplay(nullptr);
    if (!dpy) return 1;

    int screen = DefaultScreen(dpy);
    XVisualInfo* vi = GlContext::chooseVisual(dpy);
    if (!vi) { XCloseDisplay(dpy); return 1; }

    Colormap cmap = XCreateColormap(dpy, RootWindow(dpy, screen), vi->visual, AllocNone);
    XSetWindowAttributes swa = {};
    swa.colormap = cmap;
    swa.event_mask = ExposureMask | StructureNotifyMask | ButtonPressMask |
                     ButtonReleaseMask | PointerMotionMask | KeyPressMask;

    Window win = XCreateWindow(dpy, RootWindow(dpy, screen), 0, 0, 800, 600, 0,
        vi->depth, InputOutput, vi->visual, CWColormap | CWEventMask, &swa);
    XStoreName(dpy, win, "Waveform Viewer (OpenGL)");
    XMapWindow(dpy, win);
    XFree(vi);

    auto ctx = GlContext::Create(dpy, win);
    auto surface = Surface::MakeGpu(std::move(ctx), 800, 600);
    if (!surface) {
        XDestroyWindow(dpy, win);
        XCloseDisplay(dpy);
        return 1;
    }
    surface->setGlyphCache(&glyphCache);

    WaveformViewer viewer;
    viewer.setSize(800, 600);
    viewer.setData(&parser.data());

    bool running = true;
    while (running) {
        XEvent ev;
        XNextEvent(dpy, &ev);

        switch (ev.type) {
            case Expose:
                if (ev.xexpose.count == 0) {
                    surface->beginFrame();
                    viewer.paint(surface.get());
                    surface->endFrame();
                    surface->present();
                }
                break;
            case ConfigureNotify: {
                i32 w = ev.xconfigure.width;
                i32 h = ev.xconfigure.height;
                viewer.setSize(w, h);
                surface->resize(w, h);
                viewer.setData(&parser.data());
                surface->beginFrame();
                viewer.paint(surface.get());
                surface->endFrame();
                surface->present();
                break;
            }
            case ButtonPress:
                if (ev.xbutton.button == 1)
                    viewer.mouseDown(ev.xbutton.x, ev.xbutton.y);
                else if (ev.xbutton.button == 4)
                    viewer.mouseWheel(ev.xbutton.x, 1);
                else if (ev.xbutton.button == 5)
                    viewer.mouseWheel(ev.xbutton.x, -1);
                break;
            case ButtonRelease:
                if (ev.xbutton.button == 1)
                    viewer.mouseUp();
                break;
            case MotionNotify:
                viewer.mouseMove(ev.xmotion.x, ev.xmotion.y);
                break;
            case KeyPress:
                if (XLookupKeysym(&ev.xkey, 0) == XK_Escape)
                    running = false;
                break;
        }

        if (viewer.needsRepaint()) {
            surface->beginFrame();
            viewer.paint(surface.get());
            surface->endFrame();
            surface->present();
            viewer.clearRepaintFlag();
        }
    }

    XDestroyWindow(dpy, win);
    XCloseDisplay(dpy);
    return 0;
}
#endif

int main(int argc, char* argv[]) {
    bool useGpu = false;
    const char* path = nullptr;
    if (!parseArgs(argc, argv, useGpu, path)) {
        return 1;
    }

    GlyphCache glyphCache;
    if (!glyphCache.init("/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf", 13.0f)) {
        if (!glyphCache.init("/usr/share/fonts/TTF/DejaVuSansMono.ttf", 13.0f)) {
            fprintf(stderr, "Failed to load font\n");
        }
    }

#if WAVEFORM_HAS_GL
    if (useGpu) {
        int result = runGl(path, glyphCache);
        glyphCache.release();
        return result;
    }
#else
    if (useGpu) {
        fprintf(stderr, "GPU backend not available, using XCB\n");
    }
#endif

    int result = runXcb(path, glyphCache);
    glyphCache.release();
    return result;
}
