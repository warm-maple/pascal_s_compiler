# 课堂演示截图操作手册

这份说明的目标是：让你今晚不用临场想，直接按步骤跑命令、截图、插到 `Typst` 里。

## 一、要不要加更多截图

建议最多保留 3 类图，不要再多：

1. `95/95` 平台结果图  
用途：证明阶段性结果。

2. 语义错误带 caret 的终端截图  
用途：展示诊断能力。

3. `record` 成功翻译示例  
用途：展示扩展功能和代码生成效果。

不建议把模糊聊天截图放进正式 PPT。  
如果没有足够清晰、可核验的“最早通关”证据，就不要把它作为正式展示内容。

## 二、推荐演示样例

### 1. record 成功样例

源文件：

- `course_tests/success/record_nested_access.pas`

它会演示：

- `record`
- 嵌套字段访问
- Pascal-S 到 C `struct` 的映射

### 2. 语义错误样例

源文件：

- `course_tests/errors/semantic_record_unknown_field.pas`

它会演示：

- record 字段不存在
- 错误定位箭头

### 3. 语法错误样例

源文件：

- `course_tests/errors/syntax_missing_semicolon.pas`

它会演示：

- 语法错误
- caret diagnostics

## 三、怎么运行命令

下面命令都在项目根目录执行：

```powershell
cd D:\pascal-s-compiler\pascal-s-compiler\pascal-s-compiler
```

### 1. 生成 record 成功样例对应的 C 文件

```powershell
.\build-mingw2\pascc.exe .\course_tests\success\record_nested_access.pas -o .\docs\presentation\assets\record_nested_access_demo.c
```

生成后查看 Pascal-S 源文件：

```powershell
Get-Content .\course_tests\success\record_nested_access.pas
```

查看生成的 C 文件：

```powershell
Get-Content .\docs\presentation\assets\record_nested_access_demo.c
```

如果你想再把生成的 C 编译运行一下：

```powershell
C:\Code\mingw64\mingw64\bin\gcc.exe .\docs\presentation\assets\record_nested_access_demo.c -o .\docs\presentation\assets\record_nested_access_demo.exe
.\docs\presentation\assets\record_nested_access_demo.exe
```

正常输出应该是：

```text
7
```

### 2. 生成语义错误截图

```powershell
.\build-mingw2\pascc.exe .\course_tests\errors\semantic_record_unknown_field.pas
```

你会看到类似：

```text
[Semantic Error] line 7, col 9: Unknown record field: missing
    node.missing := 1;
        ^~~~~~~
```

### 3. 生成语法错误截图

```powershell
.\build-mingw2\pascc.exe .\course_tests\errors\syntax_missing_semicolon.pas
```

你会看到类似：

```text
[Syntax Error] line 4, col 1: syntax error
    begin
    ^~~~~
```

## 四、怎么截图更好看

推荐只截终端核心区域，不要把太多无关桌面内容放进去。

### 终端截图建议

- 字体放大一点
- 终端宽度不要太窄
- 一次只展示一个例子
- 让关键错误信息和 `^~~~~` 都完整出现在一屏内

### 成功样例截图建议

最自然的两种方式：

1. 左边放 `record_nested_access.pas`，右边放生成的 `record_nested_access_demo.c`
2. 只截终端里“命令 + 输出 C 文件内容”的一屏

如果来不及，优先截错误诊断图，因为它展示效果最直观。

## 五、怎么把图片插进 Typst

假设你把截图保存到：

- `docs/presentation/assets/semantic_record_error.png`

那在 `Typst` 里最常用的写法就是：

```typst
#image("assets/semantic_record_error.png", width: 85%)
```

如果要加说明文字：

```typst
#align(center)[
  #image("assets/semantic_record_error.png", width: 85%)
  #v(8pt)
  #text(15pt)[语义错误示例：record 字段不存在时的 caret diagnostics]
]
```

## 六、怎么把图片放到两栏里

如果你想左边写说明、右边放图，可以用你现在 PPT 里的 `two-col`：

```typst
#two-col(
  [
    #item([错误提示会显示源码原文和具体定位位置。])
    #item([这里展示的是 record 字段访问错误。])
  ],
  [
    #image("assets/semantic_record_error.png", width: 100%)
  ],
)
```

## 七、怎么重新导出 PDF

改完 `Typst` 后，在项目根目录执行：

```powershell
typst compile docs\presentation\midterm_report.typ docs\presentation\midterm_report.pdf
```

导出后的 PDF 在：

- `docs/presentation/midterm_report.pdf`

## 八、我的建议

如果时间有限，优先保留这两张图：

1. `95/95` 平台结果图
2. 语义错误 caret 诊断图

`record` 例子更适合在讲的时候口头展示“源文件和生成的 C”，不一定非得也做成图片。  
如果真要加，建议用代码片段或终端截图，而不是再塞一整页花哨图片。
