# Course Test Suite

This folder contains a standalone high-quality test suite for the Pascal-S course project.

Layout:

- `success/`: programs that must compile, translate to C, and produce exact expected output
- `errors/`: programs that must fail compilation and include required diagnostic substrings
- `run_course_tests.ps1`: Windows PowerShell runner for the full suite

Conventions:

- Success cases may have an optional `.in` input file and must have a matching `.out` file.
- Error cases must have a matching `.err.txt` file. Each non-empty line is treated as a required substring in the compiler output.

Example:

```powershell
.\course_tests\run_course_tests.ps1 -CompilerPath .\build-mingw2\pascc.exe
```

Current coverage:

- record layout and nested field access
- Pascal array lower bounds translated to shifted C indexes
- function invocation used as a statement
- `var` parameter lvalue validation with array elements
- Pascal integer/boolean `not`
- `program ... (input, output)` header and `read`
- multi-argument `write` with string constants
- semantic diagnostics for undeclared names, bad field access, bad reference arguments, and type mismatch
- syntax diagnostics with caret output
