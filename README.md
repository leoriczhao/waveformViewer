# waveformViewer

A minimal, cross-platform GUI component framework for building custom visualization components like waveform displays.

## Architecture

```
┌─────────────────────────────────────────┐
│  Host Application (Qt/Win32/GTK/...)    │
│  - Creates window                       │
│  - Forwards events to component         │
│  - Calls component->paint(surface)      │
└─────────────────┬───────────────────────┘
                  │
┌─────────────────▼───────────────────────┐
│  Your Component (e.g. WaveformViewer)   │
│  - Inherits from comp::Component        │
│  - Implements paint(Surface*)           │
│  - Handles events (onMouseDown, etc.)   │
└─────────────────┬───────────────────────┘
                  │
┌─────────────────▼───────────────────────┐
│  Surface (Abstract Interface)           │
│  - drawLine, fillRect, drawPolyline...  │
│  - Platform implementations:            │
│    - OpenGL, GDI, Cairo, CoreGraphics   │
└─────────────────────────────────────────┘
```

## Building

```bash
mkdir build && cd build
cmake ..
make
```

## Usage

```cpp
#include <component/component.hpp>
#include <component/opengl_surface.hpp>

class WaveformViewer : public comp::Component {
public:
    void paint(comp::Surface* s) override {
        s->fillRect({0, 0, width(), height()}, {20, 25, 30, 255});
        s->drawPolyline(points.data(), points.size(), {0, 200, 100, 255}, 1.5f);
    }
    
    void onMouseWheel(const comp::MouseEvent& e) override {
        zoom_ *= (e.wheelDelta > 0) ? 1.1f : 0.9f;
        invalidate();
    }
};
```

## License

MIT
