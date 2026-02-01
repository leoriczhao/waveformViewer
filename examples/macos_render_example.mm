#include "waveform_viewer.hpp"
#include "vcd_parser.hpp"
#include "MacSurface.hpp"

#include <CoreGraphics/CoreGraphics.h>
#include <CoreText/CoreText.h>
#include <ImageIO/ImageIO.h>

#include <cstdio>
#include <cstdlib>

using namespace wv;

static bool write_png(const char* path, CGImageRef image) {
    if (!image) return false;

    CFURLRef url = CFURLCreateFromFileSystemRepresentation(nullptr,
                                                          reinterpret_cast<const UInt8*>(path),
                                                          (CFIndex)strlen(path),
                                                          false);
    if (!url) return false;

    // Use UTI directly to avoid deprecated MobileCoreServices constants.
    CGImageDestinationRef dest = CGImageDestinationCreateWithURL(url, CFSTR("public.png"), 1, nullptr);
    CFRelease(url);
    if (!dest) return false;

    CGImageDestinationAddImage(dest, image, nullptr);
    bool ok = CGImageDestinationFinalize(dest);
    CFRelease(dest);
    return ok;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file.vcd> [out.png]\n", argv[0]);
        return 1;
    }

    const char* vcd_path = argv[1];
    const char* out_path = (argc >= 3) ? argv[2] : "waveform.png";

    VcdParser parser;
    if (!parser.parse(vcd_path)) {
        fprintf(stderr, "Failed to parse VCD: %s\n", vcd_path);
        return 1;
    }

    const int width = 1200;
    const int height = 800;

    CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
    const size_t bytes_per_row = (size_t)width * 4;

    // BGRA 8-bit, premultiplied alpha, little-endian (matches most CoreGraphics expectations)
    CGContextRef ctx = CGBitmapContextCreate(nullptr,
                                             width,
                                             height,
                                             8,
                                             bytes_per_row,
                                             cs,
                                             kCGBitmapByteOrder32Little | kCGImageAlphaPremultipliedFirst);
    CGColorSpaceRelease(cs);

    if (!ctx) {
        fprintf(stderr, "Failed to create bitmap CGContext\n");
        return 1;
    }

    // Make (0,0) top-left to match the other backends' coordinate intuition.
    CGContextTranslateCTM(ctx, 0, height);
    CGContextScaleCTM(ctx, 1.0, -1.0);

    // Clear background
    CGContextSetRGBFillColor(ctx, 1, 1, 1, 1);
    CGContextFillRect(ctx, CGRectMake(0, 0, width, height));

    MacSurface surface;
    surface.init((SurfaceID)ctx, width, height);

    WaveformViewer viewer;
    viewer.setData(&parser.data());

    surface.beginFrame();
    viewer.render({ .surface = &surface, .viewport = { (i32)width, (i32)height, 1.0f } });
    surface.endFrame();

    CGImageRef img = CGBitmapContextCreateImage(ctx);
    bool ok = write_png(out_path, img);

    if (img) CGImageRelease(img);
    CGContextRelease(ctx);

    if (!ok) {
        fprintf(stderr, "Failed to write PNG: %s\n", out_path);
        return 1;
    }

    fprintf(stdout, "Wrote %s\n", out_path);
    return 0;
}
