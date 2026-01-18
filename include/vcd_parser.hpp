#pragma once

#include "waveform_data.hpp"
#include <string>

namespace wv {

class VcdParser {
public:
    bool parse(const std::string& filename);
    const WaveformData& data() const { return data_; }
    
private:
    WaveformData data_;
    
    void parseHeader(std::istream& is);
    void parseValues(std::istream& is);
    Signal* findSignal(const std::string& id);
};

}
