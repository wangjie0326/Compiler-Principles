#include "FirstFollow.h"

#include <iostream>
#include <vector>

// EPSILON 表示空串 ε。
// 在 FIRST 集中，如果某个非终结符可以推出空串，就把 ε 放入它的 FIRST 集。
const std::string FirstFollow::EPSILON = "ε";

// 构造函数：传入 Grammar 对象的引用。
// FirstFollow 本身不保存文法副本，只通过 grammar_ 引用已有文法。
FirstFollow::FirstFollow(const Grammar& grammar)
    : grammar_(grammar) {
}

// 计算所有符号的 FIRST 集。
// FIRST(X) 表示：从符号 X 开始推导时，最先可能出现的终结符集合。
void FirstFollow::computeFirstSets() {
    // 每次重新计算前，先清空旧的 FIRST 集结果。
    first_.clear();

    // 对终结符来说，FIRST(终结符) 就是它自己。
    // 例如 FIRST(IDN) = { IDN }。
    for (const auto& terminal : grammar_.terminals()) {
        first_[terminal].insert(terminal);
    }

    // 对每个非终结符先建立一个空的 FIRST 集。
    // 后面通过不断迭代，把能推出的终结符逐步加入进去。
    for (const auto& nonterminal : grammar_.nonterminals()) {
        first_[nonterminal];
    }

    // changed 用来判断本轮迭代中 FIRST 集是否发生变化。
    // 只要还有新元素被加入，就继续迭代。
    bool changed = true;

    // FIRST 集通常存在相互依赖，所以需要循环计算直到稳定。
    while (changed) {
        changed = false;

        // 遍历每一条产生式，例如 A -> B C D。
        for (const auto& production : grammar_.productions()) {
            // lhsFirst 是产生式左部非终结符的 FIRST 集，即 FIRST(A)。
            auto& lhsFirst = first_[production.lhs];

            // 如果产生式右部为空，说明这是 A -> ε。
            // 因此 ε 应该加入 FIRST(A)。
            if (production.rhs.empty()) {
                if (addToSet(lhsFirst, EPSILON)) {
                    changed = true;
                }
                continue;
            }

            // 该变量表示：当前产生式右部的所有符号是否都能推出 ε。
            // 如果右部所有符号都能推出 ε，那么左部也能推出 ε。
            bool allCanDeriveEpsilon = true;

            // 按顺序扫描产生式右部符号。
            // 对 A -> X1 X2 X3 来说，先看 X1，再看 X2，再看 X3。
            for (const auto& symbol : production.rhs) {
                // 将 FIRST(symbol) 中除 ε 以外的元素加入 FIRST(lhs)。
                // 即 FIRST(A) += FIRST(symbol) - { ε }。
                if (addSetExceptEpsilon(lhsFirst, first_[symbol])) {
                    changed = true;
                }

                // 如果当前 symbol 不能推出 ε，说明后面的符号不可能成为整个右部的开头。
                // 因此停止继续向后扫描。
                if (first_[symbol].find(EPSILON) == first_[symbol].end()) {
                    allCanDeriveEpsilon = false;
                    break;
                }
            }

            // 如果右部所有符号都能推出 ε，那么左部也可以推出 ε。
            // 因此把 ε 加入 FIRST(lhs)。
            if (allCanDeriveEpsilon) {
                if (addToSet(lhsFirst, EPSILON)) {
                    changed = true;
                }
            }
        }
    }
}

// 计算所有非终结符的 FOLLOW 集。
// FOLLOW(A) 表示：在某个句型中，非终结符 A 后面可能紧跟的终结符集合。
void FirstFollow::computeFollowSets() {
    // FOLLOW 集的计算需要用到 FIRST 集。
    // 如果 FIRST 集还没算，就先计算 FIRST 集。
    if (first_.empty()) {
        computeFirstSets();
    }

    // 每次重新计算前，先清空旧的 FOLLOW 集结果。
    follow_.clear();

    // 对每个非终结符建立一个空 FOLLOW 集。
    for (const auto& nonterminal : grammar_.nonterminals()) {
        follow_[nonterminal];
    }

    // 增广开始符号的 FOLLOW 集中加入输入结束符。
    // 等价于教材里的 FOLLOW(S') = { $ }。
    follow_[grammar_.augmentedStartSymbol()].insert(grammar_.endToken());

    // 和 FIRST 集一样，FOLLOW 集也需要迭代到不再变化。
    bool changed = true;

    while (changed) {
        changed = false;

        // 遍历每一条产生式，例如 A -> α B β。
        for (const auto& production : grammar_.productions()) {
            // lhs 是产生式左部 A。
            const auto& lhs = production.lhs;
            // rhs 是产生式右部 α B β。
            const auto& rhs = production.rhs;

            // 逐个检查右部中的每个符号。
            for (std::size_t i = 0; i < rhs.size(); ++i) {
                const auto& symbol = rhs[i];

                // FOLLOW 只对非终结符有意义。
                // 如果当前 symbol 是终结符，就跳过。
                if (!grammar_.isNonterminal(symbol)) {
                    continue;
                }

                // beta 表示当前非终结符后面的符号序列。
                // 如果产生式是 A -> α B β，当前 symbol 是 B，那么 beta 就是 β。
                std::vector<std::string> beta;
                for (std::size_t j = i + 1; j < rhs.size(); ++j) {
                    beta.push_back(rhs[j]);
                }

                // 计算 FIRST(beta)。
                const auto firstBeta = firstOfSequence(beta);

                // 根据规则：FIRST(beta) - { ε } 加入 FOLLOW(symbol)。
                if (addSetExceptEpsilon(follow_[symbol], firstBeta)) {
                    changed = true;
                }

                // 如果 beta 为空，或者 beta 可以推出 ε，
                // 那么 FOLLOW(lhs) 也要加入 FOLLOW(symbol)。
                // 对应规则：A -> α B β 且 β => ε，则 FOLLOW(A) 加入 FOLLOW(B)。
                if (beta.empty() || firstBeta.find(EPSILON) != firstBeta.end()) {
                    if (addSet(follow_[symbol], follow_[lhs])) {
                        changed = true;
                    }
                }
            }
        }
    }
}

// 返回已经计算好的 FIRST 集。
// 返回 const 引用，表示外部只能读取，不能修改 first_。
const std::unordered_map<std::string, std::unordered_set<std::string>>& FirstFollow::firstSets() const {
    return first_;
}

// 返回已经计算好的 FOLLOW 集。
// 返回 const 引用，表示外部只能读取，不能修改 follow_。
const std::unordered_map<std::string, std::unordered_set<std::string>>& FirstFollow::followSets() const {
    return follow_;
}

// 打印 FIRST 集。
void FirstFollow::printFirstSets() const {
    // 指定输出顺序。
    // 因为 unordered_map 本身是无序的，如果不指定顺序，打印结果每次可能不一样。
    std::vector<std::string> order = {
        "S'", "P", "Stmts", "Stmt", "Assign", "Cond", "RelOp",
        "Expr", "ExprRest", "Term", "TermRest", "Factor"
    };

    std::cout << "FIRST sets:\n";

    // 按照指定顺序依次打印每个符号的 FIRST 集。
    for (const auto& symbol : order) {
        const auto it = first_.find(symbol);
        if (it == first_.end()) {
            continue;
        }

        std::cout << "FIRST(" << symbol << ") = { ";

        // firstItem 用来控制逗号格式，避免第一个元素前面多一个逗号。
        bool firstItem = true;
        for (const auto& value : it->second) {
            if (!firstItem) {
                std::cout << ", ";
            }
            std::cout << value;
            firstItem = false;
        }

        std::cout << " }\n";
    }
}

// 打印 FOLLOW 集。
void FirstFollow::printFollowSets() const {
    // 指定输出顺序，避免 unordered_map 的随机输出顺序影响阅读。
    std::vector<std::string> order = {
        "S'", "P", "Stmts", "Stmt", "Assign", "Cond", "RelOp",
        "Expr", "ExprRest", "Term", "TermRest", "Factor"
    };

    std::cout << "FOLLOW sets:\n";

    // 按照指定顺序依次打印每个符号的 FOLLOW 集。
    for (const auto& symbol : order) {
        const auto it = follow_.find(symbol);
        if (it == follow_.end()) {
            continue;
        }

        std::cout << "FOLLOW(" << symbol << ") = { ";

        // firstItem 用来控制逗号格式。
        bool firstItem = true;
        for (const auto& value : it->second) {
            if (!firstItem) {
                std::cout << ", ";
            }
            std::cout << value;
            firstItem = false;
        }

        std::cout << " }\n";
    }
}

// 向集合 target 中加入一个元素 value。
// 如果 value 是新加入的，返回 true；如果原来已经存在，返回 false。
bool FirstFollow::addToSet(std::unordered_set<std::string>& target, const std::string& value) {
    const auto result = target.insert(value);
    return result.second;
}

// 将 source 集合中的所有元素加入 target 集合。
// 只要本次确实新增了至少一个元素，就返回 true。
bool FirstFollow::addSet(
    std::unordered_set<std::string>& target,
    const std::unordered_set<std::string>& source
) {
    bool changed = false;

    for (const auto& value : source) {
        if (addToSet(target, value)) {
            changed = true;
        }
    }

    return changed;
}

// 将 source 集合中除了 ε 以外的所有元素加入 target 集合。
// 这个函数主要用于 FIRST 集和 FOLLOW 集规则中需要排除空串的情况。
bool FirstFollow::addSetExceptEpsilon(
    std::unordered_set<std::string>& target,
    const std::unordered_set<std::string>& source
) {
    bool changed = false;

    for (const auto& value : source) {
        // ε 不加入 target，直接跳过。
        if (value == EPSILON) {
            continue;
        }

        if (addToSet(target, value)) {
            changed = true;
        }
    }

    return changed;
}

// 计算一个符号序列的 FIRST 集。
// 例如 symbols = { B, C, D }，则计算 FIRST(B C D)。
std::unordered_set<std::string> FirstFollow::firstOfSequence(
    const std::vector<std::string>& symbols
) const {
    std::unordered_set<std::string> result;

    // 空序列可以直接推出 ε。
    if (symbols.empty()) {
        result.insert(EPSILON);
        return result;
    }

    // 记录整个符号序列是否都可以推出 ε。
    bool allCanDeriveEpsilon = true;

    // 从左到右扫描符号序列。
    for (const auto& symbol : symbols) {
        const auto it = first_.find(symbol);
        if (it == first_.end()) {
            allCanDeriveEpsilon = false;
            break;
        }

        // 把 FIRST(symbol) 中除了 ε 以外的元素加入结果集。
        for (const auto& value : it->second) {
            if (value != EPSILON) {
                result.insert(value);
            }
        }

        // 如果当前 symbol 不能推出 ε，后面的符号就不会成为序列开头。
        // 因此停止继续扫描。
        if (it->second.find(EPSILON) == it->second.end()) {
            allCanDeriveEpsilon = false;
            break;
        }
    }

    // 如果整个符号序列里的每个符号都能推出 ε，
    // 那么该序列本身也能推出 ε。
    if (allCanDeriveEpsilon) {
        result.insert(EPSILON);
    }

    return result;
}