$ErrorActionPreference = 'Stop'
$gccCommand = Get-Command gcc -ErrorAction SilentlyContinue
$gccPath = if ($gccCommand) { $gccCommand.Source } else { 'D:/mingw64/bin/gcc.exe' }
if (-not (Test-Path -LiteralPath $gccPath)) { throw 'MinGW GCC is required.' }
$testBinary = Join-Path ([IO.Path]::GetTempPath()) ('demo_test_' + [guid]::NewGuid().ToString('N') + '.exe')
$sources = @('test_integration.c', '../Bsp/key.c', '../Bsp/hcsr04.c', '../Bsp/uart_driver.c', '../Bsp/esp8266.c', '../Bsp/led.c') |
    ForEach-Object { Join-Path $PSScriptRoot $_ }
try {
    foreach ($networkEnabled in @($false, $true)) {
        $testDefines = @()
        if ($networkEnabled) {
            $testDefines = @('-include', "$PSScriptRoot/stubs/network_config.h")
        }
        & $gccPath -std=c99 -Wall -Wextra -Werror "-I$PSScriptRoot/stubs" "-I$PSScriptRoot/../Bsp" @testDefines @sources -o $testBinary
        if ($LASTEXITCODE -ne 0) { throw 'Host test compilation failed.' }
        & $testBinary
        if ($LASTEXITCODE -ne 0) { throw 'Host tests failed.' }
    }
    & $gccPath -std=c99 -Wall -Wextra -Werror "-I$PSScriptRoot/stubs" "-I$PSScriptRoot/../Bsp" `
        "$PSScriptRoot/test_flash.c" "$PSScriptRoot/../Bsp/gd25q80.c" "$PSScriptRoot/../Bsp/app_flash.c" -o $testBinary
    if ($LASTEXITCODE -ne 0) { throw 'Flash test compilation failed.' }
    & $testBinary
    if ($LASTEXITCODE -ne 0) { throw 'Flash tests failed.' }
    & $gccPath -std=c99 -Wall -Wextra -Werror "-I$PSScriptRoot/../Bsp" `
        "$PSScriptRoot/test_attitude.c" "$PSScriptRoot/../Bsp/attitude.c" -lm -o $testBinary
    if ($LASTEXITCODE -ne 0) { throw 'Attitude test compilation failed.' }
    & $testBinary
    if ($LASTEXITCODE -ne 0) { throw 'Attitude tests failed.' }
} finally {
    if (Test-Path -LiteralPath $testBinary) { Remove-Item -LiteralPath $testBinary }
}
