# Experiment 3 Phase 1: Semantic Infrastructure & Declaration Rules

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build the three-address code generation infrastructure and implement the base `Factor` production semantic rules that assign `.place` attributes to identifiers and number literals (the "declaration-level" binding, equivalent to 7.2节). Assignment (7.4节) and control flow (7.5节) are explicitly deferred to Phase 2.

**Architecture:** `ParseTreeNode` gains `value`/`place`/`code` attributes. A new `SemanticAnalyzer` class owns `SymbolTable`, `newtemp()`, and `gen()`, and dispatches per-production rules via `applyRule()`. The Parser embeds a `SemanticAnalyzer` and calls `applyRule()` in its reduce branch after every `createParentNode()` — satisfying the teacher's requirement of "在语法分析的每次规约时正确调用执行对应的语义规则". Circular include (`Parser.h` ↔ `SemanticAnalyzer.h`) is resolved by extracting `ParseTreeNode` to its own header.

**Tech Stack:** C++17, CMake 3.14 (existing build system, no new dependencies)

---

## File Map

| Action | File | Responsibility |
|--------|------|----------------|
| **Create** | `include/ParseTreeNode.h` | `ParseTreeNode` struct with `value`, `place`, `code` attributes |
| **Modify** | `include/Parser.h` | Replace inline struct with `#include "ParseTreeNode.h"`; add `SemanticAnalyzer` member + getter |
| **Modify** | `src/Parser.cpp` | Populate `node.value` in `createLeafNode`; call `applyRule()` after `createParentNode()` |
| **Create** | `include/SymbolTable.h` | `SymbolEntry` struct + `SymbolTable` class |
| **Create** | `src/SymbolTable.cpp` | `enter()` / `lookup()` implementation |
| **Create** | `include/SemanticAnalyzer.h` | `newtemp()`, `gen()`, `applyRule()` declaration |
| **Create** | `src/SemanticAnalyzer.cpp` | Rule dispatch switch + Factor rules 25–29 |
| **Modify** | `src/ParserMain.cpp` | Print symbol table and generated 3AC after parse tree |
| **Modify** | `Cmakelists.txt` | Add `SymbolTable.cpp` and `SemanticAnalyzer.cpp` to `parser_lib` |

---

## Background: Production IDs in this Grammar

```
// From Grammar.cpp — full list
 0: S' → P
 1: P  → Stmts
 2: Stmts → Stmt SEMI Stmts
 3: Stmts → ε
 4: Stmt  → Assign
 5: Stmt  → IF Cond THEN Stmt
 6: Stmt  → IF Cond THEN Stmt ELSE Stmt
 7: Stmt  → WHILE Cond DO Stmt
 8: Stmt  → BEGIN Stmts END
 9: Assign → IDN EQ Expr
10: Cond   → Expr RelOp Expr
11: RelOp  → GT
12: RelOp  → LT
13: RelOp  → EQ
14: RelOp  → GE
15: RelOp  → LE
16: RelOp  → NEQ
17: Expr     → Term ExprRest
18: ExprRest → ADD Term ExprRest
19: ExprRest → SUB Term ExprRest
20: ExprRest → ε
21: Term     → Factor TermRest
22: TermRest → MUL Factor TermRest
23: TermRest → DIV Factor TermRest
24: TermRest → ε
25: Factor   → IDN
26: Factor   → DEC
27: Factor   → OCT
28: Factor   → HEX
29: Factor   → SLP Expr SRP
```

**Phase 1 implements rules 25–29 only.** Rules 17–24 (arithmetic, temp allocation) and 9–10 (assignment, conditions) are Phase 2+.

---

## Background: Token value vs lexeme

The `Lexer` already performs base conversion before storing tokens:

| Token type | `lexeme` | `value` |
|------------|----------|---------|
| `DEC` | `"15"` | `"15"` |
| `OCT` | `"07"` | `"7"` (decimal) |
| `HEX` | `"0xa"` | `"10"` (decimal) |
| `IDN` | `"a3"` | `"a3"` |

Semantic rules must use `node.value` (the decimal-converted string), not `node.lexeme`, for number literals. `ParseTreeNode` currently stores only `lexeme` — this plan adds a `value` field populated from `token.value`.

---

## Task 1: Extract ParseTreeNode and add semantic attributes

**Files:**
- Create: `include/ParseTreeNode.h`
- Modify: `include/Parser.h` (lines 11–15 — remove inline struct definition)
- Modify: `src/Parser.cpp` (line 263-269 — `createLeafNode`, populate `node.value`)

- [ ] **Step 1: Create `include/ParseTreeNode.h`**

```cpp
#pragma once

#include <string>
#include <vector>

struct ParseTreeNode {
    std::string symbol;
    std::string lexeme;   // raw source text (as lexed)
    std::string value;    // processed value: for DEC/OCT/HEX, already decimal; for IDN, same as lexeme
    std::vector<int> children;

    // Semantic attributes — populated by SemanticAnalyzer during reduce
    std::string place;                  // E.place: variable or temp that holds this node's value
    std::vector<std::string> code;      // E.code: 3AC instructions generated for this subtree
};
```

- [ ] **Step 2: Replace inline ParseTreeNode in `include/Parser.h`**

In `include/Parser.h`, remove these lines (currently lines 11–15):
```cpp
struct ParseTreeNode {
    std::string symbol;
    std::string lexeme;
    std::vector<int> children;
};
```

Replace them with:
```cpp
#include "ParseTreeNode.h"
```

The `#include "ParseTreeNode.h"` should be placed alongside the other includes at the top of the file (after the `#pragma once` line, before the class definition).

- [ ] **Step 3: Populate `node.value` in `createLeafNode` in `src/Parser.cpp`**

Locate `createLeafNode` (currently around line 262):
```cpp
int Parser::createLeafNode(const std::string& symbol, const Token& token) {
    ParseTreeNode node;
    node.symbol = symbol;
    node.lexeme = token.lexeme;

    parseTreeNodes_.push_back(node);
    return static_cast<int>(parseTreeNodes_.size() - 1);
}
```

Change it to:
```cpp
int Parser::createLeafNode(const std::string& symbol, const Token& token) {
    ParseTreeNode node;
    node.symbol = symbol;
    node.lexeme = token.lexeme;
    node.value  = token.value;

    parseTreeNodes_.push_back(node);
    return static_cast<int>(parseTreeNodes_.size() - 1);
}
```

- [ ] **Step 4: Build and run all existing tests**

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
cd build && ctest --output-on-failure
```

Expected: all tests pass — the struct change is purely additive.

- [ ] **Step 5: Commit**

```
git add include/ParseTreeNode.h include/Parser.h src/Parser.cpp
git commit -m "refactor: extract ParseTreeNode to own header, add value/place/code semantic attributes"
```

---

## Task 2: Create SymbolTable

**Files:**
- Create: `include/SymbolTable.h`
- Create: `src/SymbolTable.cpp`
- Modify: `Cmakelists.txt`

- [ ] **Step 1: Write `include/SymbolTable.h`**

```cpp
#pragma once

#include <string>
#include <vector>

struct SymbolEntry {
    std::string name;
    std::string type;   // "unknown" until type inference is implemented
    int offset;         // -1 until offset allocation is implemented
};

class SymbolTable {
public:
    // Register a variable. Silently ignores duplicate names.
    void enter(const std::string& name,
               const std::string& type = "unknown",
               int offset = -1);

    // Returns pointer to entry, or nullptr if not found.
    SymbolEntry* lookup(const std::string& name);

    const std::vector<SymbolEntry>& entries() const;

private:
    std::vector<SymbolEntry> entries_;
};
```

- [ ] **Step 2: Write `src/SymbolTable.cpp`**

```cpp
#include "SymbolTable.h"

void SymbolTable::enter(const std::string& name,
                        const std::string& type,
                        int offset) {
    if (lookup(name) != nullptr) return;
    entries_.push_back({name, type, offset});
}

SymbolEntry* SymbolTable::lookup(const std::string& name) {
    for (auto& entry : entries_) {
        if (entry.name == name) return &entry;
    }
    return nullptr;
}

const std::vector<SymbolEntry>& SymbolTable::entries() const {
    return entries_;
}
```

- [ ] **Step 3: Add `SymbolTable.cpp` to `parser_lib` in `Cmakelists.txt`**

Find the `add_library(parser_lib STATIC ...)` block and add one line:
```cmake
    ${PROJECT_SOURCE_DIR}/src/SymbolTable.cpp
```

The block should then read:
```cmake
add_library(parser_lib STATIC
    ${PROJECT_SOURCE_DIR}/src/Grammar.cpp
    ${PROJECT_SOURCE_DIR}/src/FirstFollow.cpp
    ${PROJECT_SOURCE_DIR}/src/LR0Automaton.cpp
    ${PROJECT_SOURCE_DIR}/src/SLRTable.cpp
    ${PROJECT_SOURCE_DIR}/src/Parser.cpp
    ${PROJECT_SOURCE_DIR}/src/SymbolTable.cpp
)
```

- [ ] **Step 4: Build**

```
cmake --build build
```

Expected: compiles without errors.

- [ ] **Step 5: Commit**

```
git add include/SymbolTable.h src/SymbolTable.cpp Cmakelists.txt
git commit -m "feat: add SymbolTable with enter/lookup for semantic analysis"
```

---

## Task 3: Create SemanticAnalyzer and hook into Parser

This task creates the `SemanticAnalyzer` class and wires it into `Parser`'s reduce step. The switch body starts empty (no-op `default`); rules are filled in Task 4.

**Files:**
- Create: `include/SemanticAnalyzer.h`
- Create: `src/SemanticAnalyzer.cpp`
- Modify: `include/Parser.h`
- Modify: `src/Parser.cpp`
- Modify: `Cmakelists.txt`

- [ ] **Step 1: Write `include/SemanticAnalyzer.h`**

```cpp
#pragma once

#include "ParseTreeNode.h"
#include "SymbolTable.h"

#include <string>
#include <vector>

class SemanticAnalyzer {
public:
    // Called by Parser immediately after each reduce action.
    //   productionId : which production was reduced (matches Grammar::productions() IDs)
    //   nodes        : the full parse-tree node vector (children already have attributes set)
    //   parentIdx    : index of the newly created parent node (write attributes here)
    //   children     : child node indices, in RHS left-to-right order
    void applyRule(int productionId,
                   std::vector<ParseTreeNode>& nodes,
                   int parentIdx,
                   const std::vector<int>& children);

    const std::vector<std::string>& generatedCode() const;
    const SymbolTable& symbolTable() const;

private:
    int tempCount_ = 0;
    std::vector<std::string> code_;
    SymbolTable symTable_;

    std::string newtemp();
    void gen(const std::string& instr);
};
```

- [ ] **Step 2: Write `src/SemanticAnalyzer.cpp` (skeleton — rules added in Task 4)**

```cpp
#include "SemanticAnalyzer.h"

void SemanticAnalyzer::applyRule(int productionId,
                                  std::vector<ParseTreeNode>& /*nodes*/,
                                  int /*parentIdx*/,
                                  const std::vector<int>& /*children*/) {
    switch (productionId) {
        default:
            break;
    }
}

const std::vector<std::string>& SemanticAnalyzer::generatedCode() const {
    return code_;
}

const SymbolTable& SemanticAnalyzer::symbolTable() const {
    return symTable_;
}

std::string SemanticAnalyzer::newtemp() {
    return "t" + std::to_string(++tempCount_);
}

void SemanticAnalyzer::gen(const std::string& instr) {
    code_.push_back(instr);
}
```

- [ ] **Step 3: Update `include/Parser.h`**

Add these two includes at the top (after existing `#include` lines, before the class definition):
```cpp
#include "SemanticAnalyzer.h"
```

Add to the `public:` section of `Parser`:
```cpp
const SemanticAnalyzer& semanticAnalyzer() const;
```

Add to the `private:` section of `Parser`:
```cpp
SemanticAnalyzer semanticAnalyzer_;
```

- [ ] **Step 4: Call `applyRule()` in `src/Parser.cpp`'s reduce branch**

Locate this line in `Parser::parse()` (currently around line 105):
```cpp
const int parentIndex = createParentNode(production.lhs, children);
```

Immediately after it, insert:
```cpp
semanticAnalyzer_.applyRule(production.id, parseTreeNodes_, parentIndex, children);
```

The surrounding reduce block should now look like:
```cpp
const int parentIndex = createParentNode(production.lhs, children);
semanticAnalyzer_.applyRule(production.id, parseTreeNodes_, parentIndex, children);

const int gotoFromState = stateStack.back();
```

- [ ] **Step 5: Add the `semanticAnalyzer()` getter to `src/Parser.cpp`**

Append at the bottom of `Parser.cpp`:
```cpp
const SemanticAnalyzer& Parser::semanticAnalyzer() const {
    return semanticAnalyzer_;
}
```

- [ ] **Step 6: Add `SemanticAnalyzer.cpp` to `parser_lib` in `Cmakelists.txt`**

```cmake
    ${PROJECT_SOURCE_DIR}/src/SemanticAnalyzer.cpp
```

- [ ] **Step 7: Build and run all tests**

```
cmake --build build
cd build && ctest --output-on-failure
```

Expected: all existing tests pass (applyRule is still a no-op).

- [ ] **Step 8: Commit**

```
git add include/SemanticAnalyzer.h src/SemanticAnalyzer.cpp include/Parser.h src/Parser.cpp Cmakelists.txt
git commit -m "feat: hook SemanticAnalyzer into SLR Parser reduce step"
```

---

## Task 4: Implement Factor production rules (25–29)

These are the base rules of the attribute grammar. They assign `.place` to the result of each `Factor` node:
- **Identifiers**: `.place = identifier name` (also enters the name into the symbol table)
- **Number literals**: `.place = decimal string` (using the pre-converted `node.value`)
- **Parenthesized expression**: `.place` and `.code` propagated from the inner `Expr`

**Files:**
- Modify: `src/SemanticAnalyzer.cpp` — fill in the switch cases

- [ ] **Step 1: Replace the `applyRule` body in `src/SemanticAnalyzer.cpp` with the Factor rules**

```cpp
void SemanticAnalyzer::applyRule(int productionId,
                                  std::vector<ParseTreeNode>& nodes,
                                  int parentIdx,
                                  const std::vector<int>& children) {
    using sz = std::size_t;

    switch (productionId) {

        case 25: {
            // Factor → IDN  :  Factor.place = id.name; register in symbol table
            nodes[sz(parentIdx)].place = nodes[sz(children[0])].lexeme;
            symTable_.enter(nodes[sz(children[0])].lexeme);
            break;
        }

        case 26: {
            // Factor → DEC  :  Factor.place = decimal string (value == lexeme for DEC)
            nodes[sz(parentIdx)].place = nodes[sz(children[0])].value;
            break;
        }

        case 27: {
            // Factor → OCT  :  Factor.place = decimal-converted value
            // node.value already holds the decimal string (set by Lexer::octToDecStr)
            nodes[sz(parentIdx)].place = nodes[sz(children[0])].value;
            break;
        }

        case 28: {
            // Factor → HEX  :  Factor.place = decimal-converted value
            // node.value already holds the decimal string (set by Lexer::hexToDecStr)
            nodes[sz(parentIdx)].place = nodes[sz(children[0])].value;
            break;
        }

        case 29: {
            // Factor → SLP Expr SRP  :  propagate Expr.place and Expr.code
            // children[0]=SLP, children[1]=Expr, children[2]=SRP
            nodes[sz(parentIdx)].place = nodes[sz(children[1])].place;
            nodes[sz(parentIdx)].code  = nodes[sz(children[1])].code;
            break;
        }

        default:
            break;  // rules for Expr/Term/Assign/Cond: deferred to Phase 2
    }
}
```

- [ ] **Step 2: Build**

```
cmake --build build
```

Expected: no errors.

- [ ] **Step 3: Commit**

```
git add src/SemanticAnalyzer.cpp
git commit -m "feat: implement Factor semantic rules 25-29 (place attribute for IDN/DEC/OCT/HEX/paren)"
```

---

## Task 5: Update ParserMain to show semantic output

This makes the symbol table and generated 3AC visible. For Phase 1 the code list will be empty (no arithmetic rules yet); the symbol table will list all identifiers seen.

**Files:**
- Modify: `src/ParserMain.cpp`

- [ ] **Step 1: Add `#include "SemanticAnalyzer.h"` at the top of `src/ParserMain.cpp`**

The include is already pulled in transitively through `Parser.h`, but add it explicitly for clarity:
```cpp
#include "SemanticAnalyzer.h"
```

- [ ] **Step 2: Update the banner and add semantic output in `src/ParserMain.cpp`**

Change the banner (around line 46):
```cpp
std::cout << "Lab 2/3 SLR Parser + Semantic Analyzer\n";
```

After the `parser.printParseTree(std::cout)` block, add:

```cpp
    std::cout << "\n============================================\n";
    std::cout << "Generated Three-Address Code\n";
    std::cout << "============================================\n";

    const auto& codeList = parser.semanticAnalyzer().generatedCode();
    if (codeList.empty()) {
        std::cout << "(none)\n";
    } else {
        for (const auto& instr : codeList) {
            std::cout << instr << "\n";
        }
    }

    std::cout << "\n============================================\n";
    std::cout << "Symbol Table\n";
    std::cout << "============================================\n";

    const auto& symEntries = parser.semanticAnalyzer().symbolTable().entries();
    if (symEntries.empty()) {
        std::cout << "(empty)\n";
    } else {
        std::cout << std::left
                  << std::setw(16) << "Name"
                  << std::setw(12) << "Type"
                  << "Offset\n";
        std::cout << std::string(36, '-') << "\n";
        for (const auto& entry : symEntries) {
            std::cout << std::setw(16) << entry.name
                      << std::setw(12) << entry.type
                      << entry.offset << "\n";
        }
    }
```

Also add `#include <iomanip>` to the includes at the top of `ParserMain.cpp` (needed for `std::setw`/`std::left`).

- [ ] **Step 3: Create test input file `tests/test_exp3.txt`**

This is the official test case from the experiment PDF:
```
while (a3+15)>0xa do if x2 = 07 then while y<z do y = x * y / z; c=b*c+d;
```

- [ ] **Step 4: Build and run**

```
cmake --build build
build\parser_main tests\test_exp3.txt
```

Expected output includes:

```
Generated Three-Address Code
============================================
(none)

============================================
Symbol Table
============================================
Name            Type        Offset
------------------------------------
a3              unknown     -1
x2              unknown     -1
y               unknown     -1
z               unknown     -1
x               unknown     -1
c               unknown     -1
b               unknown     -1
d               unknown     -1
```

Also verify that `Factor → OCT("07")` gave `.place = "7"` and `Factor → HEX("0xa")` gave `.place = "10"` by checking the parse tree output shows correct `.place` propagation (or by adding a temporary debug print in `applyRule` for cases 27 and 28 while testing).

- [ ] **Step 5: Build and run all tests to confirm no regressions**

```
cd build && ctest --output-on-failure
```

Expected: all tests pass.

- [ ] **Step 6: Commit**

```
git add src/ParserMain.cpp tests/test_exp3.txt
git commit -m "feat: print symbol table and 3AC output in parser_main for Exp3"
```

---

## What Phase 2 will add

When the teacher covers 7.4节 (assignment) and 7.5节 (control flow), the following rules need to be implemented in `SemanticAnalyzer.cpp`'s switch. The infrastructure (newtemp, gen, SymbolTable) is already in place.

### Arithmetic rules (7.4节, left-factored grammar)

The grammar uses `Expr → Term ExprRest` and `Term → Factor TermRest` to eliminate left recursion. The challenge is that `ExprRest` and `TermRest` are right-recursive, so synthesized attributes must be passed "upward" in two steps:

**Pass-through (base cases):**
- Rule 20: `ExprRest → ε` — `ExprRest.place = ""`  (no new temp)
- Rule 24: `TermRest → ε` — `TermRest.place = ""`

**Combine with operator (recursive cases):**
- Rule 18: `ExprRest → ADD Term ExprRest`
  - If `ExprRest_child.place` is empty: `ExprRest.place = Term.place` (no-op tail)
  - Else: `tmp = newtemp(); gen(tmp + " := " + Term.place + " + " + ExprRest_child.place); ExprRest.place = tmp`
- Rule 19: `ExprRest → SUB Term ExprRest` — same pattern with `-`
- Rule 22: `TermRest → MUL Factor TermRest` — same pattern with `*`
- Rule 23: `TermRest → DIV Factor TermRest` — same pattern with `/`

**Root:**
- Rule 17: `Expr → Term ExprRest`
  - If `ExprRest.place` is empty: `Expr.place = Term.place` (pure Term)
  - Else: `Expr.place = ExprRest.place`
- Rule 21: `Term → Factor TermRest` — same logic

**Assignment:**
- Rule 9: `Assign → IDN EQ Expr`
  - `gen(IDN.lexeme + " = " + Expr.place)`

### Control flow rules (7.5节)

These require inherited attributes (`.true` / `.false` labels passed downward), which need a separate mechanism — a label-stack or inherited-attribute side-channel. Design is deferred until the teacher introduces this.

---

## Self-Review Against Requirements

| Teacher's requirement | Covered? |
|-----------------------|----------|
| 基础：每次规约时正确调用语义规则 | ✅ Task 3 hooks `applyRule()` into every reduce |
| 基础：理解属性文法含义 | ✅ Tasks 1+4 directly implement the `E.place`/`E.code` attribute grammar from slides |
| 进阶（可选）：自行设计属性文法 | ✅ The ExprRest/TermRest combine-pattern in Phase 2 is a novel left-factored attribute grammar not in the textbook |
| 数字字面量进制转换 | ✅ Rule 27 (OCT) and 28 (HEX) use `node.value` pre-converted by Lexer |
| 符号表 enter/lookup | ✅ Task 2 + Rule 25 calls `symTable_.enter(id.name)` |
| 不修改实验二已有输出 | ✅ Reduction sequence and parse tree output are unchanged; 3AC is appended after |
