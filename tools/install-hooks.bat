@echo off
REM Windows entry for tools/install-hooks.sh (Git Bash preferred over WSL stub).
setlocal
set "ROOT=%~dp0.."
cd /d "%ROOT%" || exit /b 1

set "BASH="
if exist "C:\Program Files\Git\bin\bash.exe" set "BASH=C:\Program Files\Git\bin\bash.exe"
if not defined BASH if exist "C:\Program Files\Git\usr\bin\bash.exe" set "BASH=C:\Program Files\Git\usr\bin\bash.exe"
if not defined BASH if exist "%LOCALAPPDATA%\Programs\Git\bin\bash.exe" set "BASH=%LOCALAPPDATA%\Programs\Git\bin\bash.exe"
if not defined BASH (
  echo FAIL: need Git Bash to run tools/install-hooks.sh >&2
  exit /b 1
)

"%BASH%" "%~dp0install-hooks.sh"
exit /b %ERRORLEVEL%
