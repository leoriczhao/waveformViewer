#include "vcd_parser.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace wv {

bool VcdParser::parse(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) return false;
    
    data_ = WaveformData{};
    signalIndex_.clear();
    parseHeader(file);
    parseValues(file);
    return true;
}

std::string VcdParser::trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

bool VcdParser::parseTimescaleParts(const std::string& value, const std::string& unit) {
    if (value.empty() || unit.empty()) return false;
    
    i32 val = 0;
    try {
        val = std::stoi(value);
    } catch (...) {
        return false;
    }
    
    u64 mult = 1;
    if (unit.find("ps") != std::string::npos) mult = 1;
    else if (unit.find("ns") != std::string::npos) mult = 1000;
    else if (unit.find("us") != std::string::npos) mult = 1000000;
    else if (unit.find("ms") != std::string::npos) mult = 1000000000;
    else return false;
    
    data_.timescale = u64(val) * mult;
    return true;
}

bool VcdParser::parseTimescaleToken(const std::string& token) {
    size_t pos = 0;
    while (pos < token.size() && std::isdigit(static_cast<unsigned char>(token[pos]))) pos++;
    if (pos == 0 || pos == token.size()) return false;
    
    std::string value = token.substr(0, pos);
    std::string unit = token.substr(pos);
    return parseTimescaleParts(value, unit);
}

void VcdParser::parseHeader(std::istream& is) {
    std::string line, token;
    std::vector<std::string> scope;
    
    while (std::getline(is, line)) {
        std::istringstream iss(line);
        iss >> token;
        if (token.empty()) continue;
        
        if (token == "$timescale") {
            std::string value, unit, endToken;
            iss >> value >> unit;
            if (unit == "$end") {
                parseTimescaleToken(value);
            } else if (!value.empty() && !unit.empty()) {
                parseTimescaleParts(value, unit);
            } else if (!value.empty()) {
                parseTimescaleToken(value);
            } else {
                iss >> value >> unit;
                parseTimescaleParts(value, unit);
            }
        }
        else if (token == "$scope") {
            std::string type, name;
            iss >> type >> name;
            scope.push_back(name);
        }
        else if (token == "$upscope") {
            if (!scope.empty()) scope.pop_back();
        }
        else if (token == "$var") {
            std::string type, id, name;
            i32 width;
            iss >> type >> width >> id >> name;
            
            std::string fullName;
            for (auto& s : scope) fullName += s + ".";
            fullName += name;
            
            data_.signals.push_back({fullName, id, width, {}});
            signalIndex_[id] = data_.signals.size() - 1;
        }
        else if (token == "$enddefinitions") {
            break;
        }
    }
}

void VcdParser::parseValues(std::istream& is) {
    std::string line;
    u64 currentTime = 0;
    
    while (std::getline(is, line)) {
        if (line.empty()) continue;
        
        if (line[0] == '#') {
            try {
                currentTime = std::stoull(line.substr(1));
                data_.endTime = currentTime;
            } catch (...) {}
        }
        else if (line[0] == 'b' || line[0] == 'B') {
            size_t space = line.find(' ');
            if (space != std::string::npos) {
                std::string bits = trim(line.substr(1, space - 1));
                std::string id = trim(line.substr(space + 1));
                u64 val = 0;
                for (char c : bits) {
                    val <<= 1;
                    if (c == '1') val |= 1;
                }
                if (auto* sig = findSignal(id))
                    sig->changes.push_back({currentTime, val});
            }
        }
        else if (line[0] == '0' || line[0] == '1' || line[0] == 'x' || line[0] == 'X' || line[0] == 'z' || line[0] == 'Z') {
            u64 val = (line[0] == '1') ? 1 : 0;
            std::string id = trim(line.substr(1));
            if (auto* sig = findSignal(id))
                sig->changes.push_back({currentTime, val});
        }
    }
}

Signal* VcdParser::findSignal(const std::string& id) {
    auto it = signalIndex_.find(id);
    if (it == signalIndex_.end()) return nullptr;
    return &data_.signals[it->second];
}

}
