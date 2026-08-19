@echo off
setlocal
cd /d "%~dp0.."

set "BASH="
where bash >nul 2>&1 && set "BASH=bash"
if not defined BASH if exist "%ProgramFiles%\Git\bin\bash.exe" set "BASH=%ProgramFiles%\Git\bin\bash.exe"
if not defined BASH if exist "%ProgramFiles%\Git\usr\bin\bash.exe" set "BASH=%ProgramFiles%\Git\usr\bin\bash.exe"

if not defined BASH (
	echo Git Bash not found. Install Git for Windows, then run tools\install-hooks.bat
	exit /b 1
)

"%BASH%" "%~dp0install-hooks.sh"
exit /b %ERRORLEVEL%
