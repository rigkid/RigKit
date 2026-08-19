@echo off
REM Doxygen must finish with an empty warning log (Windows).
REM A warning here means real text is missing from the generated HTML: an
REM unbackticked angle bracket truncates the rest of its comment block, and an
REM unresolved @ref renders as plain prose. Build the docs first:
REM   cmake -S . -B build ^&^& cmake --build build --target docs
setlocal EnableExtensions
cd /d "%~dp0.."

set "BUILD=%~1"
if "%BUILD%"=="" set "BUILD=build"
set "WARN=%BUILD%\docs\api\doxygen.warn"

if not exist "%WARN%" (
  echo FAIL: no %WARN% -- run: cmake --build %BUILD% --target docs
  exit /b 1
)

for %%F in ("%WARN%") do if %%~zF GTR 0 (
  echo FAIL: Doxygen reported warnings:
  type "%WARN%"
  echo.
  echo A dot before punctuation ends the brief and splits any code span around
  echo it: escape the dot outside backticks -- @c rig\.* , not `rig.*`.
  echo Wrap plain ^<angle brackets^> in backticks. Give @ref a target Doxygen
  echo emits: free functions need the full namespace ^(@ref rigkit::plot::foo^),
  echo and an undocumented member or constexpr takes backticks instead.
  echo See skills/rigkit-comments/SKILL.md.
  exit /b 1
)

echo OK: Doxygen warning log is empty
exit /b 0
