<#
    build.ps1 - Bien dich firmware SSL 96-pixel ADB ra file HEX

    Khong can make, khong can CMake. Script tu goi arm-none-eabi-gcc cho tung
    file nguon roi link lai. Duoc goi tu task cua Antigravity IDE / VS Code,
    cung co the chay truc tiep:

        powershell -ExecutionPolicy Bypass -File tools\build.ps1
        powershell -ExecutionPolicy Bypass -File tools\build.ps1 -Rebuild
        powershell -ExecutionPolicy Bypass -File tools\build.ps1 -Clean

    Ket qua: build\SSL96_ADB_TPS9266x.hex
#>

[CmdletBinding()]
param(
    [switch]$Rebuild,      # bien dich lai tat ca, bo qua kiem tra thoi gian
    [switch]$Clean,        # chi xoa thu muc build roi thoat
    [switch]$UseKeil,      # ep dung Keil MDK thay vi arm-none-eabi-gcc
    [string]$ToolchainDir, # thu muc bin cua arm-none-eabi, neu khong co trong PATH
    [switch]$Subst,        # tu tao o dia ao de rut ngan duong dan (xem muc MAX_PATH)
    [switch]$NoSubst       # dung noi bo, tranh goi de quy
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

# Thu muc goc cua project = thu muc cha cua tools\
$Root   = Split-Path -Parent $PSScriptRoot
$Build  = Join-Path $Root 'build'
$ObjDir = Join-Path $Build 'obj'
$Target = 'SSL96_ADB_TPS9266x'

function Write-Step($msg) { Write-Host "==> $msg" -ForegroundColor Cyan }
function Write-Ok  ($msg) { Write-Host "    $msg" -ForegroundColor Green }
function Write-Warn2($msg){ Write-Host "    $msg" -ForegroundColor Yellow }
function Write-Err ($msg) { Write-Host "[LOI] $msg" -ForegroundColor Red }

# ---------------------------------------------------------------------------
# Clean
# ---------------------------------------------------------------------------
if ($Clean) {
    if (Test-Path $Build) {
        Remove-Item -Recurse -Force $Build
        Write-Ok "Da xoa $Build"
    } else {
        Write-Ok 'Khong co gi de xoa.'
    }
    exit 0
}

# ---------------------------------------------------------------------------
# 0. Kiem tra gioi han do dai duong dan cua Windows (MAX_PATH = 260)
#
#    Duong dan file dai qua 260 ky tu thi cac chuong trinh Win32 doi (bao gom
#    arm-none-eabi-gcc.exe, armcc.exe cua Keil, va ca PowerShell 5.1) khong mo
#    duoc file, du file van nam do. Bien dich se that bai voi thong bao kieu
#    "No such file or directory" rat kho hieu.
#
#    File nguon sau day co duong dan dai nhat trong project.
# ---------------------------------------------------------------------------
$LongestRel = 'Libraries\CMSIS\CM3\DeviceSupport\ST\STM32F10x\startup\gcc_ride7\startup_stm32f10x_md.s'
$longestLen = (Join-Path $Root $LongestRel).Length

function Invoke-ViaSubstDrive {
    # Tao mot o dia ao tro toi thu muc goc, roi chay lai chinh script nay qua
    # o dia do. Duong dan khi ay chi con dang "X:\tools\build.ps1" nen khong
    # con vuot MAX_PATH nua. Go bo o dia ao truoc khi thoat.
    $letter = $null
    foreach ($c in [char[]]'ZYXWVUTSRQP') {
        $d = "${c}:"
        # Khong dung Test-Path vi o dia bi han che quyen se nem loi
        if (-not ([System.IO.DriveInfo]::GetDrives().Name -contains "$d\")) { $letter = $d; break }
    }
    if (-not $letter) {
        Write-Err 'Khong con chu cai o dia trong de tao o dia ao.'
        return 1
    }

    Write-Step "Tao o dia ao $letter tro toi thu muc project"
    & subst.exe $letter $Root
    if ($LASTEXITCODE -ne 0) {
        Write-Err "Tao o dia ao that bai (ma loi $LASTEXITCODE)."
        return 1
    }
    Write-Ok "$letter  ->  $Root"

    try {
        $childArgs = @('-NoProfile', '-ExecutionPolicy', 'Bypass',
                       '-File', "$letter\tools\build.ps1", '-NoSubst')
        if ($Rebuild)      { $childArgs += '-Rebuild' }
        if ($UseKeil)      { $childArgs += '-UseKeil' }
        if ($ToolchainDir) { $childArgs += @('-ToolchainDir', $ToolchainDir) }

        & powershell @childArgs
        $rc = $LASTEXITCODE
    }
    finally {
        & subst.exe $letter '/D' | Out-Null
        Write-Ok "Da go o dia ao $letter"
    }
    return $rc
}

if ($longestLen -ge 260 -and -not $NoSubst) {
    Write-Host ''
    Write-Err "Duong dan project qua dai so voi gioi han cua Windows."
    Write-Host ("    Thu muc goc      : {0} ky tu" -f $Root.Length)
    Write-Host ("    File sau day dai : {0} ky tu  (gioi han la 260)" -f $longestLen) -ForegroundColor Yellow
    Write-Host ("      ...\{0}" -f $LongestRel)
    Write-Host ''

    if ($Subst) {
        Write-Host '    Dang xu ly tam bang o dia ao...' -ForegroundColor Yellow
        Write-Host ''
        exit (Invoke-ViaSubstDrive)
    }

    Write-Host '  CACH SUA TRIET DE (nen lam):' -ForegroundColor Yellow
    Write-Host '    Chuyen ca thu muc project toi mot duong dan ngan, vi du:'
    Write-Host ''
    Write-Host '        C:\SSL96' -ForegroundColor White
    Write-Host ''
    Write-Host '    Sau do mo lai thu muc do trong IDE. Keil cung gap loi nay neu'
    Write-Host '    duong dan qua dai, nen chuyen di la giai quyet duoc ca hai.'
    Write-Host ''
    Write-Host '  CACH TAM THOI (khong can di chuyen gi):' -ForegroundColor Yellow
    Write-Host '    Chay task "Build: bien dich qua o dia ao", hoac lenh:'
    Write-Host ''
    Write-Host '        powershell -ExecutionPolicy Bypass -File tools\build.ps1 -Subst' -ForegroundColor White
    Write-Host ''
    Write-Host '    Lenh nay tao tam mot o dia ao tro toi project, bien dich qua'
    Write-Host '    o dia do roi go bo. Khong sua doi gi tren dia.'
    Write-Host ''
    exit 1
}

# ---------------------------------------------------------------------------
# 1. Tim trinh bien dich
# ---------------------------------------------------------------------------
function Find-ArmGcc {
    param([string]$Hint)

    $candidates = @()
    if ($Hint) {
        $candidates += (Join-Path $Hint 'arm-none-eabi-gcc.exe')
    }
    # Ban toolchain tai ve boi tools\get_toolchain.ps1
    $candidates += (Join-Path $PSScriptRoot 'arm-gnu\bin\arm-none-eabi-gcc.exe')

    foreach ($c in $candidates) {
        if (Test-Path $c) { return (Resolve-Path $c).Path }
    }

    $cmd = Get-Command 'arm-none-eabi-gcc.exe' -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }

    # Cac vi tri cai dat thong thuong.
    # Chi do dung hai muc: <thu muc cai dat>\bin\ va <thu muc cai dat>\*\bin\
    # Khong quet de quy toan bo Program Files, vi nhu vay co the mat vai phut
    # moi ket luan duoc la "khong co trinh bien dich".
    $roots = @(
        "$env:ProgramFiles\Arm GNU Toolchain arm-none-eabi",
        "${env:ProgramFiles(x86)}\Arm GNU Toolchain arm-none-eabi",
        "$env:ProgramFiles\GNU Arm Embedded Toolchain",
        "${env:ProgramFiles(x86)}\GNU Arm Embedded Toolchain",
        "$env:LOCALAPPDATA\Programs\Arm GNU Toolchain arm-none-eabi",
        'C:\gcc-arm-none-eabi',
        'C:\arm-gnu-toolchain'
    )

    foreach ($r in $roots) {
        if (-not (Test-Path -LiteralPath $r)) { continue }

        $direct = Join-Path $r 'bin\arm-none-eabi-gcc.exe'
        if (Test-Path -LiteralPath $direct) { return $direct }

        # Ban cai dat thuong tao them mot lop thu muc theo phien ban
        $sub = Get-ChildItem -LiteralPath $r -Directory -ErrorAction SilentlyContinue
        foreach ($d in $sub) {
            $p = Join-Path $d.FullName 'bin\arm-none-eabi-gcc.exe'
            if (Test-Path -LiteralPath $p) { return $p }
        }
    }
    return $null
}

function Find-Uv4 {
    $paths = @(
        'C:\Keil_v5\UV4\UV4.exe', 'C:\Keil\UV4\UV4.exe',
        'D:\Keil_v5\UV4\UV4.exe', 'D:\Keil\UV4\UV4.exe',
        "$env:ProgramFiles\Keil_v5\UV4\UV4.exe",
        "${env:ProgramFiles(x86)}\Keil_v5\UV4\UV4.exe",
        "${env:ProgramFiles(x86)}\Keil\UV4\UV4.exe"
    )
    foreach ($p in $paths) { if (Test-Path $p) { return $p } }
    $cmd = Get-Command 'UV4.exe' -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    return $null
}

# ---------------------------------------------------------------------------
# Duong build bang Keil (neu duoc chon hoac khong co arm gcc)
# ---------------------------------------------------------------------------
function Invoke-KeilBuild {
    param([string]$Uv4, [switch]$Full)

    $proj = Join-Path $Root 'RVMDK\Project.uvprojx'
    $log  = Join-Path $Root 'RVMDK\build_log.txt'
    $hex  = Join-Path $Root "RVMDK\Output\$Target.hex"

    if (-not (Test-Path $proj)) { Write-Err "Khong thay $proj"; exit 1 }
    if (Test-Path $hex) { Remove-Item -Force $hex }

    $mode = if ($Full) { '-r' } else { '-b' }
    Write-Step "Bien dich bang Keil MDK ($Uv4)"

    & $Uv4 $mode $proj '-j0' '-t' 'Target 1' '-o' $log | Out-Null
    $rc = $LASTEXITCODE

    if (Test-Path $log) { Get-Content $log | Write-Host }

    if ($rc -ge 2)        { Write-Err "UV4 tra ve ma loi $rc. Xem RVMDK\build_log.txt"; exit $rc }
    if (-not (Test-Path $hex)) { Write-Err 'Khong sinh ra file HEX.'; exit 1 }

    if ($rc -eq 1) { Write-Warn2 'Build xong nhung co canh bao.' }
    Write-Ok "HEX: $hex"
    exit 0
}

$gcc = $null
if (-not $UseKeil) { $gcc = Find-ArmGcc -Hint $ToolchainDir }

if ($UseKeil -or -not $gcc) {
    $uv4 = Find-Uv4
    if ($uv4) {
        Invoke-KeilBuild -Uv4 $uv4 -Full:$Rebuild
    }
    if ($UseKeil) { Write-Err 'Da chon -UseKeil nhung khong tim thay UV4.exe.'; exit 1 }

    Write-Err 'Khong tim thay trinh bien dich nao tren may.'
    Write-Host ''
    Write-Host '  Can mot trong hai thu sau:' -ForegroundColor Yellow
    Write-Host ''
    Write-Host '  A. Arm GNU Toolchain (mien phi, khuyen nghi). Chay lenh nay de tai'
    Write-Host '     va giai nen ngay trong project, khong can quyen quan tri:'
    Write-Host ''
    Write-Host '         powershell -ExecutionPolicy Bypass -File tools\get_toolchain.ps1' -ForegroundColor White
    Write-Host ''
    Write-Host '     Hoac tai thu cong tai:'
    Write-Host '     https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads'
    Write-Host ''
    Write-Host '  B. Keil MDK-ARM. Cai xong chay lai script nay, no tu nhan ra.'
    Write-Host ''
    exit 1
}

$binDir  = Split-Path -Parent $gcc
$objcopy = Join-Path $binDir 'arm-none-eabi-objcopy.exe'
$sizeExe = Join-Path $binDir 'arm-none-eabi-size.exe'

Write-Step 'Trinh bien dich'
Write-Ok $gcc
$verLine = (& $gcc '--version' | Select-Object -First 1)
Write-Ok $verLine

# ---------------------------------------------------------------------------
# 2. Danh sach ma nguon
# ---------------------------------------------------------------------------
$sources = @(
    'User\main.c'
    'User\tps9266x.c'
    'User\adb_dimming.c'
    'User\tps_diag.c'
    'User\stm32f10x_it.c'
    'User\system_stm32f10x.c'
    'User\uart.c'
    'User\delay.c'
    'User\spi.c'
    'User\I2C.c'
    'User\Command.c'
    'User\usbio.c'
    'User\USB\hw_config.c'
    'User\USB\usb_desc.c'
    'User\USB\usb_endp.c'
    'User\USB\usb_istr.c'
    'User\USB\usb_prop.c'
    'User\USB\usb_pwr.c'
    'Libraries\CMSIS\CM3\CoreSupport\core_cm3.c'
    'Libraries\STM32F10x_StdPeriph_Driver\src\misc.c'
    'Libraries\STM32F10x_StdPeriph_Driver\src\stm32f10x_exti.c'
    'Libraries\STM32F10x_StdPeriph_Driver\src\stm32f10x_gpio.c'
    'Libraries\STM32F10x_StdPeriph_Driver\src\stm32f10x_rcc.c'
    'Libraries\STM32F10x_StdPeriph_Driver\src\stm32f10x_rtc.c'
    'Libraries\STM32F10x_StdPeriph_Driver\src\stm32f10x_flash.c'
    'Libraries\STM32F10x_StdPeriph_Driver\src\stm32f10x_spi.c'
    'Libraries\STM32F10x_StdPeriph_Driver\src\stm32f10x_usart.c'
    'Libraries\STM32F10x_StdPeriph_Driver\src\stm32f10x_tim.c'
    'Libraries\STM32F10x_StdPeriph_Driver\src\stm32f10x_dma.c'
    'Libraries\STM32_USB-FS-Device_Driver\src\usb_core.c'
    'Libraries\STM32_USB-FS-Device_Driver\src\usb_init.c'
    'Libraries\STM32_USB-FS-Device_Driver\src\usb_int.c'
    'Libraries\STM32_USB-FS-Device_Driver\src\usb_mem.c'
    'Libraries\STM32_USB-FS-Device_Driver\src\usb_regs.c'
    'Libraries\STM32_USB-FS-Device_Driver\src\usb_sil.c'
)
# Ghi chu: bon file otgd_fs_*.c chi danh cho STM32F105/107, toan bo noi dung
# nam trong #ifdef STM32F10X_CL nen khong can bien dich.

$asmSources = @(
    'Libraries\CMSIS\CM3\DeviceSupport\ST\STM32F10x\startup\gcc_ride7\startup_stm32f10x_md.s'
)

$includes = @(
    'User'
    'User\USB\inc'
    'Libraries\CMSIS\CM3\DeviceSupport\ST\STM32F10x'
    'Libraries\STM32F10x_StdPeriph_Driver\inc'
    'Libraries\STM32_USB-FS-Device_Driver\inc'
    'Libraries\DSP_Lib\Include'
)

$ldScript = Join-Path $Root 'gcc\STM32F103C8_FLASH.ld'

# Kiem tra file ton tai truoc khi bien dich, bao loi som va ro rang
$missing = @()
foreach ($s in ($sources + $asmSources)) {
    if (-not (Test-Path (Join-Path $Root $s))) { $missing += $s }
}
if (-not (Test-Path $ldScript)) { $missing += 'gcc\STM32F103C8_FLASH.ld' }
if ($missing.Count -gt 0) {
    Write-Err 'Thieu file nguon:'
    $missing | ForEach-Object { Write-Host "    $_" -ForegroundColor Red }
    exit 1
}

# ---------------------------------------------------------------------------
# 3. Co bien dich
# ---------------------------------------------------------------------------
$cpu   = @('-mcpu=cortex-m3', '-mthumb')
$defs  = @('-DUSE_STDPERIPH_DRIVER', '-DSTM32F10X_MD')
$incFlags = $includes | ForEach-Object { "-I$(Join-Path $Root $_)" }

$cflags = $cpu + $defs + $incFlags + @(
    '-Og', '-g3',
    '-Wall', '-Wno-unused-parameter',
    '-ffunction-sections', '-fdata-sections',
    '-fno-common', '-fmessage-length=0'
)

$ldflags = $cpu + @(
    "-T$ldScript",
    '--specs=nano.specs', '--specs=nosys.specs',
    '-nostartfiles',
    '-Wl,--gc-sections',
    "-Wl,-Map=$(Join-Path $Build "$Target.map"),--cref"
)

# ---------------------------------------------------------------------------
# 4. Bien dich
# ---------------------------------------------------------------------------
if ($Rebuild -and (Test-Path $Build)) {
    Remove-Item -Recurse -Force $Build
    Write-Ok 'Da xoa ket qua build cu.'
}
New-Item -ItemType Directory -Force -Path $ObjDir | Out-Null

Write-Step "Bien dich $($sources.Count + $asmSources.Count) file nguon"

$objects  = @()
$compiled = 0
$skipped  = 0
$failed   = 0
$warned   = 0

# Goi mot chuong trinh ngoai va thu gom ca stdout lan stderr.
#
# Windows PowerShell 5.1 boc moi dong stderr cua chuong trinh ngoai vao mot
# ErrorRecord; khi $ErrorActionPreference = 'Stop' thi chi mot dong canh bao
# cua gcc cung lam dung ca script. Vi vay o day ha xuong 'Continue' va chi
# dua vao ma thoat de biet thanh cong hay that bai.
function Invoke-Native {
    param([string]$Exe, [string[]]$Arguments)

    $prevEAP = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        $text = (& $Exe @Arguments 2>&1 | ForEach-Object { "$_" }) -join [Environment]::NewLine
        $code = $LASTEXITCODE
    }
    finally {
        $ErrorActionPreference = $prevEAP
    }
    return [pscustomobject]@{ Output = $text; ExitCode = $code }
}

function Get-ObjName([string]$relPath) {
    # Thay \ va / bang _ de hai file cung ten o hai thu muc khong de len nhau
    $flat = $relPath -replace '[\\/]', '_'
    return ($flat -replace '\.(c|s)$', '.o')
}

foreach ($src in ($sources + $asmSources)) {
    $srcFull = Join-Path $Root $src
    $objFull = Join-Path $ObjDir (Get-ObjName $src)
    $objects += $objFull

    # Bien dich lai neu file .o chua co hoac cu hon file nguon
    if ((Test-Path $objFull) -and
        ((Get-Item $objFull).LastWriteTimeUtc -ge (Get-Item $srcFull).LastWriteTimeUtc)) {
        $skipped++
        continue
    }

    $name = Split-Path -Leaf $src
    Write-Host ("    CC  {0}" -f $name)

    $r = Invoke-Native -Exe $gcc -Arguments ($cflags + @('-c', $srcFull, '-o', $objFull))
    if ($r.ExitCode -ne 0) {
        $failed++
        Write-Host $r.Output -ForegroundColor Red
        if (Test-Path $objFull) { Remove-Item -Force $objFull }
    } elseif ($r.Output) {
        # Bien dich duoc nhung co canh bao
        Write-Host $r.Output -ForegroundColor Yellow
        $warned++
        $compiled++
    } else {
        $compiled++
    }
}

if ($failed -gt 0) {
    Write-Host ''
    Write-Err "$failed file bien dich that bai. Xem loi o tren."
    exit 1
}

Write-Ok "$compiled file bien dich moi, $skipped file khong doi."
if ($warned -gt 0) { Write-Warn2 "$warned file co canh bao (mau vang o tren)." }

# ---------------------------------------------------------------------------
# 5. Link
# ---------------------------------------------------------------------------
$elf = Join-Path $Build "$Target.elf"
$hex = Join-Path $Build "$Target.hex"
$bin = Join-Path $Build "$Target.bin"

Write-Step 'Link'
# -lm: thu vien toan hoc, can cho logf() trong phep quy doi NTC -> do C.
# Phai dat SAU danh sach object thi trinh lien ket moi tim thay ky hieu.
$r = Invoke-Native -Exe $gcc -Arguments ($objects + $ldflags + @('-o', $elf, '-lm'))
if ($r.ExitCode -ne 0) {
    Write-Host $r.Output -ForegroundColor Red
    Write-Err 'Link that bai.'
    exit 1
}
if ($r.Output) { Write-Host $r.Output -ForegroundColor Yellow }
Write-Ok (Split-Path -Leaf $elf)

# ---------------------------------------------------------------------------
# 6. Xuat HEX va BIN
# ---------------------------------------------------------------------------
Write-Step 'Xuat HEX / BIN'
$r = Invoke-Native -Exe $objcopy -Arguments @('-O', 'ihex', $elf, $hex)
if ($r.ExitCode -ne 0) { Write-Host $r.Output -ForegroundColor Red; Write-Err 'objcopy sang HEX that bai.'; exit 1 }
$r = Invoke-Native -Exe $objcopy -Arguments @('-O', 'binary', '-S', $elf, $bin)
if ($r.ExitCode -ne 0) { Write-Host $r.Output -ForegroundColor Red; Write-Err 'objcopy sang BIN that bai.'; exit 1 }

# ---------------------------------------------------------------------------
# 7. Dung luong
# ---------------------------------------------------------------------------
$FLASH_KB = 64
$RAM_KB   = 20

if (Test-Path $sizeExe) {
    $sz = (Invoke-Native -Exe $sizeExe -Arguments @($elf)).Output -split "`r?`n"
    $cols = @(($sz | Select-Object -Last 1) -split '\s+' | Where-Object { $_ -ne '' })
    if ($cols.Count -ge 3) {
        $text = [int]$cols[0]; $data = [int]$cols[1]; $bss = [int]$cols[2]
        $flashUsed = $text + $data
        $ramUsed   = $data + $bss
        $flashPct  = [math]::Round(100.0 * $flashUsed / ($FLASH_KB * 1024), 1)
        $ramPct    = [math]::Round(100.0 * $ramUsed   / ($RAM_KB   * 1024), 1)

        Write-Step 'Dung luong STM32F103C8T6'
        Write-Host ("    FLASH : {0,6} / {1,6} byte  ({2}%)" -f $flashUsed, ($FLASH_KB*1024), $flashPct)
        Write-Host ("    RAM   : {0,6} / {1,6} byte  ({2}%)" -f $ramUsed,   ($RAM_KB*1024),   $ramPct)

        if ($flashPct -gt 100) { Write-Err 'Tran FLASH.'; exit 1 }
        if ($ramPct   -gt 100) { Write-Err 'Tran RAM.';   exit 1 }
        if ($flashPct -gt 90 -or $ramPct -gt 90) { Write-Warn2 'Da dung tren 90% bo nho.' }
    }
}

Write-Host ''
Write-Host '--------------------------------------------------------------' -ForegroundColor Green
Write-Host ' BUILD THANH CONG' -ForegroundColor Green
Write-Host ("   HEX : {0}  ({1:N0} byte)" -f $hex, (Get-Item $hex).Length)
Write-Host ("   BIN : {0}  ({1:N0} byte)" -f $bin, (Get-Item $bin).Length)
Write-Host ("   ELF : {0}" -f $elf)
Write-Host '--------------------------------------------------------------' -ForegroundColor Green
Write-Host ''
Write-Host ' Nap chip: chay task "Flash: nap chip bang ST-LINK"'
Write-Host ''
exit 0
