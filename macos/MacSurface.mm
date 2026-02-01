#include "MacSurface.hpp"
#include <CoreGraphics/CGContext.h>
#include <CoreGraphics/CGGeometry.h>
#include <CoreText/CoreText.h>

namespace wv {

void MacSurface::setColor(Color c) {
    if (!ctx_) return;
    CGFloat components[] = {c.r / 255.0, c.g / 255.0, c.b / 255.0, c.a / 255.0};
    CGColorRef color = CGColorCreate(CGColorSpaceCreateDeviceRGB(), components);
    CGContextSetStrokeColorWithColor(ctx_, color);
    CGContextSetFillColorWithColor(ctx_, color);
    CFRelease(color);
}

void MacSurface::init(SurfaceID sid, i32 w, i32 h) {
    ctx_ = static_cast<CGContextRef>(sid);
    w_ = w;
    h_ = h;
}

void MacSurface::resize(i32 w, i32 h) {
    w_ = w;
    h_ = h;
}

void MacSurface::release() {
    ctx_ = nullptr;
    w_ = 0;
    h_ = 0;
}

void MacSurface::beginFrame() {
    if (!ctx_) return;
    CGContextSaveGState(ctx_);
}

void MacSurface::endFrame() {
    if (!ctx_) return;
    CGContextRestoreGState(ctx_);
}

void MacSurface::fillRect(Rect r, Color c) {
    if (!ctx_) return;
    setColor(c);
    CGRect rect = CGRectMake(r.x, r.y, r.w, r.h);
    CGContextFillRect(ctx_, rect);
}

void MacSurface::strokeRect(Rect r, Color c, f32 width) {
    if (!ctx_) return;
    setColor(c);
    CGContextSetLineWidth(ctx_, width);
    CGRect rect = CGRectMake(r.x, r.y, r.w, r.h);
    CGContextStrokeRect(ctx_, rect);
}

void MacSurface::drawLine(Point p1, Point p2, Color c, f32 width) {
    if (!ctx_) return;
    setColor(c);
    CGContextSetLineWidth(ctx_, width);
    CGContextMoveToPoint(ctx_, p1.x, p1.y);
    CGContextAddLineToPoint(ctx_, p2.x, p2.y);
    CGContextStrokePath(ctx_);
}

void MacSurface::drawPolyline(const Point* pts, i32 count, Color c, f32 width) {
    if (!ctx_ || count < 2) return;
    setColor(c);
    CGContextSetLineWidth(ctx_, width);

    CGContextMoveToPoint(ctx_, pts[0].x, pts[0].y);
    for (i32 i = 1; i < count; ++i) {
        CGContextAddLineToPoint(ctx_, pts[i].x, pts[i].y);
    }
    CGContextStrokePath(ctx_);
}

void MacSurface::drawText(Point p, std::string_view text, Color c) {
    if (!ctx_) return;

    // Create a string from the text view
    CFStringRef cfStr = CFStringCreateWithBytes(
        nullptr,
        reinterpret_cast<const UInt8*>(text.data()),
        text.size(),
        kCFStringEncodingUTF8,
        false
    );

    if (!cfStr) return;

    setColor(c);

    // Use CoreText for basic text rendering
    CTFontRef font = CTFontCreateWithName(CFSTR("Menlo"), 12.0, nullptr);
    if (!font) {
        CFRelease(cfStr);
        return;
    }

    // Create attributed string
    CFStringRef keys[] = { kCTFontAttributeName };
    CFTypeRef values[] = { font };
    CFDictionaryRef attrs = CFDictionaryCreate(
        nullptr,
        reinterpret_cast<const void**>(keys),
        values,
        1,
        &kCFTypeDictionaryKeyCallBacks,
        &kCFTypeDictionaryValueCallBacks
    );

    CFAttributedStringRef attrStr = CFAttributedStringCreate(nullptr, cfStr, attrs);
    CFRelease(cfStr);
    CFRelease(attrs);

    // Create line and draw
    CTLineRef line = CTLineCreateWithAttributedString(attrStr);
    CFRelease(attrStr);

    CGContextSetTextPosition(ctx_, p.x, p.y);
    CTLineDraw(line, ctx_);

    CFRelease(line);
    CFRelease(font);
}

void MacSurface::setClip(Rect r) {
    if (!ctx_) return;
    CGRect rect = CGRectMake(r.x, r.y, r.w, r.h);
    CGContextClipToRect(ctx_, rect);
}

void MacSurface::clearClip() {
    if (!ctx_) return;
    CGContextResetClip(ctx_);
}

}
