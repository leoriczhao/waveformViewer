#pragma once

#include "surface.hpp"
#include <vector>
#include <GL/glew.h>
#include <GL/glx.h>
#include <X11/Xlib.h>

namespace wv {

class GlSurface : public Surface {
public:
    void init(SurfaceID sid, i32 w, i32 h) override;
    void resize(i32 w, i32 h) override;
    void release() override;
    
    void beginFrame() override;
    void endFrame() override;
    
    void fillRect(Rect r, Color c) override;
    void strokeRect(Rect r, Color c, f32 width) override;
    void drawLine(Point p1, Point p2, Color c, f32 width) override;
    void drawPolyline(const Point* pts, i32 count, Color c, f32 width) override;
    void drawText(Point p, std::string_view text, Color c) override;
    
    void setClip(Rect r) override;
    void clearClip() override;
    
    void setWindow(Window win) { win_ = win; }
    bool createContext(Display* dpy, Window win, XVisualInfo* vi = nullptr);
    
private:
    Display* dpy_ = nullptr;
    Window win_ = 0;
    GLXContext ctx_ = nullptr;
    i32 w_ = 0, h_ = 0;
    
    struct Vertex {
        f32 x, y;
        u8 r, g, b, a;
    };
    
    std::vector<Vertex> lineVertices_;
    std::vector<Vertex> triVertices_;
    
    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    GLuint shader_ = 0;
    GLint resolutionLoc_ = -1;
    
    bool initGL();
    void flushLines();
    void flushTriangles();
    GLuint compileShader(GLenum type, const char* src);
    GLuint createProgram();
};

}
