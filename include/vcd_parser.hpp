#pragma once

#include "waveform_data.hpp"
#include <string>
#include <unordered_map>

namespace wv {

class VcdParser {
public:
    bool parse(const std::string& filename);
    const WaveformData& data() const { return data_; }
    
private:
    WaveformData data_;
    std::unordered_map<std::string, size_t> signalIndex_;
    
    void parseHeader(std::istream& is);
    void parseValues(std::istream& is);
    Signal* findSignal(const std::string& id);
    static std::string trim(const std::string& s);
    bool parseTimescaleToken(const std::string& token);
    bool parseTimescaleParts(const std::string& value, const std::string& unit);
};

}
