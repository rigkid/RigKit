@echo off
setlocal EnableExtensions
REM Generate pack-scoped Doxygen HTML. Prefer Git Bash / WSL for CI parity.
REM Usage: tools\generate-pack-docs.bat <packName> [hostRoot] [outDir]

set "PACK_NAME=%~1"
if "%PACK_NAME%"=="" (
	echo Usage: %~nx0 ^<packName^> [hostRoot] [outDir]
	exit /b 1
)

set "SCRIPT_DIR=%~dp0"
set "DEFAULT_ROOT=%SCRIPT_DIR%.."
if "%~2"=="" (set "RIGKIT_ROOT=%DEFAULT_ROOT%") else (set "RIGKIT_ROOT=%~2")
for %%I in ("%RIGKIT_ROOT%") do set "RIGKIT_ROOT=%%~fI"

if "%~3"=="" (
	set "RIGKIT_DOXY_OUT=%RIGKIT_ROOT%\build\docs\pack\%PACK_NAME%"
) else (
	set "RIGKIT_DOXY_OUT=%~3"
)

where bash >nul 2>&1
if errorlevel 1 (
	echo bash not found — install Git for Windows or run tools/generate-pack-docs.sh
	exit /b 1
)

bash "%SCRIPT_DIR%generate-pack-docs.sh" "%PACK_NAME%" "%RIGKIT_ROOT%" "%RIGKIT_DOXY_OUT%"
exit /b %ERRORLEVEL%
