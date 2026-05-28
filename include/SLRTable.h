#pragma once

#include "FirstFollow.h"
#include "Grammar.h"
#include "LR0Automaton.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

enum class SLRActionType {
    Shift,
    Reduce,
    Accept,
    Error
};

struct SLRAction {
    SLRActionType type = SLRActionType::Error;
    int targetState = -1;
    int productionId = -1;

    std::string toString() const;
};

struct SLRConflict {
    int state = -1;
    std::string terminal;
    SLRAction existingAction;
    SLRAction incomingAction;
    SLRAction chosenAction;
    bool resolved = false;
    std::string reason;
};

class SLRTable {
public:
    SLRTable(
        const Grammar& grammar,
        const FirstFollow& firstFollow,
        const LR0Automaton& automaton
    );

    void build();

    const std::map<std::pair<int, std::string>, SLRAction>& actionTable() const;
    const std::map<std::pair<int, std::string>, int>& gotoTable() const;
    const std::vector<SLRConflict>& conflicts() const;

    int unresolvedConflictCount() const;
    int resolvedConflictCount() const;

    void printActionTable() const;
    void printGotoTable() const;
    void printConflicts() const;
    void printSummary() const;

private:
    const Grammar& grammar_;
    const FirstFollow& firstFollow_;
    const LR0Automaton& automaton_;

    std::map<std::pair<int, std::string>, SLRAction> actionTable_;
    std::map<std::pair<int, std::string>, int> gotoTable_;
    std::vector<SLRConflict> conflicts_;

    void setAction(int state, const std::string& terminal, const SLRAction& action);
    void setGoto(int state, const std::string& nonterminal, int targetState);

    bool actionsEqual(const SLRAction& lhs, const SLRAction& rhs) const;
    bool isDanglingElseConflict(
        const std::string& terminal,
        const SLRAction& existingAction,
        const SLRAction& incomingAction
    ) const;

    SLRAction chooseShiftAction(const SLRAction& lhs, const SLRAction& rhs) const;

    const Production& productionById(int productionId) const;
};