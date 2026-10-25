param([string]$Compiler = 'C:\msys64\ucrt64\bin\gcc.exe')
$ErrorActionPreference = 'Stop'
$savedPath = $env:PATH
$env:PATH = (Split-Path $Compiler -Parent) + ';' + $env:PATH
Push-Location (Split-Path $PSScriptRoot -Parent)
try {
    $common = @('-std=c99','-O2','-Wall','-Wextra','-Werror','-Iframework/include')
    $core = @('framework/src/controller.c','framework/src/button.c','framework/src/input.c')
    & $Compiler @common @core tests/test_logic.c -o build/test_logic.exe
    if ($LASTEXITCODE -ne 0) { throw 'Logic test compilation failed' }
    & '.\build\test_logic.exe'
    if ($LASTEXITCODE -ne 0) { throw 'Logic tests failed' }
    & $Compiler @common @core framework/src/motor_framework.c tests/test_framework.c -o build/test_framework.exe
    if ($LASTEXITCODE -ne 0) { throw 'Framework test compilation failed' }
    & '.\build\test_framework.exe'
    if ($LASTEXITCODE -ne 0) { throw 'Framework tests failed' }
} finally {
    Pop-Location
    $env:PATH = $savedPath
}
