const {
  Document, Packer, Paragraph, TextRun, Table, TableRow, TableCell,
  AlignmentType, HeadingLevel, BorderStyle, WidthType, ShadingType,
  LevelFormat, VerticalAlign,
} = require('docx');
const fs = require('fs');

const HEADER_FILL  = '2E4057';
const ROW_ALT      = 'EEF4FB';
const BORDER_COLOR = 'AAAAAA';
const CODE_BG      = 'F4F4F4';

function bdr() {
  const b = { style: BorderStyle.SINGLE, size: 4, color: BORDER_COLOR };
  return { top: b, bottom: b, left: b, right: b };
}

// ── 正文段落 ─────────────────────────────────────────────────────
function p(text, bold = false) {
  return new Paragraph({
    alignment: AlignmentType.JUSTIFIED,
    spacing: { before: 80, after: 80, line: 320 },
    children: [new TextRun({ text, font: '宋体', size: 24, bold })],
  });
}

// ── 代码行 ───────────────────────────────────────────────────────
function codeLine(text) {
  return new Paragraph({
    spacing: { before: 30, after: 30 },
    shading: { fill: CODE_BG, type: ShadingType.CLEAR },
    indent: { left: 480 },
    children: [new TextRun({ text, font: 'Courier New', size: 20, color: '1A1A1A' })],
  });
}

// ── 标题 ─────────────────────────────────────────────────────────
function h1(text) {
  return new Paragraph({
    heading: HeadingLevel.HEADING_1,
    spacing: { before: 360, after: 160 },
    children: [new TextRun({ text, font: '黑体', size: 32, bold: true, color: '1B3A6B' })],
  });
}
function h2(text) {
  return new Paragraph({
    heading: HeadingLevel.HEADING_2,
    spacing: { before: 220, after: 100 },
    children: [new TextRun({ text, font: '黑体', size: 28, bold: true, color: '2E5FA3' })],
  });
}

// ── 项目符号 ─────────────────────────────────────────────────────
function bullet(text) {
  return new Paragraph({
    numbering: { reference: 'bullets', level: 0 },
    spacing: { before: 60, after: 60, line: 300 },
    children: [new TextRun({ text, font: '宋体', size: 24 })],
  });
}

// ── 空行 ─────────────────────────────────────────────────────────
function blank() {
  return new Paragraph({ spacing: { before: 60, after: 60 }, children: [] });
}

// ── 表格辅助 ─────────────────────────────────────────────────────
function hCell(text, w) {
  return new TableCell({
    borders: bdr(), width: { size: w, type: WidthType.DXA },
    shading: { fill: HEADER_FILL, type: ShadingType.CLEAR },
    margins: { top: 80, bottom: 80, left: 120, right: 120 },
    verticalAlign: VerticalAlign.CENTER,
    children: [new Paragraph({
      alignment: AlignmentType.CENTER,
      children: [new TextRun({ text, font: '黑体', size: 22, bold: true, color: 'FFFFFF' })],
    })],
  });
}
function dCell(text, w, alt = false, mono = false) {
  return new TableCell({
    borders: bdr(), width: { size: w, type: WidthType.DXA },
    shading: { fill: alt ? ROW_ALT : 'FFFFFF', type: ShadingType.CLEAR },
    margins: { top: 60, bottom: 60, left: 120, right: 120 },
    verticalAlign: VerticalAlign.CENTER,
    children: [new Paragraph({
      children: [new TextRun({
        text, color: '1A1A1A',
        font: mono ? 'Courier New' : '宋体',
        size: mono ? 20 : 22,
      })],
    })],
  });
}

// ── SDD 表构建 ────────────────────────────────────────────────────
const C = [1300, 3100, 4800];   // 列宽：编号 / 产生式 / 语义规则
function sddTable(rows) {
  return new Table({
    width: { size: C[0]+C[1]+C[2], type: WidthType.DXA },
    columnWidths: C,
    rows: [
      new TableRow({ children: [hCell('编号', C[0]), hCell('产生式', C[1]), hCell('语义规则', C[2])] }),
      ...rows.map(([id, prod, rule], i) => new TableRow({ children: [
        dCell(id,   C[0], i%2===1, false),
        dCell(prod, C[1], i%2===1, true),
        dCell(rule, C[2], i%2===1, false),
      ]})),
    ],
  });
}

// ── 架构总览表 ────────────────────────────────────────────────────
const FC = [2000, 2600, 4600];
function archTable(rows) {
  return new Table({
    width: { size: FC[0]+FC[1]+FC[2], type: WidthType.DXA },
    columnWidths: FC,
    rows: [
      new TableRow({ children: [hCell('文件', FC[0]), hCell('角色', FC[1]), hCell('主要职责', FC[2])] }),
      ...rows.map(([f, r, d], i) => new TableRow({ children: [
        dCell(f, FC[0], i%2===1, true),
        dCell(r, FC[1], i%2===1, false),
        dCell(d, FC[2], i%2===1, false),
      ]})),
    ],
  });
}

// ════════════════════════════════════════════════════════════════
// 文档内容
// ════════════════════════════════════════════════════════════════
const doc = new Document({
  numbering: {
    config: [{
      reference: 'bullets',
      levels: [{ level: 0, format: LevelFormat.BULLET, text: '●',
        alignment: AlignmentType.LEFT,
        style: { paragraph: { indent: { left: 480, hanging: 240 } } } }],
    }],
  },
  styles: { default: { document: { run: { font: '宋体', size: 24 } } } },
  sections: [{
    properties: {
      page: {
        size: { width: 11906, height: 16838 },
        margin: { top: 1440, right: 1200, bottom: 1440, left: 1440 },
      },
    },
    children: [

      // ── 封面 ────────────────────────────────────────────────────
      blank(), blank(), blank(),
      new Paragraph({
        alignment: AlignmentType.CENTER, spacing: { before: 600, after: 240 },
        children: [new TextRun({ text: '编译原理实验报告', font: '黑体', size: 56, bold: true, color: '1B3A6B' })],
      }),
      new Paragraph({
        alignment: AlignmentType.CENTER, spacing: { before: 120, after: 600 },
        children: [new TextRun({ text: '实验三  语义分析与三地址代码生成', font: '黑体', size: 36, color: '2E5FA3' })],
      }),
      blank(), blank(),
      new Paragraph({
        alignment: AlignmentType.CENTER, spacing: { before: 80, after: 80 },
        children: [new TextRun({ text: '2026 年春季学期', font: '宋体', size: 28, color: '555555' })],
      }),
      blank(), blank(), blank(), blank(),

      // ── 一、实验目的 ─────────────────────────────────────────────
      h1('一、实验目的'),
      p('本实验在实验二 SLR 语法分析器的基础上，为每次规约动作配套相应的语义规则，实现基于语法制导翻译的三地址代码生成器。具体目标包括：'),
      bullet('在 SLR 语法分析器的每次规约时调用对应产生式的语义规则；'),
      bullet('为语法树每个节点分配综合属性 .place（变量名或临时变量名）和 .code（该子树生成的三地址指令序列）；'),
      bullet('正确生成赋值语句、算术表达式、条件判断及控制流语句（if-then、if-then-else、while-do、begin-end）的三地址代码；'),
      bullet('构建符号表，规约 Factor -> IDN 时自动登记标识符；'),
      bullet('正确处理八进制与十六进制字面量（统一转换为十进制字符串）。'),
      blank(),

      // ── 二、实验内容与方法 ───────────────────────────────────────
      h1('二、实验内容与方法'),

      h2('2.1  总体架构'),
      p('本实验在实验二工程基础上新增以下模块，并对已有模块进行最小化改动：'),
      blank(),
      archTable([
        ['ParseTreeNode.h',       '新建（公共结构）',   '为语法树节点新增 value、place、code 三个语义属性'],
        ['SymbolTable.h/.cpp',    '新建（符号表）',     '实现 enter() / lookup()，去重登记标识符'],
        ['SemanticAnalyzer.h/.cpp','新建（语义核心）',  '持有符号表与代码列表，通过 applyRule() 按产生式编号分派规则'],
        ['Parser.h / Parser.cpp', '修改（最小化）',     '引入 SemanticAnalyzer 成员；规约后调用 applyRule()；createLeafNode 填充 node.value'],
        ['ParserMain.cpp',        '修改（输出扩展）',   '在语法树输出后追加三地址代码和符号表'],
        ['CMakeLists.txt',        '修改（构建）',       '将 SymbolTable.cpp、SemanticAnalyzer.cpp 加入 parser_lib'],
      ]),
      blank(),

      h2('2.2  语义规则接入点'),
      p('语义分析器通过如下两行代码接入 SLR 分析循环，满足"每次规约时正确调用语义规则"的要求：'),
      blank(),
      codeLine('const int parentIndex = createParentNode(production.lhs, children);'),
      codeLine('semanticAnalyzer_.applyRule(production.id, parseTreeNodes_, parentIndex, children);'),
      blank(),
      p('调用时机为：语法分析器完成一次规约、建立父节点之后、执行 GOTO 转换之前。此时所有子节点的 place 和 code 属性已由更早的规约填好，完全符合综合属性自底向上的计算顺序。'),
      blank(),

      h2('2.3  符号表设计'),
      p('符号表采用线性顺序表（std::vector<SymbolEntry>），每条记录含名称（name）、类型（type，暂为 "unknown"）和偏移（offset，暂为 -1）三个字段。每次规约 Factor -> IDN 时调用 enter() 登记变量名，重复名称静默忽略。'),
      blank(),

      // ── 三、三地址代码生成器 ─────────────────────────────────────
      h1('三、三地址代码生成器'),

      h2('3.1  语法制导定义'),
      p('本实验采用综合属性文法（S-attributed SDD）。每个非终结符节点携带两个综合属性：'),
      bullet('.place：节点所代表的值的载体——变量名（IDN）、十进制字面量字符串（DEC/OCT/HEX）或新生临时变量名（t1, t2, ...）；'),
      bullet('.code：该子树生成的三地址指令序列（std::vector<string>），自底向上积累，在控制流节点处插入标签与跳转指令。'),
      blank(),
      p('各产生式对应的语义规则如下表所示（newtemp() 生成新临时变量，newlabel() 生成新标号，|| 表示列表拼接）。'),
      blank(),

      p('Factor 产生式（规则 25-29）', true),
      blank(),
      sddTable([
        ['25', 'Factor -> IDN',      'Factor.place = IDN.lexeme；symTable.enter(IDN.lexeme)'],
        ['26', 'Factor -> DEC',      'Factor.place = DEC.value'],
        ['27', 'Factor -> OCT',      'Factor.place = OCT.value（词法器已转十进制，如 07 -> 7）'],
        ['28', 'Factor -> HEX',      'Factor.place = HEX.value（词法器已转十进制，如 0xa -> 10）'],
        ['29', 'Factor -> ( Expr )', 'Factor.place = Expr.place；Factor.code = Expr.code'],
      ]),
      blank(),

      p('关系运算符产生式（规则 11-16）', true),
      blank(),
      sddTable([
        ['11', 'RelOp -> GT',  'RelOp.place = ">"'],
        ['12', 'RelOp -> LT',  'RelOp.place = "<"'],
        ['13', 'RelOp -> EQ',  'RelOp.place = "="'],
        ['14', 'RelOp -> GE',  'RelOp.place = ">="'],
        ['15', 'RelOp -> LE',  'RelOp.place = "<="'],
        ['16', 'RelOp -> NEQ', 'RelOp.place = "<>"'],
      ]),
      blank(),

      p('算术表达式产生式（规则 17-24）', true),
      blank(),
      sddTable([
        ['20', 'ExprRest -> e',                  '（无操作，代码由规则 17 遍历链统一生成）'],
        ['24', 'TermRest -> e',                  '（无操作，代码由规则 21 遍历链统一生成）'],
        ['18', 'ExprRest -> ADD Term ExprRest',  '（无操作，同上）'],
        ['19', 'ExprRest -> SUB Term ExprRest',  '（无操作，同上）'],
        ['22', 'TermRest -> MUL Factor TermRest','（无操作，同上）'],
        ['23', 'TermRest -> DIV Factor TermRest','（无操作，同上）'],
        ['17', 'Expr -> Term ExprRest',
               '遍历 ExprRest 链：每步 t=newtemp()，追加 t := result op Term.place；Expr.place = 最终 t'],
        ['21', 'Term -> Factor TermRest',
               '遍历 TermRest 链：每步 t=newtemp()，追加 t := result op Factor.place；Term.place = 最终 t'],
      ]),
      blank(),

      p('条件与赋值产生式（规则 9-10）', true),
      blank(),
      sddTable([
        ['10', 'Cond -> Expr1 RelOp Expr2',
               'Cond.code = Expr1.code || Expr2.code；Cond.place = Expr1.place + " " + RelOp.place + " " + Expr2.place'],
        ['9',  'Assign -> IDN = Expr',
               'Assign.code = Expr.code || [ IDN.lexeme + " = " + Expr.place ]'],
      ]),
      blank(),

      p('控制流产生式（规则 1-8）', true),
      blank(),
      sddTable([
        ['3', 'Stmts -> e',
              'Stmts.code = []'],
        ['2', 'Stmts -> Stmt ; Stmts',
              'Stmts.code = Stmt.code || Stmts.code'],
        ['4', 'Stmt -> Assign',
              'Stmt.code = Assign.code'],
        ['8', 'Stmt -> BEGIN Stmts END',
              'Stmt.code = Stmts.code'],
        ['5', 'Stmt -> IF Cond THEN Stmt',
              'Lt=newlabel()，La=newlabel()；code = Cond.code || [if Cond.place goto Lt, goto La, Lt:] || Stmt.code || [La:]'],
        ['6', 'Stmt -> IF Cond THEN Stmt ELSE Stmt',
              'Lt/Lf/La=newlabel()；code = Cond.code || [if...goto Lt, goto Lf, Lt:] || Stmtt.code || [goto La, Lf:] || Stmte.code || [La:]'],
        ['7', 'Stmt -> WHILE Cond DO Stmt',
              'Ls/Lb/La=newlabel()；code = [Ls:] || Cond.code || [if...goto Lb, goto La, Lb:] || Stmt.code || [goto Ls, La:]'],
        ['1', 'P -> Stmts',
              'code_（全局）= Stmts.code（唯一写全局列表的时机）'],
      ]),
      blank(),

      // ── 3.2 算法基本思想 ─────────────────────────────────────────
      h2('3.2  算法基本思想'),
      p('本实验的代码生成算法建立在以下五个核心设计决策之上。'),
      blank(),

      p('（1）node.code 积累模式，而非全局直接写出', true),
      p('若对全局代码列表直接 gen()，内层语句的代码会在外层控制结构规约之前已被写出，导致无法在其前方插入循环起始标签。本实现让每个节点携带自己子树的完整代码片段（node.code），父节点在规约时自由拼接，可任意在子代码前后插入标签和跳转指令。最终仅由根规则 P -> Stmts 将顶层代码一次性写入全局列表，作为唯一的提交出口。'),
      blank(),

      p('（2）算术运算的左结合保证', true),
      p('本实验文法通过引入 ExprRest / TermRest 消除了左递归，但若在 ExprRest -> ADD Term ExprRest 规约时逐步生成代码，由于 SLR 从最深处往上规约，会先处理右侧的 ExprRest，产生右结合的错误。解决方案是：ExprRest 和 TermRest 的规约规则均不生成代码；代码生成统一在根节点 Expr（规则 17）和 Term（规则 21）规约时，通过遍历整条链从左到右依次生成临时变量，保证左结合。'),
      blank(),

      p('（3）控制流代码的包裹生成', true),
      p('对于 while-do，循环起始标签必须位于条件表达式计算代码之前，以确保每次迭代重新计算条件。由于 Cond.code 和 Stmt.code 在规约 WHILE 时均已完整就绪，只需按如下顺序拼接：'),
      blank(),
      codeLine('Ls:'),
      codeLine('<Cond.code>          // 每次迭代都重算条件表达式'),
      codeLine('if <Cond.place> goto Lb'),
      codeLine('goto La'),
      codeLine('Lb:'),
      codeLine('<Stmt.code>'),
      codeLine('goto Ls'),
      codeLine('La:'),
      blank(),
      p('对于 if-then-else，类似地在真假分支代码周围插入标签与 goto，完成完整的分支跳转结构。'),
      blank(),

      p('（4）条件表达式的 place 编码', true),
      p('Cond 节点不直接生成跳转指令（因为它不知道目标标号），而是将完整条件字符串编码进 place，例如 "t1 > 10"。上层的 IF / WHILE 节点持有 Cond.place，拼接 "if " + Cond.place + " goto " + L 即可生成条件跳转，实现了条件信息的向上传递，无需继承属性。'),
      blank(),

      p('（5）临时变量与标号的管理', true),
      p('newtemp() 返回 t1, t2, t3, ... 形式的新临时变量名，newlabel() 返回 L1, L2, L3, ... 形式的新标号。两者均在 SemanticAnalyzer 内维护全局单调递增计数器，保证整个程序范围内无重名。由于 SLR 自底向上规约顺序是确定的，临时变量和标号的编号顺序也是确定的，输出具有可重现性。'),
      blank(),

      // ── 四、实验结果 ─────────────────────────────────────────────
      h1('四、实验结果'),

      h2('4.1  测试用例'),
      p('使用实验指导书给定的官方测试用例：'),
      blank(),
      codeLine('while (a3+15)>0xa do if x2 = 07 then while y<z do y = x * y / z; c=b*c+d;'),
      blank(),
      p('该用例覆盖了所有主要特性：嵌套 while、if-then（含 dangling-else 结构）、算术混合运算（含括号、乘法优先级、左结合）、八进制与十六进制字面量、赋值语句、多语句序列。'),
      blank(),

      h2('4.2  生成的三地址代码'),
      blank(),
      codeLine('L6:'),
      codeLine('t1 := a3 + 15'),
      codeLine('if t1 > 10 goto L7'),
      codeLine('goto L8'),
      codeLine('L7:'),
      codeLine('if x2 = 7 goto L4'),
      codeLine('goto L5'),
      codeLine('L4:'),
      codeLine('L1:'),
      codeLine('if y < z goto L2'),
      codeLine('goto L3'),
      codeLine('L2:'),
      codeLine('t2 := x * y'),
      codeLine('t3 := t2 / z'),
      codeLine('y = t3'),
      codeLine('goto L1'),
      codeLine('L3:'),
      codeLine('L5:'),
      codeLine('goto L6'),
      codeLine('L8:'),
      codeLine('t4 := b * c'),
      codeLine('t5 := t4 + d'),
      codeLine('c = t5'),
      blank(),

      h2('4.3  符号表'),
      blank(),
      new Table({
        width: { size: 6000, type: WidthType.DXA },
        columnWidths: [2000, 2000, 2000],
        rows: [
          new TableRow({ children: [hCell('名称', 2000), hCell('类型', 2000), hCell('偏移', 2000)] }),
          ...['a3','x2','y','z','x','b','c','d'].map((name, i) => new TableRow({ children: [
            dCell(name,      2000, i%2===1, true),
            dCell('unknown', 2000, i%2===1, false),
            dCell('-1',      2000, i%2===1, false),
          ]})),
        ],
      }),
      blank(),

      h2('4.4  结果分析'),
      bullet('外层 while (a3+15)>0xa：L6 为循环入口，每次迭代执行 t1 := a3+15 重算条件，if t1 > 10 goto L7 进入循环体，goto L8 为出口。'),
      bullet('进制转换：OCT 字面量 07 的 place 为 "7"，HEX 字面量 0xa 的 place 为 "10"，均由词法分析器完成转换，语义规则直接使用 node.value 即可。'),
      bullet('括号表达式 (a3+15)：Factor -> (Expr) 透传 Expr 的 place 和 code，t1 正确传递至 Cond。'),
      bullet('if x2 = 07 then（无 else）：条件为假时 goto L5 直接跳过真分支，结构简洁无多余跳转。'),
      bullet('内层 while y<z：L1/L2/L3 结构正确，循环体 y = t3 之后 goto L1 回跳。'),
      bullet('x * y / z 的左结合：先 t2 := x * y，再 t3 := t2 / z，而非右结合的错误顺序。'),
      bullet('b*c+d 的优先级：先乘后加，t4 := b*c，t5 := t4+d，优先级正确。'),
      bullet('符号表：8 个标识符 a3、x2、y、z、x、b、c、d 全部登记，无重复，符合 enter() 去重逻辑。'),
      blank(),

      // ── 五、实验体会 ─────────────────────────────────────────────
      h1('五、实验体会'),
      p('本实验最大的收获是深刻理解了自底向上语法分析与语义规则计算顺序之间的内在一致性。SLR 分析器的规约顺序天然满足综合属性的计算依赖：规约一个产生式时，右部各子节点的属性早已由更深层的规约填好，因此只需在每次规约后调用一次 applyRule()，便完成了整棵子树的语义计算，不需要额外的树遍历。'),
      blank(),
      p('其次，控制流代码生成的难点在于标号的传递问题。经典教材中 if/while 语句的标号需要自顶向下传给子语句，属于继承属性，而 SLR 框架不支持继承属性。本实验通过 node.code 积累模式绕过了这一问题：每个节点持有自己子树的完整代码，控制流节点在拿到子节点代码后再包裹标签和跳转，完全在综合属性范围内解决，既正确又清晰。'),
      blank(),
      p('算术运算左结合问题也是一个值得关注的陷阱。文法为消除左递归引入了 ExprRest/TermRest，若按照"每次规约 ExprRest 时生成代码"的朴素方案，由于最深的 ExprRest 最先被规约，实际上会得到右结合的错误结果。将代码生成推迟到根节点 Expr/Term、遍历整条链后再依次生成指令，是解决这一问题的关键，也是本实验在属性文法设计上最具创新性的一点。'),
      blank(),
    ],
  }],
});

Packer.toBuffer(doc).then(buf => {
  fs.writeFileSync('D:\\compiler\\Compiler-Principles\\docs\\lab3_report.docx', buf);
  console.log('报告已生成：docs/lab3_report.docx');
});
