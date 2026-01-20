#pragma once

#include "surface.hpp"
#include <X11/Xlib.h>
#include <GL/glx.h>
#include <vector>

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
    
    bool setWindow(Window win, XVisualInfo* vi = nullptr);
    static XVisualInfo* chooseVisual(Display* dpy);
    
private:
    Display* display_ = nullptr;
    Window win_ = 0;
    GLXContext ctx_ = nullptr;
    i32 w_ = 0, h_ = 0;
    
    struct Vertex {
        f32 x, y;
        u8 r, g, b, a;
    };
    
    std::vector<Vertex> lineVertices_;
    std::vector<Vertex> triVertices_;
    
    u32 vao_ = 0;
    u32 vbo_ = 0;
    u32 shader_ = 0;
    i32 resolutionLoc_ = -1;
    
    u32 fontTex_ = 0;
    u32 textVao_ = 0;
    u32 textVbo_ = 0;
    u32 textShader_ = 0;
    i32 textResLoc_ = -1;
    
    bool initGL();
    void flushLines();
    void flushTriangles();
    u32 compileShader(u32 type, const char* src);
    u32 createProgram(const char* vs, const char* fs);
    void initFont();
};

}
