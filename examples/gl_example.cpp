#include "waveform_viewer.hpp"
#include "vcd_parser.hpp"
#include "gl_surface.hpp"
#include <X11/Xlib.h>
#include <cstdlib>

using namespace wv;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        return 1;
    }
    
    VcdParser parser;
    if (!parser.parse(argv[1])) {
        return 1;
    }
    
    Display* dpy = XOpenDisplay(nullptr);
    if (!dpy) return 1;
    
    int screen = DefaultScreen(dpy);
    Window root = RootWindow(dpy, screen);
    
    int attribs[] = {
        GLX_RGBA,
        GLX_DEPTH_SIZE, 24,
        GLX_DOUBLEBUFFER,
        None
    };
    
    XVisualInfo* vi = glXChooseVisual(dpy, screen, attribs);
    if (!vi) {
        XCloseDisplay(dpy);
        return 1;
    }
    
    Colormap cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);
    
    XSetWindowAttributes swa;
    swa.colormap = cmap;
    swa.event_mask = ExposureMask | StructureNotifyMask | 
                     ButtonPressMask | ButtonReleaseMask | PointerMotionMask;
    
    Window win = XCreateWindow(dpy, root, 0, 0, 800, 600, 0,
        vi->depth, InputOutput, vi->visual,
        CWColormap | CWEventMask, &swa);
    
    XMapWindow(dpy, win);
    XStoreName(dpy, win, "Waveform Viewer (OpenGL)");
    
    Atom wmDelete = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(dpy, win, &wmDelete, 1);
    
    GlSurface surface;
    if (!surface.createContext(dpy, win, vi)) {
        XFree(vi);
        XDestroyWindow(dpy, win);
        XCloseDisplay(dpy);
        return 1;
    }
    surface.resize(800, 600);
    
    WaveformViewer viewer;
    viewer.setSize(800, 600);
    viewer.setData(&parser.data());
    
    XFree(vi);
    
    bool running = true;
    while (running) {
        while (XPending(dpy)) {
            XEvent ev;
            XNextEvent(dpy, &ev);
            
            switch (ev.type) {
                case Expose:
                    surface.beginFrame();
                    viewer.paint(&surface);
                    surface.endFrame();
                    break;
                case ConfigureNotify: {
                    i32 w = ev.xconfigure.width;
                    i32 h = ev.xconfigure.height;
                    viewer.setSize(w, h);
                    surface.resize(w, h);
                    viewer.setData(&parser.data());
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
                case ClientMessage:
                    if ((Atom)ev.xclient.data.l[0] == wmDelete)
                        running = false;
                    break;
            }
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
