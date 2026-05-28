#include "FirstFollow.h"

#include <iostream>
#include <vector>

const std::string FirstFollow::EPSILON = "ε";

FirstFollow::FirstFollow(const Grammar& grammar)
    : grammar_(grammar) {
}

void FirstFollow::computeFirstSets() {
    first_.clear();

    for (const auto& terminal : grammar_.terminals()) {
        first_[terminal].insert(terminal);
    }

    for (const auto& nonterminal : grammar_.nonterminals()) {
        first_[nonterminal];
    }

    bool changed = true;

    while (changed) {
        changed = false;

        for (const auto& production : grammar_.productions()) {
            auto& lhsFirst = first_[production.lhs];

            if (production.rhs.empty()) {
                if (addToSet(lhsFirst, EPSILON)) {
                    changed = true;
                }
                continue;
            }

            bool allCanDeriveEpsilon = true;

            for (const auto& symbol : production.rhs) {
                if (addSetExceptEpsilon(lhsFirst, first_[symbol])) {
                    changed = true;
                }

                if (first_[symbol].find(EPSILON) == first_[symbol].end()) {
                    allCanDeriveEpsilon = false;
                    break;
                }
            }

            if (allCanDeriveEpsilon) {
                if (addToSet(lhsFirst, EPSILON)) {
                    changed = true;
                }
            }
        }
    }
}

void FirstFollow::computeFollowSets() {
    if (first_.empty()) {
        computeFirstSets();
    }

    follow_.clear();

    for (const auto& nonterminal : grammar_.nonterminals()) {
        follow_[nonterminal];
    }

    follow_[grammar_.augmentedStartSymbol()].insert(grammar_.endToken());

    bool changed = true;

    while (changed) {
        changed = false;

        for (const auto& production : grammar_.productions()) {
            const auto& lhs = production.lhs;
            const auto& rhs = production.rhs;

            for (std::size_t i = 0; i < rhs.size(); ++i) {
                const auto& symbol = rhs[i];

                if (!grammar_.isNonterminal(symbol)) {
                    continue;
                }

                std::vector<std::string> beta;
                for (std::size_t j = i + 1; j < rhs.size(); ++j) {
                    beta.push_back(rhs[j]);
                }

                const auto firstBeta = firstOfSequence(beta);

                if (addSetExceptEpsilon(follow_[symbol], firstBeta)) {
                    changed = true;
                }

                if (beta.empty() || firstBeta.find(EPSILON) != firstBeta.end()) {
                    if (addSet(follow_[symbol], follow_[lhs])) {
                        changed = true;
                    }
                }
            }
        }
    }
}

const std::unordered_map<std::string, std::unordered_set<std::string>>& FirstFollow::firstSets() const {
    return first_;
}

const std::unordered_map<std::string, std::unordered_set<std::string>>& FirstFollow::followSets() const {
    return follow_;
}

void FirstFollow::printFirstSets() const {
    std::vector<std::string> order = {
        "S'", "P", "Stmts", "Stmt", "Assign", "Cond", "RelOp",
        "Expr", "ExprRest", "Term", "TermRest", "Factor"
    };

    std::cout << "FIRST sets:\n";

    for (const auto& symbol : order) {
        const auto it = first_.find(symbol);
        if (it == first_.end()) {
            continue;
        }

        std::cout << "FIRST(" << symbol << ") = { ";

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

void FirstFollow::printFollowSets() const {
    std::vector<std::string> order = {
        "S'", "P", "Stmts", "Stmt", "Assign", "Cond", "RelOp",
        "Expr", "ExprRest", "Term", "TermRest", "Factor"
    };

    std::cout << "FOLLOW sets:\n";

    for (const auto& symbol : order) {
        const auto it = follow_.find(symbol);
        if (it == follow_.end()) {
            continue;
        }

        std::cout << "FOLLOW(" << symbol << ") = { ";

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

bool FirstFollow::addToSet(std::unordered_set<std::string>& target, const std::string& value) {
    const auto result = target.insert(value);
    return result.second;
}

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

bool FirstFollow::addSetExceptEpsilon(
    std::unordered_set<std::string>& target,
    const std::unordered_set<std::string>& source
) {
    bool changed = false;

    for (const auto& value : source) {
        if (value == EPSILON) {
            continue;
        }

        if (addToSet(target, value)) {
            changed = true;
        }
    }

    return changed;
}

std::unordered_set<std::string> FirstFollow::firstOfSequence(
    const std::vector<std::string>& symbols
) const {
    std::unordered_set<std::string> result;

    if (symbols.empty()) {
        result.insert(EPSILON);
        return result;
    }

    bool allCanDeriveEpsilon = true;

    for (const auto& symbol : symbols) {
        const auto it = first_.find(symbol);
        if (it == first_.end()) {
            allCanDeriveEpsilon = false;
            break;
        }

        for (const auto& value : it->second) {
            if (value != EPSILON) {
                result.insert(value);
            }
        }

        if (it->second.find(EPSILON) == it->second.end()) {
            allCanDeriveEpsilon = false;
            break;
        }
    }

    if (allCanDeriveEpsilon) {
        result.insert(EPSILON);
    }

    return result;
}