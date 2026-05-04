#include "LR0Automaton.h"

#include <iostream>
#include <sstream>
#include <stdexcept>

LR0Automaton::LR0Automaton(const Grammar& grammar)
    : grammar_(grammar) {
}

void LR0Automaton::build() {
    states_.clear();
    transitions_.clear();
    transitionMap_.clear();

    std::set<LR0Item> startItems;
    startItems.insert({0, 0});

    states_.push_back(closure(startItems));

    const auto symbols = orderedSymbols();

    for (std::size_t i = 0; i < states_.size(); ++i) {
        for (const auto& symbol : symbols) {
            const auto nextState = goTo(states_[i], symbol);

            if (nextState.empty()) {
                continue;
            }

            int targetIndex = findStateIndex(nextState);

            if (targetIndex == -1) {
                targetIndex = static_cast<int>(states_.size());
                states_.push_back(nextState);
            }

            const int fromIndex = static_cast<int>(i);

            transitionMap_[{fromIndex, symbol}] = targetIndex;
            transitions_.push_back({fromIndex, symbol, targetIndex});
        }
    }
}

const std::vector<std::set<LR0Item>>& LR0Automaton::states() const {
    return states_;
}

const std::vector<LR0Transition>& LR0Automaton::transitions() const {
    return transitions_;
}

std::set<LR0Item> LR0Automaton::closure(const std::set<LR0Item>& items) const {
    std::set<LR0Item> result = items;

    bool changed = true;

    while (changed) {
        changed = false;

        std::vector<LR0Item> toAdd;

        for (const auto& item : result) {
            const auto& production = productionById(item.productionId);

            if (item.dotPosition >= production.rhs.size()) {
                continue;
            }

            const auto& symbolAfterDot = production.rhs[item.dotPosition];

            if (!grammar_.isNonterminal(symbolAfterDot)) {
                continue;
            }

            for (const auto& candidate : grammar_.productions()) {
                if (candidate.lhs != symbolAfterDot) {
                    continue;
                }

                LR0Item newItem{candidate.id, 0};

                if (result.find(newItem) == result.end()) {
                    toAdd.push_back(newItem);
                }
            }
        }

        for (const auto& item : toAdd) {
            const auto inserted = result.insert(item);
            if (inserted.second) {
                changed = true;
            }
        }
    }

    return result;
}

std::set<LR0Item> LR0Automaton::goTo(
    const std::set<LR0Item>& items,
    const std::string& symbol
) const {
    std::set<LR0Item> movedItems;

    for (const auto& item : items) {
        const auto& production = productionById(item.productionId);

        if (item.dotPosition >= production.rhs.size()) {
            continue;
        }

        if (production.rhs[item.dotPosition] == symbol) {
            movedItems.insert({item.productionId, item.dotPosition + 1});
        }
    }

    if (movedItems.empty()) {
        return {};
    }

    return closure(movedItems);
}

std::string LR0Automaton::itemToString(const LR0Item& item) const {
    const auto& production = productionById(item.productionId);

    std::ostringstream oss;
    oss << production.lhs << " -> ";

    if (production.rhs.empty()) {
        oss << "·";
        return oss.str();
    }

    for (std::size_t i = 0; i <= production.rhs.size(); ++i) {
        if (i == item.dotPosition) {
            oss << "·";
            if (i < production.rhs.size()) {
                oss << ' ';
            }
        }

        if (i < production.rhs.size()) {
            oss << production.rhs[i];
            if (i + 1 < production.rhs.size()) {
                oss << ' ';
            }
        }
    }

    return oss.str();
}

void LR0Automaton::printStates() const {
    std::cout << "LR(0) item sets:\n";

    for (std::size_t i = 0; i < states_.size(); ++i) {
        std::cout << "I" << i << ":\n";

        for (const auto& item : states_[i]) {
            std::cout << "  " << itemToString(item) << "\n";
        }

        std::cout << "\n";
    }

    std::cout << "Total states: " << states_.size() << "\n";
}

void LR0Automaton::printTransitions() const {
    std::cout << "LR(0) transitions:\n";

    for (const auto& transition : transitions_) {
        std::cout << "I" << transition.fromState
                  << " --" << transition.symbol << "--> "
                  << "I" << transition.toState << "\n";
    }

    std::cout << "Total transitions: " << transitions_.size() << "\n";
}

std::vector<std::string> LR0Automaton::orderedSymbols() const {
    return {
        "P", "Stmts", "Stmt", "Assign", "Cond", "RelOp",
        "Expr", "ExprRest", "Term", "TermRest", "Factor",

        "IDN", "DEC", "OCT", "HEX",
        "IF", "THEN", "ELSE", "WHILE", "DO", "BEGIN", "END",
        "ADD", "SUB", "MUL", "DIV",
        "GT", "LT", "EQ", "GE", "LE", "NEQ",
        "SLP", "SRP", "SEMI"
    };
}

const Production& LR0Automaton::productionById(int productionId) const {
    for (const auto& production : grammar_.productions()) {
        if (production.id == productionId) {
            return production;
        }
    }

    throw std::runtime_error("Unknown production id: " + std::to_string(productionId));
}

int LR0Automaton::findStateIndex(const std::set<LR0Item>& state) const {
    for (std::size_t i = 0; i < states_.size(); ++i) {
        if (states_[i] == state) {
            return static_cast<int>(i);
        }
    }

    return -1;
}