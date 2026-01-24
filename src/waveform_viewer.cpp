#include "waveform_viewer.hpp"
#include "canvas.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace wv {

void WaveformViewer::setData(const WaveformData* data) {
    data_ = data;
    if (data_ && data_->endTime > 0) {
        timeScale_ = f64(w_ - nameWidth_) / data_->endTime;
    }
    staticLayer_.dirty = true;
    waveformLayer_.dirty = true;
    overlayLayer_.dirty = true;
}

void WaveformViewer::paint(Surface* target) {
    if (!data_ || !target) return;
    
    ensureLayers();
    if (staticLayer_.dirty) updateStaticLayer();
    if (waveformLayer_.dirty) updateWaveformLayer();
    if (overlayLayer_.dirty) updateOverlayLayer();
    
    if (staticLayer_.recording) target->submit(*staticLayer_.recording);
    if (waveformLayer_.recording) target->submit(*waveformLayer_.recording);
    if (overlayLayer_.recording) target->submit(*overlayLayer_.recording);
}

void WaveformViewer::drawTimeScale(Canvas* c) {
    Color tickColor = {90, 90, 90, 255};
    Color textColor = {180, 180, 180, 255};
    f32 y = 20;
    
    f64 visibleStart = timeOffset_;
    f64 visibleEnd = timeOffset_ + (w_ - nameWidth_) / timeScale_;
    
    f64 step = 1;
    while (step * timeScale_ < 80) step *= 10;
    
    f64 t = std::floor(visibleStart / step) * step;
    while (t <= visibleEnd) {
        f32 x = f32(nameWidth_ + (t - timeOffset_) * timeScale_);
        if (x >= nameWidth_) {
            c->drawLine({x, y}, {x, f32(h_)}, tickColor, 1);
            char buf[32];
            std::snprintf(buf, sizeof(buf), "%.0f", t);
            c->drawText({x + 2, y - 5}, buf, textColor);
        }
        t += step;
    }
}

void WaveformViewer::drawSignalNames(Canvas* c) {
    c->fillRect({0, 0, f32(nameWidth_), f32(h_)}, {45, 45, 45, 255});
    c->drawLine({f32(nameWidth_), 0}, {f32(nameWidth_), f32(h_)}, {70, 70, 70, 255}, 1);
    
    i32 y = 30;
    i32 idx = 0;
    for (const auto& sig : data_->signals) {
        // Highlight selected signal
        if (idx == selectedSignal_) {
            c->fillRect({0, f32(y), f32(nameWidth_), f32(signalHeight_)}, {60, 60, 80, 255});
        }
        
        c->drawText({5, f32(y) + f32(signalHeight_) * 0.5f}, sig.name, {220, 220, 220, 255});
        
        y += signalHeight_ + 5;
        idx++;
    }
}

void WaveformViewer::drawSignalValues(Canvas* c) {
    i32 y = 30;
    for (const auto& sig : data_->signals) {
        u64 val = getValueAtTime(sig, cursorTime_);
        std::string valStr = formatValue(val, sig.width, sig.radix);
        c->drawText({f32(nameWidth_ - 8 - valStr.length() * 7), f32(y) + f32(signalHeight_) * 0.5f},
                    valStr, {150, 220, 150, 255});
        y += signalHeight_ + 5;
    }
}

void WaveformViewer::drawSignal(Canvas* c, const Signal& sig, i32 y) {
    Color lineColor = {50, 200, 50, 255};
    f32 high = f32(y);
    f32 low = f32(y + signalHeight_ - 5);
    f32 xOff = f32(nameWidth_);
    
    if (sig.changes.empty()) return;
    
    if (sig.width > 1) {
        Color busColor = {80, 180, 220, 255};
        f32 mid = (high + low) / 2;
        
        for (size_t i = 0; i < sig.changes.size(); ++i) {
            f32 x1 = xOff + f32((sig.changes[i].time - timeOffset_) * timeScale_);
            f32 x2 = (i + 1 < sig.changes.size()) 
                ? xOff + f32((sig.changes[i + 1].time - timeOffset_) * timeScale_)
                : f32(w_);
            
            if (x2 < xOff || x1 > w_) continue;
            x1 = std::max(x1, xOff);
            x2 = std::min(x2, f32(w_));
            
            f32 slant = 3;
            if (x2 - x1 > slant * 2) {
                Point pts[] = {
                    {x1, mid}, {x1 + slant, high + 2}, {x2 - slant, high + 2},
                    {x2, mid}, {x2 - slant, low - 2}, {x1 + slant, low - 2}, {x1, mid}
                };
                c->drawPolyline(pts, 7, busColor, 1);
                
                if (x2 - x1 > 40) {
                    std::string val = formatValue(sig.changes[i].value, sig.width, sig.radix);
                    c->drawText({x1 + slant + 3, mid - 5}, val, {200, 230, 255, 255});
                }
            }
        }
        return;
    }
    
    f32 lastX = xOff;
    f32 lastY = sig.changes[0].value ? high : low;
    
    for (size_t i = 0; i < sig.changes.size(); ++i) {
        f32 x = xOff + f32((sig.changes[i].time - timeOffset_) * timeScale_);
        f32 newY = sig.changes[i].value ? high : low;
        
        if (x < xOff) { lastX = x; lastY = newY; continue; }
        if (lastX > w_) break;
        
        c->drawLine({std::max(lastX, xOff), lastY}, {x, lastY}, lineColor, 1);
        if (lastY != newY)
            c->drawLine({x, lastY}, {x, newY}, lineColor, 1);
        
        lastX = x;
        lastY = newY;
    }
    
    if (lastX < w_)
        c->drawLine({lastX, lastY}, {f32(w_), lastY}, lineColor, 1);
}

void WaveformViewer::drawCursor(Canvas* c) {
    f32 x = f32(nameWidth_ + (cursorTime_ - timeOffset_) * timeScale_);
    if (x < nameWidth_ || x > w_) return;
    
    Color cursorColor = {255, 180, 50, 255};
    c->drawLine({x, 20}, {x, f32(h_)}, cursorColor, 1);
    
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.0f", cursorTime_);
    f32 labelWidth = f32(std::strlen(buf) * 7 + 6);
    c->fillRect({x + 2, 2, labelWidth, 16}, {60, 50, 30, 255});
    c->drawText({x + 5, 12}, buf, cursorColor);
}

void WaveformViewer::ensureLayers() {
    if (layerW_ == w_ && layerH_ == h_ && staticLayer_.surface && waveformLayer_.surface && overlayLayer_.surface) {
        return;
    }
    layerW_ = w_;
    layerH_ = h_;
    staticLayer_.surface = Surface::MakeRecording(w_, h_);
    waveformLayer_.surface = Surface::MakeRecording(w_, h_);
    overlayLayer_.surface = Surface::MakeRecording(w_, h_);
    staticLayer_.recording.reset();
    waveformLayer_.recording.reset();
    overlayLayer_.recording.reset();
    staticLayer_.dirty = true;
    waveformLayer_.dirty = true;
    overlayLayer_.dirty = true;
}

void WaveformViewer::updateStaticLayer() {
    if (!staticLayer_.surface) return;
    auto* c = staticLayer_.surface->canvas();
    staticLayer_.surface->beginFrame();
    c->fillRect({0, 0, f32(w_), f32(h_)}, {32, 32, 32, 255});
    drawTimeScale(c);
    drawSignalNames(c);
    staticLayer_.surface->endFrame();
    staticLayer_.recording = staticLayer_.surface->takeRecording();
    staticLayer_.dirty = false;
}

void WaveformViewer::updateWaveformLayer() {
    if (!waveformLayer_.surface) return;
    auto* c = waveformLayer_.surface->canvas();
    waveformLayer_.surface->beginFrame();
    c->save();
    c->clipRect({f32(nameWidth_), 0, f32(w_ - nameWidth_), f32(h_)});
    i32 y = 30;
    for (const auto& sig : data_->signals) {
        drawSignal(c, sig, y);
        y += signalHeight_ + 5;
    }
    c->restore();
    waveformLayer_.surface->endFrame();
    waveformLayer_.recording = waveformLayer_.surface->takeRecording();
    waveformLayer_.dirty = false;
}

void WaveformViewer::updateOverlayLayer() {
    if (!overlayLayer_.surface) return;
    auto* c = overlayLayer_.surface->canvas();
    overlayLayer_.surface->beginFrame();
    drawSignalValues(c);
    drawCursor(c);
    overlayLayer_.surface->endFrame();
    overlayLayer_.recording = overlayLayer_.surface->takeRecording();
    overlayLayer_.dirty = false;
}

void WaveformViewer::mouseDown(i32 x, i32 y) {
    if (x < nameWidth_) {
        // Click in name area: select signal
        i32 idx = (y - 30) / (signalHeight_ + 5);
        if (data_ && idx >= 0 && idx < static_cast<i32>(data_->signals.size())) {
            selectedSignal_ = idx;
            needsRepaint_ = true;
            staticLayer_.dirty = true;
        }
        return;
    }
    dragging_ = true;
    dragStartX_ = x;
    dragStartOffset_ = timeOffset_;
}

void WaveformViewer::mouseMove(i32 x, i32) {
    if (!dragging_) return;
    f64 dx = x - dragStartX_;
    timeOffset_ = dragStartOffset_ - dx / timeScale_;
    clampTimeOffset();
    needsRepaint_ = true;
    staticLayer_.dirty = true;
    waveformLayer_.dirty = true;
    overlayLayer_.dirty = true;
}

void WaveformViewer::mouseUp() {
    dragging_ = false;
}

void WaveformViewer::mouseWheel(i32 x, i32 delta) {
    if (x < nameWidth_) return;
    
    f64 mouseTime = timeOffset_ + (x - nameWidth_) / timeScale_;
    f64 factor = delta > 0 ? 1.2 : 0.8;
    f64 newScale = timeScale_ * factor;
    
    if (newScale > 1e-10 && newScale < 1e10) {
        timeScale_ = newScale;
        timeOffset_ = mouseTime - (x - nameWidth_) / timeScale_;
        clampTimeOffset();
        needsRepaint_ = true;
        staticLayer_.dirty = true;
        waveformLayer_.dirty = true;
        overlayLayer_.dirty = true;
    }
}

void WaveformViewer::setCursorTime(f64 time) {
    cursorTime_ = time;
    needsRepaint_ = true;
    overlayLayer_.dirty = true;
}

void WaveformViewer::setCursorFromX(i32 x) {
    if (x < nameWidth_) return;
    cursorTime_ = timeOffset_ + (x - nameWidth_) / timeScale_;
    if (cursorTime_ < 0) cursorTime_ = 0;
    needsRepaint_ = true;
    overlayLayer_.dirty = true;
}

void WaveformViewer::selectSignal(i32 index) {
    if (data_ && index >= -1 && index < static_cast<i32>(data_->signals.size())) {
        selectedSignal_ = index;
        needsRepaint_ = true;
        staticLayer_.dirty = true;
    }
}

bool WaveformViewer::jumpToNextEdge() {
    if (selectedSignal_ < 0 || !data_) return false;
    const auto& sig = data_->signals[selectedSignal_];
    i32 idx = findNextEdgeIndex(sig, cursorTime_);
    if (idx >= 0 && idx < static_cast<i32>(sig.changes.size())) {
        cursorTime_ = sig.changes[idx].time;
        needsRepaint_ = true;
        overlayLayer_.dirty = true;
        return true;
    }
    return false;
}

bool WaveformViewer::jumpToPrevEdge() {
    if (selectedSignal_ < 0 || !data_) return false;
    const auto& sig = data_->signals[selectedSignal_];
    i32 idx = findPrevEdgeIndex(sig, cursorTime_);
    if (idx >= 0 && idx < static_cast<i32>(sig.changes.size())) {
        cursorTime_ = sig.changes[idx].time;
        needsRepaint_ = true;
        overlayLayer_.dirty = true;
        return true;
    }
    return false;
}

void WaveformViewer::setSignalRadix(i32 signalIndex, Radix radix) {
    // Note: This modifies data through const pointer - in real implementation,
    // radix should be stored separately or data should be non-const
    (void)signalIndex;
    (void)radix;
    needsRepaint_ = true;
    overlayLayer_.dirty = true;
}

void WaveformViewer::keyPress(i32 keycode) {
    // XCB keycodes: Left=113, Right=114, Up=111, Down=116
    // These may vary by keyboard layout
    switch (keycode) {
        case 113: // Left arrow
            jumpToPrevEdge();
            break;
        case 114: // Right arrow
            jumpToNextEdge();
            break;
        case 111: // Up arrow
            if (selectedSignal_ > 0) {
                selectedSignal_--;
                needsRepaint_ = true;
                staticLayer_.dirty = true;
            }
            break;
        case 116: // Down arrow
            if (data_ && selectedSignal_ < static_cast<i32>(data_->signals.size()) - 1) {
                selectedSignal_++;
                needsRepaint_ = true;
                staticLayer_.dirty = true;
            }
            break;
    }
}

void WaveformViewer::clampTimeOffset() {
    if (!data_) return;
    f64 visibleWidth = (w_ - nameWidth_) / timeScale_;
    f64 minOffset = -visibleWidth * 0.1;
    f64 maxOffset = data_->endTime - visibleWidth * 0.9;
    timeOffset_ = std::clamp(timeOffset_, minOffset, std::max(minOffset, maxOffset));
}

i32 WaveformViewer::findNextEdgeIndex(const Signal& sig, f64 time) {
    for (size_t i = 0; i < sig.changes.size(); ++i) {
        if (sig.changes[i].time > time) {
            return static_cast<i32>(i);
        }
    }
    return -1;
}

i32 WaveformViewer::findPrevEdgeIndex(const Signal& sig, f64 time) {
    for (size_t i = sig.changes.size(); i > 0; --i) {
        if (sig.changes[i - 1].time < time) {
            return static_cast<i32>(i - 1);
        }
    }
    return -1;
}

u64 WaveformViewer::getValueAtTime(const Signal& sig, f64 time) {
    if (sig.changes.empty()) return 0;
    
    u64 value = sig.changes[0].value;
    for (const auto& change : sig.changes) {
        if (change.time <= time) {
            value = change.value;
        } else {
            break;
        }
    }
    return value;
}

std::string WaveformViewer::formatValue(u64 value, i32 width, Radix radix) {
    char buf[72];
    switch (radix) {
        case Radix::Hex: {
            i32 hexDigits = (width + 3) / 4;
            std::snprintf(buf, sizeof(buf), "0x%0*llX", hexDigits, 
                         static_cast<unsigned long long>(value));
            break;
        }
        case Radix::Decimal:
            std::snprintf(buf, sizeof(buf), "%llu", 
                         static_cast<unsigned long long>(value));
            break;
        case Radix::Binary:
        default: {
            buf[0] = '0';
            buf[1] = 'b';
            for (i32 i = 0; i < width && i < 64; ++i) {
                buf[2 + i] = (value >> (width - 1 - i)) & 1 ? '1' : '0';
            }
            buf[2 + std::min(width, 64)] = '\0';
            break;
        }
    }
    return buf;
}

}
