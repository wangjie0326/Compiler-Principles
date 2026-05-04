#include "SLRTable.h"

#include <iostream>
#include <stdexcept>

std::string SLRAction::toString() const {
    switch (type) {
        case SLRActionType::Shift:
            return "s" + std::to_string(targetState);
        case SLRActionType::Reduce:
            return "r" + std::to_string(productionId);
        case SLRActionType::Accept:
            return "acc";
        case SLRActionType::Error:
        default:
            return "";
    }
}

SLRTable::SLRTable(
    const Grammar& grammar,
    const FirstFollow& firstFollow,
    const LR0Automaton& automaton
)
    : grammar_(grammar),
      firstFollow_(firstFollow),
      automaton_(automaton) {
}

void SLRTable::build() {
    actionTable_.clear();
    gotoTable_.clear();
    conflicts_.clear();

    for (const auto& transition : automaton_.transitions()) {
        if (grammar_.isTerminal(transition.symbol)) {
            SLRAction action;
            action.type = SLRActionType::Shift;
            action.targetState = transition.toState;
            setAction(transition.fromState, transition.symbol, action);
        } else if (grammar_.isNonterminal(transition.symbol)) {
            setGoto(transition.fromState, transition.symbol, transition.toState);
        }
    }

    const auto& states = automaton_.states();
    const auto& followSets = firstFollow_.followSets();

    for (std::size_t stateIndex = 0; stateIndex < states.size(); ++stateIndex) {
        for (const auto& item : states[stateIndex]) {
            const auto& production = productionById(item.productionId);

            if (item.dotPosition != production.rhs.size()) {
                continue;
            }

            const int currentState = static_cast<int>(stateIndex);

            if (production.lhs == grammar_.augmentedStartSymbol()) {
                SLRAction action;
                action.type = SLRActionType::Accept;
                setAction(currentState, grammar_.endToken(), action);
                continue;
            }

            const auto followIt = followSets.find(production.lhs);
            if (followIt == followSets.end()) {
                continue;
            }

            for (const auto& terminal : followIt->second) {
                SLRAction action;
                action.type = SLRActionType::Reduce;
                action.productionId = production.id;
                setAction(currentState, terminal, action);
            }
        }
    }
}

const std::map<std::pair<int, std::string>, SLRAction>& SLRTable::actionTable() const {
    return actionTable_;
}

const std::map<std::pair<int, std::string>, int>& SLRTable::gotoTable() const {
    return gotoTable_;
}

const std::vector<SLRConflict>& SLRTable::conflicts() const {
    return conflicts_;
}

int SLRTable::unresolvedConflictCount() const {
    int count = 0;

    for (const auto& conflict : conflicts_) {
        if (!conflict.resolved) {
            ++count;
        }
    }

    return count;
}

int SLRTable::resolvedConflictCount() const {
    int count = 0;

    for (const auto& conflict : conflicts_) {
        if (conflict.resolved) {
            ++count;
        }
    }

    return count;
}

void SLRTable::printActionTable() const {
    std::cout << "SLR ACTION table:\n";

    for (const auto& entry : actionTable_) {
        const int state = entry.first.first;
        const std::string& terminal = entry.first.second;
        const auto& action = entry.second;

        std::cout << "ACTION[I" << state << ", " << terminal << "] = "
                  << action.toString() << "\n";
    }

    std::cout << "Total ACTION entries: " << actionTable_.size() << "\n";
}

void SLRTable::printGotoTable() const {
    std::cout << "SLR GOTO table:\n";

    for (const auto& entry : gotoTable_) {
        const int state = entry.first.first;
        const std::string& nonterminal = entry.first.second;
        const int target = entry.second;

        std::cout << "GOTO[I" << state << ", " << nonterminal << "] = I"
                  << target << "\n";
    }

    std::cout << "Total GOTO entries: " << gotoTable_.size() << "\n";
}

void SLRTable::printConflicts() const {
    std::cout << "SLR conflicts:\n";

    if (conflicts_.empty()) {
        std::cout << "No conflicts.\n";
        return;
    }

    for (const auto& conflict : conflicts_) {
        std::cout << "State I" << conflict.state
                  << ", terminal " << conflict.terminal
                  << ": existing=" << conflict.existingAction.toString()
                  << ", incoming=" << conflict.incomingAction.toString()
                  << ", chosen=" << conflict.chosenAction.toString()
                  << ", resolved=" << (conflict.resolved ? "yes" : "no")
                  << ", reason=" << conflict.reason
                  << "\n";
    }
}

void SLRTable::printSummary() const {
    std::cout << "SLR table summary:\n";
    std::cout << "States: " << automaton_.states().size() << "\n";
    std::cout << "ACTION entries: " << actionTable_.size() << "\n";
    std::cout << "GOTO entries: " << gotoTable_.size() << "\n";
    std::cout << "Resolved conflicts: " << resolvedConflictCount() << "\n";
    std::cout << "Unresolved conflicts: " << unresolvedConflictCount() << "\n";
}

void SLRTable::setAction(int state, const std::string& terminal, const SLRAction& action) {
    const auto key = std::make_pair(state, terminal);
    const auto it = actionTable_.find(key);

    if (it == actionTable_.end()) {
        actionTable_[key] = action;
        return;
    }

    const SLRAction existingAction = it->second;

    if (actionsEqual(existingAction, action)) {
        return;
    }

    SLRConflict conflict;
    conflict.state = state;
    conflict.terminal = terminal;
    conflict.existingAction = existingAction;
    conflict.incomingAction = action;

    if (isDanglingElseConflict(terminal, existingAction, action)) {
        const SLRAction chosen = chooseShiftAction(existingAction, action);
        actionTable_[key] = chosen;

        conflict.chosenAction = chosen;
        conflict.resolved = true;
        conflict.reason = "dangling else: prefer shift so ELSE matches nearest IF";
    } else {
        conflict.chosenAction = existingAction;
        conflict.resolved = false;
        conflict.reason = "unresolved conflict";
    }

    conflicts_.push_back(conflict);
}

void SLRTable::setGoto(int state, const std::string& nonterminal, int targetState) {
    const auto key = std::make_pair(state, nonterminal);
    gotoTable_[key] = targetState;
}

bool SLRTable::actionsEqual(const SLRAction& lhs, const SLRAction& rhs) const {
    return lhs.type == rhs.type &&
           lhs.targetState == rhs.targetState &&
           lhs.productionId == rhs.productionId;
}

bool SLRTable::isDanglingElseConflict(
    const std::string& terminal,
    const SLRAction& existingAction,
    const SLRAction& incomingAction
) const {
    if (terminal != "ELSE") {
        return false;
    }

    const bool shiftReduce =
        existingAction.type == SLRActionType::Shift &&
        incomingAction.type == SLRActionType::Reduce;

    const bool reduceShift =
        existingAction.type == SLRActionType::Reduce &&
        incomingAction.type == SLRActionType::Shift;

    if (!shiftReduce && !reduceShift) {
        return false;
    }

    const SLRAction& reduceAction =
        existingAction.type == SLRActionType::Reduce ? existingAction : incomingAction;

    if (reduceAction.productionId != 5) {
        return false;
    }

    return true;
}

SLRAction SLRTable::chooseShiftAction(const SLRAction& lhs, const SLRAction& rhs) const {
    if (lhs.type == SLRActionType::Shift) {
        return lhs;
    }

    if (rhs.type == SLRActionType::Shift) {
        return rhs;
    }

    return lhs;
}

const Production& SLRTable::productionById(int productionId) const {
    for (const auto& production : grammar_.productions()) {
        if (production.id == productionId) {
            return production;
        }
    }

    throw std::runtime_error("Unknown production id: " + std::to_string(productionId));
}