#include "Lexer.h"
#include "Parser.h"
#include "SemanticAnalyzer.h"

#include <iostream>
#include <string>
#include <vector>

struct SemanticCase {
    std::string name;
    std::string source;
    bool shouldAccept = true;
};

int main() {
    std::vector<SemanticCase> cases = {
        { "nested while/if",       "while (a3+15)>0xa do if x2 = 07 then while y<z do y=x*y/z; c=b*c+d;", true  },
        { "while simple body",     "while (a>b) a=b+c;",                                                   true  },
        { "simple assignment",     "c = b + d;",                                                            true  },
        { "if with hex",           "if (x5 > 0x0b) g=l+y;",                                                true  },
        { "if-then nested while",  "if y1 = 0xa then while y2 > y3 do l = x*(y+z);",                       true  },
        { "while nested while", "while a > 0 do while b > 0 do b = b - 1;", true },
        { "while nested if", "while a > 0 do if a > b then a = a - 1 else a = b;", true },
        { "invalid identifier",    "AAA",                                                                   false },
        { "assign literal",        "a = 1;",                                                                true  },
        { "addition",              "a = b + c;",                                                            true  },
        { "precedence",            "a = b + 2 * c;",                                                        true  },
        { "multiple assignments",  "begin x = 1; y = 2; z = x + y; end;",                                  true  },
        { "while decrement",       "while a > 0 do a = a - 1;",                                            true  },
        { "if then",               "if a > b then c = 1;",                                                  true  },
        { "if then else",          "if a > b then c = 1 else c = 2;",                                      true  },
        { "dangling else",         "if a > b then if c > d then x = 1 else x = 2;",                        true  },
        { "while compound body",   "while a <= b do begin a = a + 1; b = b - 1; end;",                     true  },
        { "nested compound",       "begin a = 1; begin b = 2; c = 3; end; d = 4; end;",                    true  },
        { "hex literal",           "a = 0xa;",                                                              true  },
        { "octal literal",         "a = 07;",                                                               true  },

        // ── 类型转换测试 ──────────────────────────────────────────────────────
        { "real literal assign",   "a = 3.14;",                                                             true  },
        { "int var + real literal","a = 1; b = a + 2.5;",                                                   true  },
        { "real var + int var",    "a = 3.14; b = 1; c = a + b;",                                          true  },
        { "assign real to int var","a = 1; a = 3.14;",                                                      true  },
        { "assign int to real var","a = 3.14; a = 2;",                                                      true  },
        { "int mul real",          "a = 2 * 3.14;",                                                         true  },
        { "real mul int assign to int (rti)", "a = 1; a = 3.14 * 2;", true },
        { "mixed type in paren",   "a = 1; b = (a + 2.5) * 3;",                                            true  },
    };

    bool allOk = true;

    for (const auto& tc : cases) {
        std::cout << "=== " << tc.name << " ===\n";
        std::cout << "src: " << tc.source << "\n";

        Lexer lexer;
        lexer.setInput(tc.source);

        Parser parser(lexer);
        const bool accepted = parser.parse();

        if (accepted != tc.shouldAccept) {
            std::cout << "[FAIL] expected " << (tc.shouldAccept ? "accept" : "reject")
                      << ", got " << (accepted ? "accept" : "reject") << "\n\n";
            allOk = false;
            continue;
        }

        if (!accepted) {
            std::cout << "[PASS] rejected as expected\n\n";
            continue;
        }

        const auto& code = parser.semanticAnalyzer().generatedCode();
        if (code.empty()) {
            std::cout << "(no code generated)\n";
        } else {
            for (const auto& instr : code) {
                std::cout << "  " << instr << "\n";
            }
        }
        std::cout << "[PASS]\n\n";
    }

    std::cout << (allOk ? "[ALL PASS]\n" : "[SOME FAILED]\n");
    return allOk ? 0 : 1;
}