#pragma once

#include "Grammar.h"
#include "Lexer.h"
#include "SLRTable.h"
#include "ParseTreeNode.h"
#include "SemanticAnalyzer.h"

#include <ostream>
#include <string>
#include <vector>

class Parser {
public:
    explicit Parser(Lexer& lexer);

    bool parse();

    const std::vector<int>& reductions() const;
    const std::vector<std::string>& trace() const;
    const std::vector<std::string>& errors() const;

    const std::vector<ParseTreeNode>& parseTreeNodes() const;
    int parseTreeRoot() const;
    void printParseTree(std::ostream& os) const;

    const SemanticAnalyzer& semanticAnalyzer() const;

private:
    Lexer& lexer_;

    Grammar grammar_;
    FirstFollow firstFollow_;
    LR0Automaton automaton_;
    SLRTable table_;

    std::vector<int> reductions_;
    std::vector<std::string> trace_;
    std::vector<std::string> errors_;

    std::vector<ParseTreeNode> parseTreeNodes_;
    int parseTreeRoot_ = -1;

    SemanticAnalyzer semanticAnalyzer_;

    std::string tokenToTerminal(const Token& token) const;
    std::string tokenDisplay(const Token& token) const;

    Token nextToken();

    void addTrace(const std::string& message);
    void addError(const std::string& message);

    std::vector<std::string> expectedTerminals(int state) const;

    int createLeafNode(const std::string& symbol, const Token& token);
    int createParentNode(const std::string& symbol, const std::vector<int>& children);
    void printParseTreeNode(std::ostream& os, int nodeIndex, int depth) const;
};