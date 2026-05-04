#pragma once
/**
 * @file   Token.h
 * @brief  词法单元类型定义 —— 供实验一、二、三共享的唯一 Token 数据结构。
 *
 * 设计原则：
 *   - TokenType 枚举严格对应实验指导书中的"种别值"，名称全大写。
 *   - Token 结构体携带三个字段：类型、原始词素、属性值。
 *   - 此文件只做定义，不含任何实现，零运行时开销。
 *
 * 实验二/三对接方式：
 *   只需 #include "Token.h"，无需改动本文件。
 */
 
#include <string>
 
// TokenType — 单词种别值
/**
 * 枚举规则：
 *   - 运算符 / 分隔符名称与指导书保持一致。
 *   - 关键字用全大写（IF / THEN / ELSE / WHILE / DO / BEGIN / END）。
 *   - 特殊状态：END_OF_FILE 表示输入结束，UNKNOWN 表示无法识别的单个字符。
 */
enum class TokenType {
    // 标识符
    IDN,        ///< 标识符
 
    // 整数 
    DEC,        ///< 十进制整数
    OCT,        ///< 合法八进制整数
    HEX,        ///< 合法十六进制整数
    ILOCT,      ///< 非法八进制整数
    ILHEX,      ///< 非法十六进制整数
 
    // 运算符 
    ADD,        ///< +
    SUB,        ///< -
    MUL,        ///< *
    DIV,        ///< /
    GT,         ///< >
    LT,         ///< <
    EQ,         ///< =
    GE,         ///< >=
    LE,         ///< <=
    NEQ,        ///< <>
 
    // 分隔符 
    SLP,        ///< (
    SRP,        ///< )
    SEMI,       ///< ;
 
    // 关键字
    IF,
    THEN,
    ELSE,
    WHILE,
    DO,
    BEGIN,
    END,
 
    // 特殊 
    END_OF_FILE,  ///< 输入流结束
    UNKNOWN       ///< 无法识别的字符（错误恢复用）
};
 

// Token — 词法单元
/**
 * @struct Token
 *
 * 每次调用 Lexer::getNextToken() 返回一个 Token。
 *
 * 字段说明：
 *   type    — 种别值（TokenType 枚举）
 *   lexeme  — 源程序中的原始词素（用于错误报告和语法树展示）
 *               例：标识符 "data"、整数字面量 "0x1f"
 *   value   — 属性值（用于后续分析阶段）
 *               • 标识符  → 同 lexeme（即标识符名称字符串）
 *               • 整数    → 等价十进制数值的字符串（如 HEX "0x1f" → "31"）
 *               • 运算符 / 关键字 → "-"（指导书规定无属性值）
 *   line    — 词素所在行号（从 1 开始），便于错误定位
 *   col     — 词素起始列号（从 1 开始）
 */
struct Token {
    TokenType   type;
    std::string lexeme;  ///< 原始词素
    std::string value;   ///< 属性值
    int         line;    ///< 行号
    int         col;     ///< 列号
 
    Token()
        : type(TokenType::UNKNOWN), lexeme(""), value("-"), line(0), col(0) {}
 
    Token(TokenType t, std::string lex, std::string val, int ln = 0, int cl = 0)
        : type(t), lexeme(std::move(lex)), value(std::move(val)), line(ln), col(cl) {}
};
 

// 工具函数：TokenType → 可打印字符串
/**
 * @brief 将 TokenType 转换为指导书规定的种别值字符串。
 *        供主程序打印输出使用，也供实验二/三的调试输出使用。
 */
inline std::string tokenTypeToString(TokenType t) {
    switch (t) {
        case TokenType::IDN:         return "IDN";
        case TokenType::DEC:         return "DEC";
        case TokenType::OCT:         return "OCT";
        case TokenType::HEX:         return "HEX";
        case TokenType::ILOCT:       return "ILOCT";
        case TokenType::ILHEX:       return "ILHEX";
        case TokenType::ADD:         return "ADD";
        case TokenType::SUB:         return "SUB";
        case TokenType::MUL:         return "MUL";
        case TokenType::DIV:         return "DIV";
        case TokenType::GT:          return "GT";
        case TokenType::LT:          return "LT";
        case TokenType::EQ:          return "EQ";
        case TokenType::GE:          return "GE";
        case TokenType::LE:          return "LE";
        case TokenType::NEQ:         return "NEQ";
        case TokenType::SLP:         return "SLP";
        case TokenType::SRP:         return "SRP";
        case TokenType::SEMI:        return "SEMI";
        case TokenType::IF:          return "IF";
        case TokenType::THEN:        return "THEN";
        case TokenType::ELSE:        return "ELSE";
        case TokenType::WHILE:       return "WHILE";
        case TokenType::DO:          return "DO";
        case TokenType::BEGIN:       return "BEGIN";
        case TokenType::END:         return "END";
        case TokenType::END_OF_FILE: return "EOF";
        case TokenType::UNKNOWN:     return "UNKNOWN";
        default:                     return "?";
    }
}