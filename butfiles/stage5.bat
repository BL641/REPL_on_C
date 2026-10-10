@echo off
setlocal
call :find_root
if errorlevel 1 exit /b 1
call :find_exe
if errorlevel 1 exit /b 1
cd /d "%ROOT%"

echo Exe: %EXE%
"%EXE%" --vfs "%ROOT%\vfs\deep" --script "%ROOT%\config\stage5_start.txt" --config "%ROOT%\config\stage5.yaml"
echo ----- disk after touch -----
dir /s /b "%ROOT%\vfs\deep" | findstr /i /c:"created.txt" /c:"extra.txt" /c:"note.txt" /c:"second.txt"
if errorlevel 1 echo These files are not on disk. touch changed only memory.
pause
exit /b 0

:find_root
set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"
if exist "%ROOT%\config\stage5_start.txt" exit /b 0
for %%I in ("%ROOT%\..") do set "ROOT=%%~fI"
if exist "%ROOT%\config\stage5_start.txt" exit /b 0
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
