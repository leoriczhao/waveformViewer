#include "gl_surface.hpp"
#include <cstdio>
#include <GL/glxext.h>

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

bool GlSurface::createContext(Display* dpy, Window win, XVisualInfo* vi) {
    dpy_ = dpy;
    win_ = win;
    
    PFNGLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB = 
        (PFNGLXCREATECONTEXTATTRIBSARBPROC)glXGetProcAddressARB(
            (const GLubyte*)"glXCreateContextAttribsARB");
    
    if (glXCreateContextAttribsARB) {
        int fbcount;
        GLXFBConfig* fbc = glXChooseFBConfig(dpy_, DefaultScreen(dpy_), nullptr, &fbcount);
        if (fbc && fbcount > 0) {
            int contextAttribs[] = {
                GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
                GLX_CONTEXT_MINOR_VERSION_ARB, 3,
                GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
                None
            };
            ctx_ = glXCreateContextAttribsARB(dpy_, fbc[0], nullptr, True, contextAttribs);
            XFree(fbc);
        }
    }
    
    if (!ctx_ && vi) {
        ctx_ = glXCreateContext(dpy_, vi, nullptr, GL_TRUE);
    }
    
    if (!ctx_) {
        fprintf(stderr, "Failed to create OpenGL context\n");
        return false;
    }
    
    if (!glXMakeCurrent(dpy_, win_, ctx_)) {
        fprintf(stderr, "Failed to make OpenGL context current\n");
        glXDestroyContext(dpy_, ctx_);
        ctx_ = nullptr;
        return false;
    }
    
    if (!initGL()) {
        release();
        return false;
    }
    
    return true;
}

void GlSurface::init(SurfaceID sid, i32 w, i32 h) {
    dpy_ = static_cast<Display*>(sid);
    w_ = w;
    h_ = h;
}

void GlSurface::resize(i32 w, i32 h) {
    w_ = w;
    h_ = h;
    if (ctx_) {
        glViewport(0, 0, w, h);
    }
}

void GlSurface::release() {
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
    if (ctx_ && dpy_) {
        glXMakeCurrent(dpy_, None, nullptr);
        glXDestroyContext(dpy_, ctx_);
        ctx_ = nullptr;
    }
}

bool GlSurface::initGL() {
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        fprintf(stderr, "GLEW init failed: %s\n", glewGetErrorString(err));
        return false;
    }
    
    shader_ = createProgram();
    if (!shader_) {
        fprintf(stderr, "Failed to create shader program\n");
        return false;
    }
    
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
    
    return true;
}

GLuint GlSurface::compileShader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    
    GLint success;
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

GLuint GlSurface::createProgram() {
    GLuint vs = compileShader(GL_VERTEX_SHADER, vertexShaderSrc);
    if (!vs) return 0;
    
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSrc);
    if (!fs) {
        glDeleteShader(vs);
        return 0;
    }
    
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    
    GLint success;
    glGetProgramiv(prog, GL_LINK_STATUS, &success);
    
    glDeleteShader(vs);
    glDeleteShader(fs);
    
    if (!success) {
        char log[512];
        glGetProgramInfoLog(prog, 512, nullptr, log);
        fprintf(stderr, "Program link error: %s\n", log);
        glDeleteProgram(prog);
        return 0;
    }
    
    return prog;
}

void GlSurface::beginFrame() {
    if (!ctx_) return;
    
    glXMakeCurrent(dpy_, win_, ctx_);
    glViewport(0, 0, w_, h_);
    glDisable(GL_SCISSOR_TEST);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    lineVertices_.clear();
    triVertices_.clear();
}

void GlSurface::endFrame() {
    flushTriangles();
    flushLines();
    glXSwapBuffers(dpy_, win_);
}

void GlSurface::flushLines() {
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

void GlSurface::flushTriangles() {
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

void GlSurface::fillRect(Rect r, Color c) {
    Vertex v0 = {r.x, r.y, c.r, c.g, c.b, c.a};
    Vertex v1 = {r.x + r.w, r.y, c.r, c.g, c.b, c.a};
    Vertex v2 = {r.x + r.w, r.y + r.h, c.r, c.g, c.b, c.a};
    Vertex v3 = {r.x, r.y + r.h, c.r, c.g, c.b, c.a};
    
    triVertices_.push_back(v0);
    triVertices_.push_back(v1);
    triVertices_.push_back(v2);
    triVertices_.push_back(v0);
    triVertices_.push_back(v2);
    triVertices_.push_back(v3);
}

void GlSurface::strokeRect(Rect r, Color c, f32 width) {
    drawLine({r.x, r.y}, {r.x + r.w, r.y}, c, width);
    drawLine({r.x + r.w, r.y}, {r.x + r.w, r.y + r.h}, c, width);
    drawLine({r.x + r.w, r.y + r.h}, {r.x, r.y + r.h}, c, width);
    drawLine({r.x, r.y + r.h}, {r.x, r.y}, c, width);
}

void GlSurface::drawLine(Point p1, Point p2, Color c, f32) {
    lineVertices_.push_back({p1.x, p1.y, c.r, c.g, c.b, c.a});
    lineVertices_.push_back({p2.x, p2.y, c.r, c.g, c.b, c.a});
}

void GlSurface::drawPolyline(const Point* pts, i32 count, Color c, f32 width) {
    for (i32 i = 0; i < count - 1; ++i) {
        drawLine(pts[i], pts[i + 1], c, width);
    }
}

void GlSurface::drawText(Point, std::string_view, Color) {
}

void GlSurface::setClip(Rect r) {
    flushTriangles();
    flushLines();
    glEnable(GL_SCISSOR_TEST);
    glScissor(i32(r.x), h_ - i32(r.y + r.h), i32(r.w), i32(r.h));
}

void GlSurface::clearClip() {
    flushTriangles();
    flushLines();
    glDisable(GL_SCISSOR_TEST);
}

}
