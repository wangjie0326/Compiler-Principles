#pragma once

// 引入 string，用于保存符号名称，例如 "Expr"、"IDN"、"IF" 等。
#include <string>

// 引入 vector，用于保存产生式列表，以及产生式右部的符号序列。
#include <vector>

// 引入 unordered_set，用于保存终结符集合和非终结符集合，方便快速判断某个符号属于哪一类。
#include <unordered_set>

// Production 表示一条文法产生式。
// 例如：Expr -> Term ExprRest
struct Production {
    // 产生式编号。
    // 例如 0、1、2、3，用于后续规约时输出产生式序号。
    int id;

    // 产生式左部。
    // 例如 "Expr"。
    std::string lhs;

    // 产生式右部。
    // 例如 Expr -> Term ExprRest 中，rhs = { "Term", "ExprRest" }。
    // 如果 rhs 为空，通常表示空产生式，即 A -> ε。
    std::vector<std::string> rhs;
};

// Grammar 类用于保存实验二语法分析器使用的文法信息。
// 它集中管理产生式、终结符、非终结符、开始符号等内容。
class Grammar {
public:
    // 构造函数。
    // 在 Grammar.cpp 中会初始化文法的终结符、非终结符、开始符号和产生式。
    Grammar();

    // 返回所有产生式。
    // 例如 S' -> P、P -> Stmts、Stmt -> Assign 等。
    const std::vector<Production>& productions() const;

    // 返回终结符集合。
    // 例如 IDN、DEC、IF、THEN、ELSE、ADD、SEMI 等。
    const std::unordered_set<std::string>& terminals() const;

    // 返回非终结符集合。
    // 例如 P、Stmts、Stmt、Expr、Term、Factor 等。
    const std::unordered_set<std::string>& nonterminals() const;

    // 返回原始开始符号，本实验中是 "P"。
    const std::string& startSymbol() const;

    // 返回增广开始符号，即S
    const std::string& augmentedStartSymbol() const;

    // 返回输入结束符。
    // 在语法分析中相当于教材中的 "$"。
    const std::string& endToken() const;

    // 判断某个 symbol 是否是终结符。
    // 如果 symbol 在 terminals_ 集合中，就返回 true。
    bool isTerminal(const std::string& symbol) const;

    // 判断某个 symbol 是否是非终结符。
    // 如果 symbol 在 nonterminals_ 集合中，就返回 true。
    bool isNonterminal(const std::string& symbol) const;

    // 将一条产生式转换成字符串形式，方便打印和调试。
    // 例如把 Production 转成 "9: Assign -> IDN EQ Expr"。
    std::string productionToString(const Production& p) const;

private:
    // 保存所有产生式。
    // 每一条产生式都由 Production 结构体表示。
    std::vector<Production> productions_;

    // 保存所有终结符。
    // 终结符是不能继续推导的符号，一般来自词法分析器的 token 类型。
    std::unordered_set<std::string> terminals_;

    // 保存所有非终结符。
    // 非终结符是可以继续展开推导的语法变量。
    std::unordered_set<std::string> nonterminals_;

    // 原始开始符号。
    // 表示文法真正的开始位置。
    std::string startSymbol_;

    // 增广开始符号。
    // 用于构造 LR(0) 项目集族和 SLR 分析表。
    std::string augmentedStartSymbol_;

    // 输入结束符。
    // 用于表示输入串已经读完。
    std::string endToken_;
};