#pragma once

#include "component/types.hpp"
#include "component/surface.hpp"

namespace comp {

enum class MouseButton { None, Left, Middle, Right };
enum class KeyMod : u8 { None = 0, Shift = 1, Ctrl = 2, Alt = 4 };

struct MouseEvent {
    Point pos;
    MouseButton button = MouseButton::None;
    KeyMod mods = KeyMod::None;
    i32 wheelDelta = 0;
};

struct KeyEvent {
    i32 keyCode = 0;
    KeyMod mods = KeyMod::None;
    bool isRepeat = false;
};

class Component {
public:
    virtual ~Component() = default;
    
    void setSize(i32 w, i32 h) { width_ = w; height_ = h; onResize(w, h); }
    i32 width() const { return width_; }
    i32 height() const { return height_; }
    
    virtual void paint(Surface* surface) = 0;
    
    virtual void onResize(i32 w, i32 h) { (void)w; (void)h; }
    virtual void onMouseDown(const MouseEvent& e) { (void)e; }
    virtual void onMouseUp(const MouseEvent& e) { (void)e; }
    virtual void onMouseMove(const MouseEvent& e) { (void)e; }
    virtual void onMouseWheel(const MouseEvent& e) { (void)e; }
    virtual void onKeyDown(const KeyEvent& e) { (void)e; }
    virtual void onKeyUp(const KeyEvent& e) { (void)e; }
    
    void invalidate() { needsRepaint_ = true; }
    bool needsRepaint() const { return needsRepaint_; }
    void clearRepaintFlag() { needsRepaint_ = false; }
    
protected:
    i32 width_ = 0;
    i32 height_ = 0;
    bool needsRepaint_ = true;
};

}
