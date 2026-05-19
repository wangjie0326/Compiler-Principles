#include "Grammar.h"
#include "LR0Automaton.h"

#include <iostream>

// LR(0) 自动机模块测试程序入口。
// 该测试用于检查 LR0Automaton 是否能够根据 Grammar 正确生成项目集状态和状态转移。
int main() {
    // 创建 Grammar 对象。
    // Grammar 中保存了实验二使用的文法信息。
    Grammar grammar;

    // 创建 LR0Automaton 对象。
    // 它会基于 grammar 构造 LR(0) 项目集族。
    LR0Automaton automaton(grammar);

    // 打印测试标题。
    std::cout << "============================================\n";
    std::cout << "Lab 2 LR(0) Automaton Test\n";
    std::cout << "============================================\n";

    // 构造 LR(0) 自动机。
    // build() 内部会从初始项目开始，计算 closure 和 goto，
    // 最终生成所有 LR(0) 状态和状态转移。
    automaton.build();

    // 打印所有 LR(0) 项目集状态。
    // 例如 I0、I1、I2 等，每个状态中包含若干 LR(0) 项目。
    automaton.printStates();

    std::cout << "\n";

    // 打印 LR(0) 自动机的所有状态转移。
    // 例如 I0 --Stmt--> I3。
    automaton.printTransitions();

    // 检查自动机是否生成了状态。
    // 如果 states 为空，说明 LR(0) 项目集族构造失败。
    if (automaton.states().empty()) {
        std::cerr << "[FAIL] LR(0) automaton has no states.\n";
        return 1;
    }

    // 检查自动机是否生成了状态转移。
    // 如果 transitions 为空，说明 goto 转移构造可能存在问题。
    if (automaton.transitions().empty()) {
        std::cerr << "[FAIL] LR(0) automaton has no transitions.\n";
        return 1;
    }

    // 如果状态和转移都存在，则认为 LR(0) 自动机基本生成成功。
    std::cout << "[PASS] LR(0) automaton generated.\n";

    // 返回 0 表示测试通过。
    return 0;
}