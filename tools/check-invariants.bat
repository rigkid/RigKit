@echo off
REM Windows entry for tools/check-invariants.sh. Prefer Git Bash over the WSL
REM stub at WindowsApps\bash.exe (that stub can hang when WSL is not ready).
setlocal
set "ROOT=%~dp0.."
cd /d "%ROOT%" || exit /b 1

set "BASH="
if exist "C:\Program Files\Git\bin\bash.exe" set "BASH=C:\Program Files\Git\bin\bash.exe"
if not defined BASH if exist "C:\Program Files\Git\usr\bin\bash.exe" set "BASH=C:\Program Files\Git\usr\bin\bash.exe"
if not defined BASH if exist "%LOCALAPPDATA%\Programs\Git\bin\bash.exe" set "BASH=%LOCALAPPDATA%\Programs\Git\bin\bash.exe"
if not defined BASH (
  where bash >nul 2>&1
  if %ERRORLEVEL%==0 for /f "delims=" %%i in ('where bash') do (
    echo %%i | findstr /i "WindowsApps" >nul
    if errorlevel 1 (
      set "BASH=%%i"
      goto :run
    )
  )
)

:run
if not defined BASH (
  echo FAIL: need Git Bash to run tools/check-invariants.sh >&2
  exit /b 1
)

"%BASH%" "%~dp0check-invariants.sh"
exit /b %ERRORLEVEL%
