# 测试用例交付说明

本目录对应老师要求的“测试用例”。

## 当前包含内容

### 1. `course_tests/`

课程自建测试集，分为：

- `success/`：成功编译并得到正确输出的样例
- `errors/`：应当报错的语法/语义错误样例
- `run_course_tests.ps1`：自动化测试脚本
- `run_strict_c_checks.ps1`：严格 C11 生成代码检查脚本

### 2. `user_tests/`

开发阶段使用的本地回归测试，覆盖：

- 词法分析
- 表达式与语句
- 控制流
- 函数
- 数组
- `record`

## 测试分类说明

本次提交中的测试用例可按三类理解：

1. 基础功能测试：变量、表达式、控制流、函数、数组、输入输出
2. 扩展功能测试：`record` 结构、多级字段访问、严格生成代码检查
3. 错误处理测试：未声明标识符、字段不存在、引用参数错误、类型不匹配、缺失分号

## 代表性样例

- `record_nested_access.pas`：验证 `record` 与嵌套字段访问
- `array_lower_bound_shift.pas`：验证 Pascal-S 数组下界偏移
- `var_parameter_array_element.pas`：验证引用参数与数组元素
- `semantic_record_unknown_field.pas`：验证语义错误定位
- `syntax_missing_semicolon.pas`：验证语法错误定位

## 交付建议

提交时建议直接保留本目录当前结构，不再额外删减。这样老师若需要查看：

- 成功样例
- 失败样例
- 测试脚本
- 本地回归样例

都可以在同一目录下完成。
