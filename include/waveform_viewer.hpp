#pragma once

#include "waveform_data.hpp"
#include "surface.hpp"
#include "recording.hpp"
#include <memory>

namespace wv {

class WaveformViewer {
public:
    void setSize(i32 w, i32 h) {
        w_ = w;
        h_ = h;
        staticLayer_.dirty = true;
        waveformLayer_.dirty = true;
        overlayLayer_.dirty = true;
    }
    i32 width() const { return w_; }
    i32 height() const { return h_; }
    
    void setData(const WaveformData* data);
    void paint(Surface* target);
    
    void mouseDown(i32 x, i32 y);
    void mouseMove(i32 x, i32 y);
    void mouseUp();
    void mouseWheel(i32 x, i32 delta);
    
    // Cursor
    void setCursorTime(f64 time);
    f64 cursorTime() const { return cursorTime_; }
    void setCursorFromX(i32 x);
    
    // Signal selection & edge navigation
    void selectSignal(i32 index);
    i32 selectedSignal() const { return selectedSignal_; }
    bool jumpToNextEdge();
    bool jumpToPrevEdge();
    
    // Radix control
    void setSignalRadix(i32 signalIndex, Radix radix);
    
    // Keyboard input
    void keyPress(i32 keycode);
    
    bool needsRepaint() const { return needsRepaint_; }
    void clearRepaintFlag() { needsRepaint_ = false; }
    
    // Value formatting (public for testing)
    static std::string formatValue(u64 value, i32 width, Radix radix);
    
private:
    const WaveformData* data_ = nullptr;
    i32 w_ = 0, h_ = 0;
    f64 timeOffset_ = 0;
    f64 timeScale_ = 1.0;
    i32 signalHeight_ = 30;
    i32 nameWidth_ = 120;
    
    bool dragging_ = false;
    i32 dragStartX_ = 0;
    f64 dragStartOffset_ = 0;
    bool needsRepaint_ = false;
    
    f64 cursorTime_ = 0;
    i32 selectedSignal_ = -1;
    
    void drawSignal(Canvas* c, const Signal& sig, i32 y);
    void drawTimeScale(Canvas* c);
    void drawSignalNames(Canvas* c);
    void drawSignalValues(Canvas* c);
    void drawCursor(Canvas* c);
    
    struct Layer {
        std::unique_ptr<Surface> surface;
        std::unique_ptr<Recording> recording;
        bool dirty = true;
    };
    
    Layer staticLayer_;
    Layer waveformLayer_;
    Layer overlayLayer_;
    i32 layerW_ = 0;
    i32 layerH_ = 0;
    
    void ensureLayers();
    void updateStaticLayer();
    void updateWaveformLayer();
    void updateOverlayLayer();
    void clampTimeOffset();
    
    i32 findNextEdgeIndex(const Signal& sig, f64 time);
    i32 findPrevEdgeIndex(const Signal& sig, f64 time);
    
    u64 getValueAtTime(const Signal& sig, f64 time);
};

}
