@echo off
REM Windows entry point for tools/pack-status.sh (Git Bash or MSYS).
set "ROOT=%~dp0.."
where bash >nul 2>&1
if errorlevel 1 (
	echo bash not found. Run from Git Bash: ./tools/pack-status.sh %*
	exit /b 1
)
bash "%ROOT%\tools\pack-status.sh" %*
