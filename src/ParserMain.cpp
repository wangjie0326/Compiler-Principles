#include "Lexer.h"
#include "Parser.h"
#include "Grammar.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace {
std::string readAllFromStdin() {
    std::ostringstream inputBuffer;
    std::string line;

    while (std::getline(std::cin, line)) {
        inputBuffer << line << '\n';
    }

    return inputBuffer.str();
}

bool readAllFromFile(const std::string& filePath, std::string& content, std::string& error) {
    std::ifstream inputFile(filePath);

    if (!inputFile.is_open()) {
        error = "Cannot open source file: " + filePath;
        return false;
    }

    std::ostringstream buffer;
    buffer << inputFile.rdbuf();
    content = buffer.str();

    return true;
}

void printUsage(const char* programName) {
    std::cout << "Usage:\n";
    std::cout << "  " << programName << " <source-file>\n";
    std::cout << "  " << programName << "    # read source program from standard input\n";
}
}

int main(int argc, char* argv[]) {
    std::cout << "============================================\n";
    std::cout << "Lab 2 SLR Parser\n";
    std::cout << "============================================\n";

    std::string source;

    if (argc == 1) {
        std::cout << "Input source program, then press Ctrl+Z and Enter on Windows to finish:\n\n";
        source = readAllFromStdin();
    } else if (argc == 2) {
        std::string error;
        if (!readAllFromFile(argv[1], source, error)) {
            std::cerr << error << "\n";
            printUsage(argv[0]);
            return 1;
        }

        std::cout << "Source file: " << argv[1] << "\n";
    } else {
        printUsage(argv[0]);
        return 1;
    }

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

    if (accepted) {
        std::cout << "\nParse Tree:\n";
        parser.printParseTree(std::cout);
    }

    return accepted ? 0 : 1;
}