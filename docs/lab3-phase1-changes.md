# 实验三改动记录

**分支：** lab3-semantic

---

## 第一阶段（2026-05-13）：基础设施 + Factor 规则 25–29

**范围：** 搭建语义分析基础设施，实现 Factor 产生式规则 25–29

### 改动文件一览

| 操作 | 文件 | 说明 |
|------|------|------|
| 新建 | `include/ParseTreeNode.h` | ParseTreeNode 结构体，含语义属性 |
| 新建 | `include/SymbolTable.h` | SymbolEntry + SymbolTable 类声明 |
| 新建 | `src/SymbolTable.cpp` | enter() / lookup() 实现 |
| 新建 | `include/SemanticAnalyzer.h` | SemanticAnalyzer 类声明 |
| 新建 | `src/SemanticAnalyzer.cpp` | Factor 规则 25–29 实现 |
| 新建 | `tests/test_exp3.txt` | 实验三官方测试用例 |
| 修改 | `include/Parser.h` | 替换内联结构体，加入 SemanticAnalyzer 成员 |
| 修改 | `src/Parser.cpp` | 填充 node.value，规约后调用 applyRule() |
| 修改 | `src/ParserMain.cpp` | 新增符号表和三地址代码输出 |
| 修改 | `CMakeLists.txt` | parser_lib 加入两个新源文件 |

### 各文件详细说明

#### 新建 `include/ParseTreeNode.h`

从 `Parser.h` 中提取出 `ParseTreeNode` 结构体到独立头文件，并新增三个语义属性。

**改动前**（内联在 `Parser.h`）：
```cpp
struct ParseTreeNode {
    std::string symbol;
    std::string lexeme;
    std::vector<int> children;
};
```

**改动后**（独立文件）：
```cpp
struct ParseTreeNode {
    std::string symbol;
    std::string lexeme;
    std::string value;    // 新增：数字已转十进制，IDN 与 lexeme 相同
    std::vector<int> children;

    std::string place;               // 新增：E.place，该节点代表的变量名或数值
    std::vector<std::string> code;   // 新增：E.code，该子树生成的三地址指令序列
};
```

#### 新建 `include/SymbolTable.h` + `src/SymbolTable.cpp`

符号表，记录程序中出现的所有变量名。

```cpp
struct SymbolEntry {
    std::string name;    // 变量名
    std::string type;    // 类型，暂为 "unknown"
    int offset;          // 偏移，暂为 -1
};

void enter(name);          // 登记变量，重复忽略
SymbolEntry* lookup(name); // 查找变量，未找到返回 nullptr
```

#### 新建 `include/SemanticAnalyzer.h` + `src/SemanticAnalyzer.cpp`

语义分析器核心，持有符号表和三地址代码列表，通过 `applyRule()` 按产生式编号分派规则。

**Factor 规则 25–29：**

| case | 产生式 | 语义动作 |
|------|--------|----------|
| 25 | `Factor → IDN` | `place = id.lexeme`；调用 `symTable_.enter()` 登记变量 |
| 26 | `Factor → DEC` | `place = node.value` |
| 27 | `Factor → OCT` | `place = node.value`（词法器已转十进制，如 `"07"` → `"7"`） |
| 28 | `Factor → HEX` | `place = node.value`（词法器已转十进制，如 `"0xa"` → `"10"`） |
| 29 | `Factor → SLP Expr SRP` | `place = Expr.place`；`code = Expr.code`（向上透传） |

#### 修改 `src/Parser.cpp`

**改动一：`createLeafNode` 填充 value**
```cpp
node.value = token.value;  // 新增：保存词法器已处理好的值
```

**改动二：规约分支接入语义规则（核心）**
```cpp
const int parentIndex = createParentNode(production.lhs, children);
semanticAnalyzer_.applyRule(production.id, parseTreeNodes_, parentIndex, children); // 新增
```

### 验证结果（第一阶段）

编译：10/10 通过，零错误零警告。

符号表输出正确，进制转换验证：
- `OCT "07"` → `place = "7"` ✓
- `HEX "0xa"` → `place = "10"` ✓
- 三地址代码为空（算术规则尚未实现）✓

---

## 第二阶段（2026-05-13）：算术规则 17–24 + 赋值规则 9

**范围：** 仅修改 `src/SemanticAnalyzer.cpp`，在 switch 中补充以下规则。

### 实现思路

文法消除了左递归，用 `Expr → Term ExprRest` 和 `Term → Factor TermRest` 替代直接左递归，但这带来一个问题：

**原始教材方案（自底向上传递 place）会产生右结合**。例如 `x * y / z`，如果在规则 22/23 各自生成临时变量，由于 SLR 是先规约最深的 TermRest，会先算 `y / z` 再乘 `x`，结果错误。

**本实现方案：在根节点（规则 17/21）遍历整条链**，从左向右逐步生成指令，保证左结合性。ExprRest/TermRest 的递归规则（18/19/22/23）不生成代码，仅让规约正常发生。

### 新增 case 说明

| case | 产生式 | 实现方式 |
|------|--------|----------|
| 20 | `ExprRest → ε` | 无操作（节点无子节点，由规则 17 处理） |
| 24 | `TermRest → ε` | 无操作（节点无子节点，由规则 21 处理） |
| 18 | `ExprRest → ADD Term ExprRest` | 无操作（代码生成由规则 17 遍历链完成） |
| 19 | `ExprRest → SUB Term ExprRest` | 无操作（同上） |
| 22 | `TermRest → MUL Factor TermRest` | 无操作（代码生成由规则 21 遍历链完成） |
| 23 | `TermRest → DIV Factor TermRest` | 无操作（同上） |
| 17 | `Expr → Term ExprRest` | 遍历 ExprRest 链，从左到右生成加减指令 |
| 21 | `Term → Factor TermRest` | 遍历 TermRest 链，从左到右生成乘除指令 |
| 9  | `Assign → IDN EQ Expr` | 生成 `IDN = Expr.place` |

### 规则 17/21 的遍历逻辑（以规则 21 为例）

```
输入：x * y / z

语法树链结构：
  Term
    Factor(x)
    TermRest
      MUL
      Factor(y)
      TermRest
        DIV
        Factor(z)
        TermRest → ε

遍历过程：
  result = "x"
  第一轮：op="*", factor="y" → gen("t1 := x * y"), result="t1"
  第二轮：op="/", factor="z" → gen("t2 := t1 / z"), result="t2"
  Term.place = "t2"
```

### 验证结果（第二阶段）

测试用例：
```
while (a3+15)>0xa do if x2 = 07 then while y<z do y = x * y / z; c=b*c+d;
```

生成的三地址代码：
```
t1 := a3 + 15
t2 := x * y
t3 := t2 / z
y = t3
t4 := b * c
t5 := t4 + d
c = t5
```

验证：
- `(a3+15)` → `t1 := a3 + 15`，括号透传给外层 Cond ✓
- `x * y / z` → 先 `t2 := x * y`，再 `t3 := t2 / z`（左结合）✓
- `b * c + d` → 先 `t4 := b * c`（乘法优先），再 `t5 := t4 + d` ✓
- 赋值 `y = t3`、`c = t5` ✓

---

## 第三阶段（2026-05-13）：控制流规则 5–8, 10–16

**范围：** 修改 `include/SemanticAnalyzer.h` 和 `src/SemanticAnalyzer.cpp`，实现剩余全部规则。

### 关键设计决策：node.code 积累模式

原方案使用全局 `gen()` 推代码，无法实现控制流（因为内层代码已经提前 emit，外层无法在它前面插标签）。

**解决方案：全部规则改为 node.code 积累，只有 `P → Stmts`（规则1）才把 Stmts.code 写入全局 `code_`。**

每个节点持有自己子树的完整代码片段，父节点在规约时自由拼接，可以在子节点代码前后插入标签和跳转指令。

### 修改 `include/SemanticAnalyzer.h`

新增 `labelCount_` 成员和 `newlabel()` 声明：

```cpp
private:
    int tempCount_  = 0;
    int labelCount_ = 0;   // 新增
    ...
    std::string newlabel(); // 新增：生成 L1, L2, L3, ...
```

### 修改 `src/SemanticAnalyzer.cpp`

**全部规则一览：**

| case | 产生式 | 实现方式 |
|------|--------|----------|
| 1 | `P → Stmts` | `code_ = Stmts.code`（唯一写全局列表的地方） |
| 2 | `Stmts → Stmt SEMI Stmts` | `parent.code = Stmt.code + Stmts.code` |
| 3 | `Stmts → ε` | 无操作 |
| 4 | `Stmt → Assign` | 透传 Assign.code |
| 5 | `Stmt → IF Cond THEN Stmt` | 见下 |
| 6 | `Stmt → IF Cond THEN Stmt ELSE Stmt` | 见下 |
| 7 | `Stmt → WHILE Cond DO Stmt` | 见下 |
| 8 | `Stmt → BEGIN Stmts END` | 透传 Stmts.code |
| 9 | `Assign → IDN EQ Expr` | `Expr.code + "IDN = Expr.place"` |
| 10 | `Cond → Expr RelOp Expr` | `code=Expr1.code+Expr2.code`；`place="e1 op e2"` |
| 11–16 | `RelOp → GT/LT/EQ/GE/LE/NEQ` | `place = ">/<=/=/>=/<=/<>"` |
| 17 | `Expr → Term ExprRest` | 遍历链生成加减指令（同第二阶段，改为 node.code） |
| 18–20 | ExprRest 各规则 | 无操作 |
| 21 | `Term → Factor TermRest` | 遍历链生成乘除指令（同第二阶段，改为 node.code） |
| 22–24 | TermRest 各规则 | 无操作 |
| 25–29 | Factor 各规则 | 同第一阶段 |

**规则 5（IF-THEN）代码结构：**
```
<cond 计算代码>
if <cond.place> goto L_true
goto L_after
L_true:
<stmt 代码>
L_after:
```

**规则 6（IF-THEN-ELSE）代码结构：**
```
<cond 计算代码>
if <cond.place> goto L_true
goto L_false
L_true:
<then 代码>
goto L_after
L_false:
<else 代码>
L_after:
```

**规则 7（WHILE-DO）代码结构：**
```
L_start:
<cond 计算代码>        ← 每次循环重新计算条件表达式
if <cond.place> goto L_body
goto L_after
L_body:
<stmt 代码>
goto L_start
L_after:
```

**规则 10（Cond）：** place 存完整条件字符串，供 IF/WHILE 拼接跳转指令使用：
```cpp
parent.place = Expr1.place + " " + RelOp.place + " " + Expr2.place;
// 例："t1 > 10"，上层生成 "if t1 > 10 goto L"
```

### 验证结果（第三阶段）

测试用例：
```
while (a3+15)>0xa do if x2 = 07 then while y<z do y = x * y / z; c=b*c+d;
```

生成的三地址代码：
```
L6:
t1 := a3 + 15
if t1 > 10 goto L7
goto L8
L7:
if x2 = 7 goto L4
goto L5
L4:
L1:
if y < z goto L2
goto L3
L2:
t2 := x * y
t3 := t2 / z
y = t3
goto L1
L3:
L5:
goto L6
L8:
t4 := b * c
t5 := t4 + d
c = t5
```

验证：
- 外层 while：`L6` 开始，`L7` 循环体，`L8` 出口，`goto L6` 回跳 ✓
- 条件计算 `t1 := a3 + 15` 在 `L6:` 之后（每次循环重算）✓
- if-then：`if x2 = 7 goto L4`，`L5` 为跳过真分支后的汇合点 ✓
- 内层 while：`L1/L2/L3` 结构正确 ✓
- `x * y / z` 左结合：先 `t2 := x * y`，再 `t3 := t2 / z` ✓
- `b * c + d` 乘法优先：先 `t4 := b * c`，再 `t5 := t4 + d` ✓
- 赋值 `y = t3`，`c = t5` ✓

---

## 全部规则实现完毕

目前已实现规则 1–29，控制流 5–8 和条件规则 10–16 全部完成。
