#pragma once

#include "surface.hpp"
#include "waveform_data.hpp"

namespace wv {

class WaveformViewer {
public:
    void setSize(i32 w, i32 h) { w_ = w; h_ = h; }
    i32 width() const { return w_; }
    i32 height() const { return h_; }
    
    void setData(const WaveformData* data);
    void paint(Surface* s);
    
    void mouseDown(i32 x, i32 y);
    void mouseMove(i32 x, i32 y);
    void mouseUp();
    void mouseWheel(i32 x, i32 delta);
    bool needsRepaint() const { return needsRepaint_; }
    void clearRepaintFlag() { needsRepaint_ = false; }
    
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
    
    void drawSignal(Surface* s, const Signal& sig, i32 y);
    void drawTimeScale(Surface* s);
    void drawSignalNames(Surface* s);
    void clampTimeOffset();
};

}
