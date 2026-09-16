@echo off
rem ===========================================================================
rem  build_hex.bat - Bien dich project bang Keil MDK va xuat file HEX
rem
rem  Cach dung:
rem      build_hex.bat            biên dich phan thay doi (build)
rem      build_hex.bat rebuild    bien dich lai toan bo (rebuild all)
rem
rem  Ket qua:
rem      Output\SSL96_ADB_TPS9266x.hex   <- nap bang ST-LINK Utility
rem      Output\SSL96_ADB_TPS9266x.bin
rem      Output\SSL96_ADB_TPS9266x.axf   <- file ELF de debug
rem      build_log.txt                   <- toan bo log bien dich
rem ===========================================================================

setlocal enabledelayedexpansion
cd /d "%~dp0"

set "PROJECT=Project.uvprojx"
set "TARGET=Target 1"
set "OUTDIR=Output"
set "OUTNAME=SSL96_ADB_TPS9266x"
set "LOG=%~dp0build_log.txt"

rem --- 1. Tim UV4.exe --------------------------------------------------------
set "UV4="
for %%P in (
    "C:\Keil_v5\UV4\UV4.exe"
    "C:\Keil\UV4\UV4.exe"
    "C:\Program Files\Keil_v5\UV4\UV4.exe"
    "C:\Program Files (x86)\Keil_v5\UV4\UV4.exe"
    "C:\Program Files (x86)\Keil\UV4\UV4.exe"
    "D:\Keil_v5\UV4\UV4.exe"
    "D:\Keil\UV4\UV4.exe"
) do (
    if exist %%P set "UV4=%%~P"
)

if not defined UV4 (
    for /f "delims=" %%F in ('where UV4.exe 2^>nul') do set "UV4=%%F"
)

if not defined UV4 (
    echo.
    echo [LOI] Khong tim thay UV4.exe - Keil MDK chua duoc cai dat.
    echo.
    echo   Cach 1: Cai Keil MDK-ARM, sau do chay lai script nay.
    echo   Cach 2: Dat duong dan thu cong roi chay lai, vi du:
    echo             set UV4=D:\Keil_v5\UV4\UV4.exe
    echo             build_hex.bat
    echo   Cach 3: Dung Makefile GCC kem theo - xem README_BUILD.md
    echo.
    exit /b 1
)

echo Dung Keil tai : %UV4%

rem --- 2. Kiem tra project --------------------------------------------------
if not exist "%PROJECT%" (
    echo [LOI] Khong thay %PROJECT% trong thu muc %~dp0
    exit /b 1
)

if not exist "%OUTDIR%" mkdir "%OUTDIR%"

rem --- 3. Chon che do build --------------------------------------------------
set "MODE=-b"
set "MODETEXT=build (chi phan thay doi)"
if /i "%~1"=="rebuild" (
    set "MODE=-r"
    set "MODETEXT=rebuild all (bien dich lai toan bo)"
)

rem Xoa file hex cu de khong nham voi ban build truoc
if exist "%OUTDIR%\%OUTNAME%.hex" del /q "%OUTDIR%\%OUTNAME%.hex"

echo Che do       : %MODETEXT%
echo Dang bien dich, vui long doi...
echo.

rem --- 4. Bien dich ----------------------------------------------------------
rem  UV4 tra ve: 0 = khong loi khong canh bao
rem              1 = co canh bao nhung van tao duoc file
rem              >=2 = co loi, khong tao duoc file
"%UV4%" %MODE% "%PROJECT%" -j0 -t"%TARGET%" -o "%LOG%"
set "RC=%ERRORLEVEL%"

if exist "%LOG%" type "%LOG%"

echo.
echo ---------------------------------------------------------------
if %RC% GEQ 2 (
    echo [THAT BAI] UV4 tra ve ma loi %RC%. Xem chi tiet trong build_log.txt
    echo ---------------------------------------------------------------
    exit /b %RC%
)

if not exist "%OUTDIR%\%OUTNAME%.hex" (
    echo [THAT BAI] Khong sinh ra file HEX.
    echo Kiem tra trong Keil: Options for Target ^> Output ^> Create HEX File
    echo ---------------------------------------------------------------
    exit /b 1
)

if %RC%==1 (
    echo [THANH CONG - co canh bao] Xem build_log.txt de biet chi tiet.
) else (
    echo [THANH CONG]
)

for %%F in ("%OUTDIR%\%OUTNAME%.hex") do echo   HEX : %%~fF  (%%~zF byte^)
if exist "%OUTDIR%\%OUTNAME%.bin" (
    for %%F in ("%OUTDIR%\%OUTNAME%.bin") do echo   BIN : %%~fF  (%%~zF byte^)
)
if exist "%OUTDIR%\%OUTNAME%.axf" (
    for %%F in ("%OUTDIR%\%OUTNAME%.axf") do echo   ELF : %%~fF  (%%~zF byte^)
)
echo ---------------------------------------------------------------
echo.
echo Nap chip: dung ST-LINK Utility mo file HEX o tren, hoac chay
echo   flash_stlink.bat
echo.

endlocal
exit /b 0
