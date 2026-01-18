#pragma once

#include "types.hpp"
#include <string_view>

namespace wv {

class Surface {
public:
    virtual ~Surface() = default;
    
    virtual void init(SurfaceID sid, i32 w, i32 h) = 0;
    virtual void release() = 0;
    
    virtual void fillRect(Rect r, Color c) = 0;
    virtual void strokeRect(Rect r, Color c, f32 width = 1.0f) = 0;
    virtual void drawLine(Point p1, Point p2, Color c, f32 width = 1.0f) = 0;
    virtual void drawPolyline(const Point* pts, i32 count, Color c, f32 width = 1.0f) = 0;
    virtual void drawText(Point p, std::string_view text, Color c) = 0;
    
    virtual void setClip(Rect r) = 0;
    virtual void clearClip() = 0;
};

}
