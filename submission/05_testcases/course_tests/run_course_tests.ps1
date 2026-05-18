param(
    [string]$CompilerPath = ".\build-mingw2\pascc.exe",
    [string]$GccPath = "C:\Code\mingw64\mingw64\bin\gcc.exe",
    [string]$WorkDir = ".\course_tests\_work"
)

$ErrorActionPreference = "Stop"

function Normalize-Text([string]$text) {
    return ($text -replace "`r`n", "`n").TrimEnd()
}

if (-not (Test-Path $CompilerPath)) {
    throw "Compiler not found: $CompilerPath"
}

if (-not (Test-Path $GccPath)) {
    throw "gcc not found: $GccPath"
}

New-Item -ItemType Directory -Force -Path $WorkDir | Out-Null
$successCases = Get-ChildItem ".\course_tests\success" -Filter *.pas | Sort-Object Name
$errorCases = Get-ChildItem ".\course_tests\errors" -Filter *.pas | Sort-Object Name
$failures = New-Object System.Collections.Generic.List[string]
$passed = 0

foreach ($case in $successCases) {
    $base = Join-Path $WorkDir $case.BaseName
    $cFile = "$base.c"
    $exeFile = "$base.exe"
    $inputFile = Join-Path $case.DirectoryName ($case.BaseName + ".in")
    $expectedFile = Join-Path $case.DirectoryName ($case.BaseName + ".out")

    if (-not (Test-Path $expectedFile)) {
        $failures.Add("SUCCESS $($case.Name): missing expected output file")
        continue
    }

    & $CompilerPath $case.FullName -o $cFile 2>&1 | Out-Null
    if ($LASTEXITCODE -ne 0) {
        $failures.Add("SUCCESS $($case.Name): Pascal compilation failed")
        continue
    }

    & $GccPath -std=c99 $cFile -o $exeFile 2>&1 | Out-Null
    if ($LASTEXITCODE -ne 0) {
        $failures.Add("SUCCESS $($case.Name): generated C failed to compile")
        continue
    }

    $actual = if (Test-Path $inputFile) {
        Get-Content $inputFile | & $exeFile 2>&1 | Out-String
    } else {
        & $exeFile 2>&1 | Out-String
    }

    if ($LASTEXITCODE -ne 0) {
        $failures.Add("SUCCESS $($case.Name): executable returned non-zero")
        continue
    }

    $expected = Get-Content $expectedFile -Raw
    if ((Normalize-Text $actual) -ne (Normalize-Text $expected)) {
        $failures.Add("SUCCESS $($case.Name): output mismatch")
        continue
    }

    $passed++
}

foreach ($case in $errorCases) {
    $stderr = & $CompilerPath $case.FullName -o (Join-Path $WorkDir ($case.BaseName + ".c")) 2>&1 | Out-String
    $expectedFile = Join-Path $case.DirectoryName ($case.BaseName + ".err.txt")

    if ($LASTEXITCODE -eq 0) {
        $failures.Add("ERROR $($case.Name): expected failure but compilation succeeded")
        continue
    }

    if (-not (Test-Path $expectedFile)) {
        $failures.Add("ERROR $($case.Name): missing expected error file")
        continue
    }

    $missing = @()
    foreach ($needle in Get-Content $expectedFile) {
        if ([string]::IsNullOrWhiteSpace($needle)) {
            continue
        }
        if (-not $stderr.Contains($needle)) {
            $missing += $needle
        }
    }

    if ($missing.Count -gt 0) {
        $failures.Add("ERROR $($case.Name): missing substrings -> " + ($missing -join ", "))
        continue
    }

    $passed++
}

$total = $successCases.Count + $errorCases.Count
Write-Host "Passed $passed / $total"

if ($failures.Count -gt 0) {
    Write-Host ""
    Write-Host "Failures:"
    $failures | ForEach-Object { Write-Host " - $_" }
    exit 1
}
