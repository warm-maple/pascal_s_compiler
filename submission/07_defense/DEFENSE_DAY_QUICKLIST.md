# 答辩当天携带 / 打开清单

## 一、出门前带上

- [ ] 学生卡 / 身份证 / 学生证
- [ ] 纸质《验收登记表》
- [ ] 电脑和电源
- [ ] 鼠标（有的话）
- [ ] 手机保持可联系

## 二、到场时间

- [ ] 2026 年 5 月 20 日
- [ ] 15:20 前到教三楼 3-217
- [ ] 组员不要迟到

## 三、组长到场后立刻做

- [ ] 打开答辩 PPT
- [ ] 打开正式报告 PDF
- [ ] 打开程序使用说明 PDF
- [ ] 打开 IDE / 源码目录
- [ ] 打开编译器可执行文件所在目录
- [ ] 准备好 1 个成功样例和 1 个错误样例
- [ ] 提前进入腾讯会议等候室：`813-192-623`

## 四、建议现场提前打开的文件

- [ ] `submission/02_report/report.pdf`
- [ ] `submission/06_user_guide/user_guide.pdf`
- [ ] `submission/07_defense/MEMBER_PROMPTS.md`
- [ ] `src/main.cpp`
- [ ] `src/semantic_analyzer.cpp`
- [ ] `src/codegen.cpp`

## 五、建议准备的两条演示命令

```powershell
.\build-mingw2\pascc.exe -i .\course_tests\success\record_nested_access.pas
.\build-mingw2\pascc.exe .\course_tests\errors\semantic_record_unknown_field.pas
```

## 六、每个人至少要会说的

- 自己负责模块是什么
- 输入是什么，输出是什么
- 解决了什么问题
- 对应代码大概在哪

## 七、你作为组长的最短开场

“我们项目实现的是一个 Pascal-S 到 C 的源到源编译器，主流程包括词法分析、语法分析与 AST、语义分析和代码生成，目前头歌平台达到 95 / 95，并补充了 `record`、错误定位和自建测试体系。” 
