chcp 65001 >nul
@echo off
setlocal
call :find_root
if errorlevel 1 exit /b 1
call :find_exe
if errorlevel 1 exit /b 1
cd /d "%ROOT%"

call :variant "%ROOT%\vfs\minimal" minimal
call :variant "%ROOT%\vfs\files" files
call :variant "%ROOT%\vfs\deep" deep
pause
exit /b 0

:variant
set "SRC=%~1"
set "DEST=%TEMP%\emu_vfs_%RANDOM%%RANDOM%"
if not exist "%SRC%\." (
    echo VFS folder not found: %SRC%
    exit /b 0
)
mkdir "%DEST%"
xcopy "%SRC%\*" "%DEST%" /E /I /Q /Y >nul
echo.
echo ===== VFS %~2 =====
echo Exe: %EXE%
"%EXE%" --vfs "%DEST%" --script "%ROOT%\config\stage3_start.txt" --config "%ROOT%\config\stage3.yaml"
echo ----- disk after vfs-init -----
dir /b /a "%DEST%"
rmdir /s /q "%DEST%"
exit /b 0

:find_root
set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"
if exist "%ROOT%\config\stage3_start.txt" exit /b 0
for %%I in ("%ROOT%\..") do set "ROOT=%%~fI"
if exist "%ROOT%\config\stage3_start.txt" exit /b 0
echo Project folder not found. Put this bat in butfiles or next to config.
pause
exit /b 1

:find_exe
set "EXE="
for %%P in (
    "%ROOT%\x64\Debug\REPLV2.exe"
    "%ROOT%\x64\Debug\REPL.exe"
    "%ROOT%\..\x64\Debug\REPLV2.exe"
    "%ROOT%\..\x64\Debug\REPL.exe"
) do (
    if not defined EXE if exist "%%~P" set "EXE=%%~fP"
)
if not defined EXE (
    echo REPL exe not found.
    echo Looked near %ROOT%\x64\Debug and the folder above it.
    echo Build Debug x64 in Visual Studio first.
    pause
    exit /b 1
)
exit /b 0
