param(
    [string]$CompilerPath = ".\build-mingw2\pascc.exe",
    [string]$GccPath = "C:\Code\mingw64\mingw64\bin\gcc.exe",
    [string]$WorkDir = ".\course_tests\_strict_work"
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path $CompilerPath)) {
    throw "Compiler not found: $CompilerPath"
}

if (-not (Test-Path $GccPath)) {
    throw "gcc not found: $GccPath"
}

New-Item -ItemType Directory -Force -Path $WorkDir | Out-Null
$cases = Get-ChildItem ".\course_tests\success" -Filter *.pas | Sort-Object Name
$failures = New-Object System.Collections.Generic.List[string]
$passed = 0

foreach ($case in $cases) {
    $base = Join-Path $WorkDir $case.BaseName
    $cFile = "$base.c"

    & $CompilerPath $case.FullName -o $cFile 2>&1 | Out-Null
    if ($LASTEXITCODE -ne 0) {
        $failures.Add("$($case.Name): Pascal compilation failed")
        continue
    }

    & $GccPath -std=c11 -pedantic-errors -Wall -Wextra -Werror -c $cFile -o "$base.o" 2>&1 | Out-Null
    if ($LASTEXITCODE -ne 0) {
        $failures.Add("$($case.Name): generated C failed strict C11 compile")
        continue
    }

    $passed++
}

Write-Host "Strict C checks passed $passed / $($cases.Count)"

if ($failures.Count -gt 0) {
    Write-Host ""
    Write-Host "Failures:"
    $failures | ForEach-Object { Write-Host " - $_" }
    exit 1
}
