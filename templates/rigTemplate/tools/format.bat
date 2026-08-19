@echo off
REM Format first-party C/C++ (src, examples). Skips third_party.
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0.."

set "CLANG_FORMAT="
where clang-format >nul 2>&1 && set "CLANG_FORMAT=clang-format"

if not defined CLANG_FORMAT if exist "%ProgramFiles%\LLVM\bin\clang-format.exe" (
	set "CLANG_FORMAT=%ProgramFiles%\LLVM\bin\clang-format.exe"
)
if not defined CLANG_FORMAT if exist "%ProgramFiles%\Microsoft Visual Studio\18\Community\VC\Tools\Llvm\x64\bin\clang-format.exe" (
	set "CLANG_FORMAT=%ProgramFiles%\Microsoft Visual Studio\18\Community\VC\Tools\Llvm\x64\bin\clang-format.exe"
)
if not defined CLANG_FORMAT if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\Community\VC\Tools\Llvm\x64\bin\clang-format.exe" (
	set "CLANG_FORMAT=%ProgramFiles(x86)%\Microsoft Visual Studio\2022\Community\VC\Tools\Llvm\x64\bin\clang-format.exe"
)
if not defined CLANG_FORMAT if exist "C:\msys64\ucrt64\bin\clang-format.exe" (
	set "CLANG_FORMAT=C:\msys64\ucrt64\bin\clang-format.exe"
)

if not defined CLANG_FORMAT (
	echo clang-format not found. Install LLVM or Visual Studio LLVM tools, or add clang-format to PATH.
	exit /b 1
)

echo Using: %CLANG_FORMAT%
set COUNT=0
for /r src %%F in (*.c *.cpp *.h *.hpp) do (
	echo %%F | findstr /i /c:"\third_party\" >nul || (
		"%CLANG_FORMAT%" -i "%%F"
		set /a COUNT+=1
	)
)
if exist examples (
	for /r examples %%F in (*.c *.cpp *.h *.hpp) do (
		echo %%F | findstr /i /c:"\third_party\" >nul || (
			"%CLANG_FORMAT%" -i "%%F"
			set /a COUNT+=1
		)
	)
)

echo Formatted !COUNT! files.
endlocal
