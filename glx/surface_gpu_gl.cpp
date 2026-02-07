#include "surface.hpp"
#include "device.hpp"
#include "context.hpp"

namespace wv {

std::unique_ptr<Surface> Surface::MakeGpu(std::unique_ptr<Context> context, i32 w, i32 h) {
    if (!context) return nullptr;
    if (!context->init(w, h)) return nullptr;
    
    auto device = std::make_unique<GpuDevice>();
    auto surface = std::unique_ptr<Surface>(new Surface(std::move(device), std::move(context), nullptr));
    surface->resize(w, h);
    return surface;
}

}
