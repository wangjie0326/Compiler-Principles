#pragma once

// 引入 FIRST/FOLLOW 集模块。
// SLRTable 构造规约动作时需要使用 FOLLOW 集。
#include "FirstFollow.h"

// 引入 Grammar 文法模块。
// SLRTable 需要判断终结符、非终结符，并根据产生式编号查找产生式。
#include "Grammar.h"

// 引入 LR(0) 自动机模块。
// SLRTable 需要根据 LR(0) 项目集和状态转移来生成 ACTION/GOTO 表。
#include "LR0Automaton.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

// SLRActionType 表示 SLR 分析表中 ACTION 项的动作类型。
enum class SLRActionType {
    // 移进动作。
    // 表示读入当前终结符，并转移到目标状态。
    Shift,

    // 规约动作。
    // 表示用某条产生式进行规约。
    Reduce,

    // 接受动作。
    // 表示语法分析成功结束。
    Accept,

    // 错误动作。
    // 表示当前状态和输入符号组合下没有合法动作。
    Error
};

// SLRAction 表示 ACTION 表中的一个动作项。
// 例如 s5、r3、acc 或 error。
struct SLRAction {
    // 当前动作类型，默认是 Error。
    SLRActionType type = SLRActionType::Error;

    // 移进动作的目标状态。
    // 例如 s5 中的 5。
    // 如果不是 Shift 动作，通常保持为 -1。
    int targetState = -1;

    // 规约动作使用的产生式编号。
    // 例如 r3 中的 3。
    // 如果不是 Reduce 动作，通常保持为 -1。
    int productionId = -1;

    // 将 ACTION 动作转换成字符串形式，方便打印分析表。
    // 例如 Shift 转成 "s5"，Reduce 转成 "r3"，Accept 转成 "acc"。
    std::string toString() const;
};

// SLRConflict 表示构造 SLR 分析表时发现的冲突。
// 常见冲突包括移进—规约冲突、规约—规约冲突。
struct SLRConflict {
    // 发生冲突的状态编号。
    int state = -1;

    // 发生冲突的终结符列。
    std::string terminal;

    // 该表项中原来已经存在的动作。
    SLRAction existingAction;

    // 当前准备填入的新动作。
    SLRAction incomingAction;

    // 冲突处理后最终选择的动作。
    // 例如 dangling else 冲突中可能选择 shift。
    SLRAction chosenAction;

    // 表示该冲突是否已经被程序自动解决。
    bool resolved = false;

    // 记录冲突原因或解决策略说明。
    std::string reason;
};

// SLRTable 类用于构造 SLR(1) 分析表。
// 它根据 Grammar、FirstFollow 和 LR0Automaton 生成 ACTION 表和 GOTO 表。
class SLRTable {
public:
    // 构造函数。
    // grammar 提供文法信息；
    // firstFollow 提供 FIRST/FOLLOW 集；
    // automaton 提供 LR(0) 项目集族和状态转移。
    SLRTable(
        const Grammar& grammar,
        const FirstFollow& firstFollow,
        const LR0Automaton& automaton
    );

    // 构造 SLR(1) 分析表。
    // 包括 ACTION 表和 GOTO 表。
    void build();

    // 返回 ACTION 表。
    // key 是 {状态编号, 终结符}，value 是对应的 SLRAction。
    const std::map<std::pair<int, std::string>, SLRAction>& actionTable() const;

    // 返回 GOTO 表。
    // key 是 {状态编号, 非终结符}，value 是目标状态编号。
    const std::map<std::pair<int, std::string>, int>& gotoTable() const;

    // 返回构造分析表过程中记录的所有冲突。
    const std::vector<SLRConflict>& conflicts() const;

    // 返回尚未解决的冲突数量。
    // 如果该值大于 0，说明当前 SLR 表仍存在不可自动处理的冲突。
    int unresolvedConflictCount() const;

    // 返回已经被程序自动解决的冲突数量。
    // 例如 dangling else 冲突可能会被记录为已解决。
    int resolvedConflictCount() const;

    // 打印 ACTION 表。
    // 主要用于调试、测试和实验报告截图。
    void printActionTable() const;

    // 打印 GOTO 表。
    // 主要用于调试、测试和实验报告截图。
    void printGotoTable() const;

    // 打印冲突信息。
    // 用于观察是否存在移进—规约冲突或规约—规约冲突。
    void printConflicts() const;

    // 打印 SLR 表构造结果摘要。
    // 例如 ACTION 项数量、GOTO 项数量、冲突数量等。
    void printSummary() const;

private:
    // 保存 Grammar 对象引用。
    // SLRTable 不复制文法，而是直接使用外部传入的文法对象。
    const Grammar& grammar_;

    // 保存 FirstFollow 对象引用。
    // 用于获取已经计算好的 FOLLOW 集。
    // 注意：SLRTable 本身不负责重新计算 FIRST/FOLLOW。
    const FirstFollow& firstFollow_;

    // 保存 LR0Automaton 对象引用。
    // 用于获取 LR(0) 状态集合和状态转移。
    const LR0Automaton& automaton_;

    // ACTION 分析表。
    // key 是 {状态编号, 终结符}；
    // value 是动作，如 shift、reduce、accept。
    std::map<std::pair<int, std::string>, SLRAction> actionTable_;

    // GOTO 分析表。
    // key 是 {状态编号, 非终结符}；
    // value 是转移到的目标状态编号。
    std::map<std::pair<int, std::string>, int> gotoTable_;

    // 保存构造表过程中发现的冲突。
    std::vector<SLRConflict> conflicts_;

    // 设置 ACTION 表项。
    // 如果该位置已有不同动作，则需要记录冲突。
    void setAction(int state, const std::string& terminal, const SLRAction& action);

    // 设置 GOTO 表项。
    // 用于非终结符转移。
    void setGoto(int state, const std::string& nonterminal, int targetState);

    // 判断两个 ACTION 动作是否相同。
    // 用于检测表项冲突时判断是否是真的冲突。
    bool actionsEqual(const SLRAction& lhs, const SLRAction& rhs) const;

    // 判断当前冲突是否属于 dangling else 冲突。
    // dangling else 通常表现为在 ELSE 上出现 shift/reduce 冲突。
    bool isDanglingElseConflict(
        const std::string& terminal,
        const SLRAction& existingAction,
        const SLRAction& incomingAction
    ) const;

    // 在 shift/reduce 冲突中选择 shift 动作。
    // dangling else 的经典处理策略就是 ELSE 优先移进，
    // 使 ELSE 匹配最近的 IF。
    SLRAction chooseShiftAction(const SLRAction& lhs, const SLRAction& rhs) const;

    // 根据产生式编号查找对应的 Production。
    // 规约动作中只保存 productionId，需要通过该函数找到具体产生式。
    const Production& productionById(int productionId) const;
};

