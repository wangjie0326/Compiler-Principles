#include "Lexer.h"
#include "Parser.h"

#include <iostream>
#include <string>
#include <vector>

struct ParserCase {
    std::string name;
    std::string source;
    bool shouldAccept;
};

int main() {
    std::vector<ParserCase> cases = {
        {
            "assignment",
            "a = 1;",
            true
        },
        {
            "expression precedence",
            "a = b + 2 * c;",
            true
        },
        {
            "while statement",
            "while a > 0 do a = a - 1;",
            true
        },
        {
            "if statement",
            "if a > b then c = 1;",
            true
        },
        {
            "if else statement",
            "if a > b then c = 1 else c = 2;",
            true
        },
        {
            "compound statement",
            "begin a = 1; b = 2; end;",
            true
        },
        {
            "guide sample",
            "while (a3+15)>0xa do if x2 = 07 then while y<z do y = x * y / z; c=b*c+d;",
            true
        },

        // 附加功能：全部关系运算
        {
            "greater equal relation",
            "if a >= b then x = 1;",
            true
        },
        {
            "less equal relation",
            "if a <= b then x = 1;",
            true
        },
        {
            "not equal relation",
            "if a <> b then x = 1;",
            true
        },

        // 附加功能：begin-end 嵌套
        {
            "nested compound statement",
            "begin a = 1; begin b = 2; c = 3; end; d = 4; end;",
            true
        },

        // 附加功能：dangling else，else 应绑定最近的 if
        {
            "dangling else",
            "if a > b then if c > d then x = 1 else x = 2;",
            true
        },

        // 附加功能：while + begin-end
        {
            "while with compound body",
            "while a <= b do begin a = a + 1; b = b - 1; end;",
            true
        },

        // 错误测试
        {
            "missing expression",
            "a = ;",
            false
        },
        {
            "missing do",
            "while a > b x = 1;",
            false
        },
        {
            "missing end",
            "begin a = 1; b = 2;",
            false
        },
        {
            "missing right parenthesis",
            "a = (b + 1;",
            false
        },
        {
            "invalid octal token",
            "a = 09;",
            false
        },
        {
            "invalid hex token",
            "a = 0xg;",
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