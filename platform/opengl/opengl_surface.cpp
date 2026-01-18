#include "component/opengl_surface.hpp"
#include <GL/gl.h>

namespace comp {

void OpenGLSurface::init(SurfaceID) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
}

void OpenGLSurface::release() {}

void OpenGLSurface::beginFrame() {
    glViewport(0, 0, w_, h_);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, w_, h_, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void OpenGLSurface::endFrame() {}

void OpenGLSurface::fillRect(Rect r, Color c) {
    glColor4ub(c.r, c.g, c.b, c.a);
    glBegin(GL_QUADS);
    glVertex2f(r.x, r.y);
    glVertex2f(r.x + r.w, r.y);
    glVertex2f(r.x + r.w, r.y + r.h);
    glVertex2f(r.x, r.y + r.h);
    glEnd();
}

void OpenGLSurface::strokeRect(Rect r, Color c, f32 width) {
    glLineWidth(width);
    glColor4ub(c.r, c.g, c.b, c.a);
    glBegin(GL_LINE_LOOP);
    glVertex2f(r.x, r.y);
    glVertex2f(r.x + r.w, r.y);
    glVertex2f(r.x + r.w, r.y + r.h);
    glVertex2f(r.x, r.y + r.h);
    glEnd();
}

void OpenGLSurface::drawLine(Point p1, Point p2, Color c, f32 width) {
    glLineWidth(width);
    glColor4ub(c.r, c.g, c.b, c.a);
    glBegin(GL_LINES);
    glVertex2f(p1.x, p1.y);
    glVertex2f(p2.x, p2.y);
    glEnd();
}

void OpenGLSurface::drawPolyline(const Point* pts, i32 count, Color c, f32 width) {
    if (count < 2) return;
    glLineWidth(width);
    glColor4ub(c.r, c.g, c.b, c.a);
    glBegin(GL_LINE_STRIP);
    for (i32 i = 0; i < count; ++i) {
        glVertex2f(pts[i].x, pts[i].y);
    }
    glEnd();
}

void OpenGLSurface::drawText(Point, std::string_view, Color) {
}

void OpenGLSurface::setClip(Rect r) {
    glEnable(GL_SCISSOR_TEST);
    glScissor(i32(r.x), h_ - i32(r.y + r.h), i32(r.w), i32(r.h));
}

void OpenGLSurface::clearClip() {
    glDisable(GL_SCISSOR_TEST);
}

std::unique_ptr<OpenGLSurface> createOpenGLSurface() {
    return std::make_unique<OpenGLSurface>();
}

}
