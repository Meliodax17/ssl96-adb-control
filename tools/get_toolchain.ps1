<#
    get_toolchain.ps1 - Tai va giai nen Arm GNU Toolchain vao tools\arm-gnu

    Khong can quyen quan tri, khong dong vao PATH cua he thong. Sau khi chay
    xong, build.ps1 se tu tim thay trinh bien dich o tools\arm-gnu\bin.

    Cach dung:
        powershell -ExecutionPolicy Bypass -File tools\get_toolchain.ps1

    Neu may khong ra Internet, tai file zip o may khac roi chi duong dan:
        powershell -ExecutionPolicy Bypass -File tools\get_toolchain.ps1 -ZipFile D:\arm-gnu.zip

    Dung phien ban khac:
        powershell -ExecutionPolicy Bypass -File tools\get_toolchain.ps1 -Url <dia chi zip>
#>

[CmdletBinding()]
param(
    [string]$Url,
    [string]$ZipFile,
    [switch]$Force        # tai lai du da co san
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$DefaultUrl = 'https://developer.arm.com/-/media/Files/downloads/gnu/13.2.rel1/binrel/arm-gnu-toolchain-13.2.rel1-mingw-w64-i686-arm-none-eabi.zip'
$DownloadPage = 'https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads'

$Dest = Join-Path $PSScriptRoot 'arm-gnu'
$Gcc  = Join-Path $Dest 'bin\arm-none-eabi-gcc.exe'

function Write-Step($msg) { Write-Host "==> $msg" -ForegroundColor Cyan }
function Write-Ok  ($msg) { Write-Host "    $msg" -ForegroundColor Green }
function Write-Err ($msg) { Write-Host "[LOI] $msg" -ForegroundColor Red }

# ---------------------------------------------------------------------------
if ((Test-Path $Gcc) -and -not $Force) {
    Write-Ok 'Toolchain da co san:'
    Write-Ok $Gcc
    & $Gcc '--version' | Select-Object -First 1
    Write-Host ''
    Write-Host ' Chay task "Build: bien dich ra file HEX" de bat dau.'
    exit 0
}

# ---------------------------------------------------------------------------
# 1. Lay file zip
# ---------------------------------------------------------------------------
$tmpZip = $null

if ($ZipFile) {
    if (-not (Test-Path $ZipFile)) { Write-Err "Khong thay file `"$ZipFile`""; exit 1 }
    $tmpZip = (Resolve-Path $ZipFile).Path
    Write-Step 'Dung file zip co san'
    Write-Ok $tmpZip
}
else {
    if (-not $Url) { $Url = $DefaultUrl }
    $tmpZip = Join-Path $env:TEMP 'arm-gnu-toolchain.zip'

    Write-Step 'Tai Arm GNU Toolchain'
    Write-Ok $Url
    Write-Host '    File khoang 400 MB, tuy duong truyen co the mat vai phut...'

    try {
        $ProgressPreference = 'Continue'
        Invoke-WebRequest -Uri $Url -OutFile $tmpZip -UseBasicParsing
    }
    catch {
        Write-Err "Tai that bai: $($_.Exception.Message)"
        Write-Host ''
        Write-Host '    Dia chi tai co the da doi theo phien ban moi. Hay vao trang sau,' -ForegroundColor Yellow
        Write-Host '    tai ban Windows co ten dang arm-gnu-toolchain-<phien ban>-mingw-w64-i686-arm-none-eabi.zip' -ForegroundColor Yellow
        Write-Host ''
        Write-Host "      $DownloadPage" -ForegroundColor White
        Write-Host ''
        Write-Host '    Roi chay lai:' -ForegroundColor Yellow
        Write-Host '      powershell -ExecutionPolicy Bypass -File tools\get_toolchain.ps1 -ZipFile <duong dan file zip>' -ForegroundColor White
        exit 1
    }

    Write-Ok ("Da tai xong, {0:N0} MB" -f ((Get-Item $tmpZip).Length / 1MB))
}

# ---------------------------------------------------------------------------
# 2. Giai nen
# ---------------------------------------------------------------------------
Write-Step 'Giai nen'

if (Test-Path $Dest) { Remove-Item -Recurse -Force $Dest }
$staging = Join-Path $env:TEMP ('arm-gnu-staging-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Force -Path $staging | Out-Null

try {
    Expand-Archive -Path $tmpZip -DestinationPath $staging -Force
}
catch {
    Write-Err "Giai nen that bai: $($_.Exception.Message)"
    Remove-Item -Recurse -Force $staging -ErrorAction SilentlyContinue
    exit 1
}

# File zip cua Arm co mot thu muc goc ben trong; tim thu muc thuc su chua bin\
$found = Get-ChildItem -Path $staging -Recurse -Filter 'arm-none-eabi-gcc.exe' `
                       -ErrorAction SilentlyContinue | Select-Object -First 1
if (-not $found) {
    Write-Err 'Trong file zip khong co arm-none-eabi-gcc.exe. Co the da tai nham ban.'
    Write-Host '    Can ban danh cho Windows (mingw-w64-i686), khong phai ban Linux hay macOS.'
    Remove-Item -Recurse -Force $staging -ErrorAction SilentlyContinue
    exit 1
}

# bin\arm-none-eabi-gcc.exe  ->  thu muc goc cua toolchain la ong ba cua file
$toolRoot = Split-Path -Parent (Split-Path -Parent $found.FullName)
Move-Item -Path $toolRoot -Destination $Dest -Force

Remove-Item -Recurse -Force $staging -ErrorAction SilentlyContinue
if (-not $ZipFile) { Remove-Item -Force $tmpZip -ErrorAction SilentlyContinue }

# ---------------------------------------------------------------------------
# 3. Kiem tra
# ---------------------------------------------------------------------------
if (-not (Test-Path $Gcc)) {
    Write-Err "Sau khi giai nen van khong thay $Gcc"
    exit 1
}

Write-Step 'Kiem tra'
& $Gcc '--version' | Select-Object -First 1
Write-Host ''
Write-Host '--------------------------------------------------------------' -ForegroundColor Green
Write-Host ' CAI DAT XONG' -ForegroundColor Green
Write-Host ("   $Dest")
Write-Host '--------------------------------------------------------------' -ForegroundColor Green
Write-Host ''
Write-Host ' Chay task "Build: bien dich ra file HEX" (Ctrl+Shift+B) de bat dau.'
Write-Host ''
Write-Host ' Thu muc tools\arm-gnu chi phuc vu project nay, khong anh huong'
Write-Host ' toi phan con lai cua may. Xoa thu muc do la go bo hoan toan.'
Write-Host ''
exit 0
