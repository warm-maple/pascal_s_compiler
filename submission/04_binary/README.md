# 可运行程序交付说明

本目录对应老师要求的“可运行程序”。

## 当前包含内容

- `pascc.exe`

该文件为 Windows 环境下构建得到的 Pascal-S 编译器可执行程序，可直接在 PowerShell 或命令提示符中调用。

## 基本使用方式

```powershell
.\pascc.exe -i input.pas
```

若希望显式指定输出文件，可使用：

```powershell
.\pascc.exe -i input.pas -o output.c
```

## 说明

- 该可执行文件适用于 Windows 本地演示与复现。
- 若在头歌平台或 Linux 环境中验收，应根据源码重新构建 Linux 版本的 `pascc`。
- 不建议提交整个 `build` 目录，保留最终可执行文件即可。

## 交付前核对项

- [ ] `pascc.exe` 可正常运行
- [ ] 可正确生成 `.c` 文件
- [ ] 至少已用 1 个成功样例和 1 个错误样例验证
- [ ] 如需平台演示，已额外准备 Linux 构建命令
