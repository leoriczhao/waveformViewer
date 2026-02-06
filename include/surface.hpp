#pragma once

#include "types.hpp"
#include "pixmap.hpp"
#include "canvas.hpp"
#include "context.hpp"
#include "device.hpp"
#include <memory>

namespace wv {

class GlyphCache;

class Surface {
public:
    // Raster (CPU) surface - allocates internal pixel buffer
    static std::unique_ptr<Surface> MakeRaster(i32 w, i32 h,
                                                PixelFormat fmt = PixelFormat::BGRA8888);

    // Raster (CPU) surface - wraps host-provided pixel buffer (zero-copy)
    static std::unique_ptr<Surface> MakeRasterDirect(const PixmapInfo& info, void* pixels);

    // GPU surface - host must have GL context current
    static std::unique_ptr<Surface> MakeGpu(std::unique_ptr<Context> context, i32 w, i32 h);

    // Recording surface - for offscreen command recording
    static std::unique_ptr<Surface> MakeRecording(i32 w, i32 h);

    ~Surface();

    Canvas* canvas() const { return canvas_.get(); }

    void resize(i32 w, i32 h);
    void beginFrame();
    void endFrame();
    void submit(const Recording& recording);
    void flush();

    // Pixel access (raster surfaces only, returns nullptr for GPU/recording)
    Pixmap* peekPixels();
    const Pixmap* peekPixels() const;

    std::unique_ptr<Recording> takeRecording();
    void setGlyphCache(GlyphCache* cache);

private:
    Surface(std::unique_ptr<Device> device,
            std::unique_ptr<Context> context,
            std::unique_ptr<Pixmap> pixmap);

    std::unique_ptr<Device> device_;
    std::unique_ptr<Canvas> canvas_;
    std::unique_ptr<Context> context_;
    std::unique_ptr<Pixmap> pixmap_;
};

}
