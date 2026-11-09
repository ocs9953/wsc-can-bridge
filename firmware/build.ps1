param(
    [switch]$Clean,
    [switch]$Flash
)

Set-Location $PSScriptRoot

$cube     = "C:\ST\STM32CubeIDE_2.2.0\STM32CubeIDE\plugins"
$makePath = "$cube\com.st.stm32cube.ide.mcu.externaltools.make.win32_2.2.200.202604021615\tools\bin\make.exe"
$openocd  = "$cube\com.st.stm32cube.ide.mcu.externaltools.openocd.win32_2.4.500.202604080855\tools\bin\openocd.exe"
$scripts  = "$cube\com.st.stm32cube.ide.mcu.debug.openocd_2.3.400.202606220929\resources\openocd\st_scripts"

if ($Clean -and (Test-Path build)) { Remove-Item -Recurse -Force build }
New-Item -ItemType Directory -Force build | Out-Null

& $makePath -j16
if ($LASTEXITCODE -ne 0) { Write-Host "[ERROR] Build failed" -ForegroundColor Red; exit 1 }

if ($Flash) {
    $sn = if ($env:WSC_BOARD_SN) { $env:WSC_BOARD_SN } else { "7" }
    Write-Host "[FLASH] ST-Link SN=$sn" -ForegroundColor Cyan
    & $openocd -s $scripts -c "adapter serial $sn" -f interface/stlink-dap.cfg -f target/stm32u5x.cfg `
        -c "program build/WSC_CAN_Bridge.elf verify reset exit"
    if ($LASTEXITCODE -ne 0) { Write-Host "[ERROR] Flash failed" -ForegroundColor Red; exit 1 }
}
