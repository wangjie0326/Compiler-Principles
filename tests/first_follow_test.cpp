#include "Grammar.h"
#include "FirstFollow.h"

#include <iostream>

int main() {
    Grammar grammar;
    FirstFollow firstFollow(grammar);

    std::cout << "============================================\n";
    std::cout << "Lab 2 FIRST/FOLLOW Module Test\n";
    std::cout << "============================================\n";

    firstFollow.computeFirstSets();
    firstFollow.computeFollowSets();

    firstFollow.printFirstSets();
    std::cout << "\n";
    firstFollow.printFollowSets();

    const auto& first = firstFollow.firstSets();
    const auto& follow = firstFollow.followSets();

    bool ok = true;

    auto firstContains = [&](const std::string& symbol, const std::string& value) {
        auto it = first.find(symbol);
        if (it == first.end()) {
            return false;
        }
        return it->second.find(value) != it->second.end();
    };

    auto followContains = [&](const std::string& symbol, const std::string& value) {
        auto it = follow.find(symbol);
        if (it == follow.end()) {
            return false;
        }
        return it->second.find(value) != it->second.end();
    };

    if (!firstContains("Stmts", "IDN") ||
        !firstContains("Stmts", "IF") ||
        !firstContains("Stmts", "WHILE") ||
        !firstContains("Stmts", "BEGIN") ||
        !firstContains("Stmts", "ε")) {
        std::cerr << "[FAIL] FIRST(Stmts) is incorrect.\n";
        ok = false;
    }

    if (!firstContains("Expr", "IDN") ||
        !firstContains("Expr", "DEC") ||
        !firstContains("Expr", "OCT") ||
        !firstContains("Expr", "HEX") ||
        !firstContains("Expr", "SLP")) {
        std::cerr << "[FAIL] FIRST(Expr) is incorrect.\n";
        ok = false;
    }

    if (!firstContains("ExprRest", "ADD") ||
        !firstContains("ExprRest", "SUB") ||
        !firstContains("ExprRest", "ε")) {
        std::cerr << "[FAIL] FIRST(ExprRest) is incorrect.\n";
        ok = false;
    }

    if (!firstContains("TermRest", "MUL") ||
        !firstContains("TermRest", "DIV") ||
        !firstContains("TermRest", "ε")) {
        std::cerr << "[FAIL] FIRST(TermRest) is incorrect.\n";
        ok = false;
    }

    if (!followContains("S'", "END_TOKEN")) {
        std::cerr << "[FAIL] FOLLOW(S') should contain END_TOKEN.\n";
        ok = false;
    }

    if (!followContains("P", "END_TOKEN")) {
        std::cerr << "[FAIL] FOLLOW(P) should contain END_TOKEN.\n";
        ok = false;
    }

    if (!followContains("Stmts", "END") ||
        !followContains("Stmts", "END_TOKEN")) {
        std::cerr << "[FAIL] FOLLOW(Stmts) should contain END and END_TOKEN.\n";
        ok = false;
    }

    if (!followContains("Stmt", "SEMI") ||
        !followContains("Stmt", "ELSE")) {
        std::cerr << "[FAIL] FOLLOW(Stmt) should contain SEMI and ELSE.\n";
        ok = false;
    }

    if (!followContains("Cond", "THEN") ||
        !followContains("Cond", "DO")) {
        std::cerr << "[FAIL] FOLLOW(Cond) should contain THEN and DO.\n";
        ok = false;
    }

    if (!followContains("Expr", "GT") ||
        !followContains("Expr", "LT") ||
        !followContains("Expr", "EQ") ||
        !followContains("Expr", "GE") ||
        !followContains("Expr", "LE") ||
        !followContains("Expr", "NEQ") ||
        !followContains("Expr", "SRP") ||
        !followContains("Expr", "SEMI") ||
        !followContains("Expr", "ELSE") ||
        !followContains("Expr", "THEN") ||
        !followContains("Expr", "DO")) {
        std::cerr << "[FAIL] FOLLOW(Expr) is missing expected symbols.\n";
        ok = false;
    }

    if (!ok) {
        return 1;
    }

    std::cout << "[PASS] FIRST and FOLLOW sets look correct.\n";
    return 0;
}