#include "vcd_parser.hpp"
#include <fstream>
#include <sstream>
#include <unordered_map>

namespace wv {

bool VcdParser::parse(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) return false;
    
    data_ = WaveformData{};
    parseHeader(file);
    parseValues(file);
    return true;
}

void VcdParser::parseHeader(std::istream& is) {
    std::string line, token;
    std::vector<std::string> scope;
    
    while (std::getline(is, line)) {
        std::istringstream iss(line);
        iss >> token;
        
        if (token == "$timescale") {
            i32 val; std::string unit;
            iss >> val >> unit;
            u64 mult = 1;
            if (unit.find("ps") != std::string::npos) mult = 1;
            else if (unit.find("ns") != std::string::npos) mult = 1000;
            else if (unit.find("us") != std::string::npos) mult = 1000000;
            else if (unit.find("ms") != std::string::npos) mult = 1000000000;
            data_.timescale = val * mult;
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
            currentTime = std::stoull(line.substr(1));
            data_.endTime = currentTime;
        }
        else if (line[0] == 'b' || line[0] == 'B') {
            size_t space = line.find(' ');
            if (space != std::string::npos) {
                std::string bits = line.substr(1, space - 1);
                std::string id = line.substr(space + 1);
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
            std::string id = line.substr(1);
            if (auto* sig = findSignal(id))
                sig->changes.push_back({currentTime, val});
        }
    }
}

Signal* VcdParser::findSignal(const std::string& id) {
    for (auto& sig : data_.signals)
        if (sig.id == id) return &sig;
    return nullptr;
}

}
