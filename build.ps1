param([string]$ToolDir = 'C:\arm-gnu-toolchain-15.2.rel1-mingw-w64-i686-arm-none-eabi')
$ErrorActionPreference = 'Stop'
Push-Location $PSScriptRoot
try {
    $cc = Join-Path $ToolDir 'bin\arm-none-eabi-gcc.exe'
    $ar = Join-Path $ToolDir 'bin\arm-none-eabi-ar.exe'
    $cpu = @('-mcpu=cortex-m4','-mthumb','-mfpu=fpv4-sp-d16','-mfloat-abi=hard')
    $flags = @('-DSTM32F411xE','-Iframework/include','-Iports/stm32f411','-std=c99','-O2','-g3','-Wall','-Wextra','-Werror','-ffreestanding','-fno-common','-MMD','-MP')
    $core = @('controller','button','input','motor_framework')
    $board = @('board','clock','key','motor','uart','systick')
    $coreObjects = @()
    $boardObjects = @()
    foreach ($name in $core) {
        $object = "build/core_$name.o"
        & $cc @cpu @flags -c "framework/src/$name.c" -o $object
        if ($LASTEXITCODE -ne 0) { throw "Core compile failed: $name" }
        $coreObjects += $object
    }
    & $ar rcs build/libmotor_framework.a @coreObjects
    if ($LASTEXITCODE -ne 0) { throw 'Static library creation failed' }
    foreach ($name in $board) {
        $object = "build/port_$name.o"
        & $cc @cpu @flags -c "ports/stm32f411/$name.c" -o $object
        if ($LASTEXITCODE -ne 0) { throw "Port compile failed: $name" }
        $boardObjects += $object
    }
    & $cc @cpu @flags -c examples/stm32f411_motor/main.c -o build/main.o
    if ($LASTEXITCODE -ne 0) { throw 'Example compile failed' }
    & $cc @cpu -g3 -c ports/stm32f411/crt0.s -o build/crt0.o
    if ($LASTEXITCODE -ne 0) { throw 'Startup assembly failed' }
    & $cc @cpu build/main.o @boardObjects build/crt0.o build/libmotor_framework.a -nostdlib '-Wl,-Map=build/rom_0x08000000.map' -T ports/stm32f411/rom_0x08000000.lds -lgcc -o build/rom_0x08000000.elf
    if ($LASTEXITCODE -ne 0) { throw 'Link failed' }
    & (Join-Path $ToolDir 'bin\arm-none-eabi-objcopy.exe') -O binary build/rom_0x08000000.elf build/rom_0x08000000.bin
    if ($LASTEXITCODE -ne 0) { throw 'Binary generation failed' }
    & (Join-Path $ToolDir 'bin\arm-none-eabi-size.exe') build/rom_0x08000000.elf
    if ($LASTEXITCODE -ne 0) { throw 'Size check failed' }
} finally {
    Pop-Location
}
