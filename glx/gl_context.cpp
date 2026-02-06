#include "gl_context.hpp"
#include "glad/glad.h"
#include <cstdio>

namespace wv {

void GlStateCache::invalidate() {
    currentProgram_ = 0;
    currentVao_ = 0;
    currentVbo_ = 0;
    scissorEnabled_ = false;
    scissorX_ = scissorY_ = scissorW_ = scissorH_ = 0;
    blendEnabled_ = false;
    uniforms2f_.clear();
    uniforms4f_.clear();
    uniforms1i_.clear();
    boundTextures_.fill(0);
}

bool GlStateCache::useProgram(u32 program) {
    if (currentProgram_ == program) return false;
    currentProgram_ = program;
    glUseProgram(program);
    return true;
}

bool GlStateCache::bindVao(u32 vao) {
    if (currentVao_ == vao) return false;
    currentVao_ = vao;
    glBindVertexArray(vao);
    return true;
}

bool GlStateCache::bindVbo(u32 vbo) {
    if (currentVbo_ == vbo) return false;
    currentVbo_ = vbo;
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    return true;
}

bool GlStateCache::setScissor(bool enabled, i32 x, i32 y, i32 w, i32 h) {
    if (enabled) {
        if (scissorEnabled_ && scissorX_ == x && scissorY_ == y && 
            scissorW_ == w && scissorH_ == h) return false;
        scissorEnabled_ = true;
        scissorX_ = x; scissorY_ = y; scissorW_ = w; scissorH_ = h;
        glEnable(GL_SCISSOR_TEST);
        glScissor(x, y, w, h);
    } else {
        if (!scissorEnabled_) return false;
        scissorEnabled_ = false;
        glDisable(GL_SCISSOR_TEST);
    }
    return true;
}

bool GlStateCache::setUniform2f(i32 loc, f32 x, f32 y) {
    auto it = uniforms2f_.find(loc);
    if (it != uniforms2f_.end() && it->second.x == x && it->second.y == y) return false;
    uniforms2f_[loc] = {x, y};
    glUniform2f(loc, x, y);
    return true;
}

bool GlStateCache::setUniform4f(i32 loc, f32 x, f32 y, f32 z, f32 w) {
    auto it = uniforms4f_.find(loc);
    if (it != uniforms4f_.end() && it->second.x == x && it->second.y == y && 
        it->second.z == z && it->second.w == w) return false;
    uniforms4f_[loc] = {x, y, z, w};
    glUniform4f(loc, x, y, z, w);
    return true;
}

bool GlStateCache::setUniform1i(i32 loc, i32 v) {
    auto it = uniforms1i_.find(loc);
    if (it != uniforms1i_.end() && it->second == v) return false;
    uniforms1i_[loc] = v;
    glUniform1i(loc, v);
    return true;
}

bool GlStateCache::bindTexture(u32 unit, u32 texture) {
    if (unit >= boundTextures_.size()) return false;
    if (boundTextures_[unit] == texture) return false;
    boundTextures_[unit] = texture;
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, texture);
    return true;
}

bool GlStateCache::setBlend(bool enabled) {
    if (blendEnabled_ == enabled) return false;
    blendEnabled_ = enabled;
    if (enabled) glEnable(GL_BLEND);
    else glDisable(GL_BLEND);
    return true;
}

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

GlContext::GlContext() {
}

GlContext::~GlContext() {
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
}

std::unique_ptr<Context> Context::MakeGL() {
    auto ctx = std::unique_ptr<GlContext>(new GlContext());
    return ctx;
}

bool GlContext::init(i32 w, i32 h) {
    w_ = w;
    h_ = h;
    
    if (!gladLoadGL()) return false;
    
    return initGL();
}

void GlContext::resize(i32 w, i32 h) {
    w_ = w;
    h_ = h;
    glViewport(0, 0, w, h);
}

void GlContext::beginFrame() {
    stateCache_.invalidate();
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
    stateCache_.useProgram(shader_);
    stateCache_.setUniform2f(resolutionLoc_, f32(w_), f32(h_));
    stateCache_.bindVao(vao_);
    stateCache_.bindVbo(vbo_);
    glBufferData(GL_ARRAY_BUFFER, lineVertices_.size() * sizeof(Vertex), lineVertices_.data(), GL_STREAM_DRAW);
    glDrawArrays(GL_LINES, 0, i32(lineVertices_.size()));
    lineVertices_.clear();
}

void GlContext::flushTriangles() {
    if (triVertices_.empty()) return;
    stateCache_.useProgram(shader_);
    stateCache_.setUniform2f(resolutionLoc_, f32(w_), f32(h_));
    stateCache_.bindVao(vao_);
    stateCache_.bindVbo(vbo_);
    glBufferData(GL_ARRAY_BUFFER, triVertices_.size() * sizeof(Vertex), triVertices_.data(), GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, i32(triVertices_.size()));
    triVertices_.clear();
}

void GlContext::flushText() {
    if (textVertices_.empty()) return;
    
    stateCache_.useProgram(textShader_);
    stateCache_.setUniform2f(textResLoc_, f32(w_), f32(h_));
    stateCache_.bindTexture(0, glyphAtlasTex_);
    i32 texLoc = glGetUniformLocation(textShader_, "uTex");
    stateCache_.setUniform1i(texLoc, 0);
    i32 colorLoc = glGetUniformLocation(textShader_, "uColor");
    stateCache_.setUniform4f(colorLoc, currentTextColor_.r / 255.0f, currentTextColor_.g / 255.0f,
                currentTextColor_.b / 255.0f, currentTextColor_.a / 255.0f);
    
    stateCache_.bindVao(textVao_);
    stateCache_.bindVbo(textVbo_);
    glBufferData(GL_ARRAY_BUFFER, textVertices_.size() * sizeof(TextVertex), 
                 textVertices_.data(), GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<i32>(textVertices_.size()));
    
    textVertices_.clear();
}

void GlContext::applyOp(const CompactDrawOp& op, const DrawOpArena& arena) {
    switch (op.type) {
        case DrawOp::Type::FillRect: {
            const Rect& rect = op.data.fill.rect;
            Vertex v0 = {rect.x, rect.y, op.color.r, op.color.g, op.color.b, op.color.a};
            Vertex v1 = {rect.x + rect.w, rect.y, op.color.r, op.color.g, op.color.b, op.color.a};
            Vertex v2 = {rect.x + rect.w, rect.y + rect.h, op.color.r, op.color.g, op.color.b, op.color.a};
            Vertex v3 = {rect.x, rect.y + rect.h, op.color.r, op.color.g, op.color.b, op.color.a};
            triVertices_.push_back(v0);
            triVertices_.push_back(v1);
            triVertices_.push_back(v2);
            triVertices_.push_back(v0);
            triVertices_.push_back(v2);
            triVertices_.push_back(v3);
            break;
        }
        case DrawOp::Type::StrokeRect: {
            const Rect& rect = op.data.stroke.rect;
            Point p1 = {rect.x, rect.y};
            Point p2 = {rect.x + rect.w, rect.y};
            Point p3 = {rect.x + rect.w, rect.y + rect.h};
            Point p4 = {rect.x, rect.y + rect.h};
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
            const Point& p1 = op.data.line.p1;
            const Point& p2 = op.data.line.p2;
            lineVertices_.push_back({p1.x, p1.y, op.color.r, op.color.g, op.color.b, op.color.a});
            lineVertices_.push_back({p2.x, p2.y, op.color.r, op.color.g, op.color.b, op.color.a});
            break;
        }
        case DrawOp::Type::Polyline: {
            const Point* points = arena.getPoints(op.data.polyline.offset);
            i32 count = op.data.polyline.count;
            for (i32 i = 0; i + 1 < count; ++i) {
                Point p1 = points[i];
                Point p2 = points[i + 1];
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
            
            if (!textVertices_.empty() && 
                (op.color.r != currentTextColor_.r || op.color.g != currentTextColor_.g ||
                 op.color.b != currentTextColor_.b || op.color.a != currentTextColor_.a)) {
                flushText();
            }
            currentTextColor_ = op.color;
            
            const char* text = arena.getString(op.data.text.offset);
            f32 penX = op.data.text.pos.x;
            i32 baseline = i32(op.data.text.pos.y) + glyphCache_->ascent();
            
            for (u16 i = 0; i < op.data.text.len; ++i) {
                char ch = text[i];
                const GlyphMetrics* g = glyphCache_->getGlyph(ch);
                if (!g) continue;
                
                i32 glyphW = g->x1 - g->x0;
                i32 glyphH = g->y1 - g->y0;
                if (glyphW > 0 && glyphH > 0) {
                    f32 x0 = penX + g->x0;
                    f32 y0 = f32(baseline + g->y0);
                    f32 x1 = x0 + glyphW;
                    f32 y1 = y0 + glyphH;
                    
                    textVertices_.push_back({x0, y0, g->u0, g->v0});
                    textVertices_.push_back({x1, y0, g->u1, g->v0});
                    textVertices_.push_back({x1, y1, g->u1, g->v1});
                    textVertices_.push_back({x0, y0, g->u0, g->v0});
                    textVertices_.push_back({x1, y1, g->u1, g->v1});
                    textVertices_.push_back({x0, y1, g->u0, g->v1});
                }
                penX += g->advance;
            }
            break;
        }
        case DrawOp::Type::SetClip:
            setClipRect(op.data.clip.rect);
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
    flushText();
    stateCache_.setScissor(true, i32(r.x), h_ - i32(r.y + r.h), i32(r.w), i32(r.h));
}

void GlContext::clearClip() {
    flushTriangles();
    flushLines();
    flushText();
    stateCache_.setScissor(false, 0, 0, 0, 0);
}

void GlContext::submit(const Recording& recording) {
    lineVertices_.clear();
    triVertices_.clear();
    textVertices_.clear();
    
    const auto& arena = recording.arena();
    for (const auto& op : recording.ops()) {
        applyOp(op, arena);
    }
    
    flushTriangles();
    flushLines();
    flushText();
}

void GlContext::flush() {
    glFlush();
}

void GlContext::setGlyphCache(GlyphCache* cache) {
    glyphCache_ = cache;
}

}
