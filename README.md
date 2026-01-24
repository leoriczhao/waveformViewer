# waveformViewer

A lightweight VCD waveform viewer for digital signal visualization. Parses VCD files and renders interactive waveform displays.

## Features

- VCD file parsing
- Interactive pan (drag) and zoom (scroll wheel)
- Signal name display
- Time scale ruler

## Dependencies

- CMake 3.16+
- C++17 compiler
- XCB (X11)
- GTest (optional, for tests)

## Building

```bash
mkdir build && cd build
cmake ..
make
```

Run the example:
```bash
./waveform_example path/to/file.vcd
```

Run with OpenGL (if available):
```bash
./waveform_example --gpu path/to/file.vcd
```

## Usage

```cpp
#include "waveform_viewer.hpp"
#include "vcd_parser.hpp"
#include "surface.hpp"

using namespace wv;

// Parse VCD file
VcdParser parser;
parser.parse("signals.vcd");

// Create surface and viewer
auto surface = Surface::MakeRaster(conn, static_cast<WindowID>(win), 800, 600);

WaveformViewer viewer;
viewer.setSize(800, 600);
viewer.setData(&parser.data());

// Render
surface->beginFrame();
viewer.paint(surface.get());
surface->endFrame();
surface->present();

// Handle input
viewer.mouseDown(x, y);    // Start drag
viewer.mouseMove(x, y);    // Pan
viewer.mouseUp();          // End drag
viewer.mouseWheel(x, delta); // Zoom
```

## Architecture

```
┌─────────────────────────────────────────┐
│  VcdParser                              │
│  - Parses .vcd files                    │
│  - Produces WaveformData                │
└─────────────────┬───────────────────────┘
                  │
┌─────────────────▼───────────────────────┐
│  WaveformViewer                         │
│  - Renders layered recordings           │
│  - Submits to target Surface            │
└─────────────────┬───────────────────────┘
                  │
┌─────────────────▼───────────────────────┐
│  Canvas + Surface                       │
│  - Canvas records to Device             │
│  - Surface owns Device + Context        │
└─────────────────────────────────────────┘
```

## License

MIT
