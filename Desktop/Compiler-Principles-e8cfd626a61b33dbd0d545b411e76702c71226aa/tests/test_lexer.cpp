/**
 * @file   test_lexer.cpp
 * @brief  词法分析器单元测试。
 *
 * 不依赖任何第三方测试框架，所有断言均通过宏 ASSERT 实现，
 * 测试结果输出到 stdout，失败时打印详细信息并以非零状态退出。
 *
 * 测试分组：
 *   T01  基本示例（来自实验指导书）
 *   T02  十进制整数
 *   T03  八进制整数（合法）
 *   T04  非法八进制整数
 *   T05  十六进制整数（合法）
 *   T06  非法十六进制整数
 *   T07  标识符
 *   T08  关键字
 *   T09  运算符与分隔符（单字符）
 *   T10  双字符运算符
 *   T11  空白跳过
 *   T12  混合综合用例
 *   T13  边界与压力用例
 *   T14  行列号追踪
 */

#include "Lexer.h"
#include "Token.h"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <functional>
#include <cstdlib>

// ============================================================
// 测试基础设施
// ============================================================

static int  g_total  = 0;
static int  g_passed = 0;
static int  g_failed = 0;
static std::string g_currentSuite;

void beginSuite(const std::string& name) {
    g_currentSuite = name;
    std::cout << "\n[SUITE] " << name << "\n";
}

#define ASSERT_EQ(actual, expected, label) \
    do { \
        ++g_total; \
        if ((actual) == (expected)) { \
            ++g_passed; \
            std::cout << "  [PASS] " << (label) << "\n"; \
        } else { \
            ++g_failed; \
            std::cout << "  [FAIL] " << (label) \
                      << "\n         expected: " << (expected) \
                      << "\n         actual  : " << (actual) << "\n"; \
        } \
    } while (0)

// 辅助：扫描整个字符串，返回 Token 向量（不含 EOF）
std::vector<Token> scan(const std::string& src) {
    Lexer lex;
    lex.setInput(src);
    std::vector<Token> tokens;
    Token tok = lex.getNextToken();
    while (tok.type != TokenType::END_OF_FILE) {
        tokens.push_back(tok);
        tok = lex.getNextToken();
    }
    return tokens;
}

// 辅助：只取第 n 个（0-based）Token 的 type 字符串
std::string typeAt(const std::vector<Token>& toks, std::size_t n) {
    if (n >= toks.size()) return "OUT_OF_RANGE";
    return tokenTypeToString(toks[n].type);
}

std::string valueAt(const std::vector<Token>& toks, std::size_t n) {
    if (n >= toks.size()) return "OUT_OF_RANGE";
    return toks[n].value;
}

// ============================================================
// T01 — 指导书示例
// ============================================================
void testBookExample() {
    beginSuite("T01: 指导书示例");
    // 输入: "0 92+data>= 0x1f 09 ;\nwhile"
    // 期望输出:
    //   DEC 0 | DEC 92 | ADD - | IDN data | GE - |
    //   HEX 31 | ILOCT - | SEMI - | WHILE -
    auto toks = scan("0 92+data>= 0x1f 09 ;\nwhile");
    ASSERT_EQ((int)toks.size(), 9, "Token 数量");
    ASSERT_EQ(typeAt(toks,0), "DEC",   "toks[0] type=DEC");
    ASSERT_EQ(valueAt(toks,0), "0",    "toks[0] value=0");
    ASSERT_EQ(typeAt(toks,1), "DEC",   "toks[1] type=DEC");
    ASSERT_EQ(valueAt(toks,1), "92",   "toks[1] value=92");
    ASSERT_EQ(typeAt(toks,2), "ADD",   "toks[2] type=ADD");
    ASSERT_EQ(valueAt(toks,2), "-",    "toks[2] value=-");
    ASSERT_EQ(typeAt(toks,3), "IDN",   "toks[3] type=IDN");
    ASSERT_EQ(valueAt(toks,3), "data", "toks[3] value=data");
    ASSERT_EQ(typeAt(toks,4), "GE",    "toks[4] type=GE");
    ASSERT_EQ(typeAt(toks,5), "HEX",   "toks[5] type=HEX");
    ASSERT_EQ(valueAt(toks,5), "31",   "toks[5] value=31 (0x1f→31)");
    ASSERT_EQ(typeAt(toks,6), "ILOCT", "toks[6] type=ILOCT");
    ASSERT_EQ(valueAt(toks,6), "-",    "toks[6] value=-");
    ASSERT_EQ(typeAt(toks,7), "SEMI",  "toks[7] type=SEMI");
    ASSERT_EQ(typeAt(toks,8), "WHILE", "toks[8] type=WHILE");
}

// ============================================================
// T02 — 十进制整数
// ============================================================
void testDecimal() {
    beginSuite("T02: 十进制整数");
    auto toks = scan("0 1 9 10 99 100 2147483647");
    ASSERT_EQ((int)toks.size(), 7, "Token 数量");
    ASSERT_EQ(typeAt(toks,0), "DEC", "0 是 DEC");
    ASSERT_EQ(valueAt(toks,0), "0", "0 值=0");
    ASSERT_EQ(typeAt(toks,1), "DEC", "1 是 DEC");
    ASSERT_EQ(valueAt(toks,5), "100", "100 值=100");
    ASSERT_EQ(valueAt(toks,6), "2147483647", "大数值正确");
}

// ============================================================
// T03 — 合法八进制整数
// ============================================================
void testOctal() {
    beginSuite("T03: 合法八进制整数");
    // 01 → 1; 07 → 7; 010 → 8; 0777 → 511
    auto toks = scan("01 07 010 0777");
    ASSERT_EQ((int)toks.size(), 4, "Token 数量");
    ASSERT_EQ(typeAt(toks,0), "OCT",  "01 是 OCT");
    ASSERT_EQ(valueAt(toks,0), "1",   "01 十进制=1");
    ASSERT_EQ(typeAt(toks,1), "OCT",  "07 是 OCT");
    ASSERT_EQ(valueAt(toks,1), "7",   "07 十进制=7");
    ASSERT_EQ(typeAt(toks,2), "OCT",  "010 是 OCT");
    ASSERT_EQ(valueAt(toks,2), "8",   "010 十进制=8");
    ASSERT_EQ(typeAt(toks,3), "OCT",  "0777 是 OCT");
    ASSERT_EQ(valueAt(toks,3), "511", "0777 十进制=511");
}

// ============================================================
// T04 — 非法八进制整数
// ============================================================
void testIllegalOctal() {
    beginSuite("T04: 非法八进制整数");
    // 09 → ILOCT; 018 → ILOCT; 0789 → ILOCT; 08 → ILOCT
    auto toks = scan("09 018 0789 08");
    ASSERT_EQ((int)toks.size(), 4, "Token 数量");
    ASSERT_EQ(typeAt(toks,0), "ILOCT", "09 是 ILOCT");
    ASSERT_EQ(typeAt(toks,1), "ILOCT", "018 是 ILOCT");
    ASSERT_EQ(typeAt(toks,2), "ILOCT", "0789 是 ILOCT");
    ASSERT_EQ(typeAt(toks,3), "ILOCT", "08 是 ILOCT");
    // 属性值均为 "-"
    ASSERT_EQ(valueAt(toks,0), "-", "ILOCT 属性值=-");
}

// ============================================================
// T05 — 合法十六进制整数
// ============================================================
void testHex() {
    beginSuite("T05: 合法十六进制整数");
    // 0x0→0; 0xF→15; 0xff→255; 0X1A→26; 0xDEAD→57005
    auto toks = scan("0x0 0xF 0xff 0X1A 0xDEAD");
    ASSERT_EQ((int)toks.size(), 5, "Token 数量");
    ASSERT_EQ(typeAt(toks,0), "HEX",   "0x0 是 HEX");
    ASSERT_EQ(valueAt(toks,0), "0",    "0x0 十进制=0");
    ASSERT_EQ(typeAt(toks,1), "HEX",   "0xF 是 HEX");
    ASSERT_EQ(valueAt(toks,1), "15",   "0xF 十进制=15");
    ASSERT_EQ(valueAt(toks,2), "255",  "0xff 十进制=255");
    ASSERT_EQ(valueAt(toks,3), "26",   "0X1A 十进制=26");
    ASSERT_EQ(valueAt(toks,4), "57005","0xDEAD 十进制=57005");
}

// ============================================================
// T06 — 非法十六进制整数
// ============================================================
void testIllegalHex() {
    beginSuite("T06: 非法十六进制整数");
    // 0xg → ILHEX; 0xGZ → ILHEX; 0x1z → ILHEX
    auto toks = scan("0xg 0xGZ 0x1z");
    ASSERT_EQ((int)toks.size(), 3, "Token 数量");
    ASSERT_EQ(typeAt(toks,0), "ILHEX", "0xg 是 ILHEX");
    ASSERT_EQ(typeAt(toks,1), "ILHEX", "0xGZ 是 ILHEX");
    ASSERT_EQ(typeAt(toks,2), "ILHEX", "0x1z 是 ILHEX");
    ASSERT_EQ(valueAt(toks,0), "-",    "ILHEX 属性值=-");
}

// ============================================================
// T07 — 标识符
// ============================================================
void testIdentifier() {
    beginSuite("T07: 标识符");
    auto toks = scan("a ab a1 A Z abc123 _x");
    // '_' 不是字母也不是数字，会作为 UNKNOWN，后续 x 作为 IDN
    // 前 6 个是正常标识符
    ASSERT_EQ(typeAt(toks,0), "IDN",    "a 是 IDN");
    ASSERT_EQ(valueAt(toks,0), "a",     "IDN value=a");
    ASSERT_EQ(typeAt(toks,1), "IDN",    "ab 是 IDN");
    ASSERT_EQ(typeAt(toks,2), "IDN",    "a1 是 IDN");
    ASSERT_EQ(typeAt(toks,3), "IDN",    "A 是 IDN");
    ASSERT_EQ(typeAt(toks,4), "IDN",    "Z 是 IDN");
    ASSERT_EQ(typeAt(toks,5), "IDN",    "abc123 是 IDN");
    ASSERT_EQ(valueAt(toks,5), "abc123","IDN value=abc123");
}

// ============================================================
// T08 — 关键字
// ============================================================
void testKeywords() {
    beginSuite("T08: 关键字");
    auto toks = scan("if then else while do begin end");
    ASSERT_EQ((int)toks.size(), 7, "Token 数量=7");
    ASSERT_EQ(typeAt(toks,0), "IF",    "if → IF");
    ASSERT_EQ(typeAt(toks,1), "THEN",  "then → THEN");
    ASSERT_EQ(typeAt(toks,2), "ELSE",  "else → ELSE");
    ASSERT_EQ(typeAt(toks,3), "WHILE", "while → WHILE");
    ASSERT_EQ(typeAt(toks,4), "DO",    "do → DO");
    ASSERT_EQ(typeAt(toks,5), "BEGIN", "begin → BEGIN");
    ASSERT_EQ(typeAt(toks,6), "END",   "end → END");
    // 关键字属性值均为 "-"
    ASSERT_EQ(valueAt(toks,0), "-",    "IF 属性值=-");
    // 大写版本不是关键字
    auto toks2 = scan("IF WHILE");
    ASSERT_EQ(typeAt(toks2,0), "IDN",  "IF（大写）是 IDN");
    ASSERT_EQ(typeAt(toks2,1), "IDN",  "WHILE（大写）是 IDN");
}

// ============================================================
// T09 — 单字符运算符与分隔符
// ============================================================
void testSingleOp() {
    beginSuite("T09: 单字符运算符与分隔符");
    auto toks = scan("+ - * / = ( ) ;");
    ASSERT_EQ((int)toks.size(), 8, "Token 数量=8");
    ASSERT_EQ(typeAt(toks,0), "ADD",  "+ → ADD");
    ASSERT_EQ(typeAt(toks,1), "SUB",  "- → SUB");
    ASSERT_EQ(typeAt(toks,2), "MUL",  "* → MUL");
    ASSERT_EQ(typeAt(toks,3), "DIV",  "/ → DIV");
    ASSERT_EQ(typeAt(toks,4), "EQ",   "= → EQ");
    ASSERT_EQ(typeAt(toks,5), "SLP",  "( → SLP");
    ASSERT_EQ(typeAt(toks,6), "SRP",  ") → SRP");
    ASSERT_EQ(typeAt(toks,7), "SEMI", "; → SEMI");
}

// ============================================================
// T10 — 双字符运算符
// ============================================================
void testDoubleOp() {
    beginSuite("T10: 双字符运算符");
    auto toks = scan(">= <= <> > <");
    ASSERT_EQ((int)toks.size(), 5, "Token 数量=5");
    ASSERT_EQ(typeAt(toks,0), "GE",  ">= → GE");
    ASSERT_EQ(typeAt(toks,1), "LE",  "<= → LE");
    ASSERT_EQ(typeAt(toks,2), "NEQ", "<> → NEQ");
    ASSERT_EQ(typeAt(toks,3), "GT",  "> → GT");
    ASSERT_EQ(typeAt(toks,4), "LT",  "< → LT");

    // 紧贴的双字符
    auto toks2 = scan("a>=b a<>b");
    ASSERT_EQ(typeAt(toks2,0), "IDN", "a");
    ASSERT_EQ(typeAt(toks2,1), "GE",  ">=");
    ASSERT_EQ(typeAt(toks2,2), "IDN", "b");
    ASSERT_EQ(typeAt(toks2,3), "IDN", "a");
    ASSERT_EQ(typeAt(toks2,4), "NEQ", "<>");
    ASSERT_EQ(typeAt(toks2,5), "IDN", "b");
}

// ============================================================
// T11 — 空白跳过
// ============================================================
void testWhitespace() {
    beginSuite("T11: 空白跳过");
    // 多种空白字符
    auto toks = scan("  \t a \n\r\n b");
    ASSERT_EQ((int)toks.size(), 2, "仅 2 个 IDN");
    ASSERT_EQ(typeAt(toks,0), "IDN", "a");
    ASSERT_EQ(typeAt(toks,1), "IDN", "b");

    // 全空白
    auto toks2 = scan("   \t\n  ");
    ASSERT_EQ((int)toks2.size(), 0, "全空白 → 0 个 Token");
}

// ============================================================
// T12 — 混合综合用例
// ============================================================
void testMixed() {
    beginSuite("T12: 混合综合用例");
    // 实验二测试数据：while (a3+15)>0xa do if x2 = 07 then while y<z do y = x * y / z;
    auto toks = scan("while (a3+15)>0xa do if x2 = 07 then while y<z do y = x * y / z;");
    // 检查关键 token
    ASSERT_EQ(typeAt(toks,0),  "WHILE", "[0] while");
    ASSERT_EQ(typeAt(toks,1),  "SLP",   "[1] (");
    ASSERT_EQ(typeAt(toks,2),  "IDN",   "[2] a3");
    ASSERT_EQ(valueAt(toks,2), "a3",    "[2] value=a3");
    ASSERT_EQ(typeAt(toks,3),  "ADD",   "[3] +");
    ASSERT_EQ(typeAt(toks,4),  "DEC",   "[4] 15");
    ASSERT_EQ(typeAt(toks,5),  "SRP",   "[5] )");
    ASSERT_EQ(typeAt(toks,6),  "GT",    "[6] >");
    ASSERT_EQ(typeAt(toks,7),  "HEX",   "[7] 0xa");
    ASSERT_EQ(valueAt(toks,7), "10",    "[7] 0xa=10");
    ASSERT_EQ(typeAt(toks,8),  "DO",    "[8] do");
    ASSERT_EQ(typeAt(toks,9),  "IF",    "[9] if");
    ASSERT_EQ(typeAt(toks,10), "IDN",   "[10] x2");
    ASSERT_EQ(typeAt(toks,11), "EQ",    "[11] =");
    ASSERT_EQ(typeAt(toks,12), "OCT",   "[12] 07");
    ASSERT_EQ(valueAt(toks,12), "7",    "[12] 07 oct→dec=7");
    ASSERT_EQ(typeAt(toks,13), "THEN",  "[13] then");
}

// ============================================================
// T13 — 边界与压力
// ============================================================
void testEdgeCases() {
    beginSuite("T13: 边界与压力用例");

    // 空输入
    auto toks0 = scan("");
    ASSERT_EQ((int)toks0.size(), 0, "空输入 → 0 token");

    // 单个字符
    ASSERT_EQ((int)scan("0").size(), 1, "单个 '0' → 只有1 token");
    ASSERT_EQ(typeAt(scan("0"),0), "DEC", "单个 '0' 是 DEC");

    // 0 紧跟字母（如 0abc → DEC(0) + IDN(abc)）
    auto toks1 = scan("0abc");
    ASSERT_EQ((int)toks1.size(), 2, "0abc → 2 token");
    ASSERT_EQ(typeAt(toks1,0), "DEC", "0abc[0]=DEC");
    ASSERT_EQ(typeAt(toks1,1), "IDN", "0abc[1]=IDN");

    // 关键字不是前缀：iff / whiles → IDN
    auto toks2 = scan("iff whiles");
    ASSERT_EQ(typeAt(toks2,0), "IDN", "iff 是 IDN");
    ASSERT_EQ(typeAt(toks2,1), "IDN", "whiles 是 IDN");

    // 连续运算符不含空白
    auto toks3 = scan("a+b*c-d/e");
    ASSERT_EQ((int)toks3.size(), 9, "a+b*c-d/e → 9 token");
    ASSERT_EQ(typeAt(toks3,1), "ADD", "+");
    ASSERT_EQ(typeAt(toks3,3), "MUL", "*");
    ASSERT_EQ(typeAt(toks3,5), "SUB", "-");
    ASSERT_EQ(typeAt(toks3,7), "DIV", "/");

    // 0x 后无合法字符（容错）
    auto toks4 = scan("0x+1");
    ASSERT_EQ(typeAt(toks4,0), "ILHEX", "0x 后紧跟非字母数字 → ILHEX");

    // 大数八进制
    auto toks5 = scan("0777777");
    ASSERT_EQ(typeAt(toks5,0), "OCT", "大八进制合法");

    // 混合非法：0x1fGz → ILHEX
    auto toks6 = scan("0x1fGz");
    ASSERT_EQ(typeAt(toks6,0), "ILHEX", "0x1fGz 是 ILHEX");

    // >= 后面紧跟数字
    auto toks7 = scan("a>=3");
    ASSERT_EQ(typeAt(toks7,1), "GE",  ">=");
    ASSERT_EQ(typeAt(toks7,2), "DEC", "3");

    // peek 接口测试
    {
        Lexer lex;
        lex.setInput("42 abc");
        Token p1 = lex.peek();
        Token p2 = lex.peek();   // 连续 peek 应返回同一个
        Token g1 = lex.getNextToken();
        Token g2 = lex.getNextToken();
        ASSERT_EQ(tokenTypeToString(p1.type), "DEC",  "peek 返回 DEC");
        ASSERT_EQ(p1.value, p2.value,                 "连续 peek 结果相同");
        ASSERT_EQ(tokenTypeToString(g1.type), "DEC",  "getNextToken 消耗 peek");
        ASSERT_EQ(tokenTypeToString(g2.type), "IDN",  "第二个 getNextToken = IDN");
    }
}

// ============================================================
// T14 — 行列号追踪
// ============================================================
void testLineCol() {
    beginSuite("T14: 行列号追踪");
    Lexer lex;
    lex.setInput("ab\ncd ef");
    Token t1 = lex.getNextToken();
    Token t2 = lex.getNextToken();
    Token t3 = lex.getNextToken();
    ASSERT_EQ(t1.line, 1, "ab 在第 1 行");
    ASSERT_EQ(t1.col,  1, "ab 从第 1 列开始");
    ASSERT_EQ(t2.line, 2, "cd 在第 2 行");
    ASSERT_EQ(t2.col,  1, "cd 从第 1 列开始");
    ASSERT_EQ(t3.line, 2, "ef 在第 2 行");
    ASSERT_EQ(t3.col,  4, "ef 从第 4 列开始");
}

// ============================================================
// 入口
// ============================================================
int main() {
    std::cout << "============================================\n";
    std::cout << "  词法分析器单元测试\n";
    std::cout << "============================================\n";

    testBookExample();
    testDecimal();
    testOctal();
    testIllegalOctal();
    testHex();
    testIllegalHex();
    testIdentifier();
    testKeywords();
    testSingleOp();
    testDoubleOp();
    testWhitespace();
    testMixed();
    testEdgeCases();
    testLineCol();

    std::cout << "\n============================================\n";
    std::cout << "  结果：" << g_passed << "/" << g_total << " 通过"
              << (g_failed > 0 ? "，" + std::to_string(g_failed) + " 失败" : "，全部通过！")
              << "\n";
    std::cout << "============================================\n";

    return (g_failed > 0) ? 1 : 0;
}