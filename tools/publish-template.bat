@echo off
setlocal
set ROOT=%~dp0..
where bash >nul 2>&1
if errorlevel 1 (
	echo bash not found. Run from Git Bash: ./tools/publish-template.sh %*
	exit /b 1
)
bash "%ROOT%\tools\publish-template.sh" %*
exit /b %ERRORLEVEL%
