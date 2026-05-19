#include "FirstFollow.h"
#include "Grammar.h"
#include "LR0Automaton.h"
#include "SLRTable.h"

#include <iostream>

// SLR 分析表模块测试程序入口。
// 该测试用于验证 SLRTable 是否能够基于 Grammar、FIRST/FOLLOW 集和 LR(0) 自动机
// 正确生成 ACTION 表、GOTO 表，并正确处理 dangling else 冲突。
int main() {
    // 创建 Grammar 对象。
    // Grammar 中保存实验二使用的文法信息。
    Grammar grammar;

    // 创建 FirstFollow 对象，用于计算 FIRST 集和 FOLLOW 集。
    FirstFollow firstFollow(grammar);

    // 计算 FIRST 集。
    firstFollow.computeFirstSets();

    // 计算 FOLLOW 集。
    // SLR 分析表中的 reduce 动作需要依赖 FOLLOW 集。
    firstFollow.computeFollowSets();

    // 创建 LR(0) 自动机对象。
    LR0Automaton automaton(grammar);

    // 构造 LR(0) 项目集族和状态转移。
    automaton.build();

    // 创建 SLR 分析表对象。
    // SLRTable 会综合使用 grammar、firstFollow 和 automaton 来构造 ACTION/GOTO 表。
    SLRTable table(grammar, firstFollow, automaton);

    // 构造 SLR 分析表。
    table.build();

    // 打印测试标题。
    std::cout << "============================================\n";
    std::cout << "Lab 2 SLR Table Test\n";
    std::cout << "============================================\n";

    // 打印 SLR 分析表的概要信息。
    // 一般包括状态数量、ACTION/GOTO 表项数量、冲突数量等。
    table.printSummary();

    std::cout << "\n";

    // 打印 SLR 表构造过程中发现的冲突。
    // 本实验文法中主要关注 dangling else 引起的移进-规约冲突。
    table.printConflicts();

    std::cout << "\n";

    // 检查 LR(0) 自动机状态数量是否符合当前文法的预期。
    // 这里的 55 是当前 Grammar 下生成的 LR(0) 状态总数。
    if (automaton.states().size() != 55) {
        std::cerr << "[FAIL] Expected 55 LR(0) states.\n";
        return 1;
    }

    // 检查 ACTION 表是否为空。
    // ACTION 表用于根据“当前状态 + 当前输入终结符”决定移进、规约、接受或报错。
    if (table.actionTable().empty()) {
        std::cerr << "[FAIL] ACTION table is empty.\n";
        return 1;
    }

    // 检查 GOTO 表是否为空。
    // GOTO 表用于规约后根据“当前状态 + 非终结符”跳转到新状态。
    if (table.gotoTable().empty()) {
        std::cerr << "[FAIL] GOTO table is empty.\n";
        return 1;
    }

    // 检查是否存在未解决冲突。
    // 如果未解决冲突数量不为 0，说明 SLR 表仍存在无法自动处理的冲突。
    if (table.unresolvedConflictCount() != 0) {
        std::cerr << "[FAIL] There are unresolved SLR conflicts.\n";
        return 1;
    }

    // 检查已解决冲突数量。
    // 当前文法预期只有一个 dangling else 冲突，并且该冲突已通过选择 shift 解决。
    if (table.resolvedConflictCount() != 1) {
        std::cerr << "[FAIL] Expected exactly one resolved dangling-else conflict.\n";
        return 1;
    }

    // 检查接受动作。
    // 在状态 I1 遇到输入结束符 END_TOKEN 时，应该执行 accept。
    auto acceptIt = table.actionTable().find({1, "END_TOKEN"});
    if (acceptIt == table.actionTable().end() ||
        acceptIt->second.type != SLRActionType::Accept) {
        std::cerr << "[FAIL] ACTION[I1, END_TOKEN] should be accept.\n";
        return 1;
    }

    // 检查 dangling else 冲突处的 ELSE 动作。
    // 在状态 I41 遇到 ELSE 时，应选择 shift 到 I49，
    // 这样可以让 ELSE 和最近的 IF 匹配。
    auto elseIt = table.actionTable().find({41, "ELSE"});
    if (elseIt == table.actionTable().end() ||
        elseIt->second.type != SLRActionType::Shift ||
        elseIt->second.targetState != 49) {
        std::cerr << "[FAIL] ACTION[I41, ELSE] should be shift to I49.\n";
        return 1;
    }

    // 检查同一状态下遇到 SEMI 时的规约动作。
    // 在状态 I41 遇到 SEMI 时，应按产生式 5 进行规约，
    // 即 Stmt -> IF Cond THEN Stmt。
    auto semiIt = table.actionTable().find({41, "SEMI"});
    if (semiIt == table.actionTable().end() ||
        semiIt->second.type != SLRActionType::Reduce ||
        semiIt->second.productionId != 5) {
        std::cerr << "[FAIL] ACTION[I41, SEMI] should be reduce by production 5.\n";
        return 1;
    }

    // 如果以上检查全部通过，则说明 SLR 分析表构造结果符合预期。
    std::cout << "[PASS] SLR table generated correctly.\n";

    // 返回 0 表示测试通过。
    return 0;
}