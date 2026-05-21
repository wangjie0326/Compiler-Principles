/**
 * @file   main.cpp
 * @brief  实验一主程序。
 *
 * 功能：
 *   从标准输入读取源程序（可多行），调用 Lexer::getNextToken()
 *   逐个取出词法单元，按"种别值  属性值"格式输出到标准输出。
 *
 * 编译与运行：
 *   cd build && cmake .. -DCMAKE_BUILD_TYPE=Release && make
 *   echo "0 92+data>= 0x1f 09 ;" | ./lexer_main
 *   ./lexer_main < tests/testcases/sample.txt
 *
 * 输出格式（每行一个 Token）：
 *   <种别值>\t<属性值>
 * 例：
 *   DEC     0
 *   DEC     92
 *   ADD     -
 *   IDN     data
 *   GE      -
 *   HEX     31
 *   ILOCT   -
 *   SEMI    -
 *   WHILE   -
 */

#include "../include/Lexer.h"
#include "../include/Token.h"
#include <iostream>
#include <sstream>
#include <string>

int main() {
    // ---- 从标准输入读取全部源程序 ----
    std::ostringstream oss;
    oss << std::cin.rdbuf();
    std::string source = oss.str();

    // ---- 初始化词法分析器 ----
    Lexer lexer;
    lexer.setInput(source);

    // ---- 循环取 Token 并打印 ----
    Token tok = lexer.getNextToken();
    while (tok.type != TokenType::END_OF_FILE) {
        std::cout << tokenTypeToString(tok.type) << "\t" << tok.value << "\n";
        tok = lexer.getNextToken();
    }

    // ---- 打印词法错误（若有） ----
    const auto& errs = lexer.errors();
    if (!errs.empty()) {
        std::cerr << "\n--- 词法错误 ---\n";
        for (const auto& e : errs) {
            std::cerr << "Line " << e.line << ", Col " << e.col
                      << ": " << e.message << "\n";
        }
    }

    return 0;
}