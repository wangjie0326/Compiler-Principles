#pragma once

#include <string>
#include <vector>

struct ParseTreeNode {
    std::string symbol;
    std::string lexeme;
    std::string value;
    std::vector<int> children;

    std::string place;
    std::vector<std::string> code;
    std::string type = "int";   // 新增类型属性，默认为 int
};
