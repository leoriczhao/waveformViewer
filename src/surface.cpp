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
    const auto& arena = recording.arena();
    for (const auto& op : recording.ops()) {
        switch (op.type) {
            case DrawOp::Type::FillRect:
                device_->fillRect(op.data.fill.rect, op.color);
                break;
            case DrawOp::Type::StrokeRect:
                device_->strokeRect(op.data.stroke.rect, op.color, op.width);
                break;
            case DrawOp::Type::Line:
                device_->drawLine(op.data.line.p1, op.data.line.p2, op.color, op.width);
                break;
            case DrawOp::Type::Polyline:
                if (op.data.polyline.count > 0) {
                    device_->drawPolyline(arena.getPoints(op.data.polyline.offset), 
                                        op.data.polyline.count, op.color, op.width);
                }
                break;
            case DrawOp::Type::Text:
                device_->drawText(op.data.text.pos, arena.getString(op.data.text.offset), op.color);
                break;
            case DrawOp::Type::SetClip:
                device_->setClipRect(op.data.clip.rect);
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
