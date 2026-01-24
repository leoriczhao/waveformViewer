#pragma once

#include "context.hpp"
#include "glyph_cache.hpp"
#include "recording.hpp"
#include <X11/Xlib.h>
#include <GL/glx.h>
#include <memory>
#include <vector>

namespace wv {

class GlContext : public Context {
public:
    static std::unique_ptr<GlContext> Create(Display* dpy, Window win);

    ~GlContext() override;

    bool init(i32 w, i32 h) override;
    void beginFrame() override;
    void resize(i32 w, i32 h) override;
    void submit(const Recording& recording) override;
    void present() override;
    void setGlyphCache(GlyphCache* cache) override;

    static XVisualInfo* chooseVisual(Display* dpy);

private:
    GlContext(Display* dpy, Window win);

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

    u32 textVao_ = 0;
    u32 textVbo_ = 0;
    u32 textShader_ = 0;
    i32 textResLoc_ = -1;

    GlyphCache* glyphCache_ = nullptr;
    u32 glyphAtlasTex_ = 0;

    bool initGL();
    void flushLines();
    void flushTriangles();
    u32 compileShader(u32 type, const char* src);
    u32 createProgram(const char* vs, const char* fs);
    void initTextPipeline();
    void updateGlyphAtlas();
    void applyOp(const DrawOp& op);
    void setClipRect(Rect r);
    void clearClip();
};

}
