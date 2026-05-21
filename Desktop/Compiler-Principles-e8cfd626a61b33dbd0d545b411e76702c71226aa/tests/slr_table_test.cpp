#include "FirstFollow.h"
#include "Grammar.h"
#include "LR0Automaton.h"
#include "SLRTable.h"

#include <iostream>

int main() {
    Grammar grammar;

    FirstFollow firstFollow(grammar);
    firstFollow.computeFirstSets();
    firstFollow.computeFollowSets();

    LR0Automaton automaton(grammar);
    automaton.build();

    SLRTable table(grammar, firstFollow, automaton);
    table.build();

    std::cout << "============================================\n";
    std::cout << "Lab 2 SLR Table Test\n";
    std::cout << "============================================\n";

    table.printSummary();
    std::cout << "\n";

    table.printConflicts();
    std::cout << "\n";

    if (automaton.states().size() != 55) {
        std::cerr << "[FAIL] Expected 55 LR(0) states.\n";
        return 1;
    }

    if (table.actionTable().empty()) {
        std::cerr << "[FAIL] ACTION table is empty.\n";
        return 1;
    }

    if (table.gotoTable().empty()) {
        std::cerr << "[FAIL] GOTO table is empty.\n";
        return 1;
    }

    if (table.unresolvedConflictCount() != 0) {
        std::cerr << "[FAIL] There are unresolved SLR conflicts.\n";
        return 1;
    }

    if (table.resolvedConflictCount() != 1) {
        std::cerr << "[FAIL] Expected exactly one resolved dangling-else conflict.\n";
        return 1;
    }

    auto acceptIt = table.actionTable().find({1, "END_TOKEN"});
    if (acceptIt == table.actionTable().end() ||
        acceptIt->second.type != SLRActionType::Accept) {
        std::cerr << "[FAIL] ACTION[I1, END_TOKEN] should be accept.\n";
        return 1;
    }

    auto elseIt = table.actionTable().find({41, "ELSE"});
    if (elseIt == table.actionTable().end() ||
        elseIt->second.type != SLRActionType::Shift ||
        elseIt->second.targetState != 49) {
        std::cerr << "[FAIL] ACTION[I41, ELSE] should be shift to I49.\n";
        return 1;
    }

    auto semiIt = table.actionTable().find({41, "SEMI"});
    if (semiIt == table.actionTable().end() ||
        semiIt->second.type != SLRActionType::Reduce ||
        semiIt->second.productionId != 5) {
        std::cerr << "[FAIL] ACTION[I41, SEMI] should be reduce by production 5.\n";
        return 1;
    }

    std::cout << "[PASS] SLR table generated correctly.\n";
    return 0;
}