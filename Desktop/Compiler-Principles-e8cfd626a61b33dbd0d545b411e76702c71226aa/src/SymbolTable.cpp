#include "SymbolTable.h"

void SymbolTable::enter(const std::string& name,
                        const std::string& type,
                        int offset) {
    if (lookup(name) != nullptr) return;
    entries_.push_back({name, type, offset});
}

SymbolEntry* SymbolTable::lookup(const std::string& name) {
    for (auto& entry : entries_) {
        if (entry.name == name) return &entry;
    }
    return nullptr;
}

const std::vector<SymbolEntry>& SymbolTable::entries() const {
    return entries_;
}
