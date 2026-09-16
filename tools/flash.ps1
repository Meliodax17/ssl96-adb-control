<#
    flash.ps1 - Nap file HEX vao STM32F103C8T6 bang ST-LINK

    Chay tu task cua IDE, hoac truc tiep:
        powershell -ExecutionPolicy Bypass -File tools\flash.ps1
        powershell -ExecutionPolicy Bypass -File tools\flash.ps1 -HexFile duong\dan.hex
#>

[CmdletBinding()]
param(
    [string]$HexFile,     # de trong thi tu tim file HEX moi nhat
    [switch]$NoReset      # khong reset chip sau khi nap
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$Root   = Split-Path -Parent $PSScriptRoot
$Target = 'SSL96_ADB_TPS9266x'

function Write-Step($msg) { Write-Host "==> $msg" -ForegroundColor Cyan }
function Write-Ok  ($msg) { Write-Host "    $msg" -ForegroundColor Green }
function Write-Err ($msg) { Write-Host "[LOI] $msg" -ForegroundColor Red }

# ---------------------------------------------------------------------------
# 1. Tim file HEX
# ---------------------------------------------------------------------------
if (-not $HexFile) {
    # Uu tien ban moi nhat giua build cua GCC va build cua Keil
    $candidates = @(@(
        (Join-Path $Root "build\$Target.hex"),
        (Join-Path $Root "RVMDK\Output\$Target.hex")
    ) | Where-Object { Test-Path $_ })

    if ($candidates.Count -eq 0) {
        Write-Err 'Khong tim thay file HEX nao.'
        Write-Host '    Chay task "Build: bien dich ra file HEX" truoc.'
        exit 1
    }

    $HexFile = ($candidates | Sort-Object { (Get-Item $_).LastWriteTimeUtc } -Descending |
                Select-Object -First 1)
}

if (-not (Test-Path $HexFile)) { Write-Err "Khong thay file `"$HexFile`""; exit 1 }
$HexFile = (Resolve-Path $HexFile).Path

$age = (Get-Date) - (Get-Item $HexFile).LastWriteTime
Write-Step 'File se nap'
Write-Ok $HexFile
Write-Ok ("Kich thuoc {0:N0} byte, tao cach day {1:N0} phut" -f (Get-Item $HexFile).Length, $age.TotalMinutes)

# ---------------------------------------------------------------------------
# 2. Tim ST-LINK_CLI
# ---------------------------------------------------------------------------
function Find-StLinkCli {
    $paths = @(
        "${env:ProgramFiles(x86)}\STMicroelectronics\STM32 ST-LINK Utility\ST-LINK Utility\ST-LINK_CLI.exe",
        "$env:ProgramFiles\STMicroelectronics\STM32 ST-LINK Utility\ST-LINK Utility\ST-LINK_CLI.exe"
    )
    foreach ($p in $paths) { if (Test-Path $p) { return $p } }
    $cmd = Get-Command 'ST-LINK_CLI.exe' -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    return $null
}

# STM32CubeProgrammer la ban thay the moi hon cua ST-LINK Utility
function Find-CubeProg {
    $paths = @(
        "$env:ProgramFiles\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe",
        "${env:ProgramFiles(x86)}\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe"
    )
    foreach ($p in $paths) { if (Test-Path $p) { return $p } }
    $cmd = Get-Command 'STM32_Programmer_CLI.exe' -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    return $null
}

$cli  = Find-StLinkCli
$cube = $null
if (-not $cli) { $cube = Find-CubeProg }

if (-not $cli -and -not $cube) {
    Write-Err 'Khong tim thay cong cu nap chip.'
    Write-Host '    Cai mot trong hai:'
    Write-Host '      - STM32 ST-LINK Utility  (ST-LINK_CLI.exe)'
    Write-Host '      - STM32CubeProgrammer    (STM32_Programmer_CLI.exe)'
    exit 1
}

# ---------------------------------------------------------------------------
# 3. Nap
# ---------------------------------------------------------------------------
if ($cli) {
    Write-Step "Nap bang ST-LINK Utility"
    Write-Ok $cli

    $cliArgs = @('-c', 'SWD', 'UR', '-P', $HexFile, '-V', 'while_programming')
    if (-not $NoReset) { $cliArgs += '-Rst' }

    & $cli @cliArgs
    $rc = $LASTEXITCODE
}
else {
    Write-Step 'Nap bang STM32CubeProgrammer'
    Write-Ok $cube

    $cliArgs = @('-c', 'port=SWD', 'mode=UR', '-w', $HexFile, '-v')
    if (-not $NoReset) { $cliArgs += '-rst' }

    & $cube @cliArgs
    $rc = $LASTEXITCODE
}

Write-Host ''
if ($rc -eq 0) {
    Write-Host '--------------------------------------------------------------' -ForegroundColor Green
    Write-Host ' NAP THANH CONG' -ForegroundColor Green
    Write-Host '--------------------------------------------------------------' -ForegroundColor Green
    Write-Host ''
    Write-Host ' Kiem tra nhanh: den PC13 tren board'
    Write-Host '   sang lien tuc = giao tiep voi module binh thuong'
    Write-Host '   nhay nhanh    = chua noi duoc voi module'
} else {
    Write-Err "Nap that bai, ma loi $rc."
    Write-Host '    Kiem tra lai:'
    Write-Host '      - ST-LINK da cam vao may chua'
    Write-Host '      - day SWD (SWDIO = PA13, SWCLK = PA14, GND) da noi dung chua'
    Write-Host '      - board da co nguon chua'
}

exit $rc
