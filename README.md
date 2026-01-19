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

## Usage

```cpp
#include "waveform_viewer.hpp"
#include "vcd_parser.hpp"
#include "xcb_surface.hpp"

using namespace wv;

// Parse VCD file
VcdParser parser;
parser.parse("signals.vcd");

// Create surface and viewer
XcbSurface surface;
surface.init(conn, 800, 600);

WaveformViewer viewer;
viewer.setSize(800, 600);
viewer.setData(&parser.data());

// Render
viewer.paint(&surface);

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
│  - Renders signals via Surface          │
│  - Handles pan/zoom interaction         │
└─────────────────┬───────────────────────┘
                  │
┌─────────────────▼───────────────────────┐
│  Surface (abstract)                     │
│  - fillRect, drawLine, drawPolyline...  │
│  - XcbSurface (current implementation)  │
└─────────────────────────────────────────┘
```

## License

MIT
