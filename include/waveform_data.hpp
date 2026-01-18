#pragma once

#include "types.hpp"
#include <string>
#include <vector>

namespace wv {

struct SignalChange {
    u64 time;
    u64 value;
};

struct Signal {
    std::string name;
    std::string id;
    i32 width;
    std::vector<SignalChange> changes;
};

struct WaveformData {
    u64 timescale = 1;
    u64 endTime = 0;
    std::vector<Signal> signals;
};

}
