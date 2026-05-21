#include "SemanticAnalyzer.h"
// 判断是否为标签定义行（形如 "Lx:"，无空格）
static bool isLabelDef(const std::string& s) {
    return s.size() > 1 && s.back() == ':' && s.find(' ') == std::string::npos;
}

// 取标签名（去掉末尾 ':'）
static std::string labelName(const std::string& s) {
    return s.substr(0, s.size() - 1);
}

// 判断是否为无条件跳转（"goto Lx"）
static bool isUnconditionalGoto(const std::string& s) {
    return s.size() > 5 && s.substr(0, 5) == "goto ";
}

// 取 "goto Lx" 中的目标标签名
static std::string gotoTarget(const std::string& s) {
    return s.substr(5);
}

// 将 code 中所有对 from 标签的引用（整词匹配）替换为 to
static void replaceAllRefs(std::vector<std::string>& code,
                            const std::string& from,
                            const std::string& to) {
    for (auto& instr : code) {
        std::size_t pos = 0;
        while ((pos = instr.find(from, pos)) != std::string::npos) {
            bool pre  = (pos == 0 ||
                         (!std::isalnum((unsigned char)instr[pos - 1]) &&
                          instr[pos - 1] != '_'));
            bool post = (pos + from.size() >= instr.size() ||
                         (!std::isalnum((unsigned char)instr[pos + from.size()]) &&
                          instr[pos + from.size()] != '_'));
            if (pre && post) {
                instr.replace(pos, from.size(), to);
                pos += to.size();
            } else {
                pos += from.size();
            }
        }
    }
}

// ============================================================
// coerce —— 生成类型转换包装（若需要）
//
// 规则（对应课件 slide 50）：
//   int  → real : itr(place)
//   real → int  : rti(place)
//   相同类型    : place（原样返回）
// ============================================================
static std::string coerce(const std::string& fromType,
                           const std::string& toType,
                           const std::string& place) {
    if (fromType == toType) return place;
    if (toType == "real")   return "itr(" + place + ")";   // int to real
    return                         "rti(" + place + ")";   // real to int
}

void SemanticAnalyzer::applyRule(int productionId,
                                  std::vector<ParseTreeNode>& nodes,
                                  int parentIdx,
                                  const std::vector<int>& children) {
    using sz = std::size_t;
    auto& parent = nodes[sz(parentIdx)];

    switch (productionId) {

        // ── P / Stmts 规则 ───────────────────────────────────────────────────

        case 1: {
            // P → Stmts
            code_ = nodes[sz(children[0])].code;
            postProcess();
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
            int C_idx  = children[1];
            int S1_idx = children[3];
            auto& S1 = nodes[sz(S1_idx)];

            std::string C_true = newlabel();
            std::string S_next = newlabel();

            std::vector<std::string>& S_code = parent.code;
            S_code = genCondCode(nodes, C_idx, C_true, S_next);
            S_code.push_back(C_true + ":");
            for (auto& i : S1.code) S_code.push_back(i);
            S_code.push_back(S_next + ":");
            break;
        }

        case 6: {
            // S → if C then S₁ else S₂
            int C_idx  = children[1];
            int S1_idx = children[3];
            int S2_idx = children[5];
            auto& S1 = nodes[sz(S1_idx)];
            auto& S2 = nodes[sz(S2_idx)];

            std::string C_true  = newlabel();
            std::string C_false = newlabel();
            std::string S_next  = newlabel();

            std::vector<std::string>& S_code = parent.code;
            S_code = genCondCode(nodes, C_idx, C_true, C_false);
            S_code.push_back(C_true + ":");
            for (auto& i : S1.code) S_code.push_back(i);
            S_code.push_back("goto " + S_next);
            S_code.push_back(C_false + ":");
            for (auto& i : S2.code) S_code.push_back(i);
            S_code.push_back(S_next + ":");
            break;
        }

        case 7: {
            // S → while C do S₁
            int C_idx  = children[1];
            int S1_idx = children[3];
            auto& S1 = nodes[sz(S1_idx)];

            std::string S_begin = newlabel();
            std::string C_true  = newlabel();
            std::string S_next  = newlabel();

            std::vector<std::string>& S_code = parent.code;
            S_code.push_back(S_begin + ":");
            for (auto& i : genCondCode(nodes, C_idx, C_true, S_next))
                S_code.push_back(i);
            S_code.push_back(C_true + ":");
            for (auto& i : S1.code) S_code.push_back(i);
            S_code.push_back("goto " + S_begin);
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
            //
            // 类型转换（slide 50）：
            //   if id.type = E.type  → gen(id := E.place)
            //   if id.type = real    → gen(id := itr(E.place))   // int to real
            //   else                 → gen(id := rti(E.place))   // real to int
            //
            // 由于语言无显式类型声明，变量类型由首次赋值推断：
            //   首次赋值（getType 返回 "unknown"）→ 无需转换，同时记录类型。
            //   后续赋值类型不同时 → 插入转换指令，变量保持原类型。
            parent.code = nodes[sz(children[2])].code;
            const std::string& idName    = nodes[sz(children[0])].lexeme;
            const std::string& exprPlace = nodes[sz(children[2])].place;
            const std::string& exprType  = nodes[sz(children[2])].type;  // ← NEW
            std::string idType = symTable_.getType(idName);               // ← NEW

            if (idType == "unknown") {
                // 首次赋值：直接赋值并记录类型
                parent.code.push_back(idName + " = " + exprPlace);
                symTable_.setType(idName, exprType);                      // ← NEW
            } else {
                // 后续赋值：按 slide 50 规则决定是否插入转换
                parent.code.push_back(
                    idName + " = " + coerce(exprType, idType, exprPlace)); // ← NEW
            }
            symTable_.enter(idName);
            break;
        }

        // ── Cond 规则 ────────────────────────────────────────────────────────

        case 10: {
            // Cond → Expr1 RelOp Expr2
            auto& Expr1 = nodes[sz(children[0])];
            auto& Expr2 = nodes[sz(children[2])];

            parent.code = Expr1.code;
            for (auto& i : Expr2.code) parent.code.push_back(i);
            parent.place = Expr1.place
                         + " " + nodes[sz(children[1])].place
                         + " " + Expr2.place;
            break;
        }

        // ── RelOp 规则 ────────────────────────────────────────────────────────

        case 11: parent.place = ">";  break;
        case 12: parent.place = "<";  break;
        case 13: parent.place = "=";  break;
        case 14: parent.place = ">="; break;
        case 15: parent.place = "<="; break;
        case 16: parent.place = "<>"; break;

        // ── Expr 规则 ────────────────────────────────────────────────────────

        case 17: {
            // Expr → Term ExprRest
            //
            // 类型转换（slide 50，E→E₁+E₂ 部分）：
            //   E₁.type = E₂.type → gen(t := E₁.place + E₂.place)
            //   E₁.type = real    → gen(t := E₁.place + itr(E₂.place))
            //   else              → gen(t := itr(E₁.place) + E₂.place); type=real
            parent.code = nodes[sz(children[0])].code;
            std::string result     = nodes[sz(children[0])].place;
            std::string resultType = nodes[sz(children[0])].type;  // ← NEW
            int restIdx = children[1];

            while (!nodes[sz(restIdx)].children.empty()) {
                auto& rest = nodes[sz(restIdx)];
                for (auto& i : nodes[sz(rest.children[1])].code)
                    parent.code.push_back(i);
                const std::string op       = (nodes[sz(rest.children[0])].symbol == "ADD") ? "+" : "-";
                const std::string termPlace = nodes[sz(rest.children[1])].place;
                const std::string termType  = nodes[sz(rest.children[1])].type;  // ← NEW
                std::string t = newtemp();

                // ── 类型提升逻辑（← NEW）────────────────────────────────────
                if (resultType == termType) {
                    // 同类型：直接运算
                    parent.code.push_back(t + " := " + result + " " + op + " " + termPlace);
                } else if (resultType == "real") {
                    // 左 real、右 int：右侧 itr
                    parent.code.push_back(t + " := " + result + " " + op + " itr(" + termPlace + ")");
                } else {
                    // 左 int、右 real：左侧 itr，结果提升为 real
                    parent.code.push_back(t + " := itr(" + result + ") " + op + " " + termPlace);
                    resultType = "real";
                }
                // ─────────────────────────────────────────────────────────────
                result  = t;
                restIdx = rest.children[2];
            }
            parent.place = result;
            parent.type  = resultType;  // ← NEW：向上传播结果类型
            break;
        }

        case 18:
        case 19:
        case 20:
            break;

        // ── Term 规则 ────────────────────────────────────────────────────────

        case 21: {
            // Term → Factor TermRest
            //
            // 类型转换逻辑与 case 17 对称（乘除同加减）。
            parent.code = nodes[sz(children[0])].code;
            std::string result     = nodes[sz(children[0])].place;
            std::string resultType = nodes[sz(children[0])].type;  // ← NEW
            int restIdx = children[1];

            while (!nodes[sz(restIdx)].children.empty()) {
                auto& rest = nodes[sz(restIdx)];
                for (auto& i : nodes[sz(rest.children[1])].code)
                    parent.code.push_back(i);
                const std::string op        = (nodes[sz(rest.children[0])].symbol == "MUL") ? "*" : "/";
                const std::string factPlace = nodes[sz(rest.children[1])].place;
                const std::string factType  = nodes[sz(rest.children[1])].type;  // ← NEW
                std::string t = newtemp();

                // ── 类型提升逻辑（← NEW）────────────────────────────────────
                if (resultType == factType) {
                    parent.code.push_back(t + " := " + result + " " + op + " " + factPlace);
                } else if (resultType == "real") {
                    parent.code.push_back(t + " := " + result + " " + op + " itr(" + factPlace + ")");
                } else {
                    parent.code.push_back(t + " := itr(" + result + ") " + op + " " + factPlace);
                    resultType = "real";
                }
                // ─────────────────────────────────────────────────────────────
                result  = t;
                restIdx = rest.children[2];
            }
            parent.place = result;
            parent.type  = resultType;  // ← NEW
            break;
        }

        case 22:
        case 23:
        case 24:
            break;

        // ── Factor 规则 ──────────────────────────────────────────────────────

        case 25: {
            // Factor → IDN
            parent.place = nodes[sz(children[0])].lexeme;
            // ← NEW：从符号表取变量类型，未赋值变量默认为 "int"
            const std::string t = symTable_.getType(parent.place);
            parent.type = (t == "unknown") ? "int" : t;
            symTable_.enter(nodes[sz(children[0])].lexeme);
            break;
        }

        case 26: {
            // Factor → DEC
            parent.place = nodes[sz(children[0])].value;
            parent.type  = "int";   // ← NEW
            break;
        }

        case 27: {
            // Factor → OCT
            parent.place = nodes[sz(children[0])].value;
            parent.type  = "int";   // ← NEW
            break;
        }

        case 28: {
            // Factor → HEX
            parent.place = nodes[sz(children[0])].value;
            parent.type  = "int";   // ← NEW
            break;
        }

        case 29: {
            // Factor → SLP Expr SRP
            parent.place = nodes[sz(children[1])].place;
            parent.code  = nodes[sz(children[1])].code;
            parent.type  = nodes[sz(children[1])].type;   // ← NEW：传播括号内类型
            break;
        }

        // ── 新增：实数字面量 ──────────────────────────────────────────────────

        case 30: {
            // Factor → REAL   （如 3.14、2.0；由词法器识别为 REAL token）
            // 对应 slide 50 中引入实数类型的基础情形。
            parent.place = nodes[sz(children[0])].value;   // 字面量字符串，如 "3.14"
            parent.type  = "real";                         // ← NEW
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
    return "t" + std::to_string(++tempCount_);
}

std::string SemanticAnalyzer::newlabel() {
    return "L" + std::to_string(++labelCount_);
}

void SemanticAnalyzer::gen(const std::string& instr) {
    code_.push_back(instr);
}

// ============================================================
// genCondCode —— 真假出口法条件跳转代码生成器
// ============================================================
std::vector<std::string> SemanticAnalyzer::genCondCode(
        std::vector<ParseTreeNode>& nodes,
        int condIdx,
        const std::string& C_true,
        const std::string& C_false) {
    using sz = std::size_t;
    auto& cond  = nodes[sz(condIdx)];
    auto& Expr1 = nodes[sz(cond.children[0])];
    auto& RelOp = nodes[sz(cond.children[1])];
    auto& Expr2 = nodes[sz(cond.children[2])];

    std::vector<std::string> C_code;
    for (auto& i : Expr1.code) C_code.push_back(i);
    for (auto& i : Expr2.code) C_code.push_back(i);
    C_code.push_back(
        "if " + Expr1.place + " " + RelOp.place + " " + Expr2.place + " goto " + C_true);
    C_code.push_back("goto " + C_false);
    return C_code;
}

// ============================================================
// postProcess —— 两阶段后处理，消除标签冗余与跳转链
// ============================================================
void SemanticAnalyzer::postProcess() {
    // 阶段一：合并相邻标签对
    bool changed = true;
    while (changed) {
        changed = false;
        for (std::size_t i = 0; i + 1 < code_.size(); ++i) {
            if (isLabelDef(code_[i]) && isLabelDef(code_[i + 1])) {
                replaceAllRefs(code_, labelName(code_[i]), labelName(code_[i + 1]));
                code_.erase(code_.begin() + static_cast<std::ptrdiff_t>(i));
                changed = true;
                break;
            }
        }
    }

    // 阶段二：消除跳转链（Lx: goto Ly）
    changed = true;
    while (changed) {
        changed = false;
        for (std::size_t i = 0; i + 1 < code_.size(); ++i) {
            if (isLabelDef(code_[i]) && isUnconditionalGoto(code_[i + 1])) {
                const std::string from = labelName(code_[i]);
                const std::string to   = gotoTarget(code_[i + 1]);
                if (from != to) {
                    replaceAllRefs(code_, from, to);
                    code_.erase(code_.begin() + static_cast<std::ptrdiff_t>(i));
                    changed = true;
                    break;
                }
            }
        }
    }
}