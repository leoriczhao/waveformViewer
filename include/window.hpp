#pragma once

#include "types.hpp"

namespace wv {

enum class CursorShape { Arrow, IBeam, Hand, SizeH, SizeV };

class Window {
public:
    virtual ~Window() = default;
    
    virtual i32 width() const = 0;
    virtual i32 height() const = 0;
    
    virtual void invalidate() = 0;
    virtual void invalidateRect(Rect r) = 0;
    virtual void setCursor(CursorShape cursor) = 0;
    
    virtual void setScrollPos(i32 x, i32 y) = 0;
    virtual Point scrollPos() const = 0;
};

}
