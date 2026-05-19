#include "Grammar.h"

#include <iostream>

// Grammar 模块测试程序入口。
// 该测试用于检查 Grammar 类是否能正确初始化文法信息，
// 包括开始符号、增广开始符号、输入结束符和产生式列表。
int main() {
    // 创建 Grammar 对象。
    // 构造函数会在内部初始化实验二使用的文法。
    Grammar grammar;

    // 打印测试标题。
    std::cout << "============================================\n";
    std::cout << "实验二 Grammar 模块测试\n";
    std::cout << "============================================\n";

    // 打印原始开始符号。
    // 例如本实验中通常是 P。
    std::cout << "Start symbol: " << grammar.startSymbol() << "\n";

    // 打印增广开始符号。
    // LR/SLR 分析通常需要加入一个新的开始符号，例如 S'。
    std::cout << "Augmented start symbol: " << grammar.augmentedStartSymbol() << "\n";

    // 打印输入结束符。
    // 在语法分析中相当于教材里的 $。
    std::cout << "End token: " << grammar.endToken() << "\n\n";

    // 打印所有产生式。
    std::cout << "Productions:\n";

    // 遍历 Grammar 中保存的产生式列表。
    for (const auto& production : grammar.productions()) {
        // 将每条产生式转换成字符串形式后输出。
        // 例如：9: Assign -> IDN EQ Expr。
        std::cout << grammar.productionToString(production) << "\n";
    }

    // 打印产生式总数。
    std::cout << "\nTotal productions: " << grammar.productions().size() << "\n";

    // 检查产生式数量是否符合预期。
    // 当前实验文法应包含 30 条产生式。
    if (grammar.productions().size() != 30) {
        std::cerr << "[FAIL] Production count should be 30.\n";
        return 1;
    }

    // 如果产生式数量正确，则说明 Grammar 模块基本工作正常。
    std::cout << "[PASS] Grammar module works.\n";

    // 返回 0 表示测试通过。
    return 0;
}