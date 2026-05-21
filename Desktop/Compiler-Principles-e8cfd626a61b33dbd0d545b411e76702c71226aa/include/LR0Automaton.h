#pragma once

#include "Grammar.h"

#include <cstddef>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

struct LR0Item {
    int productionId;
    std::size_t dotPosition;

    bool operator<(const LR0Item& other) const {
        if (productionId != other.productionId) {
            return productionId < other.productionId;
        }
        return dotPosition < other.dotPosition;
    }

    bool operator==(const LR0Item& other) const {
        return productionId == other.productionId &&
               dotPosition == other.dotPosition;
    }
};

struct LR0Transition {
    int fromState;
    std::string symbol;
    int toState;
};

class LR0Automaton {
public:
    explicit LR0Automaton(const Grammar& grammar);

    void build();

    const std::vector<std::set<LR0Item>>& states() const;
    const std::vector<LR0Transition>& transitions() const;

    std::set<LR0Item> closure(const std::set<LR0Item>& items) const;
    std::set<LR0Item> goTo(const std::set<LR0Item>& items, const std::string& symbol) const;

    std::string itemToString(const LR0Item& item) const;

    void printStates() const;
    void printTransitions() const;

private:
    const Grammar& grammar_;

    std::vector<std::set<LR0Item>> states_;
    std::vector<LR0Transition> transitions_;
    std::map<std::pair<int, std::string>, int> transitionMap_;

    std::vector<std::string> orderedSymbols() const;
    const Production& productionById(int productionId) const;
    int findStateIndex(const std::set<LR0Item>& state) const;
};