# AGENTS.md - Coding Agent Guidelines

## Build Commands

```bash
# Configure and build
mkdir -p build && cd build
cmake ..
make

# Build with specific options
cmake -DWV_ENABLE_EXAMPLES=ON -DWV_ENABLE_TESTS=ON ..
cmake -DWV_OFFICIAL_BUILD=ON -DWV_WERROR=ON ..
cmake -DWV_ENABLE_GL=OFF ..
cmake -DWV_SHARED_LIB=ON ..

# Clean build
rm -rf build && mkdir build && cd build && cmake .. && make
```

## Test Commands

```bash
# Run all tests
cd build && ./waveform_tests

# Run single test (GTest filter)
./waveform_tests --gtest_filter=VcdParserTest.ParsesTimescale
./waveform_tests --gtest_filter=WaveformViewerTest.*

# Run tests with verbose output
./waveform_tests --gtest_output=xml:test_results.xml

# List available tests
./waveform_tests --gtest_list_tests
```

## Project Structure

```
waveformViewer/
├── include/           # Public headers
│   ├── types.hpp      # Type aliases (i32, u32, f32, etc.)
│   ├── surface.hpp    # Abstract rendering interface
│   ├── waveform_data.hpp
│   ├── waveform_viewer.hpp
│   └── vcd_parser.hpp
├── src/               # Implementation files
├── xcb/               # XCB surface implementation
├── examples/          # Example applications
└── tests/             # GTest test files
```

## Code Style Guidelines

### Namespace

All code lives in the `wv` namespace:

```cpp
namespace wv {
// code here
}
```

### Type Aliases

Use project-defined type aliases from `types.hpp`:

```cpp
using i32 = int32_t;
using u32 = uint32_t;
using u64 = uint64_t;
using u8 = uint8_t;
using f32 = float;
using f64 = double;
```

Prefer these over raw types: `i32` not `int`, `f32` not `float`.

### Naming Conventions

| Element | Convention | Example |
|---------|------------|---------|
| Classes/Structs | PascalCase | `WaveformViewer`, `SignalChange` |
| Functions/Methods | camelCase | `parseHeader()`, `setData()` |
| Variables | camelCase | `currentTime`, `visibleStart` |
| Member variables | trailing underscore | `data_`, `w_`, `h_`, `timeScale_` |
| Constants | camelCase or UPPER_SNAKE | `tickColor`, `XCB_EVENT_MASK_EXPOSURE` |
| Namespaces | lowercase | `wv` |

### Header Guards

Use `#pragma once` (not include guards):

```cpp
#pragma once

#include "types.hpp"
// ...
```

### Include Order

1. Corresponding header (for .cpp files)
2. Project headers
3. System/library headers

```cpp
#include "vcd_parser.hpp"    // Corresponding header first
#include "waveform_data.hpp" // Project headers
#include <fstream>           // System headers
#include <sstream>
```

### Class Layout

```cpp
class MyClass {
public:
    // Constructors/destructor
    // Public methods
    
private:
    // Private methods
    // Member variables (with trailing underscore)
};
```

### Formatting

- 4-space indentation
- Opening brace on same line
- No space before parentheses in function calls
- Space after keywords (`if`, `for`, `while`)

```cpp
void WaveformViewer::paint(Surface* s) {
    if (!data_) return;
    
    for (const auto& sig : data_->signals) {
        drawSignal(s, sig, y);
        y += signalHeight_ + 5;
    }
}
```

### Error Handling

- Return `bool` for operations that can fail
- Use early returns for error conditions
- Avoid exceptions except for parsing external input

```cpp
bool VcdParser::parse(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) return false;
    
    // ... parsing logic
    return true;
}
```

### Pointers and References

- Use raw pointers for non-owning references
- Use `const` for read-only access
- Prefer references for guaranteed non-null parameters

```cpp
void setData(const WaveformData* data);  // Non-owning, nullable
void drawSignal(Surface* s, const Signal& sig, i32 y);  // Non-null surface, const ref
```

### Virtual Methods

- Use `override` keyword for overridden methods
- Use `= default` for trivial destructors

```cpp
class Surface {
public:
    virtual ~Surface() = default;
    virtual void fillRect(Rect r, Color c) = 0;
};

class XcbSurface : public Surface {
public:
    void fillRect(Rect r, Color c) override;
};
```

### Struct Initialization

Use aggregate initialization:

```cpp
Color tickColor = {80, 80, 90, 255};
Rect clip = {f32(nameWidth_), 0, f32(w_ - nameWidth_), f32(h_)};
```

### Comments

- Minimal comments; code should be self-documenting
- No redundant comments explaining obvious code
- Use comments for non-obvious logic or TODOs

## Dependencies

- CMake 3.16+
- C++17 compiler
- XCB library (`libxcb-dev` on Debian/Ubuntu)
- GTest (optional, for tests)

## Testing Patterns

Use GTest fixtures for shared setup:

```cpp
class VcdParserTest : public ::testing::Test {
protected:
    void writeVcd(const char* content) {
        std::ofstream f("/tmp/test.vcd");
        f << content;
    }
};

TEST_F(VcdParserTest, ParsesTimescale) {
    writeVcd(R"($timescale 1ns $end)");
    VcdParser parser;
    ASSERT_TRUE(parser.parse("/tmp/test.vcd"));
    EXPECT_EQ(parser.data().timescale, 1000);
}
```

## Common Patterns

### Repaint Flag Pattern

```cpp
bool needsRepaint_ = false;

void someAction() {
    // ... modify state
    needsRepaint_ = true;
}

bool needsRepaint() const { return needsRepaint_; }
void clearRepaintFlag() { needsRepaint_ = false; }
```

### Coordinate Transformation

Time-to-pixel: `x = offset + (time - timeOffset_) * timeScale_`
Pixel-to-time: `time = timeOffset_ + (x - offset) / timeScale_`
