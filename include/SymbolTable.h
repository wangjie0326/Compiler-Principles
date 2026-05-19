#pragma once

#include <string>
#include <vector>

struct SymbolEntry {
    std::string name;
    std::string type;
    int offset;
};

class SymbolTable {
public:
    void enter(const std::string& name,
               const std::string& type = "unknown",
               int offset = -1);

    SymbolEntry* lookup(const std::string& name);

    const std::vector<SymbolEntry>& entries() const;

private:
    std::vector<SymbolEntry> entries_;
};
