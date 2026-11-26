$ErrorActionPreference = 'Stop'
$gcc = Get-Command gcc -ErrorAction SilentlyContinue
$gccPath = if ($gcc) { $gcc.Source } else { 'D:\mingw64\bin\gcc.exe' }
if (-not (Test-Path -LiteralPath $gccPath)) { throw 'Install MinGW GCC or add gcc to PATH.' }
$testBinary = Join-Path ([IO.Path]::GetTempPath()) ('motor_test_' + [guid]::NewGuid().ToString('N') + '.exe')
try {
    & $gccPath -std=c99 -Wall -Wextra -Werror "-I$PSScriptRoot/stubs" "-I$PSScriptRoot/../Bsp" `
        "$PSScriptRoot/test_motor.c" "$PSScriptRoot/../Bsp/motor.c" "$PSScriptRoot/../Bsp/motor_pwm.c" `
        "$PSScriptRoot/../Bsp/pid.c" -lm -o $testBinary
    if ($LASTEXITCODE -ne 0) { throw 'Host test compilation failed.' }
    & $testBinary
    if ($LASTEXITCODE -ne 0) { throw 'Host tests failed.' }
} finally {
    if (Test-Path -LiteralPath $testBinary) { Remove-Item -LiteralPath $testBinary }
}
