#include "surface.hpp"
#include "device.hpp"

namespace wv {

std::unique_ptr<Surface> Surface::MakeRecording(i32 w, i32 h) {
    auto device = std::make_unique<GpuDevice>();
    device->resize(w, h);
    return std::unique_ptr<Surface>(new Surface(std::move(device), nullptr, nullptr));
}

}
