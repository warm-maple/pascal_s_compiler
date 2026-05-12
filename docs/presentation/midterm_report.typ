#set page(
  paper: "presentation-16-9",
  margin: (x: 1.15cm, y: 0.85cm),
)

#set text(font: ("Microsoft YaHei", "SimHei"), size: 18pt)
#set par(leading: 0.72em)

#let title-color = rgb("#1F4E79")
#let accent-color = rgb("#2F7D32")
#let soft-color = rgb("#EEF4FB")
#let light-accent = rgb("#F2F8F2")

#let slide(title, body) = [
  #rect(
    width: 100%,
    inset: 14pt,
    fill: soft-color,
    radius: 10pt,
    stroke: (paint: title-color, thickness: 1pt),
  )[
    #text(23pt, weight: "bold", fill: title-color)[#title]
  ]
  #v(10pt)
  #body
]

#let item(body) = [
  #text(16pt)[• #body]
  #v(4pt)
]

#let two-col(left, right) = table(
  columns: 2,
  gutter: 12pt,
  stroke: none,
  inset: 0pt,
  [#left],
  [#right],
)

#let result-shot = "assets/educoder_95_95.png"
#let semantic-shot = "assets/semantic_record_error.png"
#let record-demo-shot = "assets/record_translation_demo.png"

#slide(
  [Pascal-S 编译器中期汇报],
  [
    #v(1.1cm)
    #align(center)[
      #text(34pt, weight: "bold")[从“能跑”到“可维护”的编译器流水线改造]
      #v(18pt)
      #text(21pt, fill: title-color)[Pascal-S → C Source-to-Source Compiler]
      #v(32pt)
      #text(20pt)[课程：编译原理与技术课程设计]
      #v(10pt)
      #text(20pt)[汇报时间：2026 年 4 月 15 日]
      #v(10pt)
      #text(19pt)[组长汇报 + 模块负责人补充]
    ]
    #v(1.5cm)
    #rect(
      width: 100%,
      inset: 14pt,
      fill: light-accent,
      radius: 8pt,
      stroke: (paint: accent-color, thickness: 1pt),
    )[
      #text(17pt)[当前阶段成果：平台测试 #strong[95/95] 全通过，已形成可继续扩展的语法分析 → 语义分析 → 代码生成流水线。]
    ]
  ],
)

#pagebreak()

#slide(
  [1. 项目目标与当前进展],
  [
    #two-col(
      [
        #item([目标：实现一个 Pascal-S 到 C 的源到源编译器，支持基本语法、控制流、函数、数组、错误报告，并在平台测试中正确翻译与运行。])
        #item([阶段性任务：完成总体设计与详细设计，打通完整编译流程，并在测试平台上稳定通过公开与隐藏用例。])
        #item([当前状态：编译器主流程、语义检查、记录类型扩展、自建测试集和平台构建链都已完成。])
        #item([核心结果：Educoder 平台最终评测 #strong[95/95]，本地自建课程测试与严格 C11 检查也已通过。])
        #v(10pt)
        #rect(
          width: 100%,
          inset: 12pt,
          fill: rgb("#FFF9E8"),
          radius: 8pt,
          stroke: (paint: rgb("#C08A00"), thickness: 1pt),
        )[
          #text(16pt)[本阶段重点不是“再堆功能”，而是把编译器升级成更清晰、可维护、可扩展的工程结构。]
        ]
      ],
      [
        #align(center)[
          #image(result-shot, width: 100%)
          #v(8pt)
          #text(15pt, fill: title-color)[平台阶段性结果：Educoder 评测 #strong[95/95] 全通过]
        ]
      ],
    )
  ],
)

#pagebreak()

#slide(
  [2. 总体架构设计],
  [
    #align(center)[
      #table(
        columns: 5,
        gutter: 10pt,
        fill: (x, y) => if y == 0 { soft-color } else { white },
        stroke: (x, y) => 1pt + title-color,
        inset: 8pt,
        [输入源码],
        [词法 / 语法分析],
        [AST],
        [语义分析],
        [C 代码生成],

        [`.pas` 文件],
        [`lexer.l + parser.y`],
        [`ast.*`],
        [`semantic_analyzer.*` + `symbol_table.*` + `type_resolver.*`],
        [`codegen.*` 输出 `.c`],
      )
    ]
    #v(14pt)
    #item([旧结构的问题：解析阶段混入了符号表操作和类型判断，导致 parser、副作用、代码生成耦合严重。])
    #item([新结构的改进：parser 只负责建 AST；semantic analyzer 独立遍历 AST；codegen 基于语义结果输出 C。])
    #item([收益：模块边界更清晰，功能扩展时不会把逻辑写散在多个阶段里。])
  ],
)

#pagebreak()

#slide(
  [3. 前端设计：词法分析、语法分析、AST],
  [
    #two-col(
      [
        #text(18pt, weight: "bold", fill: title-color)[词法分析]
        #v(8pt)
        #item([基于 `flex`，识别关键字、标识符、常量、运算符和注释。])
        #item([维护行列号，为后续 syntax / semantic error 的 caret diagnostics 提供定位信息。])
        #item([扩展了布尔字面量和与 `record` 相关的词法入口。])
      ],
      [
        #text(18pt, weight: "bold", fill: title-color)[语法分析与 AST]
        #v(8pt)
        #item([基于 `bison` 按 Pascal-S 文法构建 AST。])
        #item([引入 `ParserState` 管理解析期上下文，减少静态全局状态。])
        #item([AST 统一承载声明、表达式、语句、函数调用、数组访问、record 字段访问等结构。])
      ],
    )
    #v(14pt)
    #rect(
      width: 100%,
      inset: 12pt,
      fill: light-accent,
      radius: 8pt,
      stroke: (paint: accent-color, thickness: 1pt),
    )[
      #text(16pt)[这一层的目标是“把程序结构解析正确”，而不是“顺便做类型检查”。]
    ]
  ],
)

#pagebreak()

#slide(
  [4. 词法分析实现要点],
  [
    #two-col(
      [
        #item([`flex` 根据正则规则自动生成词法分析器，开发时只需描述“什么字符模式对应什么 token”。])
        #item([关键字采用大小写不敏感匹配，例如 `program`、`const`、`var` 都能被识别。])
        #item([标识符统一转为小写存储，符合 Pascal-S 大小写不敏感的语言特性。])
        #item([词法阶段同时维护 `line / column`，为后续 caret diagnostics 提供定位基础。])
      ],
      [
        #table(
          columns: 2,
          gutter: 8pt,
          fill: (x, y) => if y == 0 { soft-color } else { white },
          stroke: (x, y) => 1pt + title-color,
          inset: 6pt,
          [输入模式], [输出 token],
          [`program`], [`PROGRAM`],
          [`abc123`], [`IDENTIFIER`],
          [`123`], [`INTEGER_LITERAL`],
          [`true` / `false`], [`BOOLEAN_LITERAL`],
          [`:=`], [`ASSIGN`],
        )
        #v(12pt)
        #rect(
          width: 100%,
          inset: 10pt,
          fill: light-accent,
          radius: 8pt,
          stroke: (paint: accent-color, thickness: 1pt),
        )[
          #text(15pt)[讲解主线：字符流先被切分成 token 流，后面的 parser 才能按文法进行归约。]
        ]
      ],
    )
  ],
)

#pagebreak()

#slide(
  [5. 中端设计：语义分析与类型系统],
  [
    #item([新增 `SemanticAnalyzer`，独立完成变量、常量、函数、参数、作用域和类型相关检查。])
    #item([通过 `SymbolTable` 管理符号插入与查找，支持作用域入栈 / 出栈。])
    #item([通过 `TypeResolver` 统一表达式类型推断、record 字段解析、数组元素类型查询，避免 semantic 和 codegen 各写一套逻辑。])
    #item([已支持的典型语义检查：未声明标识符、赋值类型不匹配、参数错误、引用参数左值约束、record 字段不存在等。])
    #v(12pt)
    #align(center)[
      #rect(
        width: 88%,
        inset: 12pt,
        fill: rgb("#FAF3FF"),
        radius: 8pt,
        stroke: (paint: rgb("#7A3DB8"), thickness: 1pt),
      )[
        #text(16pt)[关键改进：#strong[类型知识只维护一份]，避免语义分析与代码生成出现类型规则漂移。]
      ]
    ]
  ],
)

#pagebreak()

#slide(
  [6. 语义分析实现要点],
  [
    #two-col(
      [
        #item([`SymbolTable` 采用作用域栈结构：进入函数时入栈，离开函数时出栈。])
        #item([`SemanticAnalyzer` 遍历 AST，对变量使用、函数调用、赋值语句和条件表达式进行合法性检查。])
        #item([`TypeResolver` 统一处理表达式类型、数组元素类型和 record 字段类型，避免多个模块重复推断。])
        #item([典型错误包括：未声明标识符、参数数量或类型错误、record 字段不存在、引用参数不是左值。])
      ],
      [
        #table(
          columns: 2,
          gutter: 8pt,
          fill: (x, y) => if y == 0 { soft-color } else { white },
          stroke: (x, y) => 1pt + title-color,
          inset: 6pt,
          [阶段], [作用],
          [符号表], [回答“这个名字是什么”],
          [作用域], [回答“当前位置能不能访问它”],
          [类型检查], [回答“这条语句在语义上是否合法”],
          [TypeResolver], [回答“这个表达式最终是什么类型”],
        )
        #v(12pt)
        #rect(
          width: 100%,
          inset: 10pt,
          fill: rgb("#FAF3FF"),
          radius: 8pt,
          stroke: (paint: rgb("#7A3DB8"), thickness: 1pt),
        )[
          #text(15pt)[讲解主线：语法正确不代表程序正确，语义分析负责判断这棵 AST 在“含义上”是否成立。]
        ]
      ],
    )
  ],
)

#pagebreak()

#slide(
  [7. 后端设计：代码生成与语言扩展],
  [
    #two-col(
      [
        #text(18pt, weight: "bold", fill: title-color)[代码生成]
        #v(8pt)
        #item([输出 C 目标代码，支持声明、表达式、控制流、函数、数组、过程调用。])
        #item([统一调用参数 lowering 与声明输出，减少重复代码。])
        #item([尽量生成更标准的 C，避免依赖 GNU 特有扩展。])
      ],
      [
        #text(18pt, weight: "bold", fill: title-color)[本阶段新增支持]
        #v(8pt)
        #item([`record` 结构定义与多级字段访问。])
        #item([`program x(input, output);` 头部。])
        #item([Pascal 数组任意下界到 C 下标偏移映射。])
        #item([函数调用作语句、布尔字面量与更完善的表达式翻译。])
      ],
    )
    #v(16pt)
    #rect(
      width: 100%,
      inset: 12pt,
      fill: rgb("#FFF3F1"),
      radius: 8pt,
      stroke: (paint: rgb("#B7410E"), thickness: 1pt),
    )[
      #text(16pt)[后端目标不只是“把代码拼出来”，而是把 Pascal-S 语义稳定地映射为 C。]
    ]
  ],
)

#pagebreak()

#slide(
  [8. 代码生成实现要点],
  [
    #two-col(
      [
        #item([代码生成阶段遍历 AST，把声明、语句、表达式和函数翻译成等价的 C 代码。])
        #item([Pascal 数组允许任意下界，因此生成数组访问时要做下标偏移；`record` 则映射为 C 的 `struct`。])
        #item([`var` 参数会被翻译成指针参数，函数和过程调用还要处理参数求值顺序与临时变量。])
        #item([本阶段对调用 lowering 与声明输出做了去重，使代码生成逻辑更集中、更容易维护。])
      ],
      [
        #align(center)[
          #image(record-demo-shot, width: 100%)
          #v(6pt)
          #text(14pt, fill: title-color)[`record` 样例翻译结果：Pascal-S 结构被映射成 C `struct`]
        ]
      ],
    )
  ],
)

#pagebreak()

#slide(
  [9. 错误诊断],
  [
    #two-col(
      [
        #item([错误提示从简单行号升级为 caret diagnostics：显示源码原文，并在出错位置下方标注定位箭头。])
        #item([诊断覆盖 syntax error 和 semantic error，便于调试和课堂展示。])
        #item([这一能力依赖词法阶段的行列号记录、主流程中的源码缓存，以及错误模块中的统一输出逻辑。])
        #item([相比只报“Error + 行号”的方式，这种形式更接近现代编译器，也更适合课堂展示。])
      ],
      [
        #align(center)[
          #image(semantic-shot, width: 100%)
          #v(6pt)
          #text(14pt, fill: title-color)[语义错误示例：record 字段不存在时的 caret diagnostics]
        ]
      ],
    )
  ],
)

#pagebreak()

#slide(
  [10. 测试验证],
  [
    #item([除平台测试外，新增 `course_tests/` 课程自建测试集，覆盖成功用例与错误用例。])
    #item([增加严格生成代码检查，以更严格的 C11 方式验证输出 `.c` 的规范性。])
    #item([验证目标不是只看通过率，还要证明生成结果可编译、可运行、可定位问题。])
    #v(14pt)
    #align(center)[
      #table(
        columns: 3,
        gutter: 10pt,
        fill: (x, y) => if y == 0 { soft-color } else { white },
        stroke: (x, y) => 1pt + title-color,
        inset: 8pt,
        [验证维度], [结果], [说明],
        [Educoder 平台], [95/95], [公开 + 隐藏用例全部通过],
        [课程自建测试], [13/13], [成功与失败场景均覆盖],
        [严格 C11 检查], [8/8], [验证生成 C 的规范性],
      )
    ]
  ],
)

#pagebreak()

#slide(
  [11. 项目亮点与加分项],
  [
    #item([在基础 Pascal-S 功能之外，补充支持了 `record` 结构与多级字段访问，使语言子集更完整。])
    #item([将错误提示升级为带源码行和定位标记的诊断形式，改善了可调试性与展示效果。])
    #item([围绕平台数据之外补充课程自建测试集，使功能验证从“能过样例”扩展到“能自证正确”。])
    #item([将语义分析从 parser 中拆分出来，形成更清晰的语法分析 → 语义分析 → 代码生成流水线。])
    #item([在输出侧增加更严格的 C11 检查，使项目不仅关注通过率，也关注生成结果的规范性。])
    #v(10pt)
    #rect(
      width: 100%,
      inset: 12pt,
      fill: rgb("#F7F3FF"),
      radius: 8pt,
      stroke: (paint: rgb("#7A3DB8"), thickness: 1pt),
    )[
      #text(16pt)[这些改动让项目从“完成基本翻译”提升到“具备扩展性、诊断能力与自我验证能力”的阶段。]
    ]
  ],
)

#pagebreak()

#slide(
  [12. 当前问题与下一步计划],
  [
    #item([当前仍可继续优化的方向：进一步拆分 `codegen` 大文件、继续瘦身 `parser.y`、补充更多教学型文档。])
    #item([下一步工作：补齐最终报告材料、整理模块说明、继续沉淀自建测试集，保证后续演示与验收稳定。])
    #item([最终交付目标：不仅“平台通过”，还要让代码结构、测试设计和文档表达都能支撑课程设计答辩。])
    #v(14pt)
    #align(center)[
      #text(24pt, weight: "bold", fill: accent-color)[阶段结论：功能已达标，接下来重点转向文档表达与展示质量。]
    ]
  ],
)
