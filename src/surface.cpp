#include "surface.hpp"
#include "canvas.hpp"
#include "context.hpp"
#include "device.hpp"

namespace wv {

Surface::Surface(std::unique_ptr<Device> device, std::unique_ptr<Context> context)
    : device_(std::move(device)), context_(std::move(context)) {
    canvas_ = std::make_unique<Canvas>(device_.get());
}

Surface::~Surface() = default;

void Surface::resize(i32 w, i32 h) {
    device_->resize(w, h);
    if (context_) {
        context_->resize(w, h);
    }
}

void Surface::beginFrame() {
    device_->beginFrame();
    if (context_) {
        context_->beginFrame();
    }
}

void Surface::endFrame() {
    device_->endFrame();
}

void Surface::submit(const Recording& recording) {
    if (context_) {
        context_->submit(recording);
        return;
    }
    for (const auto& op : recording.ops()) {
        switch (op.type) {
            case DrawOp::Type::FillRect:
                device_->fillRect(op.rect, op.color);
                break;
            case DrawOp::Type::StrokeRect:
                device_->strokeRect(op.rect, op.color, op.width);
                break;
            case DrawOp::Type::Line:
                device_->drawLine(op.p1, op.p2, op.color, op.width);
                break;
            case DrawOp::Type::Polyline:
                if (!op.points.empty()) {
                    device_->drawPolyline(op.points.data(), static_cast<i32>(op.points.size()), op.color, op.width);
                }
                break;
            case DrawOp::Type::Text:
                device_->drawText(op.p1, op.text, op.color);
                break;
            case DrawOp::Type::SetClip:
                device_->setClipRect(op.rect);
                break;
            case DrawOp::Type::ClearClip:
                device_->resetClip();
                break;
        }
    }
}

void Surface::present() {
    if (context_) {
        auto recording = device_->finishRecording();
        if (recording) {
            context_->submit(*recording);
        }
        context_->present();
    } else {
        device_->present();
    }
}

std::unique_ptr<Recording> Surface::takeRecording() {
    return device_->finishRecording();
}

void Surface::setGlyphCache(GlyphCache* cache) {
    if (context_) {
        context_->setGlyphCache(cache);
    }
    device_->setGlyphCache(cache);
}

}
