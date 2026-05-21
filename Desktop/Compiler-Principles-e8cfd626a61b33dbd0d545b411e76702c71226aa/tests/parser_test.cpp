#include "Lexer.h"
#include "Parser.h"

#include <iostream>
#include <string>
#include <vector>

struct ParserCase {
    std::string name;
    std::string source;
    bool shouldAccept;
    bool printTree;
};

int main() {
    std::vector<ParserCase> cases = {
        {
            "assignment",
            "a = 1;",
            true,
            true
        },
        {
            "expression precedence",
            "a = b + 2 * c;",
            true,
            false
        },
        {
            "while statement",
            "while a > 0 do a = a - 1;",
            true,
            false
        },
        {
            "if statement",
            "if a > b then c = 1;",
            true,
            false
        },
        {
            "if else statement",
            "if a > b then c = 1 else c = 2;",
            true,
            true
        },
        {
            "compound statement",
            "begin a = 1; b = 2; end;",
            true,
            true
        },
        {
            "guide sample",
            "while (a3+15)>0xa do if x2 = 07 then while y<z do y = x * y / z; c=b*c+d;",
            true,
            false
        },

        // 附加功能：全部关系运算
        {
            "greater equal relation",
            "if a >= b then x = 1;",
            true,
            false
        },
        {
            "less equal relation",
            "if a <= b then x = 1;",
            true,
            false
        },
        {
            "not equal relation",
            "if a <> b then x = 1;",
            true,
            false
        },

        // 附加功能：begin-end 嵌套
        {
            "nested compound statement",
            "begin a = 1; begin b = 2; c = 3; end; d = 4; end;",
            true,
            false
        },

        // 附加功能：dangling else，else 应绑定最近的 if
        {
            "dangling else",
            "if a > b then if c > d then x = 1 else x = 2;",
            true,
            true
        },

        // 附加功能：while + begin-end
        {
            "while with compound body",
            "while a <= b do begin a = a + 1; b = b - 1; end;",
            true,
            false
        },

        // 错误测试
        {
            "missing expression",
            "a = ;",
            false,
            false
        },
        {
            "missing do",
            "while a > b x = 1;",
            false,
            false
        },
        {
            "missing end",
            "begin a = 1; b = 2;",
            false,
            false
        },
        {
            "missing right parenthesis",
            "a = (b + 1;",
            false,
            false
        },
        {
            "invalid octal token",
            "a = 09;",
            false,
            false
        },
        {
            "invalid hex token",
            "a = 0xg;",
            false,
            false
        }
    };

    bool allOk = true;

    for (const auto& testCase : cases) {
        Lexer lexer;
        lexer.setInput(testCase.source);

        Parser parser(lexer);
        const bool accepted = parser.parse();

        std::cout << "============================================\n";
        std::cout << "[CASE] " << testCase.name << "\n";
        std::cout << "Source: " << testCase.source << "\n";
        std::cout << "Expected: " << (testCase.shouldAccept ? "accept" : "reject") << "\n";
        std::cout << "Accepted: " << (accepted ? "yes" : "no") << "\n";

        if (!parser.errors().empty()) {
            std::cout << "Errors:\n";
            for (const auto& error : parser.errors()) {
                std::cout << "  " << error << "\n";
            }
        }

        std::cout << "Trace:\n";
        for (const auto& step : parser.trace()) {
            std::cout << "  " << step << "\n";
        }

        if (accepted) {
            if (parser.parseTreeRoot() < 0) {
                std::cerr << "[FAIL] Parse tree root is empty for accepted case: "
                          << testCase.name << "\n";
                allOk = false;
            }

            if (parser.parseTreeNodes().empty()) {
                std::cerr << "[FAIL] Parse tree nodes are empty for accepted case: "
                          << testCase.name << "\n";
                allOk = false;
            }

            if (testCase.printTree) {
                std::cout << "Parse Tree:\n";
                parser.printParseTree(std::cout);
            }
        }

        if (accepted != testCase.shouldAccept) {
            std::cerr << "[FAIL] Unexpected parser result for case: "
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
    std::cout << "[PASS] Parser tests passed.\n";
    return 0;
}