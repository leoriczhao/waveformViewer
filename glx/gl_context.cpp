#include "gl_context.hpp"
#include "glad/glad.h"
#include <cstdio>

namespace wv {

static const char* vertexShaderSrc = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec4 aColor;
out vec4 vColor;
uniform vec2 uResolution;
void main() {
    vec2 pos = (aPos / uResolution) * 2.0 - 1.0;
    pos.y = -pos.y;
    gl_Position = vec4(pos, 0.0, 1.0);
    vColor = aColor;
}
)";

static const char* fragmentShaderSrc = R"(
#version 330 core
in vec4 vColor;
out vec4 fragColor;
void main() {
    fragColor = vColor;
}
)";

static const char* textVertexShaderSrc = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUv;
out vec2 vUv;
uniform vec2 uResolution;
void main() {
    vec2 pos = (aPos / uResolution) * 2.0 - 1.0;
    pos.y = -pos.y;
    gl_Position = vec4(pos, 0.0, 1.0);
    vUv = aUv;
}
)";

static const char* textFragmentShaderSrc = R"(
#version 330 core
in vec2 vUv;
out vec4 fragColor;
uniform sampler2D uTex;
uniform vec4 uColor;
void main() {
    float a = texture(uTex, vUv).r;
    fragColor = vec4(uColor.rgb, uColor.a * a);
}
)";

GlContext::GlContext(Display* dpy, Window win) : display_(dpy), win_(win) {
}

GlContext::~GlContext() {
    if (ctx_) {
        glXMakeCurrent(display_, win_, ctx_);
    }
    if (textShader_) {
        glDeleteProgram(textShader_);
        textShader_ = 0;
    }
    if (textVbo_) {
        glDeleteBuffers(1, &textVbo_);
        textVbo_ = 0;
    }
    if (textVao_) {
        glDeleteVertexArrays(1, &textVao_);
        textVao_ = 0;
    }
    if (glyphAtlasTex_) {
        glDeleteTextures(1, &glyphAtlasTex_);
        glyphAtlasTex_ = 0;
    }
    if (shader_) {
        glDeleteProgram(shader_);
        shader_ = 0;
    }
    if (vbo_) {
        glDeleteBuffers(1, &vbo_);
        vbo_ = 0;
    }
    if (vao_) {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }
    if (ctx_) {
        glXMakeCurrent(display_, None, nullptr);
        glXDestroyContext(display_, ctx_);
        ctx_ = nullptr;
    }
}

std::unique_ptr<GlContext> GlContext::Create(Display* dpy, Window win) {
    return std::unique_ptr<GlContext>(new GlContext(dpy, win));
}

XVisualInfo* GlContext::chooseVisual(Display* dpy) {
    int attribs[] = {GLX_RGBA, GLX_DOUBLEBUFFER, GLX_DEPTH_SIZE, 0, None};
    return glXChooseVisual(dpy, DefaultScreen(dpy), attribs);
}

bool GlContext::init(i32 w, i32 h) {
    w_ = w;
    h_ = h;
    
    XVisualInfo* vi = chooseVisual(display_);
    if (!vi) return false;
    
    ctx_ = glXCreateContext(display_, vi, nullptr, True);
    XFree(vi);
    if (!ctx_) return false;
    if (!glXMakeCurrent(display_, win_, ctx_)) return false;
    if (!gladLoadGL()) return false;
    
    return initGL();
}

void GlContext::resize(i32 w, i32 h) {
    w_ = w;
    h_ = h;
    glViewport(0, 0, w, h);
}

void GlContext::beginFrame() {
    if (!ctx_) return;
    glXMakeCurrent(display_, win_, ctx_);
    glViewport(0, 0, w_, h_);
    glDisable(GL_SCISSOR_TEST);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

bool GlContext::initGL() {
    shader_ = createProgram(vertexShaderSrc, fragmentShaderSrc);
    if (!shader_) return false;
    resolutionLoc_ = glGetUniformLocation(shader_, "uResolution");
    
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    
    glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (void*)(2 * sizeof(f32)));
    glEnableVertexAttribArray(1);
    
    glBindVertexArray(0);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    initTextPipeline();
    
    return true;
}

u32 GlContext::compileShader(u32 type, const char* src) {
    u32 shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    
    i32 success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(shader, 512, nullptr, log);
        fprintf(stderr, "Shader compile error: %s\n", log);
        glDeleteShader(shader);
        return 0;
    }
    
    return shader;
}

u32 GlContext::createProgram(const char* vs, const char* fs) {
    u32 v = compileShader(GL_VERTEX_SHADER, vs);
    if (!v) return 0;
    u32 f = compileShader(GL_FRAGMENT_SHADER, fs);
    if (!f) {
        glDeleteShader(v);
        return 0;
    }
    u32 prog = glCreateProgram();
    glAttachShader(prog, v);
    glAttachShader(prog, f);
    glLinkProgram(prog);
    
    i32 success;
    glGetProgramiv(prog, GL_LINK_STATUS, &success);
    glDeleteShader(v);
    glDeleteShader(f);
    if (!success) {
        char log[512];
        glGetProgramInfoLog(prog, 512, nullptr, log);
        fprintf(stderr, "Program link error: %s\n", log);
        glDeleteProgram(prog);
        return 0;
    }
    
    return prog;
}

void GlContext::initTextPipeline() {
    textShader_ = createProgram(textVertexShaderSrc, textFragmentShaderSrc);
    if (!textShader_) return;
    textResLoc_ = glGetUniformLocation(textShader_, "uResolution");
    
    glGenVertexArrays(1, &textVao_);
    glGenBuffers(1, &textVbo_);
    glBindVertexArray(textVao_);
    glBindBuffer(GL_ARRAY_BUFFER, textVbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(f32) * 24, nullptr, GL_STREAM_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(f32) * 4, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(f32) * 4, (void*)(2 * sizeof(f32)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

void GlContext::flushLines() {
    if (lineVertices_.empty()) return;
    glUseProgram(shader_);
    glUniform2f(resolutionLoc_, f32(w_), f32(h_));
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, lineVertices_.size() * sizeof(Vertex), lineVertices_.data(), GL_STREAM_DRAW);
    glDrawArrays(GL_LINES, 0, i32(lineVertices_.size()));
    glBindVertexArray(0);
    lineVertices_.clear();
}

void GlContext::flushTriangles() {
    if (triVertices_.empty()) return;
    glUseProgram(shader_);
    glUniform2f(resolutionLoc_, f32(w_), f32(h_));
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, triVertices_.size() * sizeof(Vertex), triVertices_.data(), GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, i32(triVertices_.size()));
    glBindVertexArray(0);
    triVertices_.clear();
}

void GlContext::applyOp(const DrawOp& op) {
    switch (op.type) {
        case DrawOp::Type::FillRect: {
            Vertex v0 = {op.rect.x, op.rect.y, op.color.r, op.color.g, op.color.b, op.color.a};
            Vertex v1 = {op.rect.x + op.rect.w, op.rect.y, op.color.r, op.color.g, op.color.b, op.color.a};
            Vertex v2 = {op.rect.x + op.rect.w, op.rect.y + op.rect.h, op.color.r, op.color.g, op.color.b, op.color.a};
            Vertex v3 = {op.rect.x, op.rect.y + op.rect.h, op.color.r, op.color.g, op.color.b, op.color.a};
            triVertices_.push_back(v0);
            triVertices_.push_back(v1);
            triVertices_.push_back(v2);
            triVertices_.push_back(v0);
            triVertices_.push_back(v2);
            triVertices_.push_back(v3);
            break;
        }
        case DrawOp::Type::StrokeRect: {
            Point p1 = {op.rect.x, op.rect.y};
            Point p2 = {op.rect.x + op.rect.w, op.rect.y};
            Point p3 = {op.rect.x + op.rect.w, op.rect.y + op.rect.h};
            Point p4 = {op.rect.x, op.rect.y + op.rect.h};
            lineVertices_.push_back({p1.x, p1.y, op.color.r, op.color.g, op.color.b, op.color.a});
            lineVertices_.push_back({p2.x, p2.y, op.color.r, op.color.g, op.color.b, op.color.a});
            lineVertices_.push_back({p2.x, p2.y, op.color.r, op.color.g, op.color.b, op.color.a});
            lineVertices_.push_back({p3.x, p3.y, op.color.r, op.color.g, op.color.b, op.color.a});
            lineVertices_.push_back({p3.x, p3.y, op.color.r, op.color.g, op.color.b, op.color.a});
            lineVertices_.push_back({p4.x, p4.y, op.color.r, op.color.g, op.color.b, op.color.a});
            lineVertices_.push_back({p4.x, p4.y, op.color.r, op.color.g, op.color.b, op.color.a});
            lineVertices_.push_back({p1.x, p1.y, op.color.r, op.color.g, op.color.b, op.color.a});
            break;
        }
        case DrawOp::Type::Line: {
            lineVertices_.push_back({op.p1.x, op.p1.y, op.color.r, op.color.g, op.color.b, op.color.a});
            lineVertices_.push_back({op.p2.x, op.p2.y, op.color.r, op.color.g, op.color.b, op.color.a});
            break;
        }
        case DrawOp::Type::Polyline: {
            for (i32 i = 0; i + 1 < static_cast<i32>(op.points.size()); ++i) {
                Point p1 = op.points[i];
                Point p2 = op.points[i + 1];
                lineVertices_.push_back({p1.x, p1.y, op.color.r, op.color.g, op.color.b, op.color.a});
                lineVertices_.push_back({p2.x, p2.y, op.color.r, op.color.g, op.color.b, op.color.a});
            }
            break;
        }
        case DrawOp::Type::Text: {
            flushTriangles();
            flushLines();
            updateGlyphAtlas();
            if (!glyphCache_ || !glyphAtlasTex_ || !textShader_) break;
            
            glUseProgram(textShader_);
            glUniform2f(textResLoc_, f32(w_), f32(h_));
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, glyphAtlasTex_);
            glUniform1i(glGetUniformLocation(textShader_, "uTex"), 0);
            i32 colorLoc = glGetUniformLocation(textShader_, "uColor");
            glUniform4f(colorLoc, op.color.r / 255.0f, op.color.g / 255.0f,
                        op.color.b / 255.0f, op.color.a / 255.0f);
            
            f32 penX = op.p1.x;
            i32 baseline = i32(op.p1.y) + glyphCache_->ascent();
            
            for (char ch : op.text) {
                const GlyphMetrics* g = glyphCache_->getGlyph(ch);
                if (!g) continue;
                
                i32 glyphW = g->x1 - g->x0;
                i32 glyphH = g->y1 - g->y0;
                if (glyphW > 0 && glyphH > 0) {
                    f32 x0 = penX + g->x0;
                    f32 y0 = f32(baseline + g->y0);
                    f32 x1 = x0 + glyphW;
                    f32 y1 = y0 + glyphH;
                    f32 verts[] = {
                        x0, y0, g->u0, g->v0,
                        x1, y0, g->u1, g->v0,
                        x1, y1, g->u1, g->v1,
                        x0, y0, g->u0, g->v0,
                        x1, y1, g->u1, g->v1,
                        x0, y1, g->u0, g->v1
                    };
                    glBindVertexArray(textVao_);
                    glBindBuffer(GL_ARRAY_BUFFER, textVbo_);
                    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STREAM_DRAW);
                    glDrawArrays(GL_TRIANGLES, 0, 6);
                }
                penX += g->advance;
            }
            glBindVertexArray(0);
            break;
        }
        case DrawOp::Type::SetClip:
            setClipRect(op.rect);
            break;
        case DrawOp::Type::ClearClip:
            clearClip();
            break;
    }
}

void GlContext::updateGlyphAtlas() {
    if (!glyphCache_ || !glyphCache_->atlasDirty()) return;
    
    i32 w = glyphCache_->atlasWidth();
    i32 h = glyphCache_->atlasHeight();
    const u8* data = glyphCache_->atlasData();
    
    if (!glyphAtlasTex_) {
        glGenTextures(1, &glyphAtlasTex_);
        glBindTexture(GL_TEXTURE_2D, glyphAtlasTex_);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    } else {
        glBindTexture(GL_TEXTURE_2D, glyphAtlasTex_);
    }
    
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, data);
    glyphCache_->markClean();
}

void GlContext::setClipRect(Rect r) {
    flushTriangles();
    flushLines();
    glEnable(GL_SCISSOR_TEST);
    glScissor(i32(r.x), h_ - i32(r.y + r.h), i32(r.w), i32(r.h));
}

void GlContext::clearClip() {
    flushTriangles();
    flushLines();
    glDisable(GL_SCISSOR_TEST);
}

void GlContext::submit(const Recording& recording) {
    if (!ctx_) return;
    lineVertices_.clear();
    triVertices_.clear();
    
    for (const auto& op : recording.ops()) {
        applyOp(op);
    }
    
    flushTriangles();
    flushLines();
}

void GlContext::present() {
    if (ctx_) {
        glXSwapBuffers(display_, win_);
    }
}

void GlContext::setGlyphCache(GlyphCache* cache) {
    glyphCache_ = cache;
}

}
