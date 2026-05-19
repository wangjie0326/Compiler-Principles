#pragma once

// 引入 Grammar 类。
// LR0Automaton 需要基于 Grammar 中的产生式、终结符、非终结符来构造 LR(0) 项目集族。
#include "Grammar.h"

#include <cstddef>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

// LR0Item 表示一个 LR(0) 项目。
// 一个 LR(0) 项目由“产生式编号 + 点号位置”共同确定。
// 例如产生式 Expr -> Term ExprRest，
// 若 dotPosition = 1，则表示 Expr -> Term · ExprRest。
struct LR0Item {
    // 当前项目对应的产生式编号。
    // 通过 productionId 可以在 Grammar 的 productions_ 中找到具体产生式。
    int productionId;

    // 点号在产生式右部中的位置。
    // dotPosition = 0 表示点号在最前面；
    // dotPosition = rhs.size() 表示点号在最后，说明该产生式已经识别完成。
    std::size_t dotPosition;

    // 小于号重载。
    // 因为 LR0Item 会被放入 std::set 中，
    // std::set 需要知道两个 LR0Item 如何排序。
    bool operator<(const LR0Item& other) const {
        // 先按产生式编号排序。
        if (productionId != other.productionId) {
            return productionId < other.productionId;
        }

        // 如果产生式编号相同，再按点号位置排序。
        return dotPosition < other.dotPosition;
    }

    // 等于号重载。
    // 用于判断两个 LR(0) 项目是否完全相同。
    bool operator==(const LR0Item& other) const {
        return productionId == other.productionId &&
               dotPosition == other.dotPosition;
    }
};

// LR0Transition 表示 LR(0) 自动机中的一条状态转移。
// 例如 I0 --Expr--> I5。
struct LR0Transition {
    // 转移的起始状态编号。
    int fromState;

    // 转移时读入的文法符号。
    // 该符号可以是终结符，也可以是非终结符。
    std::string symbol;

    // 转移到的目标状态编号。
    int toState;
};

// LR0Automaton 类用于构造 LR(0) 项目集族。
// 它的核心功能是计算 closure、goto，并生成所有 LR(0) 状态和状态转移。
class LR0Automaton {
public:
    // 构造函数。
    // explicit 防止隐式类型转换。
    // 参数 grammar 表示构造 LR(0) 自动机所依据的文法。
    explicit LR0Automaton(const Grammar& grammar);

    // 构造 LR(0) 项目集族。
    // 从初始项目开始，不断计算 closure 和 goto，生成所有状态和转移。
    void build();

    // 返回所有 LR(0) 状态。
    // 每个状态是一个 LR0Item 的集合。
    const std::vector<std::set<LR0Item>>& states() const;

    // 返回所有状态转移。
    // 每条转移由起始状态、转移符号和目标状态组成。
    const std::vector<LR0Transition>& transitions() const;

    // 计算项目集闭包 closure(I)。
    // 如果项目中点号后面是非终结符，则需要加入该非终结符对应产生式的初始项目。
    std::set<LR0Item> closure(const std::set<LR0Item>& items) const;

    // 计算 goTo(I, symbol)。
    // 表示从项目集 I 读入 symbol 后能到达的下一个项目集。
    std::set<LR0Item> goTo(const std::set<LR0Item>& items, const std::string& symbol) const;

    // 将一个 LR(0) 项目转换为字符串形式，便于打印和调试。
    // 例如输出为 Expr -> Term · ExprRest。
    std::string itemToString(const LR0Item& item) const;

    // 打印所有 LR(0) 项目集状态。
    // 可用于调试，也可以作为实验报告中的运行结果截图依据。
    void printStates() const;

    // 打印 LR(0) 自动机中的所有状态转移。
    void printTransitions() const;

private:
    // 保存 Grammar 对象的引用。
    // LR0Automaton 不复制文法，而是直接使用外部传入的 Grammar。
    const Grammar& grammar_;

    // 保存所有 LR(0) 项目集状态。
    // states_[0] 表示 I0，states_[1] 表示 I1，依此类推。
    std::vector<std::set<LR0Item>> states_;

    // 保存所有状态转移记录。
    // 主要用于打印和查看自动机结构。
    std::vector<LR0Transition> transitions_;

    // 保存状态转移的快速查找表。
    // key 是二元组：{当前状态编号, 转移符号}；
    // value 是目标状态编号。
    std::map<std::pair<int, std::string>, int> transitionMap_;

    // 返回一个固定顺序的文法符号列表。
    // 构造自动机时，会按照这个顺序尝试对每个符号计算 goto。
    std::vector<std::string> orderedSymbols() const;

    // 根据产生式编号查找对应的 Production。
    // LR0Item 中只保存 productionId，需要通过该函数找到具体产生式内容。
    const Production& productionById(int productionId) const;

    // 判断某个项目集状态是否已经存在。
    // 如果存在，返回已有状态编号；
    // 如果不存在，返回 -1。
    int findStateIndex(const std::set<LR0Item>& state) const;
};