#include "Parser.h"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <stack>

Parser::Parser(Lexer& lexer)
    : lexer_(lexer),
      grammar_(),
      firstFollow_(grammar_),
      automaton_(grammar_),
      table_(grammar_, firstFollow_, automaton_) {
    firstFollow_.computeFirstSets();
    firstFollow_.computeFollowSets();

    automaton_.build();

    table_.build();
}

bool Parser::parse() {
    reductions_.clear();
    trace_.clear();
    errors_.clear();

    std::vector<int> stateStack;
    std::vector<std::string> symbolStack;

    stateStack.push_back(0);

    Token currentToken = nextToken();
    std::string currentTerminal = tokenToTerminal(currentToken);

    while (true) {
        const int currentState = stateStack.back();

        const auto actionIt = table_.actionTable().find({currentState, currentTerminal});

        if (actionIt == table_.actionTable().end()) {
            std::ostringstream oss;
            oss << "Syntax error at line " << currentToken.line
                << ", column " << currentToken.col
                << ": unexpected " << tokenDisplay(currentToken)
                << " in state I" << currentState
                << ". Expected one of: ";

            const auto expected = expectedTerminals(currentState);
            if (expected.empty()) {
                oss << "(none)";
            } else {
                for (std::size_t i = 0; i < expected.size(); ++i) {
                    if (i > 0) {
                        oss << ", ";
                    }
                    oss << expected[i];
                }
            }

            addError(oss.str());
            return false;
        }

        const SLRAction action = actionIt->second;

        if (action.type == SLRActionType::Shift) {
            std::ostringstream oss;
            oss << "shift " << tokenDisplay(currentToken)
                << ", go to I" << action.targetState;
            addTrace(oss.str());

            symbolStack.push_back(currentTerminal);
            stateStack.push_back(action.targetState);

            currentToken = nextToken();
            currentTerminal = tokenToTerminal(currentToken);
        } else if (action.type == SLRActionType::Reduce) {
            const auto& production = grammar_.productions().at(static_cast<std::size_t>(action.productionId));

            for (std::size_t i = 0; i < production.rhs.size(); ++i) {
                if (!symbolStack.empty()) {
                    symbolStack.pop_back();
                }

                if (stateStack.size() > 1) {
                    stateStack.pop_back();
                }
            }

            const int gotoFromState = stateStack.back();
            const auto gotoIt = table_.gotoTable().find({gotoFromState, production.lhs});

            if (gotoIt == table_.gotoTable().end()) {
                std::ostringstream oss;
                oss << "Internal parser error: missing GOTO[I"
                    << gotoFromState << ", " << production.lhs << "]";
                addError(oss.str());
                return false;
            }

            symbolStack.push_back(production.lhs);
            stateStack.push_back(gotoIt->second);

            reductions_.push_back(production.id);

            std::ostringstream oss;
            oss << "reduce by " << grammar_.productionToString(production)
                << ", goto I" << gotoIt->second;
            addTrace(oss.str());
        } else if (action.type == SLRActionType::Accept) {
            addTrace("accept");
            return true;
        } else {
            addError("Internal parser error: invalid SLR action.");
            return false;
        }
    }
}

const std::vector<int>& Parser::reductions() const {
    return reductions_;
}

const std::vector<std::string>& Parser::trace() const {
    return trace_;
}

const std::vector<std::string>& Parser::errors() const {
    return errors_;
}

std::string Parser::tokenToTerminal(const Token& token) const {
    switch (token.type) {
        case TokenType::IDN: return "IDN";
        case TokenType::DEC: return "DEC";
        case TokenType::OCT: return "OCT";
        case TokenType::HEX: return "HEX";

        case TokenType::IF: return "IF";
        case TokenType::THEN: return "THEN";
        case TokenType::ELSE: return "ELSE";
        case TokenType::WHILE: return "WHILE";
        case TokenType::DO: return "DO";
        case TokenType::BEGIN: return "BEGIN";
        case TokenType::END: return "END";

        case TokenType::ADD: return "ADD";
        case TokenType::SUB: return "SUB";
        case TokenType::MUL: return "MUL";
        case TokenType::DIV: return "DIV";

        case TokenType::GT: return "GT";
        case TokenType::LT: return "LT";
        case TokenType::EQ: return "EQ";
        case TokenType::GE: return "GE";
        case TokenType::LE: return "LE";
        case TokenType::NEQ: return "NEQ";

        case TokenType::SLP: return "SLP";
        case TokenType::SRP: return "SRP";
        case TokenType::SEMI: return "SEMI";

        case TokenType::END_OF_FILE: return grammar_.endToken();

        case TokenType::ILOCT:
        case TokenType::ILHEX:
        case TokenType::UNKNOWN:
        default:
            return "UNKNOWN";
    }
}

std::string Parser::tokenDisplay(const Token& token) const {
    std::ostringstream oss;

    oss << tokenTypeToString(token.type);

    if (!token.lexeme.empty()) {
        oss << "('" << token.lexeme << "')";
    }

    return oss.str();
}

Token Parser::nextToken() {
    Token token = lexer_.getNextToken();

    if (token.type == TokenType::ILOCT ||
        token.type == TokenType::ILHEX ||
        token.type == TokenType::UNKNOWN) {
        std::ostringstream oss;
        oss << "Lexical error at line " << token.line
            << ", column " << token.col
            << ": invalid token " << tokenDisplay(token);
        addError(oss.str());
    }

    return token;
}

void Parser::addTrace(const std::string& message) {
    trace_.push_back(message);
}

void Parser::addError(const std::string& message) {
    errors_.push_back(message);
}

std::vector<std::string> Parser::expectedTerminals(int state) const {
    std::vector<std::string> result;

    for (const auto& entry : table_.actionTable()) {
        if (entry.first.first == state) {
            result.push_back(entry.first.second);
        }
    }

    std::sort(result.begin(), result.end());
    return result;
}