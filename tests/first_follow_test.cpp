#include "Grammar.h"
#include "FirstFollow.h"

#include <iostream>

// FIRST/FOLLOW 模块测试程序入口。
// 该测试用于验证 FirstFollow 类计算出的 FIRST 集和 FOLLOW 集是否符合预期。
int main() {
    // 创建 Grammar 对象。
    // Grammar 中保存了实验二语法分析使用的文法、终结符、非终结符和产生式。
    Grammar grammar;

    // 创建 FirstFollow 对象。
    // 它会基于 grammar 中的文法信息计算 FIRST 集和 FOLLOW 集。
    FirstFollow firstFollow(grammar);

    // 打印测试标题。
    std::cout << "============================================\n";
    std::cout << "Lab 2 FIRST/FOLLOW Module Test\n";
    std::cout << "============================================\n";

    // 计算 FIRST 集。
    firstFollow.computeFirstSets();

    // 计算 FOLLOW 集。
    // FOLLOW 集的计算依赖 FIRST 集，因此要在 FIRST 集之后计算。
    firstFollow.computeFollowSets();

    // 打印 FIRST 集，方便人工检查。
    firstFollow.printFirstSets();

    std::cout << "\n";

    // 打印 FOLLOW 集，方便人工检查。
    firstFollow.printFollowSets();

    // 获取已经计算好的 FIRST 集。
    const auto& first = firstFollow.firstSets();

    // 获取已经计算好的 FOLLOW 集。
    const auto& follow = firstFollow.followSets();

    // ok 用来记录当前测试是否全部通过。
    // 只要有一个检查失败，就会被置为 false。
    bool ok = true;

    // 定义一个 lambda 函数，用于判断 FIRST(symbol) 中是否包含 value。
    auto firstContains = [&](const std::string& symbol, const std::string& value) {
        // 先查找 symbol 是否存在于 FIRST 集表中。
        auto it = first.find(symbol);

        // 如果没有找到该符号的 FIRST 集，返回 false。
        if (it == first.end()) {
            return false;
        }

        // 判断 value 是否在 FIRST(symbol) 中。
        return it->second.find(value) != it->second.end();
    };

    // 定义一个 lambda 函数，用于判断 FOLLOW(symbol) 中是否包含 value。
    auto followContains = [&](const std::string& symbol, const std::string& value) {
        // 先查找 symbol 是否存在于 FOLLOW 集表中。
        auto it = follow.find(symbol);

        // 如果没有找到该符号的 FOLLOW 集，返回 false。
        if (it == follow.end()) {
            return false;
        }

        // 判断 value 是否在 FOLLOW(symbol) 中。
        return it->second.find(value) != it->second.end();
    };

    // 检查 FIRST(Stmts)。
    // Stmts 可以推出以 IDN、IF、WHILE、BEGIN 开头的语句序列，也可以推出空串 ε。
    if (!firstContains("Stmts", "IDN") ||
        !firstContains("Stmts", "IF") ||
        !firstContains("Stmts", "WHILE") ||
        !firstContains("Stmts", "BEGIN") ||
        !firstContains("Stmts", "ε")) {
        std::cerr << "[FAIL] FIRST(Stmts) is incorrect.\n";
        ok = false;
    }

    // 检查 FIRST(Expr)。
    // 表达式可以以标识符、不同进制整数或左括号开头。
    if (!firstContains("Expr", "IDN") ||
        !firstContains("Expr", "DEC") ||
        !firstContains("Expr", "OCT") ||
        !firstContains("Expr", "HEX") ||
        !firstContains("Expr", "SLP")) {
        std::cerr << "[FAIL] FIRST(Expr) is incorrect.\n";
        ok = false;
    }

    // 检查 FIRST(ExprRest)。
    // ExprRest 对应表达式后续部分，可以以 ADD、SUB 开头，也可以为空。
    if (!firstContains("ExprRest", "ADD") ||
        !firstContains("ExprRest", "SUB") ||
        !firstContains("ExprRest", "ε")) {
        std::cerr << "[FAIL] FIRST(ExprRest) is incorrect.\n";
        ok = false;
    }

    // 检查 FIRST(TermRest)。
    // TermRest 对应项的后续部分，可以以 MUL、DIV 开头，也可以为空。
    if (!firstContains("TermRest", "MUL") ||
        !firstContains("TermRest", "DIV") ||
        !firstContains("TermRest", "ε")) {
        std::cerr << "[FAIL] FIRST(TermRest) is incorrect.\n";
        ok = false;
    }

    // 检查 FOLLOW(S')。
    // 增广开始符号 S' 后面应包含输入结束符 END_TOKEN。
    if (!followContains("S'", "END_TOKEN")) {
        std::cerr << "[FAIL] FOLLOW(S') should contain END_TOKEN.\n";
        ok = false;
    }

    // 检查 FOLLOW(P)。
    // P 是原始开始符号，分析完整个程序后也应遇到输入结束符。
    if (!followContains("P", "END_TOKEN")) {
        std::cerr << "[FAIL] FOLLOW(P) should contain END_TOKEN.\n";
        ok = false;
    }

    // 检查 FOLLOW(Stmts)。
    // Stmts 可能出现在 BEGIN Stmts END 中，因此后面可能跟 END；
    // 同时作为程序整体时，后面也可能是 END_TOKEN。
    if (!followContains("Stmts", "END") ||
        !followContains("Stmts", "END_TOKEN")) {
        std::cerr << "[FAIL] FOLLOW(Stmts) should contain END and END_TOKEN.\n";
        ok = false;
    }

    // 检查 FOLLOW(Stmt)。
    // Stmt 在语句序列中后面通常跟 SEMI；
    // 在 if-then-else 文法中，Stmt 后面也可能跟 ELSE。
    if (!followContains("Stmt", "SEMI") ||
        !followContains("Stmt", "ELSE")) {
        std::cerr << "[FAIL] FOLLOW(Stmt) should contain SEMI and ELSE.\n";
        ok = false;
    }

    // 检查 FOLLOW(Cond)。
    // 条件 Cond 在 if 语句中后面跟 THEN；
    // 在 while 语句中后面跟 DO。
    if (!followContains("Cond", "THEN") ||
        !followContains("Cond", "DO")) {
        std::cerr << "[FAIL] FOLLOW(Cond) should contain THEN and DO.\n";
        ok = false;
    }

    // 检查 FOLLOW(Expr)。
    // Expr 可能出现在条件表达式、赋值语句、括号表达式等位置，
    // 因此后继符号集合较多。
    if (!followContains("Expr", "GT") ||
        !followContains("Expr", "LT") ||
        !followContains("Expr", "EQ") ||
        !followContains("Expr", "GE") ||
        !followContains("Expr", "LE") ||
        !followContains("Expr", "NEQ") ||
        !followContains("Expr", "SRP") ||
        !followContains("Expr", "SEMI") ||
        !followContains("Expr", "ELSE") ||
        !followContains("Expr", "THEN") ||
        !followContains("Expr", "DO")) {
        std::cerr << "[FAIL] FOLLOW(Expr) is missing expected symbols.\n";
        ok = false;
    }

    // 如果任意检查失败，返回 1，表示测试失败。
    if (!ok) {
        return 1;
    }

    // 所有检查均通过，输出测试通过信息。
    std::cout << "[PASS] FIRST and FOLLOW sets look correct.\n";

    // 返回 0，表示测试成功。
    return 0;
}