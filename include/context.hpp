#pragma once

#include "types.hpp"

namespace wv {

class GlyphCache;
class Recording;

class Context {
public:
    virtual ~Context() = default;

    virtual bool init(i32 w, i32 h) = 0;
    virtual void beginFrame() = 0;
    virtual void resize(i32 w, i32 h) = 0;
    virtual void submit(const Recording& recording) = 0;
    virtual void present() = 0;
    virtual void setGlyphCache(GlyphCache* cache) = 0;
};

}
