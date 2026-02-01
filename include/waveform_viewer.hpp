#pragma once

#include "surface.hpp"
#include "waveform_data.hpp"

namespace wv {

struct Viewport {
    i32 width = 0;
    i32 height = 0;
    // Device pixel ratio (Retina scaling). Coordinates passed to the viewer are
    // expected to be in logical pixels/points (i.e. already divided by dpr).
    f32 dpr = 1.0f;
};

struct PointerEvent {
    enum class Type {
        Down,
        Move,
        Up,
        Wheel,
    };

    Type type = Type::Move;
    i32 x = 0;
    i32 y = 0;

    // Vertical scroll amount. Positive = zoom in, negative = zoom out.
    // Units are host-defined. For mouse wheels you might send +/-1; for trackpads
    // you can send a smaller continuous value.
    f32 wheelY = 0.0f;

    // True if this came from a high-resolution device (e.g. trackpad).
    bool precise = false;
};

struct RenderContext {
    Surface* surface = nullptr;
    Viewport viewport;
};

class WaveformViewer {
public:
    void setData(const WaveformData* data);

    // Host drives input. We do not manage an event loop.
    void input(const PointerEvent& e);

    // Host drives rendering (typically inside its paint callback).
    void render(const RenderContext& ctx);

    bool needsRepaint() const { return needsRepaint_; }
    void clearRepaint() { needsRepaint_ = false; }

private:
    const WaveformData* data_ = nullptr;

    i32 w_ = 0, h_ = 0;
    f64 timeOffset_ = 0;
    f64 timeScale_ = 1.0;
    i32 signalHeight_ = 30;
    i32 nameWidth_ = 100;

    bool dragging_ = false;
    i32 dragStartX_ = 0;
    f64 dragStartOffset_ = 0;
    bool needsRepaint_ = false;

    bool hasInitScale_ = false;

    void drawSignal(Surface* s, const Signal& sig, i32 y);
    void drawTimeScale(Surface* s);
    void drawSignalNames(Surface* s);
    void clampTimeOffset();

    void ensureInitialScale();
};

}