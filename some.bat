@echo off
cd /d "%~dp0"
echo Bat folder: %cd%
echo Exe: %cd%\x64\Debug\REPL.exe
if not exist "%cd%\x64\Debug\REPL.exe" (
    echo REPL.exe not found
    pause
    exit /b 1
)
if not exist "%cd%\smart.txt" (
    echo smart.txt not found
    pause
    exit /b 1
)
"%cd%\x64\Debug\REPL.exe" --vfs from-cli --script "%cd%\smart.txt" --config "%cd%\config.yaml"
echo Exit code: %ERRORLEVEL%
pause