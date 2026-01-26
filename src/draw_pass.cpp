#include "draw_pass.hpp"

namespace wv {

DrawPass DrawPass::create(const Recording& recording) {
    DrawPass pass;
    const auto& ops = recording.ops();
    
    if (ops.empty()) return pass;
    
    std::vector<SortKey> keys;
    keys.reserve(ops.size());
    
    u16 currentClipId = 0;
    u8 sequence = 0;
    
    for (u32 i = 0; i < ops.size(); ++i) {
        const auto& op = ops[i];
        
        if (op.type == DrawOp::Type::SetClip) {
            ++currentClipId;
            continue;
        }
        if (op.type == DrawOp::Type::ClearClip) {
            currentClipId = 0;
            continue;
        }
        
        keys.push_back(SortKey::make(currentClipId, op.type, op.color, sequence++, i));
        if (sequence == 255) sequence = 0;
    }
    
    std::stable_sort(keys.begin(), keys.end());
    
    pass.sortedIndices_.reserve(keys.size());
    for (const auto& key : keys) {
        pass.sortedIndices_.push_back(key.opIndex);
    }
    
    return pass;
}

}
