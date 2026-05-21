  /**
 * @file   Lexer.cpp
 * @brief  词法分析器实现 —— 基于手工状态机，严格对应正规文法与状态转换图。
 *
 * ╔══════════════════════════════════════════════════════════════╗
 * ║                       状态机总览                            ║
 * ╠══════════════════════════════════════════════════════════════╣
 * ║  START                                                       ║
 * ║    ├─ 空白 ──────────────────────────────→ 跳过，继续 START ║
 * ║    ├─ 字母 ──────────────────────────────→ IN_ID            ║
 * ║    ├─ '0'  ──────────────────────────────→ IN_ZERO          ║
 * ║    ├─ '1'-'9' ───────────────────────────→ IN_DEC           ║
 * ║    ├─ 运算符字符 ────────────────────────→ IN_OP            ║
 * ║    └─ EOF ───────────────────────────────→ END_OF_FILE       ║
 * ║                                                              ║
 * ║  IN_ZERO                                                     ║
 * ║    ├─ 'x'/'X' ───────────────────────────→ IN_HEX_PREFIX    ║
 * ║    ├─ '0'-'7' ───────────────────────────→ IN_OCT           ║
 * ║    ├─ '8'/'9' ───────────────────────────→ IN_ILOCT         ║
 * ║    └─ 其他    ───────────────────────────→ 接受 DEC "0"      ║
 * ║                                                              ║
 * ║  IN_OCT  (累积八进制数字)                                    ║
 * ║    ├─ '0'-'7' ───────────────────────────→ 继续 IN_OCT      ║
 * ║    ├─ '8'/'9' ───────────────────────────→ 转 IN_ILOCT      ║
 * ║    ├─ 字母    ───────────────────────────→ 转 IN_ID（作标识符）║
 * ║    └─ 其他    ───────────────────────────→ 接受 OCT          ║
 * ║                                                              ║
 * ║  IN_ILOCT (累积非法八进制，允许含8/9)                        ║
 * ║    ├─ '0'-'9' ───────────────────────────→ 继续             ║
 * ║    ├─ 字母    ───────────────────────────→ 转 IN_ID          ║
 * ║    └─ 其他    ───────────────────────────→ 接受 ILOCT        ║
 * ║                                                              ║
 * ║  IN_HEX_PREFIX → IN_HEX / IN_ILHEX                          ║
 * ║    ├─ 合法十六进制字符 ───────────────────→ IN_HEX           ║
 * ║    ├─ 非法十六进制字符(g-z/G-Z) ─────────→ IN_ILHEX         ║
 * ║    └─ 其他（空白/EOF/运算符）─────────────→ 接受 HEX/ILHEX   ║
 * ║                                                              ║
 * ║  IN_DEC                                                      ║
 * ║    ├─ '0'-'9' ───────────────────────────→ 继续             ║
 * ║    └─ 其他    ───────────────────────────→ 接受 DEC          ║
 * ║                                                              ║
 * ║  IN_ID                                                       ║
 * ║    ├─ 字母 | 数字 ────────────────────────→ 继续            ║
 * ║    └─ 其他 ───────────────────────────────→ 接受，查关键字表 ║
 * ║                                                              ║
 * ║  IN_OP                                                       ║
 * ║    ├─ '>' → 接受 GT；若下一字符为 '='，吃掉，接受 GE         ║
 * ║    ├─ '<' → 若下一为 '=' 接受 LE；若下一为 '>' 接受 NEQ      ║
 * ║    │         否则接受 LT                                     ║
 * ║    └─ 其他单字符直接接受                                    ║
 * ╚══════════════════════════════════════════════════════════════╝
 *
 * 正规式（来自实验指导书）：
 *   标识符    : letter (letter | digit)*
 *   DEC       : 0 | [1-9][0-9]*
 *   OCT       : 0[0-7]+
 *   HEX       : 0(x|X)[0-9a-fA-F]+
 *   ILOCT     : 0[0-9]*[89][0-9]*      (含 8 或 9 的"八进制"前缀串)
 *   ILHEX     : 0(x|X)[0-9a-zA-Z]*[g-zG-Z][0-9a-zA-Z]*
 */

#include "Lexer.h"
#include <cctype>
#include <stdexcept>
#include <sstream>

// ============================================================
// 关键字表（静态常量，构造一次）
// ============================================================
const std::unordered_map<std::string, TokenType> Lexer::KEYWORDS = {
    {"if",    TokenType::IF},
    {"then",  TokenType::THEN},
    {"else",  TokenType::ELSE},
    {"while", TokenType::WHILE},
    {"do",    TokenType::DO},
    {"begin", TokenType::BEGIN},
    {"end",   TokenType::END},
};

// ============================================================
// 构造 / 析构
// ============================================================

Lexer::Lexer()
    : pos_(0), line_(1), col_(1), hasPeek_(false) {}

Lexer::Lexer(std::istream& is)
    : pos_(0), line_(1), col_(1), hasPeek_(false)
{
    std::ostringstream ss;
    ss << is.rdbuf();
    source_ = ss.str();
}

// ============================================================
// 输入设置
// ============================================================

void Lexer::setInput(const std::string& src) {
    source_  = src;
    pos_     = 0;
    line_    = 1;
    col_     = 1;
    hasPeek_ = false;
    errors_.clear();
}

void Lexer::reset() {
    pos_     = 0;
    line_    = 1;
    col_     = 1;
    hasPeek_ = false;
    errors_.clear();
}

// ============================================================
// 辅助查询
// ============================================================

bool Lexer::isEOF() const {
    return pos_ >= source_.size();
}

const std::vector<LexError>& Lexer::errors() const {
    return errors_;
}

// ============================================================
// 字符操作
// ============================================================

char Lexer::current() const {
    if (pos_ >= source_.size()) return '\0';
    return source_[pos_];
}

char Lexer::peekChar() const {
    if (pos_ + 1 >= source_.size()) return '\0';
    return source_[pos_ + 1];
}

char Lexer::advance() {
    if (pos_ >= source_.size()) return '\0';
    char c = source_[pos_++];
    if (c == '\n') {
        ++line_;
        col_ = 1;
    } else {
        ++col_;
    }
    return c;
}

void Lexer::skipWhitespace() {
    while (!isEOF() && isWhitespace(current())) {
        advance();
    }
}

// ============================================================
// 字符分类
// ============================================================

bool Lexer::isLetter(char c) {
    return std::isalpha(static_cast<unsigned char>(c));
}

bool Lexer::isDigit(char c) {
    return std::isdigit(static_cast<unsigned char>(c));
}

bool Lexer::isOctDigit(char c) {
    return c >= '0' && c <= '7';
}

bool Lexer::isHexDigit(char c) {
    return (c >= '0' && c <= '9') ||
           (c >= 'a' && c <= 'f') ||
           (c >= 'A' && c <= 'F');
}

bool Lexer::isIllegalOctChar(char c) {
    return c == '8' || c == '9';
}

bool Lexer::isIllegalHexChar(char c) {
    return (c >= 'g' && c <= 'z') ||
           (c >= 'G' && c <= 'Z');
}

bool Lexer::isAlphanumeric(char c) {
    return std::isalnum(static_cast<unsigned char>(c));
}

bool Lexer::isWhitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

// ============================================================
// 数值转换
// ============================================================

std::string Lexer::octToDecStr(const std::string& oct) {
    // oct 为纯数字串（不含前缀 '0'）
    long long val = 0;
    for (char c : oct) {
        val = val * 8 + (c - '0');
    }
    return std::to_string(val);
}

std::string Lexer::hexToDecStr(const std::string& hex) {
    // hex 为不含 "0x"/"0X" 的十六进制字符串
    long long val = 0;
    for (char c : hex) {
        val *= 16;
        if (c >= '0' && c <= '9') val += c - '0';
        else if (c >= 'a' && c <= 'f') val += c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') val += c - 'A' + 10;
        // 非法字符（ILHEX 路径）按 0 处理，不影响 ILHEX 的识别结果
    }
    return std::to_string(val);
}

// ============================================================
// 识别标识符 / 关键字
// ============================================================
/**
 * 进入条件：current() 是字母。
 * 正规式：letter (letter | digit)*
 * 正规文法：
 *   S → letter A
 *   A → letter A | digit A | ε
 */
Token Lexer::scanIdentifierOrKeyword() {
    int startLine = line_, startCol = col_;
    std::string lexeme;

    // 消耗首字母及后续字母/数字
    while (!isEOF() && isAlphanumeric(current())) {
        lexeme += advance();
    }

    // 查关键字表
    auto it = KEYWORDS.find(lexeme);
    if (it != KEYWORDS.end()) {
        // 关键字：属性值为 "-"
        return Token(it->second, lexeme, "-", startLine, startCol);
    }
    // 标识符：属性值为词素本身
    return Token(TokenType::IDN, lexeme, lexeme, startLine, startCol);
}

// ============================================================
// 识别数字（DEC / OCT / HEX / ILOCT / ILHEX）
// ============================================================
/**
 * 进入条件：current() 是数字字符（'0'-'9'）。
 *
 * 状态转移：
 *
 *   [START_NUM]
 *     '0' ──→ [IN_ZERO]
 *               'x'/'X' ──→ [IN_HEX_PREFIX]
 *                             合法十六进制字符 ──→ [IN_HEX / IN_ILHEX 混合]
 *               '0'-'7' ──→ [IN_OCT / IN_ILOCT 混合]
 *               '8'/'9' ──→ [IN_ILOCT]
 *               其他    ──→ 接受 DEC "0"
 *     '1'-'9' ──→ [IN_DEC]
 *                  '0'-'9' ──→ 继续
 *                  字母    ──→ 转 [IN_ID]（如 9abc 被当做非法情形，以标识符处理）
 *                  其他    ──→ 接受 DEC
 *
 * 注意：八进制与非法八进制共用同一个循环，用 hasIllegal 标记。
 *       十六进制与非法十六进制同理。
 */
Token Lexer::scanNumber() {
    int startLine = line_, startCol = col_;

    char first = advance();   // 消耗第一个数字字符

    // ----------------------------------------------------------------
    // 情形 A：以 '0' 开头
    // ----------------------------------------------------------------
    if (first == '0') {
        char next = current();

        // --- A1: 十六进制 0(x|X)... ---
        if (next == 'x' || next == 'X') {
            advance();  // 消耗 'x'/'X'

            // 十六进制数字/非法字符 混合收集
            std::string digits;
            bool hasIllegal = false;

            // 必须至少有一个字符
            if (isEOF() || (!isAlphanumeric(current()))) {
                // 0x 后面没有任何字符：作为非法十六进制返回（容错）
                errors_.push_back({"非法十六进制：缺少有效数字", startLine, startCol});
                return Token(TokenType::ILHEX, "0" + std::string(1, next), "-", startLine, startCol);
            }

            while (!isEOF() && isAlphanumeric(current())) {
                char c = current();
                if (isIllegalHexChar(c)) hasIllegal = true;
                digits += advance();
            }

            // 判断是否含非法字符
            if (hasIllegal) {
                std::string lexeme = "0" + std::string(1, next) + digits;
                return Token(TokenType::ILHEX, lexeme, "-", startLine, startCol);
            } else {
                std::string lexeme = "0" + std::string(1, next) + digits;
                return Token(TokenType::HEX, lexeme, hexToDecStr(digits), startLine, startCol);
            }
        }

        // --- A2: 八进制 / 非法八进制 0[0-9]+  或  孤立 '0' ---
        if (isDigit(next)) {
            std::string digits;
            bool hasIllegal = false;

            while (!isEOF() && isDigit(current())) {
                char c = current();
                if (isIllegalOctChar(c)) hasIllegal = true;
                digits += advance();
            }

            // 如果后面紧跟字母 → 整体变成标识符（如 "0abc"）
            // 按指导书要求：数字开头后跟字母，当前文法不支持此形式，
            // 但为了容错（不崩溃），将整段当作 UNKNOWN / ILOCT 处理。
            // 此处选择：八进制 / 非法八进制不会和字母直接相连（指导书示例无此用例），
            // 若出现则在后续 getNextToken 循环中由 UNKNOWN 处理单个非法字符。
            std::string lexeme = "0" + digits;
            if (hasIllegal) {
                return Token(TokenType::ILOCT, lexeme, "-", startLine, startCol);
            } else {
                // 合法八进制，将 "0" + digits 的数值转为十进制
                return Token(TokenType::OCT, lexeme, octToDecStr(digits), startLine, startCol);
            }
        }

        // --- A3: 孤立 '0'，十进制零 ---
        return Token(TokenType::DEC, "0", "0", startLine, startCol);
    }

    // ----------------------------------------------------------------
    // 情形 B：以 '1'-'9' 开头 → 十进制
    // ----------------------------------------------------------------
    std::string digits(1, first);
    while (!isEOF() && isDigit(current())) {
        digits += advance();
    }
    // 新增：检测小数点后跟数字 → REAL token
    if (!isEOF() && current() == '.' && isDigit(peekChar())) {
        digits += advance();  // 吃掉 '.'
        while (!isEOF() && isDigit(current())) digits += advance();
        return Token(TokenType::REAL, digits, digits, startLine, startCol);
    }
    return Token(TokenType::DEC, digits, digits, startLine, startCol);
}

// ============================================================
// 识别运算符 / 分隔符
// ============================================================
/**
 * 进入条件：current() 是运算符或分隔符字符。
 *
 * 双字符运算符需要向前预读一个字符：
 *   '>' + '=' → GE
 *   '<' + '=' → LE
 *   '<' + '>' → NEQ
 * 其余均为单字符。
 */
Token Lexer::scanOperatorOrDelimiter() {
    int startLine = line_, startCol = col_;
    char c = advance();

    switch (c) {
        case '+': return Token(TokenType::ADD,  "+", "-", startLine, startCol);
        case '-': return Token(TokenType::SUB,  "-", "-", startLine, startCol);
        case '*': return Token(TokenType::MUL,  "*", "-", startLine, startCol);
        case '/': return Token(TokenType::DIV,  "/", "-", startLine, startCol);
        case '(': return Token(TokenType::SLP,  "(", "-", startLine, startCol);
        case ')': return Token(TokenType::SRP,  ")", "-", startLine, startCol);
        case ';': return Token(TokenType::SEMI, ";", "-", startLine, startCol);
        case '=': return Token(TokenType::EQ,   "=", "-", startLine, startCol);

        case '>':
            if (!isEOF() && current() == '=') {
                advance();
                return Token(TokenType::GE, ">=", "-", startLine, startCol);
            }
            return Token(TokenType::GT, ">", "-", startLine, startCol);

        case '<':
            if (!isEOF() && current() == '=') {
                advance();
                return Token(TokenType::LE, "<=", "-", startLine, startCol);
            }
            if (!isEOF() && current() == '>') {
                advance();
                return Token(TokenType::NEQ, "<>", "-", startLine, startCol);
            }
            return Token(TokenType::LT, "<", "-", startLine, startCol);

        default:
            // 不可识别字符：记录错误，返回 UNKNOWN
            {
                std::string lex(1, c);
                std::string msg = "非法字符 '" + lex + "'";
                errors_.push_back({msg, startLine, startCol});
                return Token(TokenType::UNKNOWN, lex, "-", startLine, startCol);
            }
    }
}

// ============================================================
// getNextToken() — 核心对外接口
// ============================================================
Token Lexer::getNextToken() {
    // 如果有预读缓存，直接返回
    if (hasPeek_) {
        hasPeek_ = false;
        return peekBuf_;
    }

    // 跳过空白
    skipWhitespace();

    // 到达输入末尾
    if (isEOF()) {
        return Token(TokenType::END_OF_FILE, "", "-", line_, col_);
    }

    char c = current();

    // ---- 分派到对应识别子函数 ----
    if (isLetter(c)) {
        return scanIdentifierOrKeyword();
    }
    if (isDigit(c)) {
        return scanNumber();
    }
    // 运算符 / 分隔符 / 未知字符
    return scanOperatorOrDelimiter();
}

// ============================================================
// peek() — 预读
// ============================================================
Token Lexer::peek() {
    if (!hasPeek_) {
        peekBuf_ = getNextToken();
        hasPeek_ = true;
    }
    return peekBuf_;
}