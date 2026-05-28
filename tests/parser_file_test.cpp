#include "Lexer.h"
#include "Parser.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {
bool writeTextFile(const std::string& filePath, const std::string& content) {
    std::ofstream output(filePath);
    if (!output.is_open()) {
        return false;
    }

    output << content;
    return true;
}

bool readTextFile(const std::string& filePath, std::string& content) {
    std::ifstream input(filePath);
    if (!input.is_open()) {
        return false;
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    content = buffer.str();
    return true;
}
}

struct FileParserCase {
    std::string name;
    std::string filePath;
    std::string source;
    bool shouldAccept;
};

int main() {
    std::vector<FileParserCase> cases = {
        {
            "valid source file",
            "parser_valid_input.txt",
            "begin a = 1; b = 2; end;",
            true
        },
        {
            "invalid source file",
            "parser_invalid_input.txt",
            "begin a = 1; b = 2;",
            false
        }
    };

    bool allOk = true;

    for (const auto& testCase : cases) {
        if (!writeTextFile(testCase.filePath, testCase.source)) {
            std::cerr << "[FAIL] Cannot create test file: "
                      << testCase.filePath << "\n";
            return 1;
        }

        std::string sourceFromFile;
        if (!readTextFile(testCase.filePath, sourceFromFile)) {
            std::cerr << "[FAIL] Cannot read test file: "
                      << testCase.filePath << "\n";
            return 1;
        }

        Lexer lexer;
        lexer.setInput(sourceFromFile);

        Parser parser(lexer);
        const bool accepted = parser.parse();

        std::cout << "============================================\n";
        std::cout << "[CASE] " << testCase.name << "\n";
        std::cout << "File: " << testCase.filePath << "\n";
        std::cout << "Source: " << sourceFromFile << "\n";
        std::cout << "Expected: " << (testCase.shouldAccept ? "accept" : "reject") << "\n";
        std::cout << "Accepted: " << (accepted ? "yes" : "no") << "\n";

        if (!parser.errors().empty()) {
            std::cout << "Errors:\n";
            for (const auto& error : parser.errors()) {
                std::cout << "  " << error << "\n";
            }
        }

        if (accepted) {
            if (parser.parseTreeRoot() < 0 || parser.parseTreeNodes().empty()) {
                std::cerr << "[FAIL] Parse tree should not be empty for accepted file case: "
                          << testCase.name << "\n";
                allOk = false;
            }

            std::cout << "Parse Tree:\n";
            parser.printParseTree(std::cout);
        }

        if (accepted != testCase.shouldAccept) {
            std::cerr << "[FAIL] Unexpected parser result for file case: "
                      << testCase.name << "\n";
            allOk = false;
        } else {
            std::cout << "[PASS] " << testCase.name << "\n";
        }
    }

    if (!allOk) {
        return 1;
    }

    std::cout << "============================================\n";
    std::cout << "[PASS] Parser file input tests passed.\n";
    return 0;
}