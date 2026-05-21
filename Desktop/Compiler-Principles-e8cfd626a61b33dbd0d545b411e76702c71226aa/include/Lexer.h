#pragma once
/**
 * @file   Lexer.h
 * @brief  词法分析器对外公开接口。
 *
 * 使用方式（实验二 / 三直接 include 本文件）：
 * -----------------------------------------------
 *   #include "Lexer.h"
 *
 *   Lexer lexer;
 *   lexer.setInput(sourceString);        // 方式 A：传入字符串
 *   // 或：Lexer lexer(std::cin);        // 方式 B：绑定输入流
 *
 *   Token tok = lexer.getNextToken();
 *   while (tok.type != TokenType::END_OF_FILE) {
 *       // 使用 tok.type, tok.value, tok.lexeme ...
 *       tok = lexer.getNextToken();
 *   }
 *
 * 注意事项：
 *   - getNextToken() 是唯一对外的扫描接口，每次调用推进一个词素。
 *   - peek() 可预读下一个 Token 而不消耗它（实验二 LL/LR 分析时有用）。
 *   - reset() 可将输入指针重置到起始位置（测试用）。
 *   - 词法错误（UNKNOWN / ILOCT / ILHEX）会在内部记录，可通过 errors() 取出。
 */

#include "Token.h"
#include <istream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>


// LexError — 词法错误记录
struct LexError {
    std::string message;
    int         line;
    int         col;
};


// Lexer — 词法分析器
/**
 * @class Lexer
 *
 * 基于手工状态机实现的词法分析器。
 * 状态图见 docs/state_diagram.md。
 *
 * 内部状态说明（LexerState 枚举在 Lexer.cpp 中定义，不暴露到接口层）：
 *   START          — 初始 / 跳过空白
 *   IN_ID          — 正在识别标识符或关键字
 *   IN_ZERO        — 读到 '0'，判断后续（十进制0 / 八进制 / 十六进制）
 *   IN_DEC         — 正在识别非零十进制整数
 *   IN_OCT         — 正在识别八进制整数（含可能转为 ILOCT）
 *   IN_HEX_PREFIX  — 读到 '0x'/'0X'，等待十六进制数字
 *   IN_HEX         — 正在识别十六进制整数（含可能转为 ILHEX）
 *   IN_OP          — 正在识别运算符（单字符 / 双字符前瞻）
 */
class Lexer {
public:
    // ---- 构造 / 析构 ------------------------------------
    Lexer();
    explicit Lexer(std::istream& is);  ///< 绑定外部输入流

    // ---- 输入设置 ----------------------------------------
    /**
     * @brief 以字符串形式设置待分析源程序，并重置所有状态。
     *        实验主程序从 cin 一次性读入后调用此接口。
     */
    void setInput(const std::string& src);

    /**
     * @brief 重置扫描指针到起始位置（保留源程序内容）。
     *        主要用于测试用例的多次扫描。
     */
    void reset();

    // ---- 核心接口 ----------------------------------------
    /**
     * @brief  返回输入流中下一个词法单元，并推进内部指针。
     *         到达输入末尾时返回 TokenType::END_OF_FILE。
     *
     * @return 一个填好 type / lexeme / value / line / col 的 Token。
     */
    Token getNextToken();

    /**
     * @brief  预读下一个 Token 但不消耗（指针不推进）。
     *         供实验二的预测分析 / LR 分析使用。
     */
    Token peek();

    // ---- 辅助查询 ----------------------------------------
    /**
     * @brief  返回当前所有词法错误（UNKNOWN 字符等）。
     *         可在扫描完成后统一打印。
     */
    const std::vector<LexError>& errors() const;

    /**
     * @brief  是否已到达输入末尾。
     */
    bool isEOF() const;

private:
    // ---- 内部字符操作 ------------------------------------
    char  advance();          ///< 消耗当前字符，返回它，并移动到下一个
    char  current() const;    ///< 查看当前字符（不消耗）
    char  peekChar() const;   ///< 查看下一个字符（不消耗，用于双字符运算符）
    void  skipWhitespace();   ///< 跳过空格 / 制表符 / 换行

    // ---- 分类识别子函数 ----------------------------------
    Token scanIdentifierOrKeyword();  ///< 识别标识符 / 关键字
    Token scanNumber();               ///< 识别十进制 / 八进制 / 十六进制 / 非法整数
    Token scanOperatorOrDelimiter();  ///< 识别运算符 / 分隔符

    // ---- 数值转换 ----------------------------------------
    std::string octToDecStr(const std::string& oct);  ///< 八进制字符串 → 十进制字符串
    std::string hexToDecStr(const std::string& hex);  ///< 十六进制字符串 → 十进制字符串

    // ---- 字符判断 ----------------------------------------
    static bool isLetter(char c);
    static bool isDigit(char c);
    static bool isOctDigit(char c);
    static bool isHexDigit(char c);
    static bool isIllegalOctChar(char c);   ///< '8' 或 '9'
    static bool isIllegalHexChar(char c);   ///< 'g'-'z' 或 'G'-'Z'
    static bool isAlphanumeric(char c);
    static bool isWhitespace(char c);

    // ---- 关键字表 ----------------------------------------
    static const std::unordered_map<std::string, TokenType> KEYWORDS;

    // ---- 成员变量 ----------------------------------------
    std::string          source_;    ///< 待分析源程序全文
    std::size_t          pos_;       ///< 当前扫描位置（字节偏移）
    int                  line_;      ///< 当前行号（从 1 开始）
    int                  col_;       ///< 当前列号（从 1 开始）

    bool                 hasPeek_;   ///< 是否持有预读缓存
    Token                peekBuf_;   ///< 预读缓存

    std::vector<LexError> errors_;   ///< 词法错误列表
};