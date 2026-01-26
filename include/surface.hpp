#pragma once

#include "types.hpp"
#include "canvas.hpp"
#include "context.hpp"
#include "device.hpp"
#include <memory>

namespace wv {

class GlyphCache;

class Surface {
public:
    static std::unique_ptr<Surface> MakeRaster(SurfaceID sid, WindowID win, i32 w, i32 h);
    static std::unique_ptr<Surface> MakeGpu(std::unique_ptr<Context> context, i32 w, i32 h);
    static std::unique_ptr<Surface> MakeRecording(i32 w, i32 h);

    ~Surface();

    Canvas* canvas() const { return canvas_.get(); }

    void resize(i32 w, i32 h);
    void beginFrame();
    void endFrame();
    void submit(const Recording& recording);
    void present();
    std::unique_ptr<Recording> takeRecording();
    void setGlyphCache(GlyphCache* cache);

private:
    Surface(std::unique_ptr<Device> device, std::unique_ptr<Context> context);

    std::unique_ptr<Device> device_;
    std::unique_ptr<Canvas> canvas_;
    std::unique_ptr<Context> context_;
};

}
