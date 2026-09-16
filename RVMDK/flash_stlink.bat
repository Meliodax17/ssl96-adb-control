@echo off
rem ===========================================================================
rem  flash_stlink.bat - Nap file HEX vao STM32F103C8T6 bang ST-LINK
rem
rem  Cach dung:
rem      flash_stlink.bat                  nap Output\SSL96_ADB_TPS9266x.hex
rem      flash_stlink.bat duong_dan.hex    nap file chi dinh
rem
rem  Yeu cau: da cai "STM32 ST-LINK Utility" (co ST-LINK_CLI.exe)
rem  va ST-LINK da cam vao may, noi toi chan SWD cua board.
rem ===========================================================================

setlocal
cd /d "%~dp0"

set "HEXFILE=%~1"
if "%HEXFILE%"=="" set "HEXFILE=Output\SSL96_ADB_TPS9266x.hex"

if not exist "%HEXFILE%" (
    echo [LOI] Khong thay file "%HEXFILE%"
    echo Hay chay build_hex.bat truoc.
    exit /b 1
)

rem --- Tim ST-LINK_CLI.exe --------------------------------------------------
set "CLI="
for %%P in (
    "C:\Program Files (x86)\STMicroelectronics\STM32 ST-LINK Utility\ST-LINK Utility\ST-LINK_CLI.exe"
    "C:\Program Files\STMicroelectronics\STM32 ST-LINK Utility\ST-LINK Utility\ST-LINK_CLI.exe"
) do (
    if exist %%P set "CLI=%%~P"
)

if not defined CLI (
    for /f "delims=" %%F in ('where ST-LINK_CLI.exe 2^>nul') do set "CLI=%%F"
)

if not defined CLI (
    echo [LOI] Khong tim thay ST-LINK_CLI.exe.
    echo Cai "STM32 ST-LINK Utility", hoac mo file HEX bang giao dien do thay cho script nay:
    echo   %HEXFILE%
    exit /b 1
)

echo Dung ST-LINK CLI tai : %CLI%
echo Nap file             : %HEXFILE%
echo.

"%CLI%" -c SWD UR -P "%HEXFILE%" -V "while_programming" -Rst
set "RC=%ERRORLEVEL%"

echo.
if %RC%==0 (
    echo [THANH CONG] Da nap va reset chip.
) else (
    echo [THAT BAI] ST-LINK_CLI tra ve ma loi %RC%.
    echo Kiem tra: ST-LINK da cam chua, day SWD dung chua, board da co nguon chua.
)

endlocal
exit /b %RC%
