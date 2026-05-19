#include "SemanticAnalyzer.h"

void SemanticAnalyzer::applyRule(int productionId,
                                  std::vector<ParseTreeNode>& nodes,
                                  int parentIdx,
                                  const std::vector<int>& children) {
    using sz = std::size_t;
    auto& parent = nodes[sz(parentIdx)];

    switch (productionId) {

        // ── P / Stmts 规则 ───────────────────────────────────────────────────

        case 1: {
            // P → Stmts : 把 Stmts 积累的代码提升到全局 code_
            code_ = nodes[sz(children[0])].code;
            break;
        }

        case 2: {
            // Stmts → Stmt SEMI Stmts
            parent.code = nodes[sz(children[0])].code;
            for (auto& i : nodes[sz(children[2])].code) parent.code.push_back(i);
            break;
        }

        case 3:  // Stmts → ε
            break;

        // ── Stmt 规则 ────────────────────────────────────────────────────────

        case 4: {
            // Stmt → Assign
            parent.code = nodes[sz(children[0])].code;
            break;
        }

        case 5: {
            // S → if C then S₁
            // children: IF(0) C(1) THEN(2) S₁(3)
            //
            // 真假出口法（slide 70）：
            //   C.true  = newlab              ← S₁ 入口
            //   C.false = S.next              ← 条件假直接跳过 then 体
            //   S.code  = C.code
            //           ∥ gen(C.true ':') ∥ S₁.code
            //           ∥ gen(S.next ':')
            int C_idx  = children[1];
            int S1_idx = children[3];
            auto& S1 = nodes[sz(S1_idx)];

            std::string C_true = newlabel();   // C.true  = S₁ 入口
            std::string S_next = newlabel();   // C.false = S.next

            std::vector<std::string>& S_code = parent.code;
            S_code = genCondCode(nodes, C_idx, C_true, S_next);
            S_code.push_back(C_true + ":");
            for (auto& i : S1.code) S_code.push_back(i);
            S_code.push_back(S_next + ":");
            break;
        }

        case 6: {
            // S → if C then S₁ else S₂
            // children: IF(0) C(1) THEN(2) S₁(3) ELSE(4) S₂(5)
            //
            // 真假出口法（slide 73）：
            //   C.true  = newlab              ← then 体入口
            //   C.false = newlab              ← else 体入口
            //   S.next  = S₁.next = S₂.next  ← 两条分支共用出口
            //   S.code  = C.code
            //           ∥ gen(C.true ':') ∥ S₁.code ∥ gen('goto' S.next)
            //           ∥ gen(C.false ':') ∥ S₂.code
            //           ∥ gen(S.next ':')
            int C_idx  = children[1];
            int S1_idx = children[3];
            int S2_idx = children[5];
            auto& S1 = nodes[sz(S1_idx)];
            auto& S2 = nodes[sz(S2_idx)];

            std::string C_true  = newlabel();   // C.true  = then 体入口
            std::string C_false = newlabel();   // C.false = else 体入口
            std::string S_next  = newlabel();   // S₁.next = S₂.next = S.next

            std::vector<std::string>& S_code = parent.code;
            S_code = genCondCode(nodes, C_idx, C_true, C_false);
            S_code.push_back(C_true + ":");
            for (auto& i : S1.code) S_code.push_back(i);
            S_code.push_back("goto " + S_next);   // S₁ 执行完跳过 else 体
            S_code.push_back(C_false + ":");
            for (auto& i : S2.code) S_code.push_back(i);
            S_code.push_back(S_next + ":");
            break;
        }

        case 7: {
            // S → while C do S₁
            // children: WHILE(0) C(1) DO(2) S₁(3)
            //
            // 真假出口法（slide 78）：
            //   S.begin = newlab   ← 每轮循环回跳到此重新判断条件
            //   C.true  = newlab   ← 条件真，进入循环体
            //   C.false = S.next   ← 条件假，退出循环
            //   S.code  = gen(S.begin ':')
            //           ∥ C.code
            //           ∥ gen(C.true ':') ∥ S₁.code ∥ gen('goto' S.begin)
            //           ∥ gen(S.next ':')
            int C_idx  = children[1];
            int S1_idx = children[3];
            auto& S1 = nodes[sz(S1_idx)];

            std::string S_begin = newlabel();   // S.begin：循环回跳点
            std::string C_true  = newlabel();   // C.true  = S₁ 入口
            std::string S_next  = newlabel();   // C.false = S.next：退出点

            std::vector<std::string>& S_code = parent.code;
            S_code.push_back(S_begin + ":");
            for (auto& i : genCondCode(nodes, C_idx, C_true, S_next))
                S_code.push_back(i);
            S_code.push_back(C_true + ":");
            for (auto& i : S1.code) S_code.push_back(i);
            S_code.push_back("goto " + S_begin);   // 回到 S.begin 重新判断条件
            S_code.push_back(S_next + ":");
            break;
        }

        case 8: {
            // Stmt → BEGIN Stmts END
            parent.code = nodes[sz(children[1])].code;
            break;
        }

        // ── 赋值规则 ─────────────────────────────────────────────────────────

        case 9: {
            // Assign → IDN EQ Expr
            // children: IDN(0) EQ(1) Expr(2)
            parent.code = nodes[sz(children[2])].code;
            parent.code.push_back(nodes[sz(children[0])].lexeme + " = " + nodes[sz(children[2])].place);
            break;
        }

        // ── Cond 规则 ────────────────────────────────────────────────────────

        case 10: {
            // Cond → Expr1 RelOp Expr2
            // children: Expr1(0) RelOp(1) Expr2(2)
            //
            // 真假出口法：Cond 自身不生成跳转指令。
            // 真正的 C.code 由父规则（IF/WHILE）调用 genCondCode() 时生成。
            // 这里透传子节点数据供 genCondCode 读取。
            auto& Expr1 = nodes[sz(children[0])];
            auto& Expr2 = nodes[sz(children[2])];

            parent.code = Expr1.code;
            for (auto& i : Expr2.code) parent.code.push_back(i);
            // place 备用（genCondCode 直接读孩子节点，不依赖此字段）
            parent.place = Expr1.place
                         + " " + nodes[sz(children[1])].place
                         + " " + Expr2.place;
            break;
        }

        // ── RelOp 规则（place 存运算符符号）──────────────────────────────────

        case 11: parent.place = ">";  break;  // RelOp → GT
        case 12: parent.place = "<";  break;  // RelOp → LT
        case 13: parent.place = "=";  break;  // RelOp → EQ
        case 14: parent.place = ">="; break;  // RelOp → GE
        case 15: parent.place = "<="; break;  // RelOp → LE
        case 16: parent.place = "<>"; break;  // RelOp → NEQ

        // ── Expr 规则 ────────────────────────────────────────────────────────

        case 17: {
            // Expr → Term ExprRest
            //
            // 遍历 ExprRest 链，从左到右生成加减指令（保证左结合）。
            // ExprRest 本身不生成代码（case 18/19/20 均无操作），
            // 代码生成统一在此处完成，避免右结合错误。
            parent.code = nodes[sz(children[0])].code;  // Term.code
            std::string result = nodes[sz(children[0])].place;
            int restIdx = children[1];

            while (!nodes[sz(restIdx)].children.empty()) {
                // ExprRest → ADD/SUB(0)  Term(1)  ExprRest(2)
                auto& rest = nodes[sz(restIdx)];
                for (auto& i : nodes[sz(rest.children[1])].code)
                    parent.code.push_back(i);
                const std::string op = (nodes[sz(rest.children[0])].symbol == "ADD") ? "+" : "-";
                std::string t = newtemp();  // 申请临时变量存储本次加/减的结果
                parent.code.push_back(t + " := " + result + " " + op + " " + nodes[sz(rest.children[1])].place);
                result  = t;  // 将临时变量向前传递，作为下一次运算的左操作数
                restIdx = rest.children[2];
            }
            parent.place = result;
            break;
        }

        case 18:  // ExprRest → ADD Term ExprRest  (代码由规则 17 遍历链统一生成)
        case 19:  // ExprRest → SUB Term ExprRest
        case 20:  // ExprRest → ε
            break;

        // ── Term 规则 ────────────────────────────────────────────────────────

        case 21: {
            // Term → Factor TermRest
            //
            // 遍历 TermRest 链，从左到右生成乘除指令（保证左结合）。
            // 例：x * y / z 生成 t1 := x * y; t2 := t1 / z，而非右结合的错误顺序。
            parent.code = nodes[sz(children[0])].code;  // Factor.code
            std::string result = nodes[sz(children[0])].place;
            int restIdx = children[1];

            while (!nodes[sz(restIdx)].children.empty()) {
                // TermRest → MUL/DIV(0)  Factor(1)  TermRest(2)
                auto& rest = nodes[sz(restIdx)];
                for (auto& i : nodes[sz(rest.children[1])].code)
                    parent.code.push_back(i);
                const std::string op = (nodes[sz(rest.children[0])].symbol == "MUL") ? "*" : "/";
                std::string t = newtemp();  // 申请临时变量存储本次乘/除的结果
                parent.code.push_back(t + " := " + result + " " + op + " " + nodes[sz(rest.children[1])].place);
                result  = t;  // 将临时变量向前传递，作为下一次运算的左操作数
                restIdx = rest.children[2];
            }
            parent.place = result;
            break;
        }

        case 22:  // TermRest → MUL Factor TermRest  (代码由规则 21 遍历链统一生成)
        case 23:  // TermRest → DIV Factor TermRest
        case 24:  // TermRest → ε
            break;

        // ── Factor 规则 ──────────────────────────────────────────────────────

        case 25: {
            // Factor → IDN
            parent.place = nodes[sz(children[0])].lexeme;
            symTable_.enter(nodes[sz(children[0])].lexeme);
            break;
        }

        case 26: {
            // Factor → DEC
            parent.place = nodes[sz(children[0])].value;
            break;
        }

        case 27: {
            // Factor → OCT  (value 已由词法器转为十进制)
            parent.place = nodes[sz(children[0])].value;
            break;
        }

        case 28: {
            // Factor → HEX  (value 已由词法器转为十进制)
            parent.place = nodes[sz(children[0])].value;
            break;
        }

        case 29: {
            // Factor → SLP Expr SRP  :  透传括号内表达式的结果
            parent.place = nodes[sz(children[1])].place;
            parent.code  = nodes[sz(children[1])].code;
            break;
        }

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
    // 先自增再使用，确保每次返回唯一名称（t1, t2, ...）
    return "t" + std::to_string(++tempCount_);
}

std::string SemanticAnalyzer::newlabel() {
    // 先自增再使用，确保每次返回唯一名称（L1, L2, ...）；嵌套结构下不会冲突
    return "L" + std::to_string(++labelCount_);
}

void SemanticAnalyzer::gen(const std::string& instr) {
    code_.push_back(instr);
}

// ============================================================
// genCondCode —— 真假出口法条件跳转代码生成器
//
// 对应课件规则（slide 64）：
//   Cond → Expr1 relop Expr2
//   C.code = Expr1.code ∥ Expr2.code
//          ∥ gen('if' Expr1.place relop Expr2.place 'goto' C.true)
//          ∥ gen('goto' C.false)
// ============================================================
std::vector<std::string> SemanticAnalyzer::genCondCode(
        std::vector<ParseTreeNode>& nodes,
        int condIdx,
        const std::string& C_true,
        const std::string& C_false) {
    using sz = std::size_t;
    auto& cond  = nodes[sz(condIdx)];
    auto& Expr1 = nodes[sz(cond.children[0])];   // 左操作数
    auto& RelOp = nodes[sz(cond.children[1])];   // 比较运算符（place = ">", "<", ...）
    auto& Expr2 = nodes[sz(cond.children[2])];   // 右操作数

    std::vector<std::string> C_code;
    for (auto& i : Expr1.code) C_code.push_back(i);
    for (auto& i : Expr2.code) C_code.push_back(i);
    C_code.push_back(
        "if " + Expr1.place + " " + RelOp.place + " " + Expr2.place + " goto " + C_true);
    C_code.push_back("goto " + C_false);
    return C_code;
}
