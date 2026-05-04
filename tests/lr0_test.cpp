#include "Grammar.h"
#include "LR0Automaton.h"

#include <iostream>

int main() {
    Grammar grammar;
    LR0Automaton automaton(grammar);

    std::cout << "============================================\n";
    std::cout << "Lab 2 LR(0) Automaton Test\n";
    std::cout << "============================================\n";

    automaton.build();

    automaton.printStates();
    std::cout << "\n";
    automaton.printTransitions();

    if (automaton.states().empty()) {
        std::cerr << "[FAIL] LR(0) automaton has no states.\n";
        return 1;
    }

    if (automaton.transitions().empty()) {
        std::cerr << "[FAIL] LR(0) automaton has no transitions.\n";
        return 1;
    }

    std::cout << "[PASS] LR(0) automaton generated.\n";
    return 0;
}