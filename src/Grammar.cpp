#include "Grammar.h"

#include <sstream>

Grammar::Grammar()
    : startSymbol_("P"),
      augmentedStartSymbol_("S'"),
      endToken_("END_TOKEN") {
    terminals_ = {
        "IDN", "DEC", "OCT", "HEX", "REAL",
        "IF", "THEN", "ELSE", "WHILE", "DO", "BEGIN", "END",
        "ADD", "SUB", "MUL", "DIV",
        "GT", "LT", "EQ", "GE", "LE", "NEQ",
        "SLP", "SRP", "SEMI",
        "END_TOKEN"
    };

    nonterminals_ = {
        "S'", "P", "Stmts", "Stmt", "Assign", "Cond", "RelOp",
        "Expr", "ExprRest", "Term", "TermRest", "Factor"
    };

    productions_ = {
        {0, "S'", {"P"}},
        {1, "P", {"Stmts"}},
        {2, "Stmts", {"Stmt", "SEMI", "Stmts"}},
        {3, "Stmts", {}},

        {4, "Stmt", {"Assign"}},
        {5, "Stmt", {"IF", "Cond", "THEN", "Stmt"}},
        {6, "Stmt", {"IF", "Cond", "THEN", "Stmt", "ELSE", "Stmt"}},
        {7, "Stmt", {"WHILE", "Cond", "DO", "Stmt"}},
        {8, "Stmt", {"BEGIN", "Stmts", "END"}},

        {9, "Assign", {"IDN", "EQ", "Expr"}},
        {10, "Cond", {"Expr", "RelOp", "Expr"}},

        {11, "RelOp", {"GT"}},
        {12, "RelOp", {"LT"}},
        {13, "RelOp", {"EQ"}},
        {14, "RelOp", {"GE"}},
        {15, "RelOp", {"LE"}},
        {16, "RelOp", {"NEQ"}},

        {17, "Expr", {"Term", "ExprRest"}},
        {18, "ExprRest", {"ADD", "Term", "ExprRest"}},
        {19, "ExprRest", {"SUB", "Term", "ExprRest"}},
        {20, "ExprRest", {}},

        {21, "Term", {"Factor", "TermRest"}},
        {22, "TermRest", {"MUL", "Factor", "TermRest"}},
        {23, "TermRest", {"DIV", "Factor", "TermRest"}},
        {24, "TermRest", {}},

        {25, "Factor", {"IDN"}},
        {26, "Factor", {"DEC"}},
        {27, "Factor", {"OCT"}},
        {28, "Factor", {"HEX"}},
        {29, "Factor", {"SLP", "Expr", "SRP"}},
        {30, "Factor", {"REAL"}}//新增
    };
}

const std::vector<Production>& Grammar::productions() const {
    return productions_;
}

const std::unordered_set<std::string>& Grammar::terminals() const {
    return terminals_;
}

const std::unordered_set<std::string>& Grammar::nonterminals() const {
    return nonterminals_;
}

const std::string& Grammar::startSymbol() const {
    return startSymbol_;
}

const std::string& Grammar::augmentedStartSymbol() const {
    return augmentedStartSymbol_;
}

const std::string& Grammar::endToken() const {
    return endToken_;
}

bool Grammar::isTerminal(const std::string& symbol) const {
    return terminals_.find(symbol) != terminals_.end();
}

bool Grammar::isNonterminal(const std::string& symbol) const {
    return nonterminals_.find(symbol) != nonterminals_.end();
}

std::string Grammar::productionToString(const Production& p) const {
    std::ostringstream oss;
    oss << p.id << ": " << p.lhs << " -> ";

    if (p.rhs.empty()) {
        oss << "ε";
    } else {
        for (std::size_t i = 0; i < p.rhs.size(); ++i) {
            if (i > 0) {
                oss << ' ';
            }
            oss << p.rhs[i];
        }
    }

    return oss.str();
}