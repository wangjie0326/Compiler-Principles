#pragma once

#include <string>
#include <vector>
#include <unordered_set>

struct Production {
    int id;
    std::string lhs;
    std::vector<std::string> rhs;
};

class Grammar {
public:
    Grammar();

    const std::vector<Production>& productions() const;
    const std::unordered_set<std::string>& terminals() const;
    const std::unordered_set<std::string>& nonterminals() const;

    const std::string& startSymbol() const;
    const std::string& augmentedStartSymbol() const;
    const std::string& endToken() const;

    bool isTerminal(const std::string& symbol) const;
    bool isNonterminal(const std::string& symbol) const;

    std::string productionToString(const Production& p) const;

private:
    std::vector<Production> productions_;
    std::unordered_set<std::string> terminals_;
    std::unordered_set<std::string> nonterminals_;

    std::string startSymbol_;
    std::string augmentedStartSymbol_;
    std::string endToken_;
};