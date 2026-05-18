#set page(
  paper: "presentation-16-9",
  margin: (x: 1.0cm, y: 0.76cm),
)

#set text(font: ("Microsoft YaHei", "SimHei"), size: 17pt, fill: rgb("#1B2430"))
#set par(leading: 0.72em)

#let navy = rgb("#17324D")
#let blue = rgb("#2F64B3")
#let teal = rgb("#157F6B")
#let gold = rgb("#C48A1D")
#let muted = rgb("#607080")
#let border = rgb("#D8E1EB")
#let pale-blue = rgb("#EEF4FB")
#let pale-green = rgb("#EEF8F4")
#let pale-gold = rgb("#FFF8EA")
#let pale-red = rgb("#FFF4F4")
#let white = rgb("#FFFFFF")
#let mono = ("Consolas", "Courier New")

#let title-bar(title) = [
  #text(25pt, weight: "bold", fill: navy)[#title]
  #v(6pt)
  #line(length: 100%, stroke: (paint: border, thickness: 1pt))
]

#let slide(title, body) = [
  #title-bar(title)
  #v(14pt)
  #body
]

#let card(title, body, fill: white, stroke-color: border, inset-size: 13pt) = rect(
  width: 100%,
  fill: fill,
  inset: inset-size,
  radius: 11pt,
  stroke: (paint: stroke-color, thickness: 1pt),
)[
  #text(12.3pt, weight: "bold", fill: navy)[#title]
  #v(5pt)
  #body
]

#let bullet(body) = [
  #grid(
    columns: (14pt, 1fr),
    gutter: 4pt,
    [#text(12pt, fill: blue)[●]],
    [#body],
  )
  #v(5pt)
]

#let stat(title, value, detail, fill: white, stroke-color: border) = card(
  title,
  [
    #text(27pt, weight: "bold", fill: navy)[#value]
    #v(6pt)
    #text(13.1pt, fill: muted)[#detail]
  ],
  fill: fill,
  stroke-color: stroke-color,
)

#let chip(text-body, fill: pale-blue, stroke-color: blue) = rect(
  inset: (x: 12pt, y: 5pt),
  radius: 999pt,
  fill: fill,
  stroke: (paint: stroke-color, thickness: 1pt),
)[#text(12pt, weight: "bold", fill: stroke-color)[#text-body]]

#let code-block(code, size: 11.2pt) = rect(
  width: 100%,
  fill: rgb("#F7F9FC"),
  inset: 10pt,
  radius: 9pt,
  stroke: (paint: border, thickness: 1pt),
)[
  #set text(font: mono, size: size, fill: rgb("#1B2430"))
  #raw(block: true, lang: "text", code)
]

#let two-col(left, right, widths: (1fr, 1fr), gutter: 16pt) = table(
  columns: widths,
  gutter: gutter,
  inset: 0pt,
  stroke: none,
  [#left],
  [#right],
)

#let three-col(a, b, c, gutter: 12pt) = table(
  columns: 3,
  gutter: gutter,
  inset: 0pt,
  stroke: none,
  [#a],
  [#b],
  [#c],
)

#let step(title, detail, fill: white, stroke-color: border) = rect(
  width: 100%,
  fill: fill,
  inset: 9pt,
  radius: 10pt,
  stroke: (paint: stroke-color, thickness: 1pt),
)[
  #align(center)[
    #text(14.2pt, weight: "bold", fill: navy)[#title]
    #v(4pt)
    #text(10.6pt, fill: muted)[#detail]
  ]
]

#let small-note(text-body) = [
  #v(7pt)
  #text(11.2pt, fill: muted)[#text-body]
]

#let result-shot = "assets/educoder_95_95_crop.png"

// 1. 封面
#rect(
  width: 100%,
  height: 100%,
  fill: navy,
  radius: 18pt,
  inset: 24pt,
)[
  #text(11.5pt, weight: "bold", fill: rgb("#B7C9DC"), tracking: 0.6pt)[编译原理与技术课程设计 · 期末验收答辩]
  #v(18pt)
  #text(31pt, weight: "bold", fill: white)[Pascal-S 到 C 源到源编译器]
  #v(9pt)
  #text(17pt, fill: rgb("#DEE7F1"))[
    以 AST 为中间表示，完成词法分析、语法分析、语义检查与 C11 代码生成
  ]
  #v(20pt)
  #three-col(
    [#chip([完整流水线], fill: rgb("#183A57"), stroke-color: rgb("#7FA8E2"))],
    [#chip([record + repeat], fill: rgb("#183A57"), stroke-color: rgb("#78C9BA"))],
    [#chip([错误定位], fill: rgb("#183A57"), stroke-color: rgb("#E0B15C"))],
  )
  #v(30pt)
  #two-col(
    [
      #text(13.2pt, fill: rgb("#C7D6E6"))[课程：编译原理与技术课程设计]
      #v(6pt)
      #text(13.2pt, fill: rgb("#C7D6E6"))[指导教师：王吴凡]
      #v(6pt)
      #text(13.2pt, fill: rgb("#C7D6E6"))[班级：304]
      #v(6pt)
      #text(13.2pt, fill: rgb("#C7D6E6"))[组长：郝政伟]
    ],
    [
      #align(right)[
        #card(
          [平台结果],
          [
            #text(35pt, weight: "bold", fill: white)[95 / 95]
            #v(4pt)
            #text(13pt, fill: rgb("#DDE7F1"))[Educoder 平台全部通过]
          ],
          fill: rgb("#183A57"),
          stroke-color: rgb("#4F749A"),
          inset-size: 15pt,
        )
      ]
    ],
    widths: (1.25fr, 0.88fr),
  )
]

#pagebreak()

// 2. 成果总览
#slide(
  [1. 项目成果总览],
  [
    #table(
      columns: 2,
      rows: 2,
      gutter: 14pt,
      inset: 0pt,
      stroke: none,
      [
        #stat(
          [编译主线],
          [Pascal-S → C],
          [实现从 Pascal-S 源程序到 C11 源程序的完整翻译],
          fill: pale-blue,
          stroke-color: blue,
        )
      ],
      [
        #stat(
          [平台结果],
          [95 / 95],
          [Educoder 公开与隐藏测试全部通过],
          fill: pale-green,
          stroke-color: teal,
        )
      ],
      [
        #stat(
          [扩展功能],
          [record + repeat],
          [支持 record、多级字段访问，以及 `repeat ... until` 语句扩展],
          fill: pale-gold,
          stroke-color: gold,
        )
      ],
      [
        #stat(
          [工程验证],
          [16 / 16 · 9 / 9],
          [自建测试通过；生成 C11 代码通过严格编译检查],
          fill: pale-blue,
          stroke-color: blue,
        )
      ],
    )
  ],
)

#pagebreak()

// 3. 架构
#slide(
  [2. 整体架构：以 AST 为中间表示的四阶段流水线],
  [
    #table(
      columns: (0.82fr, 0.18fr, 1fr, 0.18fr, 1fr, 0.18fr, 1.05fr, 0.18fr, 0.92fr),
      gutter: 4pt,
      inset: 0pt,
      stroke: none,
      [#step([`.pas`], [输入源程序])],
      [#align(center + horizon)[#text(20pt, fill: blue)[→]]],
      [#step([词法分析], [`lexer.l` → token + line/column], fill: pale-blue, stroke-color: blue)],
      [#align(center + horizon)[#text(20pt, fill: blue)[→]]],
      [#step([语法分析], [`parser.y` → AST], fill: pale-blue, stroke-color: blue)],
      [#align(center + horizon)[#text(20pt, fill: blue)[→]]],
      [#step([语义分析], [符号表 + 类型检查], fill: pale-green, stroke-color: teal)],
      [#align(center + horizon)[#text(20pt, fill: blue)[→]]],
      [#step([代码生成], [输出 `.c`], fill: pale-gold, stroke-color: gold)],
    )
    #v(18pt)
    #three-col(
      [
        #card(
          [前端],
          [
            #bullet([lexer 把字符流切分成 token，并同步记录行列号。])
            #bullet([parser 按 Pascal-S 文法归约并构建 AST。])
          ],
          fill: pale-blue,
          stroke-color: blue,
        )
      ],
      [
        #card(
          [中端],
          [
            #bullet([semantic 独立遍历 AST，完成声明绑定、作用域管理和类型检查。])
            #bullet([错误若已发生，则停止进入代码生成。])
          ],
          fill: pale-green,
          stroke-color: teal,
        )
      ],
      [
        #card(
          [后端],
          [
            #bullet([codegen 在语义正确后生成 C11 代码。])
            #bullet([Pascal-S 与 C 的差异通过统一映射规则处理。])
          ],
          fill: pale-gold,
          stroke-color: gold,
        )
      ],
    )
    #small-note([代码位置：`src/main.cpp`、`src/lexer.l`、`src/parser.y`、`src/semantic_analyzer.*`、`src/codegen.*`])
  ],
)

#pagebreak()

// 4. AST and semantic split
#slide(
  [3. 核心设计：parser 只建树，semantic 独立做合法性检查],
  [
    #two-col(
      [
        #card(
          [为什么不把语义检查写进 parser],
          [
            #bullet([如果在语法动作里同时做符号表和类型判断，文法规则会越来越难维护。])
            #bullet([一旦增加 `record`、引用参数或数组下界偏移，逻辑会散落到多个阶段。])
            #bullet([代码生成阶段还会被迫再猜一次类型，容易前后不一致。])
          ],
          fill: pale-red,
          stroke-color: rgb("#C96767"),
        )
      ],
      [
        #card(
          [当前主流程],
          [
            #code-block(
              "yyparse();\nroot_ast->accept(analyzer);\nstd::string c = cg.generate(root_ast);",
              size: 11pt,
            )
            #v(7pt)
            #bullet([`yyparse()` 只产生 AST。])
            #bullet([`SemanticAnalyzer` 独立完成语义检查。])
            #bullet([若语义失败，则不生成错误的 `.c` 文件。])
          ],
          fill: white,
          stroke-color: border,
        )
      ],
      widths: (1fr, 1.05fr),
    )
  ],
)

#pagebreak()

// 5. Semantic example
#slide(
  [4. 语义分析示例：声明绑定与 record 字段检查],
  [
    #two-col(
      [
        #card(
          [输入样例],
          [
            #code-block(
              "program SemanticRecordUnknownField(input, output);\nvar\n  node: record\n    value: integer;\n  end;\nbegin\n  node.missing := 1;\nend.",
              size: 10.3pt,
            )
          ],
          fill: white,
          stroke-color: border,
        )
      ],
      [
        #card(
          [检查过程],
          [
            #bullet([声明阶段：把 `node` 作为 `record` 变量插入当前作用域。])
            #bullet([使用阶段：遇到 `node.missing` 时，先查找 `node` 的声明。])
            #bullet([类型阶段：确认 `node` 是 `record` 后，查询字段表。])
            #bullet([错误阶段：字段 `missing` 不存在，输出语义错误并停止生成。])
          ],
          fill: pale-green,
          stroke-color: teal,
        )
      ],
      widths: (0.96fr, 1.04fr),
    )
    #small-note([代码位置：`src/symbol_table.*`、`src/semantic_analyzer.*`、`src/type_resolver.*`])
  ],
)

#pagebreak()

// 6. Codegen
#slide(
  [5. 代码生成：把 Pascal-S 结构映射到 C11],
  [
    #two-col(
      [
        #card(
          [主要映射规则],
          [
            #table(
              columns: (1.1fr, 1.45fr),
              gutter: 5pt,
              inset: 6pt,
              fill: (x, y) => if y == 0 { pale-blue } else { white },
              stroke: (paint: border, thickness: 1pt),
              [Pascal-S 特性], [C11 中映射],
              [`record`], [`struct`],
              [`var` 参数], [指针参数],
              [函数名赋值], [`_retval` + `return`],
              [`array[1..n]`], [下标偏移],
              [`boolean`], [`int` / C 布尔表达式],
            )
          ],
          fill: white,
          stroke-color: border,
        )
      ],
      [
        #card(
          [record → struct 示例],
          [
            #text(11.8pt, weight: "bold", fill: muted)[Pascal-S]
            #v(4pt)
            #code-block(
              "point: record\n  x: integer;\n  inner: record\n    y: integer;\n  end;\nend;",
              size: 9.6pt,
            )
            #v(5pt)
            #text(11.8pt, weight: "bold", fill: muted)[C11]
            #v(4pt)
            #code-block(
              "struct pas_record_1 { int y; };\nstruct pas_record_2 {\n  int x;\n  struct pas_record_1 inner;\n};",
              size: 9.6pt,
            )
          ],
          fill: pale-gold,
          stroke-color: gold,
        )
      ],
      widths: (0.94fr, 1.06fr),
    )
    #small-note([代码位置：`src/codegen.*`])
  ],
)

#pagebreak()

// 7. Diagnostics
#slide(
  [6. 错误诊断：从“报行号”升级到 caret 定位],
  [
    #set text(size: 15.8pt)
    #two-col(
      [
        #card(
          [语义错误输出示例],
          [
            #code-block(
              "[Semantic Error] line 7, col 9: Unknown record field: missing\n        node.missing := 1;\n            ^~~~~~~",
              size: 10.6pt,
            )
          ],
          fill: pale-gold,
          stroke-color: gold,
        )
      ],
      [
        #card(
          [实现思路],
          [
            #bullet([词法阶段记录 `line / column`。])
            #bullet([主程序预先缓存源文件每一行文本。])
            #bullet([语法阶段基于 `error` 产生式，在分号、`END`、`UNTIL` 等同步点做 panic-mode 恢复。])
            #bullet([语法和语义阶段统一通过 `ErrorHandler` 输出源码行、箭头定位，并在部分场景下继续报告后续语法问题。])
          ],
          fill: white,
          stroke-color: border,
        )
      ],
      widths: (1fr, 1fr),
    )
  ],
)

#pagebreak()

// 8. Testing
#slide(
  [7. 测试验证],
  [
    #set text(size: 12.2pt)
    #two-col(
      [
        #card(
          [平台结果],
          [
            #text(31pt, weight: "bold", fill: navy)[95 / 95]
            #v(3pt)
            #text(11.6pt, fill: muted)[Educoder 公开与隐藏测试全部通过]
            #v(8pt)
            #align(center)[#image(result-shot, width: 76%)]
          ],
          fill: white,
          stroke-color: border,
          inset-size: 10pt,
        )
      ],
      [
        #card(
          [验证体系],
          [
            #table(
              columns: (1.15fr, 0.75fr, 1.9fr),
              gutter: 4pt,
              inset: 4.5pt,
              fill: (x, y) => if y == 0 { pale-blue } else { white },
              stroke: (paint: border, thickness: 1pt),
              [验证项], [结果], [覆盖内容],
              [Educoder], [95 / 95], [公开与隐藏评测],
              [自建测试], [16 / 16], [成功样例 + 错误样例],
              [严格 C11], [9 / 9], [生成 C 可严格编译],
              [错误覆盖], [7 / 7], [语义错误 + 语法恢复],
            )
          ],
          fill: white,
          stroke-color: border,
          inset-size: 10pt,
        )
      ],
      widths: (0.83fr, 1.17fr),
    )
  ],
)

#pagebreak()

// 9. Team + conclusion
#slide(
  [8. 分工与总结],
  [
    #set text(size: 12.1pt)
    #table(
      columns: (0.9fr, 1.45fr, 1.55fr),
      gutter: 4pt,
      inset: 5pt,
      fill: (x, y) => if y == 0 { pale-blue } else { white },
      stroke: (paint: border, thickness: 1pt),
      [成员], [主要参与内容], [讲解重点],
      [郝政伟], [整体架构、主程序组织、模块联调、最终验收整合], [总体流程、结果展示、现场演示],
      [张睿渤], [词法分析与定位支持], [token、注释处理、行列号与 `yylex()` 接口],
      [周柄名], [语法分析与解析状态], [文法规则、AST 构建、语法错误检测],
      [蔡禹煊], [AST 与公共数据结构], [节点结构、Visitor 访问方式],
      [李良顺], [语义分析与符号表], [作用域栈、声明绑定、类型检查],
      [孔力帆], [类型系统与错误诊断支持], [`TypeResolver`、错误提示输出、类型兼容规则],
      [吴锡华], [目标代码输出验证与测试支撑], [测试样例、构建运行流程、使用说明],
    )
    #v(10pt)
    #text(13.8pt)[总结：我们完成了 Pascal-S 到 C 的完整翻译流程，并通过 #strong[95 / 95 平台测试]、#strong[`record` 与 `repeat-until` 扩展]、#strong[本地测试闭环] 展示了项目的完整性。]
  ],
)
