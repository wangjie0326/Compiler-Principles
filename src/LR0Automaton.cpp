#include "LR0Automaton.h"

#include <iostream>
#include <sstream>
#include <stdexcept>

// LR0Automaton 构造函数。
// 保存传入的 Grammar 引用，后续所有 LR(0) 项目集构造都基于该文法进行。
LR0Automaton::LR0Automaton(const Grammar& grammar)
    : grammar_(grammar) {
}

// 构造 LR(0) 项目集族和状态转移关系。
// 主要流程：
// 1. 从增广文法的初始项目开始；
// 2. 对初始项目求 closure，得到 I0；
// 3. 对每个状态和每个文法符号计算 goto；
// 4. 如果 goto 得到的新状态没出现过，就加入 states_；
// 5. 记录状态之间的转移。
void LR0Automaton::build() {
    // 清空之前构造过的状态集合。
    states_.clear();

    // 清空之前构造过的转移列表。
    transitions_.clear();

    // 清空用于快速查找转移的 map。
    transitionMap_.clear();

    // startItems 保存初始 LR(0) 项目。
    // 这里 {0, 0} 表示：
    // 使用编号为 0 的产生式，并且点号位置在最前面。
    std::set<LR0Item> startItems;
    startItems.insert({0, 0});

    // 对初始项目求闭包，得到 LR(0) 自动机的初始状态 I0。
    states_.push_back(closure(startItems));

    // 获取一个固定顺序的文法符号列表。
    // 后续会对每个状态、每个符号都尝试计算 goto。
    const auto symbols = orderedSymbols();

    // 逐个遍历状态。
    // 注意 states_.size() 会在循环过程中增长，因为发现新状态时会 push_back。
    for (std::size_t i = 0; i < states_.size(); ++i) {
        // 对当前状态尝试读入每一个文法符号。
        for (const auto& symbol : symbols) {
            // 计算 goto(Ii, symbol)，即从状态 Ii 经过 symbol 能到达的项目集。
            const auto nextState = goTo(states_[i], symbol);

            // 如果 goto 结果为空，说明当前状态没有经过该符号的有效转移。
            if (nextState.empty()) {
                continue;
            }

            // 查看 nextState 是否已经在 states_ 中存在。
            int targetIndex = findStateIndex(nextState);

            // 如果该状态还不存在，就把它作为一个新状态加入 states_。
            if (targetIndex == -1) {
                targetIndex = static_cast<int>(states_.size());
                states_.push_back(nextState);
            }

            // 当前状态编号。
            const int fromIndex = static_cast<int>(i);

            // 在 transitionMap_ 中记录转移，便于后续通过“状态 + 符号”查找目标状态。
            transitionMap_[{fromIndex, symbol}] = targetIndex;

            // 在 transitions_ 中保存一条完整转移记录，便于打印和调试。
            transitions_.push_back({fromIndex, symbol, targetIndex});
        }
    }
}

// 返回所有 LR(0) 状态。
// 每个状态本质上是一个 LR0Item 的集合。
const std::vector<std::set<LR0Item>>& LR0Automaton::states() const {
    return states_;
}

// 返回所有 LR(0) 状态转移。
// 每条转移包括：起始状态、经过的符号、目标状态。
const std::vector<LR0Transition>& LR0Automaton::transitions() const {
    return transitions_;
}

// 计算 LR(0) 项目集闭包 closure(I)。
// 如果项目集中存在 A -> α · B β，且 B 是非终结符，
// 就需要把所有 B -> · γ 的项目加入闭包中。
// 这个过程反复进行，直到没有新项目可加入。
std::set<LR0Item> LR0Automaton::closure(const std::set<LR0Item>& items) const {
    // result 初始为传入的项目集。
    std::set<LR0Item> result = items;

    // changed 表示本轮闭包计算中是否加入了新项目。
    bool changed = true;

    // 只要闭包中还有新项目加入，就继续迭代。
    while (changed) {
        changed = false;

        // 临时保存本轮需要新增的项目。
        // 不直接在遍历 result 时插入，是为了避免遍历过程中修改容器带来的问题。
        std::vector<LR0Item> toAdd;

        // 遍历当前闭包中的每一个项目。
        for (const auto& item : result) {
            // 根据项目中的产生式编号，找到对应产生式。
            const auto& production = productionById(item.productionId);

            // 如果点号已经在产生式右部最后，说明该项目无法继续扩展闭包。
            if (item.dotPosition >= production.rhs.size()) {
                continue;
            }

            // 取出点号后面的符号。
            const auto& symbolAfterDot = production.rhs[item.dotPosition];

            // 只有点号后面是非终结符时，才需要加入该非终结符的所有产生式项目。
            if (!grammar_.isNonterminal(symbolAfterDot)) {
                continue;
            }

            // 遍历所有产生式，寻找左部等于 symbolAfterDot 的产生式。
            for (const auto& candidate : grammar_.productions()) {
                if (candidate.lhs != symbolAfterDot) {
                    continue;
                }

                // 对 B -> γ，构造项目 B -> · γ。
                LR0Item newItem{candidate.id, 0};

                // 如果该项目还不在闭包中，就加入待添加列表。
                if (result.find(newItem) == result.end()) {
                    toAdd.push_back(newItem);
                }
            }
        }

        // 将本轮收集到的新项目加入 result。
        for (const auto& item : toAdd) {
            const auto inserted = result.insert(item);
            if (inserted.second) {
                changed = true;
            }
        }
    }

    // 返回最终稳定后的闭包。
    return result;
}

// 计算 goto(I, X)。
// 含义是：在项目集 I 中，所有点号后面是符号 X 的项目，
// 都把点号向右移动一位，然后对移动后的项目集求 closure。
std::set<LR0Item> LR0Automaton::goTo(
    const std::set<LR0Item>& items,
    const std::string& symbol
) const {
    // movedItems 保存点号越过 symbol 后得到的项目。
    std::set<LR0Item> movedItems;

    // 遍历项目集中的每个项目。
    for (const auto& item : items) {
        // 找到该项目对应的产生式。
        const auto& production = productionById(item.productionId);

        // 如果点号已经在末尾，无法再移动。
        if (item.dotPosition >= production.rhs.size()) {
            continue;
        }

        // 如果点号后面的符号正好是当前要转移的 symbol，
        // 就把点号向右移动一位，形成新项目。
        if (production.rhs[item.dotPosition] == symbol) {
            movedItems.insert({item.productionId, item.dotPosition + 1});
        }
    }

    // 如果没有任何项目可以经过 symbol 移动，goto 结果为空。
    if (movedItems.empty()) {
        return {};
    }

    // 对移动后的项目集求闭包，得到完整的目标状态。
    return closure(movedItems);
}

// 将一个 LR(0) 项目转换成字符串，方便打印。
// 例如把某个项目输出为：Expr -> Term · ExprRest。
std::string LR0Automaton::itemToString(const LR0Item& item) const {
    // 根据项目中的产生式编号找到对应产生式。
    const auto& production = productionById(item.productionId);

    std::ostringstream oss;
    oss << production.lhs << " -> ";

    // 如果产生式右部为空，说明是空产生式。
    // 这里直接输出点号，表示该项目已经没有右部符号。
    if (production.rhs.empty()) {
        oss << "·";
        return oss.str();
    }

    // 遍历产生式右部，并在 dotPosition 对应的位置插入点号。
    for (std::size_t i = 0; i <= production.rhs.size(); ++i) {
        // 当遍历位置等于点号位置时，输出点号。
        if (i == item.dotPosition) {
            oss << "·";
            if (i < production.rhs.size()) {
                oss << ' ';
            }
        }

        // 输出产生式右部的符号。
        if (i < production.rhs.size()) {
            oss << production.rhs[i];
            if (i + 1 < production.rhs.size()) {
                oss << ' ';
            }
        }
    }

    return oss.str();
}

// 打印所有 LR(0) 项目集状态。
// 主要用于检查自动机构造是否正确，也可以作为实验报告截图依据。
void LR0Automaton::printStates() const {
    std::cout << "LR(0) item sets:\n";

    // 逐个打印状态 I0、I1、I2……
    for (std::size_t i = 0; i < states_.size(); ++i) {
        std::cout << "I" << i << ":\n";

        // 打印当前状态中的每一个 LR(0) 项目。
        for (const auto& item : states_[i]) {
            std::cout << "  " << itemToString(item) << "\n";
        }

        std::cout << "\n";
    }

    // 打印状态总数。
    std::cout << "Total states: " << states_.size() << "\n";
}

// 打印 LR(0) 自动机的所有状态转移。
// 例如：I0 --Stmt--> I3。
void LR0Automaton::printTransitions() const {
    std::cout << "LR(0) transitions:\n";

    // 逐条打印状态转移。
    for (const auto& transition : transitions_) {
        std::cout << "I" << transition.fromState
                  << " --" << transition.symbol << "--> "
                  << "I" << transition.toState << "\n";
    }

    // 打印转移总数。
    std::cout << "Total transitions: " << transitions_.size() << "\n";
}

// 返回固定顺序的文法符号列表。
// 这里既包含非终结符，也包含终结符。
// 构造自动机时会依次尝试对这些符号计算 goto。
std::vector<std::string> LR0Automaton::orderedSymbols() const {
    return {
        // 非终结符。
        "P", "Stmts", "Stmt", "Assign", "Cond", "RelOp",
        "Expr", "ExprRest", "Term", "TermRest", "Factor",

        // 终结符。
        "IDN", "DEC", "OCT", "HEX",
        "IF", "THEN", "ELSE", "WHILE", "DO", "BEGIN", "END",
        "ADD", "SUB", "MUL", "DIV",
        "GT", "LT", "EQ", "GE", "LE", "NEQ",
        "SLP", "SRP", "SEMI"
    };
}

// 根据产生式编号查找对应的 Production。
// 如果找不到对应编号，说明程序内部状态或文法数据有问题。
const Production& LR0Automaton::productionById(int productionId) const {
    // 遍历文法中的所有产生式。
    for (const auto& production : grammar_.productions()) {
        if (production.id == productionId) {
            return production;
        }
    }

    // 如果没有找到对应产生式，抛出运行时错误。
    throw std::runtime_error("Unknown production id: " + std::to_string(productionId));
}

// 查找某个状态是否已经存在于 states_ 中。
// 如果存在，返回该状态编号；
// 如果不存在，返回 -1。
int LR0Automaton::findStateIndex(const std::set<LR0Item>& state) const {
    // 逐个比较已有状态。
    for (std::size_t i = 0; i < states_.size(); ++i) {
        if (states_[i] == state) {
            return static_cast<int>(i);
        }
    }

    // 没找到说明这是一个新状态。
    return -1;
}