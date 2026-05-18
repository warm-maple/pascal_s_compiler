# 最终提交前执行清单

这份清单的目标是：在正式提交前，用一次完整流程确认“材料、程序、测试、打包”都处于可交状态。

## A. 先确认文档与表格

1. 附件1分组表已完成最终填写
2. 附件2正式报告 PDF 已导出
3. 附件3验收登记表已填写完成并检查贡献率总和为 100%
4. 《程序使用说明》PDF 已导出

## B. 重新确认编译器可运行

在项目根目录执行：

```powershell
cmake -S . -B build-mingw2 -G "MinGW Makefiles"
cmake --build build-mingw2 -j 4
```

然后至少手工验证两条命令：

```powershell
.\build-mingw2\pascc.exe -i .\course_tests\success\record_nested_access.pas
.\build-mingw2\pascc.exe .\course_tests\errors\semantic_record_unknown_field.pas
```

检查点：

- 成功样例能生成 `.c`
- 错误样例能输出带定位箭头的诊断

## C. 跑一遍测试

```powershell
.\course_tests\run_course_tests.ps1 -CompilerPath .\build-mingw2\pascc.exe
.\course_tests\run_strict_c_checks.ps1 -CompilerPath .\build-mingw2\pascc.exe
```

若时间允许，再补充确认平台结果截图和本地截图是否齐全。

## D. 更新交付目录

1. 重新导出源码压缩包到 `submission/03_source/`
2. 确认 `submission/04_binary/pascc.exe` 为最新版本
3. 确认 `submission/05_testcases/` 中测试目录完整
4. 确认 `submission/02_report/report.pdf` 和 `submission/06_user_guide/user_guide.pdf` 为最新版本

## E. 最终检查提交 6 项材料

对应目录如下：

1. 附件1：`submission/01_group_form/`
2. 附件2：`submission/02_report/`
3. 源程序：`submission/03_source/`
4. 可运行程序：`submission/04_binary/`
5. 测试用例：`submission/05_testcases/`
6. 程序使用说明：`submission/06_user_guide/`

## F. 答辩前一天建议

1. 再打开一次报告 PDF 和 PPT
2. 再检查一次每个人的提示词
3. 再确认一次证件、时间、地点和平台通过数
4. 把需要现场打开的文件提前放到一个容易找到的位置
