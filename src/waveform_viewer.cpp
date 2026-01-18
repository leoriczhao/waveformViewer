#include "waveform_viewer.hpp"
#include <algorithm>
#include <cmath>

namespace wv {

void WaveformViewer::setData(const WaveformData* data) {
    data_ = data;
    if (data_ && data_->endTime > 0) {
        timeScale_ = f64(w_ - nameWidth_) / data_->endTime;
    }
}

void WaveformViewer::paint(Surface* s) {
    s->fillRect({0, 0, f32(w_), f32(h_)}, {30, 30, 35, 255});
    
    if (!data_) return;
    
    drawTimeScale(s);
    drawSignalNames(s);
    
    s->setClip({f32(nameWidth_), 0, f32(w_ - nameWidth_), f32(h_)});
    
    i32 y = 30;
    for (const auto& sig : data_->signals) {
        drawSignal(s, sig, y);
        y += signalHeight_ + 5;
    }
    
    s->clearClip();
}

void WaveformViewer::drawTimeScale(Surface* s) {
    Color tickColor = {80, 80, 90, 255};
    f32 y = 20;
    
    f64 visibleStart = timeOffset_;
    f64 visibleEnd = timeOffset_ + (w_ - nameWidth_) / timeScale_;
    f64 range = visibleEnd - visibleStart;
    
    f64 step = 1;
    while (step * timeScale_ < 50) step *= 10;
    
    f64 t = std::floor(visibleStart / step) * step;
    while (t <= visibleEnd) {
        f32 x = f32(nameWidth_ + (t - timeOffset_) * timeScale_);
        if (x >= nameWidth_) {
            s->drawLine({x, y}, {x, f32(h_)}, tickColor, 1);
        }
        t += step;
    }
}

void WaveformViewer::drawSignalNames(Surface* s) {
    s->fillRect({0, 0, f32(nameWidth_), f32(h_)}, {40, 40, 45, 255});
    s->drawLine({f32(nameWidth_), 0}, {f32(nameWidth_), f32(h_)}, {60, 60, 70, 255}, 1);
    
    i32 y = 30;
    for (const auto& sig : data_->signals) {
        s->drawText({5, f32(y + signalHeight_ / 2)}, sig.name, {200, 200, 200, 255});
        y += signalHeight_ + 5;
    }
}

void WaveformViewer::drawSignal(Surface* s, const Signal& sig, i32 y) {
    Color lineColor = {0, 200, 100, 255};
    f32 high = f32(y);
    f32 low = f32(y + signalHeight_ - 5);
    f32 xOff = f32(nameWidth_);
    
    if (sig.changes.empty()) return;
    
    f32 lastX = xOff;
    f32 lastY = sig.changes[0].value ? high : low;
    
    for (size_t i = 0; i < sig.changes.size(); ++i) {
        f32 x = xOff + f32((sig.changes[i].time - timeOffset_) * timeScale_);
        f32 newY = sig.changes[i].value ? high : low;
        
        if (x < xOff) { lastX = x; lastY = newY; continue; }
        if (lastX > w_) break;
        
        s->drawLine({std::max(lastX, xOff), lastY}, {x, lastY}, lineColor, 1);
        if (lastY != newY)
            s->drawLine({x, lastY}, {x, newY}, lineColor, 1);
        
        lastX = x;
        lastY = newY;
    }
    
    if (lastX < w_)
        s->drawLine({lastX, lastY}, {f32(w_), lastY}, lineColor, 1);
}

void WaveformViewer::mouseDown(i32 x, i32) {
    if (x < nameWidth_) return;
    dragging_ = true;
    dragStartX_ = x;
    dragStartOffset_ = timeOffset_;
}

void WaveformViewer::mouseMove(i32 x, i32) {
    if (!dragging_) return;
    f64 dx = x - dragStartX_;
    timeOffset_ = dragStartOffset_ - dx / timeScale_;
    needsRepaint_ = true;
}

void WaveformViewer::mouseUp() {
    dragging_ = false;
}

void WaveformViewer::mouseWheel(i32 x, i32 delta) {
    if (x < nameWidth_) return;
    
    f64 mouseTime = timeOffset_ + (x - nameWidth_) / timeScale_;
    f64 factor = delta > 0 ? 1.2 : 0.8;
    timeScale_ *= factor;
    timeOffset_ = mouseTime - (x - nameWidth_) / timeScale_;
    
    needsRepaint_ = true;
}

}
