#include "Lexer.h"
#include "Parser.h"

#include <iostream>
#include <string>
#include <vector>

// ParserCase 表示一个语法分析测试用例。
// 每个测试用例包含名称、源程序、期望结果以及是否打印语法树。
struct ParserCase {
    // 测试用例名称。
    std::string name;

    // 待测试的源程序字符串。
    std::string source;

    // 期望语法分析是否通过。
    // true 表示期望接受，false 表示期望拒绝。
    bool shouldAccept;

    // 是否打印该测试用例的语法树。
    bool printTree;
};

// Parser 模块测试程序入口。
// 该测试用于验证语法分析器能否正确接受合法程序、拒绝非法程序，
// 同时检查规约过程和语法树生成是否正常。
int main() {
    // 构造一组语法分析测试用例。
    // 其中既包含合法程序，也包含错误程序。
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

        // 附加功能：全部关系运算。
        // 测试 >=、<=、<> 等扩展关系运算符是否能被语法分析器识别。
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

        // 附加功能：begin-end 嵌套。
        // 测试复合语句内部继续嵌套复合语句的情况。
        {
            "nested compound statement",
            "begin a = 1; begin b = 2; c = 3; end; d = 4; end;",
            true,
            false
        },

        // 附加功能：dangling else，else 应绑定最近的 if。
        // 该用例用于验证 dangling else 冲突处理策略是否正确。
        {
            "dangling else",
            "if a > b then if c > d then x = 1 else x = 2;",
            true,
            true
        },

        // 附加功能：while + begin-end。
        // 测试 while 的循环体是复合语句的情况。
        {
            "while with compound body",
            "while a <= b do begin a = a + 1; b = b - 1; end;",
            true,
            false
        },

        // 错误测试。
        // 以下用例用于验证语法分析器能否正确拒绝非法输入。
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

    // allOk 用来记录所有测试用例是否全部通过。
    // 只要有一个用例失败，就会被置为 false。
    bool allOk = true;

    // 逐个执行测试用例。
    for (const auto& testCase : cases) {
        // 创建词法分析器。
        Lexer lexer;

        // 将当前测试用例的源程序输入给词法分析器。
        lexer.setInput(testCase.source);

        // 创建语法分析器。
        // Parser 会从 lexer 中获取 token，并执行 SLR 语法分析。
        Parser parser(lexer);

        // 执行语法分析。
        const bool accepted = parser.parse();

        // 打印当前测试用例的基本信息。
        std::cout << "============================================\n";
        std::cout << "[CASE] " << testCase.name << "\n";
        std::cout << "Source: " << testCase.source << "\n";
        std::cout << "Expected: " << (testCase.shouldAccept ? "accept" : "reject") << "\n";
        std::cout << "Accepted: " << (accepted ? "yes" : "no") << "\n";

        // 如果分析过程中存在错误信息，则输出错误列表。
        if (!parser.errors().empty()) {
            std::cout << "Errors:\n";

            // 逐条打印错误信息。
            for (const auto& error : parser.errors()) {
                std::cout << "  " << error << "\n";
            }
        }

        // 打印语法分析过程。
        // trace 中记录了 shift、reduce、accept 等步骤。
        std::cout << "Trace:\n";
        for (const auto& step : parser.trace()) {
            std::cout << "  " << step << "\n";
        }

        // 如果语法分析成功，则进一步检查语法树是否生成。
        if (accepted) {
            // 成功接受的程序应该有有效的语法树根节点。
            if (parser.parseTreeRoot() < 0) {
                std::cerr << "[FAIL] Parse tree root is empty for accepted case: "
                          << testCase.name << "\n";
                allOk = false;
            }

            // 成功接受的程序应该生成语法树节点。
            if (parser.parseTreeNodes().empty()) {
                std::cerr << "[FAIL] Parse tree nodes are empty for accepted case: "
                          << testCase.name << "\n";
                allOk = false;
            }

            // 如果当前测试用例要求打印语法树，则输出语法树。
            if (testCase.printTree) {
                std::cout << "Parse Tree:\n";
                parser.printParseTree(std::cout);
            }
        }

        // 判断实际分析结果是否符合预期。
        if (accepted != testCase.shouldAccept) {
            std::cerr << "[FAIL] Unexpected parser result for case: "
                      << testCase.name << "\n";
            allOk = false;
        } else {
            // 当前测试用例通过。
            std::cout << "[PASS] " << testCase.name << "\n";
        }
    }

    // 如果任意测试用例失败，返回 1 表示测试失败。
    if (!allOk) {
        return 1;
    }

    // 所有测试用例均通过。
    std::cout << "============================================\n";
    std::cout << "[PASS] Parser tests passed.\n";

    // 返回 0 表示测试成功。
    return 0;
}