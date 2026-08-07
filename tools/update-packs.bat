@echo off
setlocal
REM Windows entry point for tools/update-packs.sh (Git Bash or MSYS).
set ROOT=%~dp0..
where bash >nul 2>&1
if errorlevel 1 (
	echo bash not found. Run from Git Bash: ./tools/update-packs.sh %*
	exit /b 1
)
bash "%ROOT%\tools\update-packs.sh" %*
exit /b %ERRORLEVEL%
