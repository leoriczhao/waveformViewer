#include "waveform_viewer.hpp"
#include "vcd_parser.hpp"
#include "gl_surface.hpp"
#include <X11/Xlib.h>
#include <GL/glx.h>

using namespace wv;

int main(int argc, char* argv[]) {
    if (argc < 2) return 1;
    
    VcdParser parser;
    if (!parser.parse(argv[1])) return 1;
    
    Display* dpy = XOpenDisplay(nullptr);
    if (!dpy) return 1;
    
    int screen = DefaultScreen(dpy);
    XVisualInfo* vi = GlSurface::chooseVisual(dpy);
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
    
    GlSurface surface;
    surface.init(dpy, 800, 600);
    surface.setWindow(win, vi);
    XFree(vi);
    
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
                    surface.beginFrame();
                    viewer.paint(&surface);
                    surface.endFrame();
                }
                break;
            case ConfigureNotify: {
                i32 w = ev.xconfigure.width;
                i32 h = ev.xconfigure.height;
                viewer.setSize(w, h);
                surface.resize(w, h);
                viewer.setData(&parser.data());
                surface.beginFrame();
                viewer.paint(&surface);
                surface.endFrame();
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
            surface.beginFrame();
            viewer.paint(&surface);
            surface.endFrame();
            viewer.clearRepaintFlag();
        }
    }
    
    surface.release();
    XDestroyWindow(dpy, win);
    XCloseDisplay(dpy);
    return 0;
}
