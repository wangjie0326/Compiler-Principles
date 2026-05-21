#include "Grammar.h"

#include <iostream>

int main() {
    Grammar grammar;

    std::cout << "============================================\n";
    std::cout << "实验二 Grammar 模块测试\n";
    std::cout << "============================================\n";

    std::cout << "Start symbol: " << grammar.startSymbol() << "\n";
    std::cout << "Augmented start symbol: " << grammar.augmentedStartSymbol() << "\n";
    std::cout << "End token: " << grammar.endToken() << "\n\n";

    std::cout << "Productions:\n";
    for (const auto& production : grammar.productions()) {
        std::cout << grammar.productionToString(production) << "\n";
    }

    std::cout << "\nTotal productions: " << grammar.productions().size() << "\n";

    if (grammar.productions().size() != 30) {
        std::cerr << "[FAIL] Production count should be 30.\n";
        return 1;
    }

    std::cout << "[PASS] Grammar module works.\n";
    return 0;
}