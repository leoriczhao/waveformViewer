#include <component/component.hpp>
#include <component/opengl_surface.hpp>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <cmath>
#include <vector>

using namespace comp;

class WaveformViewer : public Component {
public:
    void setData(const float* data, i32 count) {
        data_.assign(data, data + count);
        invalidate();
    }
    
    void paint(Surface* s) override {
        s->fillRect({0, 0, f32(width()), f32(height())}, {20, 25, 30, 255});
        
        f32 centerY = height() / 2.0f;
        s->drawLine({0, centerY}, {f32(width()), centerY}, {50, 55, 60, 255}, 1);
        
        if (data_.empty()) return;
        
        std::vector<Point> pts(data_.size());
        f32 xScale = f32(width()) / data_.size();
        f32 yScale = height() * 0.4f;
        
        for (size_t i = 0; i < data_.size(); ++i) {
            pts[i] = {i * xScale, centerY - data_[i] * yScale};
        }
        
        s->drawPolyline(pts.data(), i32(pts.size()), {0, 200, 100, 255}, 1.5f);
    }
    
    void onMouseWheel(const MouseEvent& e) override {
        zoom_ *= (e.wheelDelta > 0) ? 1.1f : 0.9f;
        invalidate();
    }
    
private:
    std::vector<float> data_;
    f32 zoom_ = 1.0f;
};

int main() {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    
    SDL_Window* win = SDL_CreateWindow("Waveform Viewer", 
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    SDL_GLContext ctx = SDL_GL_CreateContext(win);
    
    auto surface = createOpenGLSurface();
    WaveformViewer viewer;
    
    std::vector<float> wave(512);
    for (size_t i = 0; i < wave.size(); ++i) {
        wave[i] = std::sin(i * 0.05f) * 0.8f;
    }
    viewer.setData(wave.data(), wave.size());
    
    int w, h;
    SDL_GetWindowSize(win, &w, &h);
    viewer.setSize(w, h);
    surface->resize(w, h);
    
    bool running = true;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
                case SDL_QUIT: running = false; break;
                case SDL_WINDOWEVENT:
                    if (e.window.event == SDL_WINDOWEVENT_RESIZED) {
                        viewer.setSize(e.window.data1, e.window.data2);
                        surface->resize(e.window.data1, e.window.data2);
                    }
                    break;
                case SDL_MOUSEWHEEL: {
                    MouseEvent me;
                    me.wheelDelta = e.wheel.y;
                    viewer.onMouseWheel(me);
                    break;
                }
            }
        }
        
        if (viewer.needsRepaint()) {
            glClear(GL_COLOR_BUFFER_BIT);
            surface->beginFrame();
            viewer.paint(surface.get());
            surface->endFrame();
            SDL_GL_SwapWindow(win);
            viewer.clearRepaintFlag();
        }
        
        SDL_Delay(16);
    }
    
    SDL_GL_DeleteContext(ctx);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
