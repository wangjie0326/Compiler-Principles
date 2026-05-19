#include "SLRTable.h"

#include <iostream>
#include <stdexcept>

// 将 SLRAction 动作转换成字符串形式。
// 这个函数主要用于打印 ACTION 表。
// 例如：Shift 转成 s5，Reduce 转成 r3，Accept 转成 acc。
std::string SLRAction::toString() const {
    switch (type) {
        // 移进动作：输出 s + 目标状态编号。
        case SLRActionType::Shift:
            return "s" + std::to_string(targetState);

        // 规约动作：输出 r + 产生式编号。
        case SLRActionType::Reduce:
            return "r" + std::to_string(productionId);

        // 接受动作：输出 acc。
        case SLRActionType::Accept:
            return "acc";

        // 错误动作或默认情况：输出空字符串。
        case SLRActionType::Error:
        default:
            return "";
    }
}

// SLRTable 构造函数。
// 保存 Grammar、FirstFollow、LR0Automaton 三个对象的引用。
// 注意：这里不重新计算 FIRST/FOLLOW，也不重新构造 LR(0) 自动机，只使用外部已经准备好的结果。
SLRTable::SLRTable(
    const Grammar& grammar,
    const FirstFollow& firstFollow,
    const LR0Automaton& automaton
)
    : grammar_(grammar),
      firstFollow_(firstFollow),
      automaton_(automaton) {
}

// 构造 SLR(1) 分析表。
// 主要生成两个表：
// 1. ACTION 表：处理终结符上的 shift / reduce / accept 动作。
// 2. GOTO 表：处理非终结符上的状态转移。
void SLRTable::build() {
    // 清空旧的 ACTION 表。
    actionTable_.clear();

    // 清空旧的 GOTO 表。
    gotoTable_.clear();

    // 清空旧的冲突记录。
    conflicts_.clear();

    // 第一部分：根据 LR(0) 自动机中的状态转移填写 shift 动作和 GOTO 项。
    // 如果转移符号是终结符，就填 ACTION 表中的 shift。
    // 如果转移符号是非终结符，就填 GOTO 表。
    for (const auto& transition : automaton_.transitions()) {
        // 如果状态转移边上的符号是终结符，则对应 ACTION 表中的移进动作。
        if (grammar_.isTerminal(transition.symbol)) {
            // 构造一个 shift 动作。
            SLRAction action;
            action.type = SLRActionType::Shift;
            action.targetState = transition.toState;

            // 填入 ACTION[起始状态, 终结符]。
            setAction(transition.fromState, transition.symbol, action);
        } else if (grammar_.isNonterminal(transition.symbol)) {
            // 如果状态转移边上的符号是非终结符，则对应 GOTO 表。
            // 填入 GOTO[起始状态, 非终结符] = 目标状态。
            setGoto(transition.fromState, transition.symbol, transition.toState);
        }
    }

    // 获取 LR(0) 自动机中的所有状态。
    const auto& states = automaton_.states();

    // 获取已经由 FirstFollow 模块计算好的 FOLLOW 集。
    // 注意：这里是使用 FOLLOW 集，不是在这里重新计算 FOLLOW 集。
    const auto& followSets = firstFollow_.followSets();

    // 第二部分：根据 LR(0) 项目集中的完整项目填写 reduce 或 accept 动作。
    // 完整项目指点号已经在产生式右部末尾，例如 A -> α ·。
    for (std::size_t stateIndex = 0; stateIndex < states.size(); ++stateIndex) {
        // 遍历当前状态中的每一个 LR(0) 项目。
        for (const auto& item : states[stateIndex]) {
            // 根据项目中的产生式编号找到具体产生式。
            const auto& production = productionById(item.productionId);

            // 如果点号还没有到产生式右部末尾，说明该项目还不能规约。
            // 这种项目不负责填写 reduce 或 accept。
            if (item.dotPosition != production.rhs.size()) {
                continue;
            }

            // 当前 LR(0) 状态编号。
            const int currentState = static_cast<int>(stateIndex);

            // 如果完整项目的左部是增广开始符号，
            // 说明已经识别完整个输入串，应填 accept 动作。
            if (production.lhs == grammar_.augmentedStartSymbol()) {
                // 构造 accept 动作。
                SLRAction action;
                action.type = SLRActionType::Accept;

                // 在输入结束符列填入 accept。
                // 即 ACTION[currentState, END_TOKEN] = acc。
                setAction(currentState, grammar_.endToken(), action);
                continue;
            }

            // 查找当前产生式左部的 FOLLOW 集。
            // 对完整项目 A -> α ·，SLR 的规则是：
            // 对 FOLLOW(A) 中的每个终结符 a，填 ACTION[state, a] = reduce A -> α。
            const auto followIt = followSets.find(production.lhs);

            // 如果没有找到 FOLLOW 集，就跳过。
            // 正常情况下非终结符都应该有 FOLLOW 集。
            if (followIt == followSets.end()) {
                continue;
            }

            // 在 FOLLOW(产生式左部) 的每一个终结符列上填写 reduce 动作。
            for (const auto& terminal : followIt->second) {
                // 构造 reduce 动作。
                SLRAction action;
                action.type = SLRActionType::Reduce;
                action.productionId = production.id;

                // 填入 ACTION[currentState, terminal] = reduce production.id。
                setAction(currentState, terminal, action);
            }
        }
    }
}

// 返回 ACTION 表。
// 返回 const 引用，避免拷贝，同时防止外部修改 actionTable_。
const std::map<std::pair<int, std::string>, SLRAction>& SLRTable::actionTable() const {
    return actionTable_;
}

// 返回 GOTO 表。
// 返回 const 引用，避免拷贝，同时防止外部修改 gotoTable_。
const std::map<std::pair<int, std::string>, int>& SLRTable::gotoTable() const {
    return gotoTable_;
}

// 返回冲突记录表。
// 每个冲突记录包含状态、终结符、已有动作、新动作、选择动作和原因。
const std::vector<SLRConflict>& SLRTable::conflicts() const {
    return conflicts_;
}

// 统计尚未解决的冲突数量。
// resolved == false 的冲突会被计入。
int SLRTable::unresolvedConflictCount() const {
    int count = 0;

    // 遍历所有冲突记录。
    for (const auto& conflict : conflicts_) {
        // 如果冲突未解决，则计数加一。
        if (!conflict.resolved) {
            ++count;
        }
    }

    return count;
}

// 统计已经解决的冲突数量。
// resolved == true 的冲突会被计入。
int SLRTable::resolvedConflictCount() const {
    int count = 0;

    // 遍历所有冲突记录。
    for (const auto& conflict : conflicts_) {
        // 如果冲突已经解决，则计数加一。
        if (conflict.resolved) {
            ++count;
        }
    }

    return count;
}

// 打印 ACTION 表。
// ACTION 表用于说明在某个状态、遇到某个终结符时应该执行什么动作。
void SLRTable::printActionTable() const {
    std::cout << "SLR ACTION table:\n";

    // 遍历 ACTION 表中的所有表项。
    for (const auto& entry : actionTable_) {
        // entry.first 是 key，即 {状态编号, 终结符}。
        const int state = entry.first.first;
        const std::string& terminal = entry.first.second;

        // entry.second 是对应的 ACTION 动作。
        const auto& action = entry.second;

        // 打印格式：ACTION[I状态, 终结符] = 动作。
        std::cout << "ACTION[I" << state << ", " << terminal << "] = "
                  << action.toString() << "\n";
    }

    // 打印 ACTION 表项总数。
    std::cout << "Total ACTION entries: " << actionTable_.size() << "\n";
}

// 打印 GOTO 表。
// GOTO 表用于说明在某个状态、归约出某个非终结符后应该转到哪个状态。
void SLRTable::printGotoTable() const {
    std::cout << "SLR GOTO table:\n";

    // 遍历 GOTO 表中的所有表项。
    for (const auto& entry : gotoTable_) {
        // entry.first 是 key，即 {状态编号, 非终结符}。
        const int state = entry.first.first;
        const std::string& nonterminal = entry.first.second;

        // entry.second 是目标状态编号。
        const int target = entry.second;

        // 打印格式：GOTO[I状态, 非终结符] = I目标状态。
        std::cout << "GOTO[I" << state << ", " << nonterminal << "] = I"
                  << target << "\n";
    }

    // 打印 GOTO 表项总数。
    std::cout << "Total GOTO entries: " << gotoTable_.size() << "\n";
}

// 打印 SLR 表构造过程中发现的冲突。
// 包括冲突状态、冲突终结符、已有动作、新动作、最终选择动作等。
void SLRTable::printConflicts() const {
    std::cout << "SLR conflicts:\n";

    // 如果没有冲突，直接打印提示并返回。
    if (conflicts_.empty()) {
        std::cout << "No conflicts.\n";
        return;
    }

    // 逐条打印冲突信息。
    for (const auto& conflict : conflicts_) {
        std::cout << "State I" << conflict.state
                  << ", terminal " << conflict.terminal
                  << ": existing=" << conflict.existingAction.toString()
                  << ", incoming=" << conflict.incomingAction.toString()
                  << ", chosen=" << conflict.chosenAction.toString()
                  << ", resolved=" << (conflict.resolved ? "yes" : "no")
                  << ", reason=" << conflict.reason
                  << "\n";
    }
}

// 打印 SLR 分析表的汇总信息。
// 主要用于快速查看表构造结果。
void SLRTable::printSummary() const {
    std::cout << "SLR table summary:\n";

    // 打印 LR(0) 自动机状态数。
    std::cout << "States: " << automaton_.states().size() << "\n";

    // 打印 ACTION 表项数量。
    std::cout << "ACTION entries: " << actionTable_.size() << "\n";

    // 打印 GOTO 表项数量。
    std::cout << "GOTO entries: " << gotoTable_.size() << "\n";

    // 打印已解决冲突数量。
    std::cout << "Resolved conflicts: " << resolvedConflictCount() << "\n";

    // 打印未解决冲突数量。
    std::cout << "Unresolved conflicts: " << unresolvedConflictCount() << "\n";
}

// 设置 ACTION 表中的一个表项。
// 如果该表项原来为空，则直接填入。
// 如果该表项已经存在不同动作，则说明发生冲突，需要记录。
void SLRTable::setAction(int state, const std::string& terminal, const SLRAction& action) {
    // ACTION 表的 key 是 {状态编号, 终结符}。
    const auto key = std::make_pair(state, terminal);

    // 查找该 ACTION 表项是否已经存在。
    const auto it = actionTable_.find(key);

    // 如果该表项还不存在，则直接写入新动作。
    if (it == actionTable_.end()) {
        actionTable_[key] = action;
        return;
    }

    // 如果表项已经存在，取出原有动作。
    const SLRAction existingAction = it->second;

    // 如果原动作和新动作完全相同，则不算冲突，直接返回。
    if (actionsEqual(existingAction, action)) {
        return;
    }

    // 如果同一个 ACTION 表项中出现两个不同动作，则说明发生冲突。
    SLRConflict conflict;
    conflict.state = state;
    conflict.terminal = terminal;
    conflict.existingAction = existingAction;
    conflict.incomingAction = action;

    // 判断当前冲突是否是 dangling else 冲突。
    // 如果是 dangling else 冲突，则采用优先移进 shift 的策略自动解决。
    if (isDanglingElseConflict(terminal, existingAction, action)) {
        // 在 shift/reduce 冲突中选择 shift 动作。
        const SLRAction chosen = chooseShiftAction(existingAction, action);

        // 将 ACTION 表项更新为选择后的动作。
        actionTable_[key] = chosen;

        // 记录冲突处理结果。
        conflict.chosenAction = chosen;
        conflict.resolved = true;
        conflict.reason = "dangling else: prefer shift so ELSE matches nearest IF";
    } else {
        // 如果不是 dangling else 冲突，则当前程序不自动解决。
        // 保留原来的动作，并将冲突标记为未解决。
        conflict.chosenAction = existingAction;
        conflict.resolved = false;
        conflict.reason = "unresolved conflict";
    }

    // 将冲突记录保存下来，方便后续打印和检查。
    conflicts_.push_back(conflict);
}

// 设置 GOTO 表中的一个表项。
// GOTO 表用于非终结符转移。
void SLRTable::setGoto(int state, const std::string& nonterminal, int targetState) {
    // GOTO 表的 key 是 {状态编号, 非终结符}。
    const auto key = std::make_pair(state, nonterminal);

    // 直接写入目标状态。
    gotoTable_[key] = targetState;
}

// 判断两个 SLRAction 是否完全相同。
// 只有动作类型、目标状态、产生式编号都相同，才认为相同。
bool SLRTable::actionsEqual(const SLRAction& lhs, const SLRAction& rhs) const {
    return lhs.type == rhs.type &&
           lhs.targetState == rhs.targetState &&
           lhs.productionId == rhs.productionId;
}

// 判断当前冲突是否是 dangling else 冲突。
// dangling else 冲突通常表现为：
// 在 ELSE 终结符上同时出现 shift 和 reduce 动作。
// 这里进一步限定 reduce 的产生式编号为 5，
// 即 Stmt -> IF Cond THEN Stmt。
bool SLRTable::isDanglingElseConflict(
    const std::string& terminal,
    const SLRAction& existingAction,
    const SLRAction& incomingAction
) const {
    // dangling else 冲突只发生在 ELSE 终结符列。
    if (terminal != "ELSE") {
        return false;
    }

    // 判断是否是 shift/reduce 冲突：
    // 情况一：已有动作是 Shift，新动作是 Reduce。
    const bool shiftReduce =
        existingAction.type == SLRActionType::Shift &&
        incomingAction.type == SLRActionType::Reduce;

    // 情况二：已有动作是 Reduce，新动作是 Shift。
    const bool reduceShift =
        existingAction.type == SLRActionType::Reduce &&
        incomingAction.type == SLRActionType::Shift;

    // 如果既不是 shift/reduce，也不是 reduce/shift，就不是 dangling else 冲突。
    if (!shiftReduce && !reduceShift) {
        return false;
    }

    // 取出冲突中的 reduce 动作。
    // dangling else 对应的 reduce 一般是：
    // Stmt -> IF Cond THEN Stmt
    const SLRAction& reduceAction =
        existingAction.type == SLRActionType::Reduce ? existingAction : incomingAction;

    // 这里通过产生式编号判断是否为 dangling else 对应的短 if 产生式。
    // productionId == 5 通常对应：
    // 5: Stmt -> IF Cond THEN Stmt
    if (reduceAction.productionId != 5) {
        return false;
    }

    return true;
}

// 在两个冲突动作中选择 shift 动作。
// 主要用于 dangling else 冲突的处理。
// 选择 shift 可以让 ELSE 归属于最近的 IF。
SLRAction SLRTable::chooseShiftAction(const SLRAction& lhs, const SLRAction& rhs) const {
    // 如果 lhs 是 shift，就返回 lhs。
    if (lhs.type == SLRActionType::Shift) {
        return lhs;
    }

    // 如果 rhs 是 shift，就返回 rhs。
    if (rhs.type == SLRActionType::Shift) {
        return rhs;
    }

    // 理论上调用该函数时应该至少有一个 shift。
    // 如果没有 shift，则默认返回 lhs。
    return lhs;
}

// 根据产生式编号查找对应的 Production。
// reduce 动作中只保存 productionId，
// 需要通过该函数找到具体产生式内容。
const Production& SLRTable::productionById(int productionId) const {
    // 遍历 Grammar 中保存的所有产生式。
    for (const auto& production : grammar_.productions()) {
        if (production.id == productionId) {
            return production;
        }
    }

    // 如果找不到对应编号，说明程序内部状态或文法数据有问题。
    throw std::runtime_error("Unknown production id: " + std::to_string(productionId));
}