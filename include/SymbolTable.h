#pragma once
#include <map>
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
    std::string getType(const std::string& name) const {
    auto it = types_.find(name);
    return it != types_.end() ? it->second : "unknown";
}
void setType(const std::string& name, const std::string& type) {
    types_[name] = type;
}

private:
    std::vector<SymbolEntry> entries_;
    std::map<std::string, std::string> types_;   // 新增
};
