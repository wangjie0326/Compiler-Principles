#pragma once

#include "ParseTreeNode.h"
#include "SymbolTable.h"

#include <string>
#include <vector>

class SemanticAnalyzer {
public:
    void applyRule(int productionId,
                   std::vector<ParseTreeNode>& nodes,
                   int parentIdx,
                   const std::vector<int>& children);

    const std::vector<std::string>& generatedCode() const;
    const SymbolTable& symbolTable() const;

private:
    int tempCount_  = 0;  // 临时变量计数器：每调用 newtemp() 自增一次，生成 t1, t2, t3...
    int labelCount_ = 0;  // 跳转标签计数器：每调用 newlabel() 自增一次，生成 L1, L2, L3...
    std::vector<std::string> code_;
    SymbolTable symTable_;

    std::string newtemp();
    std::string newlabel();
    void gen(const std::string& instr);

    // 真假出口法：生成条件跳转指令序列（C.code）
    // condIdx — Cond 节点下标；C_true/C_false — 继承的真假出口标签
    std::vector<std::string> genCondCode(
        std::vector<ParseTreeNode>& nodes,
        int condIdx,
        const std::string& C_true,
        const std::string& C_false);
};
