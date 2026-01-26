#pragma once

#include "context.hpp"
#include "glyph_cache.hpp"
#include "recording.hpp"
#include <X11/Xlib.h>
#include <GL/glx.h>
#include <memory>
#include <vector>
#include <unordered_map>
#include <array>

namespace wv {

class GlStateCache {
public:
    void invalidate();  // Reset all cached state
    
    // Returns true if state actually changed
    bool useProgram(u32 program);
    bool bindVao(u32 vao);
    bool bindVbo(u32 vbo);
    bool setScissor(bool enabled, i32 x, i32 y, i32 w, i32 h);
    bool setUniform2f(i32 loc, f32 x, f32 y);
    bool setUniform4f(i32 loc, f32 x, f32 y, f32 z, f32 w);
    bool setUniform1i(i32 loc, i32 v);
    bool bindTexture(u32 unit, u32 texture);
    bool setBlend(bool enabled);
    
private:
    u32 currentProgram_ = 0;
    u32 currentVao_ = 0;
    u32 currentVbo_ = 0;
    bool scissorEnabled_ = false;
    i32 scissorX_ = 0, scissorY_ = 0, scissorW_ = 0, scissorH_ = 0;
    bool blendEnabled_ = false;
    
    // Uniform cache: location -> value
    struct Uniform2f { f32 x, y; };
    struct Uniform4f { f32 x, y, z, w; };
    std::unordered_map<i32, Uniform2f> uniforms2f_;
    std::unordered_map<i32, Uniform4f> uniforms4f_;
    std::unordered_map<i32, i32> uniforms1i_;
    
    std::array<u32, 8> boundTextures_ = {};
};

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

    struct TextVertex {
        f32 x, y;    // position
        f32 u, v;    // texture coords
    };

    std::vector<Vertex> lineVertices_;
    std::vector<Vertex> triVertices_;
    std::vector<TextVertex> textVertices_;
    Color currentTextColor_ = {};

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

    GlStateCache stateCache_;

    bool initGL();
    void flushLines();
    void flushTriangles();
    void flushText();
    u32 compileShader(u32 type, const char* src);
    u32 createProgram(const char* vs, const char* fs);
    void initTextPipeline();
    void updateGlyphAtlas();
    void applyOp(const CompactDrawOp& op, const DrawOpArena& arena);
    void setClipRect(Rect r);
    void clearClip();
};

}
