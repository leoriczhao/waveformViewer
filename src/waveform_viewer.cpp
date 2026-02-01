#include "waveform_viewer.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace wv {

void WaveformViewer::setData(const WaveformData* data) {
    data_ = data;
    hasInitScale_ = false; // re-init scale on next render when we know viewport
    needsRepaint_ = true;
}

void WaveformViewer::ensureInitialScale() {
    if (hasInitScale_) return;
    if (!data_) return;
    if (w_ <= nameWidth_) return;
    if (data_->endTime <= 0) return;

    // Fit the entire waveform into the visible area (excluding name column).
    timeScale_ = f64(w_ - nameWidth_) / data_->endTime;
    if (timeScale_ <= 0) timeScale_ = 1.0;
    timeOffset_ = 0;
    hasInitScale_ = true;
}

void WaveformViewer::render(const RenderContext& ctx) {
    if (!ctx.surface) return;

    // Update viewport
    if (ctx.viewport.width != w_ || ctx.viewport.height != h_) {
        w_ = ctx.viewport.width;
        h_ = ctx.viewport.height;
        hasInitScale_ = false; // viewport change can re-fit if desired
    }

    // Always ensure we have a sane initial scale once we know dimensions.
    ensureInitialScale();

    Surface* s = ctx.surface;

    s->fillRect({0, 0, f32(w_), f32(h_)}, {32, 32, 32, 255});
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

void WaveformViewer::input(const PointerEvent& e) {
    switch (e.type) {
        case PointerEvent::Type::Down: {
            if (e.x < nameWidth_) return;
            dragging_ = true;
            dragStartX_ = e.x;
            dragStartOffset_ = timeOffset_;
            break;
        }
        case PointerEvent::Type::Move: {
            if (!dragging_) return;
            f64 dx = e.x - dragStartX_;
            timeOffset_ = dragStartOffset_ - dx / timeScale_;
            clampTimeOffset();
            needsRepaint_ = true;
            break;
        }
        case PointerEvent::Type::Up: {
            dragging_ = false;
            break;
        }
        case PointerEvent::Type::Wheel: {
            if (e.x < nameWidth_) return;
            if (!data_) return;

            // Zoom around the mouse pointer.
            f64 mouseTime = timeOffset_ + (e.x - nameWidth_) / timeScale_;

            // Continuous zoom factor.
            // wheelY > 0 => zoom in; < 0 => zoom out.
            f64 factor = std::pow(1.2, (f64)e.wheelY);
            f64 newScale = timeScale_ * factor;

            if (newScale > 1e-10 && newScale < 1e10) {
                timeScale_ = newScale;
                timeOffset_ = mouseTime - (e.x - nameWidth_) / timeScale_;
                clampTimeOffset();
                needsRepaint_ = true;
            }
            break;
        }
    }
}

void WaveformViewer::drawTimeScale(Surface* s) {
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
            s->drawLine({x, y}, {x, f32(h_)}, tickColor, 1);
            char buf[32];
            std::snprintf(buf, sizeof(buf), "%.0f", t);
            s->drawText({x + 2, y - 5}, buf, textColor);
        }
        t += step;
    }
}

void WaveformViewer::drawSignalNames(Surface* s) {
    s->fillRect({0, 0, f32(nameWidth_), f32(h_)}, {45, 45, 45, 255});
    s->drawLine({f32(nameWidth_), 0}, {f32(nameWidth_), f32(h_)}, {70, 70, 70, 255}, 1);

    i32 y = 30;
    for (const auto& sig : data_->signals) {
        s->drawText({5, f32(y) + f32(signalHeight_) * 0.5f}, sig.name, {220, 220, 220, 255});
        y += signalHeight_ + 5;
    }
}

void WaveformViewer::drawSignal(Surface* s, const Signal& sig, i32 y) {
    Color lineColor = {50, 200, 50, 255};
    f32 high = f32(y);
    f32 low = f32(y + signalHeight_ - 5);
    f32 xOff = f32(nameWidth_);

    if (sig.changes.empty()) return;

    f32 lastX = xOff;
    f32 lastY = sig.changes[0].value ? high : low;

    for (size_t i = 0; i < sig.changes.size(); ++i) {
        f32 x = xOff + f32((sig.changes[i].time - timeOffset_) * timeScale_);
        f32 newY = sig.changes[i].value ? high : low;

        if (x < xOff) {
            lastX = x;
            lastY = newY;
            continue;
        }
        if (lastX > w_) break;

        s->drawLine({std::max(lastX, xOff), lastY}, {x, lastY}, lineColor, 1);
        if (lastY != newY) s->drawLine({x, lastY}, {x, newY}, lineColor, 1);

        lastX = x;
        lastY = newY;
    }

    if (lastX < w_) s->drawLine({lastX, lastY}, {f32(w_), lastY}, lineColor, 1);
}

void WaveformViewer::clampTimeOffset() {
    if (!data_) return;
    if (w_ <= nameWidth_) return;

    f64 visibleWidth = (w_ - nameWidth_) / timeScale_;
    f64 minOffset = -visibleWidth * 0.1;
    f64 maxOffset = data_->endTime - visibleWidth * 0.9;
    timeOffset_ = std::clamp(timeOffset_, minOffset, std::max(minOffset, maxOffset));
}

}