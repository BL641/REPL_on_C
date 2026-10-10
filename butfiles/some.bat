@echo off
setlocal
call :find_root
if errorlevel 1 exit /b 1
call :find_exe
if errorlevel 1 exit /b 1
cd /d "%ROOT%"

echo Bat folder: %~dp0
echo Project: %ROOT%
echo Exe: %EXE%
"%EXE%" --vfs from-cli --script "%ROOT%\config\smart.txt" --config "%ROOT%\config\config.yaml"
echo Exit code: %ERRORLEVEL%
pause
exit /b 0

:find_root
set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"
if exist "%ROOT%\config\smart.txt" exit /b 0
for %%I in ("%ROOT%\..") do set "ROOT=%%~fI"
if exist "%ROOT%\config\smart.txt" exit /b 0
echo smart.txt not found in config.
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
