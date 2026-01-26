#pragma once

#include "types.hpp"
#include <string>
#include <vector>

namespace wv {

enum class Radix { Binary, Hex, Decimal };

struct SignalChange {
    u64 time;
    u64 value;
};

struct Signal {
    std::string name;
    std::string id;
    i32 width;
    std::vector<SignalChange> changes;
    Radix radix = Radix::Hex;
};

struct WaveformData {
    u64 timescale = 1;
    u64 endTime = 0;
    std::vector<Signal> signals;
};

}
