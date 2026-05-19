#include "Parser.h"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <stack>

// Parser 构造函数。
// 参数 lexer 是词法分析器对象，Parser 会通过它不断获取 token。
// 初始化列表中依次构造文法、FIRST/FOLLOW、LR(0) 自动机和 SLR 分析表。
Parser::Parser(Lexer& lexer)
    : lexer_(lexer),
      grammar_(),
      firstFollow_(grammar_),
      automaton_(grammar_),
      table_(grammar_, firstFollow_, automaton_) {
    // 先计算 FIRST 集。
    firstFollow_.computeFirstSets();

    // 再计算 FOLLOW 集。
    // SLR 分析表中的规约动作需要用到 FOLLOW 集。
    firstFollow_.computeFollowSets();

    // 构造 LR(0) 项目集族和状态转移。
    automaton_.build();

    // 根据 Grammar、FOLLOW 集和 LR(0) 自动机构造 SLR(1) 分析表。
    table_.build();
}

// 执行语法分析。
// 返回 true 表示语法分析成功，返回 false 表示语法分析失败。
bool Parser::parse() {
    // 清空上一次分析留下的规约序列。
    reductions_.clear();

    // 清空上一次分析留下的移进/规约过程记录。
    trace_.clear();

    // 清空上一次分析留下的错误信息。
    errors_.clear();

    // 清空上一次构造的语法树节点。
    parseTreeNodes_.clear();

    // 初始化语法树根节点编号。
    // -1 表示当前还没有有效语法树根。
    parseTreeRoot_ = -1;

    // 状态栈：保存 LR/SLR 分析过程中的状态编号。
    std::vector<int> stateStack;

    // 符号栈：保存已经移进或规约得到的文法符号。
    std::vector<std::string> symbolStack;

    // 语法树节点栈：与符号栈对应，用于构造语法树。
    std::vector<int> nodeStack;

    // 初始状态为 I0，因此状态栈先压入 0。
    stateStack.push_back(0);

    // 从词法分析器读取第一个 token。
    Token currentToken = nextToken();

    // 将 token 类型转换成语法分析表中的终结符名称。
    std::string currentTerminal = tokenToTerminal(currentToken);

    // SLR 分析主循环。
    // 每一轮根据“当前状态 + 当前输入终结符”查询 ACTION 表。
    while (true) {
        // 当前状态就是状态栈栈顶。
        const int currentState = stateStack.back();

        // 在 ACTION 表中查找 ACTION[currentState, currentTerminal]。
        const auto actionIt = table_.actionTable().find({currentState, currentTerminal});

        // 如果 ACTION 表中没有对应动作，说明出现语法错误。
        if (actionIt == table_.actionTable().end()) {
            std::ostringstream oss;

            // 构造详细的语法错误信息：
            // 包括出错行列、当前 token、当前状态以及期望的终结符集合。
            oss << "Syntax error at line " << currentToken.line
                << ", column " << currentToken.col
                << ": unexpected " << tokenDisplay(currentToken)
                << " in state I" << currentState
                << ". Expected one of: ";

            // 获取当前状态下所有可能接受的终结符。
            const auto expected = expectedTerminals(currentState);

            // 如果没有任何期望终结符，就输出 none。
            if (expected.empty()) {
                oss << "(none)";
            } else {
                // 否则逐个输出期望的终结符。
                for (std::size_t i = 0; i < expected.size(); ++i) {
                    if (i > 0) {
                        oss << ", ";
                    }
                    oss << expected[i];
                }
            }

            // 保存错误信息。
            addError(oss.str());

            // 语法分析失败。
            return false;
        }

        // 取出当前 ACTION 动作。
        const SLRAction action = actionIt->second;

        // 如果当前动作为 Shift，执行移进。
        if (action.type == SLRActionType::Shift) {
            std::ostringstream oss;

            // 记录移进过程。
            oss << "shift " << tokenDisplay(currentToken)
                << ", go to I" << action.targetState;
            addTrace(oss.str());

            // 为当前 token 创建一个语法树叶子节点。
            const int leafIndex = createLeafNode(currentTerminal, currentToken);

            // 将当前终结符压入符号栈。
            symbolStack.push_back(currentTerminal);

            // 将 shift 后的目标状态压入状态栈。
            stateStack.push_back(action.targetState);

            // 将叶子节点编号压入节点栈。
            nodeStack.push_back(leafIndex);

            // 继续读取下一个 token。
            currentToken = nextToken();

            // 更新当前输入终结符。
            currentTerminal = tokenToTerminal(currentToken);
        } else if (action.type == SLRActionType::Reduce) {
            // 如果当前动作为 Reduce，执行规约。
            // 根据规约动作中的 productionId 找到对应产生式。
            const auto& production = grammar_.productions().at(static_cast<std::size_t>(action.productionId));

            // children 用于保存本次规约产生的父节点的子节点。
            std::vector<int> children;

            // 根据产生式右部长度，从状态栈、符号栈和节点栈中弹出相应数量的元素。
            for (std::size_t i = 0; i < production.rhs.size(); ++i) {
                // 弹出符号栈中的一个符号。
                if (!symbolStack.empty()) {
                    symbolStack.pop_back();
                }

                // 弹出状态栈中的一个状态。
                // 状态栈底部的初始状态不能被弹空，所以要求 size > 1。
                if (stateStack.size() > 1) {
                    stateStack.pop_back();
                }

                // 弹出节点栈中的一个节点，并记录为父节点的子节点。
                if (!nodeStack.empty()) {
                    children.push_back(nodeStack.back());
                    nodeStack.pop_back();
                }
            }

            // 因为弹栈时是从右往左弹出的，
            // 所以 children 当前顺序是反的，需要反转回来。
            std::reverse(children.begin(), children.end());

            // 根据产生式左部创建一个父节点。
            const int parentIndex = createParentNode(production.lhs, children);

            // 规约后，要根据当前状态和产生式左部查 GOTO 表。
            const int gotoFromState = stateStack.back();

            // 查找 GOTO[gotoFromState, production.lhs]。
            const auto gotoIt = table_.gotoTable().find({gotoFromState, production.lhs});

            // 如果 GOTO 表中没有对应项，说明分析表或状态栈存在内部错误。
            if (gotoIt == table_.gotoTable().end()) {
                std::ostringstream oss;
                oss << "Internal parser error: missing GOTO[I"
                    << gotoFromState << ", " << production.lhs << "]";
                addError(oss.str());
                return false;
            }

            // 将规约后的非终结符压入符号栈。
            symbolStack.push_back(production.lhs);

            // 将 GOTO 目标状态压入状态栈。
            stateStack.push_back(gotoIt->second);

            // 将新建的父节点压入节点栈。
            nodeStack.push_back(parentIndex);

            // 记录本次使用的规约产生式编号。
            reductions_.push_back(production.id);

            std::ostringstream oss;

            // 记录规约过程。
            oss << "reduce by " << grammar_.productionToString(production)
                << ", goto I" << gotoIt->second;
            addTrace(oss.str());
        } else if (action.type == SLRActionType::Accept) {
            // 如果当前动作为 Accept，说明语法分析成功。

            // 如果节点栈非空，栈顶就是最终语法树根节点。
            if (!nodeStack.empty()) {
                parseTreeRoot_ = nodeStack.back();
            }

            // 记录接受动作。
            addTrace("accept");

            // 语法分析成功。
            return true;
        } else {
            // 如果 ACTION 类型不是 Shift、Reduce、Accept，
            // 说明分析表中出现非法动作。
            addError("Internal parser error: invalid SLR action.");
            return false;
        }
    }
}

// 返回规约产生式编号序列。
// 该序列记录了语法分析过程中每次 reduce 使用的产生式编号。
const std::vector<int>& Parser::reductions() const {
    return reductions_;
}

// 返回移进/规约过程记录。
// trace_ 中保存了分析过程中每一步 shift、reduce、accept 的文字说明。
const std::vector<std::string>& Parser::trace() const {
    return trace_;
}

// 返回错误信息列表。
// 如果语法分析或词法分析过程中出现错误，会记录到 errors_ 中。
const std::vector<std::string>& Parser::errors() const {
    return errors_;
}

// 返回语法树节点数组。
// 每个节点用 ParseTreeNode 表示。
const std::vector<ParseTreeNode>& Parser::parseTreeNodes() const {
    return parseTreeNodes_;
}

// 返回语法树根节点编号。
// 如果语法分析失败或语法树为空，则通常为 -1。
int Parser::parseTreeRoot() const {
    return parseTreeRoot_;
}

// 打印语法树。
// 如果语法树为空，则输出提示信息。
void Parser::printParseTree(std::ostream& os) const {
    // 检查根节点编号是否合法。
    if (parseTreeRoot_ < 0 ||
        static_cast<std::size_t>(parseTreeRoot_) >= parseTreeNodes_.size()) {
        os << "(empty parse tree)\n";
        return;
    }

    // 从根节点开始递归打印语法树。
    printParseTreeNode(os, parseTreeRoot_, 0);
}

// 将词法分析器返回的 Token 类型转换成语法分析表中使用的终结符名称。
// Parser 查询 ACTION 表时，需要使用这些字符串终结符。
std::string Parser::tokenToTerminal(const Token& token) const {
    switch (token.type) {
        case TokenType::IDN: return "IDN";
        case TokenType::DEC: return "DEC";
        case TokenType::OCT: return "OCT";
        case TokenType::HEX: return "HEX";

        case TokenType::IF: return "IF";
        case TokenType::THEN: return "THEN";
        case TokenType::ELSE: return "ELSE";
        case TokenType::WHILE: return "WHILE";
        case TokenType::DO: return "DO";
        case TokenType::BEGIN: return "BEGIN";
        case TokenType::END: return "END";

        case TokenType::ADD: return "ADD";
        case TokenType::SUB: return "SUB";
        case TokenType::MUL: return "MUL";
        case TokenType::DIV: return "DIV";

        case TokenType::GT: return "GT";
        case TokenType::LT: return "LT";
        case TokenType::EQ: return "EQ";
        case TokenType::GE: return "GE";
        case TokenType::LE: return "LE";
        case TokenType::NEQ: return "NEQ";

        case TokenType::SLP: return "SLP";
        case TokenType::SRP: return "SRP";
        case TokenType::SEMI: return "SEMI";

        // 文件结束 token 转换为文法中定义的输入结束符。
        case TokenType::END_OF_FILE: return grammar_.endToken();

        // 非法 token 或未知 token 统一返回 UNKNOWN。
        // 后续语法分析通常会因此报错。
        case TokenType::ILOCT:
        case TokenType::ILHEX:
        case TokenType::UNKNOWN:
        default:
            return "UNKNOWN";
    }
}

// 将 token 转换成人能看懂的显示字符串。
// 例如 IDN("abc") 会显示为 IDN('abc')。
std::string Parser::tokenDisplay(const Token& token) const {
    std::ostringstream oss;

    // 先输出 token 类型名称。
    oss << tokenTypeToString(token.type);

    // 如果 token 有具体词素，则一起输出。
    if (!token.lexeme.empty()) {
        oss << "('" << token.lexeme << "')";
    }

    return oss.str();
}

// 从词法分析器中读取下一个 token。
// 同时检查词法分析阶段是否产生非法 token。
Token Parser::nextToken() {
    // 调用 Lexer 获取下一个 token。
    Token token = lexer_.getNextToken();

    // 如果 token 类型是非法八进制、非法十六进制或未知字符，
    // 就记录词法错误信息。
    if (token.type == TokenType::ILOCT ||
        token.type == TokenType::ILHEX ||
        token.type == TokenType::UNKNOWN) {
        std::ostringstream oss;
        oss << "Lexical error at line " << token.line
            << ", column " << token.col
            << ": invalid token " << tokenDisplay(token);
        addError(oss.str());
    }

    // 返回读取到的 token。
    return token;
}

// 添加一条分析过程记录。
void Parser::addTrace(const std::string& message) {
    trace_.push_back(message);
}

// 添加一条错误信息。
void Parser::addError(const std::string& message) {
    errors_.push_back(message);
}

// 返回某个状态下 ACTION 表中可能接受的终结符集合。
// 主要用于语法错误提示，告诉用户当前位置期望哪些符号。
std::vector<std::string> Parser::expectedTerminals(int state) const {
    std::vector<std::string> result;

    // 遍历 ACTION 表，找出当前状态下所有存在动作的终结符。
    for (const auto& entry : table_.actionTable()) {
        if (entry.first.first == state) {
            result.push_back(entry.first.second);
        }
    }

    // 排序后输出，使错误提示更稳定、可读。
    std::sort(result.begin(), result.end());
    return result;
}

// 创建语法树叶子节点。
// 叶子节点通常对应输入 token，例如 IDN、DEC、IF 等终结符。
int Parser::createLeafNode(const std::string& symbol, const Token& token) {
    ParseTreeNode node;

    // 保存节点对应的符号。
    node.symbol = symbol;

    // 保存 token 的词素内容。
    node.lexeme = token.lexeme;

    // 将节点加入语法树节点数组。
    parseTreeNodes_.push_back(node);

    // 返回新节点的下标。
    return static_cast<int>(parseTreeNodes_.size() - 1);
}

// 创建语法树父节点。
// 父节点通常对应规约得到的非终结符。
int Parser::createParentNode(const std::string& symbol, const std::vector<int>& children) {
    ParseTreeNode node;

    // 保存父节点对应的非终结符。
    node.symbol = symbol;

    // 保存该节点的子节点编号列表。
    node.children = children;

    // 将节点加入语法树节点数组。
    parseTreeNodes_.push_back(node);

    // 返回新节点的下标。
    return static_cast<int>(parseTreeNodes_.size() - 1);
}

// 递归打印语法树节点。
// depth 表示当前节点所在层数，用于控制缩进。
void Parser::printParseTreeNode(std::ostream& os, int nodeIndex, int depth) const {
    // 如果节点编号不合法，直接返回。
    if (nodeIndex < 0 ||
        static_cast<std::size_t>(nodeIndex) >= parseTreeNodes_.size()) {
        return;
    }

    // 取出当前节点。
    const auto& node = parseTreeNodes_[static_cast<std::size_t>(nodeIndex)];

    // 根据当前深度输出缩进。
    for (int i = 0; i < depth; ++i) {
        os << "  ";
    }

    // 输出节点符号。
    os << node.symbol;

    // 如果是叶子节点，并且有词素内容，就输出词素。
    if (!node.lexeme.empty()) {
        os << "('" << node.lexeme << "')";
    } else if (node.children.empty()) {
        // 如果没有词素、也没有子节点，说明这是一个 ε 节点。
        os << " -> ε";
    }

    os << "\n";

    // 递归打印所有子节点。
    for (const int childIndex : node.children) {
        printParseTreeNode(os, childIndex, depth + 1);
    }
}