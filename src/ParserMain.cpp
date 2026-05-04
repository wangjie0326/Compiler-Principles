#include "Lexer.h"
#include "Parser.h"
#include "Grammar.h"

#include <iostream>
#include <sstream>
#include <string>

int main() {
    std::cout << "============================================\n";
    std::cout << "Lab 2 SLR Parser\n";
    std::cout << "============================================\n";
    std::cout << "Input source program, then press Ctrl+Z and Enter on Windows to finish:\n\n";

    std::ostringstream inputBuffer;
    std::string line;

    while (std::getline(std::cin, line)) {
        inputBuffer << line << '\n';
    }

    const std::string source = inputBuffer.str();

    if (source.empty()) {
        std::cout << "No input provided.\n";
        return 0;
    }

    Lexer lexer;
    lexer.setInput(source);

    Parser parser(lexer);
    const bool accepted = parser.parse();

    std::cout << "\n============================================\n";
    std::cout << "Parse Result\n";
    std::cout << "============================================\n";

    if (accepted) {
        std::cout << "Syntax analysis succeeded.\n";
    } else {
        std::cout << "Syntax analysis failed.\n";
    }

    if (!parser.errors().empty()) {
        std::cout << "\nErrors:\n";
        for (const auto& error : parser.errors()) {
            std::cout << "  " << error << "\n";
        }
    }

    std::cout << "\nShift/Reduce Trace:\n";
    for (const auto& step : parser.trace()) {
        std::cout << "  " << step << "\n";
    }

    std::cout << "\nReduction Production Sequence:\n";
    Grammar grammar;
    const auto& productions = grammar.productions();

    for (const int productionId : parser.reductions()) {
        if (productionId >= 0 &&
            static_cast<std::size_t>(productionId) < productions.size()) {
            std::cout << "  " << grammar.productionToString(
                productions[static_cast<std::size_t>(productionId)]
            ) << "\n";
        }
    }

    return accepted ? 0 : 1;
}