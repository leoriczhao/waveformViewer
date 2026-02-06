#include "waveform_viewer.hpp"
#include "vcd_parser.hpp"
#include "surface.hpp"
#include "glyph_cache.hpp"
#include <xcb/xcb.h>
#include <cstdio>
#include <cstdint>
#include <cstring>

#if WAVEFORM_HAS_GL
#include "context.hpp"
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

// Helper: blit Pixmap to XCB window
static void blitToXcb(xcb_connection_t* conn, xcb_window_t win, xcb_gcontext_t gc,
                       const wv::Pixmap& pixmap) {
    xcb_put_image(conn, XCB_IMAGE_FORMAT_Z_PIXMAP, win, gc,
                  pixmap.width(), pixmap.height(), 0, 0, 0, 24,
                  pixmap.info().computeByteSize(),
                  pixmap.addr8());
    xcb_flush(conn);
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

    // Create GC for blitting
    xcb_gcontext_t gc = xcb_generate_id(conn);
    u32 gcMask = XCB_GC_GRAPHICS_EXPOSURES;
    u32 gcValues[] = {0};
    xcb_create_gc(conn, gc, win, gcMask, gcValues);

    // Create raster surface (BGRA for XCB)
    auto surface = Surface::MakeRaster(800, 600, PixelFormat::BGRA8888);
    if (!surface) {
        xcb_destroy_window(conn, win);
        xcb_disconnect(conn);
        return 1;
    }
    surface->setGlyphCache(&glyphCache);

    WaveformViewer viewer;
    viewer.setSize(800, 600);
    viewer.setData(&parser.data());

    auto renderAndBlit = [&]() {
        surface->beginFrame();
        viewer.paint(surface.get());
        surface->endFrame();
        surface->flush();
        blitToXcb(conn, win, gc, *surface->peekPixels());
    };

    bool running = true;
    while (running) {
        xcb_generic_event_t* ev = xcb_wait_for_event(conn);
        if (!ev) break;

        switch (ev->response_type & ~0x80) {
            case XCB_EXPOSE:
                renderAndBlit();
                break;
            case XCB_CONFIGURE_NOTIFY: {
                auto* cfg = reinterpret_cast<xcb_configure_notify_event_t*>(ev);
                viewer.setSize(cfg->width, cfg->height);
                surface->resize(cfg->width, cfg->height);
                viewer.setData(&parser.data());
                renderAndBlit();
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
            renderAndBlit();
            viewer.clearRepaintFlag();
        }
    }

    xcb_free_gc(conn, gc);
    xcb_destroy_window(conn, win);
    xcb_disconnect(conn);
    return 0;
}

#if WAVEFORM_HAS_GL
static XVisualInfo* chooseGlVisual(Display* dpy) {
    int attribs[] = {GLX_RGBA, GLX_DOUBLEBUFFER, GLX_DEPTH_SIZE, 0, None};
    return glXChooseVisual(dpy, DefaultScreen(dpy), attribs);
}

static int runGl(const char* path, GlyphCache& glyphCache) {
    VcdParser parser;
    if (!parser.parse(path)) return 1;

    Display* dpy = XOpenDisplay(nullptr);
    if (!dpy) return 1;

    int screen = DefaultScreen(dpy);
    XVisualInfo* vi = chooseGlVisual(dpy);
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

    // Host creates and activates GL context
    GLXContext glxCtx = glXCreateContext(dpy, chooseGlVisual(dpy), nullptr, True);
    if (!glxCtx) {
        XDestroyWindow(dpy, win);
        XCloseDisplay(dpy);
        return 1;
    }
    glXMakeCurrent(dpy, win, glxCtx);

    // waveformViewer only needs to know GL is current
    auto ctx = Context::MakeGL();
    auto surface = Surface::MakeGpu(std::move(ctx), 800, 600);
    if (!surface) {
        glXDestroyContext(dpy, glxCtx);
        XDestroyWindow(dpy, win);
        XCloseDisplay(dpy);
        return 1;
    }
    surface->setGlyphCache(&glyphCache);

    WaveformViewer viewer;
    viewer.setSize(800, 600);
    viewer.setData(&parser.data());

    auto renderAndPresent = [&]() {
        surface->beginFrame();
        viewer.paint(surface.get());
        surface->endFrame();
        surface->flush();
        glXSwapBuffers(dpy, win);  // Host presents
    };

    bool running = true;
    while (running) {
        XEvent ev;
        XNextEvent(dpy, &ev);

        switch (ev.type) {
            case Expose:
                if (ev.xexpose.count == 0) {
                    renderAndPresent();
                }
                break;
            case ConfigureNotify: {
                i32 w = ev.xconfigure.width;
                i32 h = ev.xconfigure.height;
                viewer.setSize(w, h);
                surface->resize(w, h);
                viewer.setData(&parser.data());
                renderAndPresent();
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
            renderAndPresent();
            viewer.clearRepaintFlag();
        }
    }

    surface.reset();
    glXMakeCurrent(dpy, None, nullptr);
    glXDestroyContext(dpy, glxCtx);
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
        fprintf(stderr, "GPU backend not available, using software raster\n");
    }
#endif

    int result = runXcb(path, glyphCache);
    glyphCache.release();
    return result;
}
