#pragma once

// 引入文法模块。
// Parser 内部需要 Grammar 来获取产生式、开始符号、结束符等文法信息。
#include "Grammar.h"

// 引入词法分析器模块。
// Parser 需要从 Lexer 中不断读取 token。
#include "Lexer.h"

// 引入 FIRST/FOLLOW 集模块。
// Parser 初始化时会计算 FIRST/FOLLOW 集，并供 SLRTable 构造分析表使用。
#include "FirstFollow.h"

// 引入 LR(0) 自动机模块。
// Parser 初始化时会构造 LR(0) 项目集族。
#include "LR0Automaton.h"

// 引入 SLR 分析表模块。
// Parser 需要根据 SLRTable 中的 ACTION/GOTO 表执行语法分析。
#include "SLRTable.h"

#include <ostream>
#include <string>
#include <vector>

// ParseTreeNode 表示语法分析树中的一个节点。
// 每个节点可以是终结符节点，也可以是非终结符节点。
struct ParseTreeNode {
    // 当前节点对应的文法符号。
    // 例如 "Stmt"、"Expr"、"IDN" 等。
    std::string symbol;

    // 当前节点对应的词素内容。
    // 通常只有叶子节点才有 lexeme，例如 IDN 的具体变量名。
    std::string lexeme;

    // 当前节点的子节点编号列表。
    // 子节点编号对应 parseTreeNodes_ 数组中的下标。
    std::vector<int> children;
};

// Parser 类是语法分析器主体。
// 它从 Lexer 获取 token，使用 SLR 分析表进行移进/规约分析，
// 并记录规约序列、分析过程、错误信息和语法树。
class Parser {
public:
    // 构造函数。
    // 传入 Lexer 引用，Parser 会通过该词法分析器读取 token。
    explicit Parser(Lexer& lexer);

    // 执行语法分析。
    // 返回 true 表示语法分析成功，返回 false 表示语法分析失败。
    bool parse();

    // 返回语法分析过程中的规约产生式编号序列。
    const std::vector<int>& reductions() const;

    // 返回移进/规约过程记录。
    // 主要用于输出 Shift/Reduce Trace。
    const std::vector<std::string>& trace() const;

    // 返回语法分析过程中产生的错误信息。
    const std::vector<std::string>& errors() const;

    // 返回语法分析树的所有节点。
    const std::vector<ParseTreeNode>& parseTreeNodes() const;

    // 返回语法分析树根节点编号。
    // 如果分析失败或没有语法树，通常为 -1。
    int parseTreeRoot() const;

    // 打印语法分析树。
    // os 表示输出流，可以是 std::cout，也可以是文件流。
    void printParseTree(std::ostream& os) const;

private:
    // 词法分析器引用。
    // Parser 不复制 Lexer，而是直接从外部传入的 lexer_ 中读取 token。
    Lexer& lexer_;

    // 文法对象。
    // 用于获取产生式、终结符、非终结符、开始符号等信息。
    Grammar grammar_;

    // FIRST/FOLLOW 集计算对象。
    // 基于 grammar_ 计算 FIRST 集和 FOLLOW 集。
    FirstFollow firstFollow_;

    // LR(0) 自动机对象。
    // 基于 grammar_ 构造 LR(0) 项目集族和状态转移。
    LR0Automaton automaton_;

    // SLR 分析表对象。
    // 基于 grammar_、firstFollow_ 和 automaton_ 构造 ACTION/GOTO 表。
    SLRTable table_;

    // 保存语法分析过程中使用的规约产生式编号。
    std::vector<int> reductions_;

    // 保存语法分析过程中的移进、规约、接受等步骤描述。
    std::vector<std::string> trace_;

    // 保存语法分析或词法分析过程中产生的错误信息。
    std::vector<std::string> errors_;

    // 保存语法分析树的所有节点。
    // 每个节点通过数组下标互相引用。
    std::vector<ParseTreeNode> parseTreeNodes_;

    // 语法树根节点编号。
    // 初始为 -1，表示还没有生成有效语法树。
    int parseTreeRoot_ = -1;

    // 将 Lexer 返回的 Token 类型转换为语法分析表中的终结符字符串。
    // 例如 TokenType::IDN 转换为 "IDN"。
    std::string tokenToTerminal(const Token& token) const;

    // 将 token 转换为便于显示的字符串。
    // 主要用于错误信息和分析过程输出。
    std::string tokenDisplay(const Token& token) const;

    // 从词法分析器中读取下一个 token。
    // 同时会检查非法 token 并记录词法错误。
    Token nextToken();

    // 添加一条语法分析过程记录。
    void addTrace(const std::string& message);

    // 添加一条错误信息。
    void addError(const std::string& message);

    // 返回某个状态下 ACTION 表中可接受的终结符集合。
    // 主要用于语法错误提示，说明当前位置期望哪些符号。
    std::vector<std::string> expectedTerminals(int state) const;

    // 创建语法树叶子节点。
    // 叶子节点通常对应输入 token。
    int createLeafNode(const std::string& symbol, const Token& token);

    // 创建语法树父节点。
    // 父节点通常对应一次规约产生的非终结符。
    int createParentNode(const std::string& symbol, const std::vector<int>& children);

    // 递归打印语法树节点。
    // nodeIndex 表示当前节点编号，depth 表示当前节点深度。
    void printParseTreeNode(std::ostream& os, int nodeIndex, int depth) const;
};