@echo off
REM Machine-checkable slices of the Ten Commandments (Windows).
setlocal EnableExtensions
cd /d "%~dp0.."

set "FAIL=0"

REM --- 8. No ImGui in src/ ---
for /r src %%F in (*.c *.cpp *.h *.hpp) do (
  findstr /I /R /C:"include.*imgui" "%%F" >nul 2>&1 && (
    echo FAIL: ImGui include in src/ ^(%%F^)
    set FAIL=1
  )
  findstr /C:"ImGui::" "%%F" >nul 2>&1 && (
    echo FAIL: ImGui:: call in src/ ^(%%F^)
    set FAIL=1
  )
)

REM --- 4. rigComponent = data ---
if exist packs\rigComponent\src (
  for /r packs\rigComponent\src %%F in (S*.cpp S*.h *System*.cpp *System*.h *Systems*.cpp *Systems*.h) do (
    if exist "%%F" (
      echo FAIL: systems-shaped file in rigComponent ^(%%F^)
      set FAIL=1
    )
  )
)

REM --- 6. Positive vocabulary misuse ---
where rg >nul 2>&1
if errorlevel 1 (
  echo WARN: rg not on PATH — skipping vocabulary / archaeology sweeps on Windows.
  echo       Install ripgrep, or run tools\check-invariants.sh via Git Bash.
) else (
  rg -n --glob "!**/third_party/**" --glob "!**/build/**" --glob "!**/check-invariants.*" -e "addons/" -e "checkout_addon" -e "\bIAddon\b" -e "\bMAddon\b" -e "\bAddonRegistry\b" AGENTS.md docs .cursor\rules .cursor\skills skills src examples tools cmake templates >nul 2>&1 && (
    echo FAIL: banned vocabulary ^(use pack / packs/^)
    rg -n --glob "!**/third_party/**" --glob "!**/build/**" --glob "!**/check-invariants.*" -e "addons/" -e "checkout_addon" -e "\bIAddon\b" -e "\bMAddon\b" -e "\bAddonRegistry\b" AGENTS.md docs .cursor\rules .cursor\skills skills src examples tools cmake templates
    set FAIL=1
  )
  rg -n -i --glob "!**/third_party/**" --glob "!**/build/**" --glob "!**/check-invariants.*" -e "\bformerly\b" -e "day-one" -e "day one" src examples >nul 2>&1 && (
    echo FAIL: archaeology phrasing in src/ or examples/
    rg -n -i --glob "!**/third_party/**" --glob "!**/build/**" --glob "!**/check-invariants.*" -e "\bformerly\b" -e "day-one" -e "day one" src examples
    set FAIL=1
  )
  rg -n -i --glob "!**/third_party/**" --glob "!**/build/**" --glob "!**/check-invariants.*" --glob "!**/commandments.md" --glob "!**/rigkit-deslop/**" -e "not implemented yet" -e "rigGCode scaffold" -e "\(scaffold\)" src examples packs >nul 2>&1 && (
    echo FAIL: fake-stub phrasing ^(finish or do not publish^)
    rg -n -i --glob "!**/third_party/**" --glob "!**/build/**" --glob "!**/check-invariants.*" --glob "!**/commandments.md" --glob "!**/rigkit-deslop/**" -e "not implemented yet" -e "rigGCode scaffold" -e "\(scaffold\)" src examples packs
    set FAIL=1
  )
)

REM --- Manifest license fields (SPDX in pack.json / app.json) ---
for /d %%D in (packs\*) do (
  if exist "%%D\pack.json" (
    findstr /C:"\"license\"" "%%D\pack.json" >nul
    if errorlevel 1 (
      echo FAIL: missing pack.json license field ^(%%D\pack.json^)
      set FAIL=1
    )
  )
)
if exist templates\rigTemplate\pack.json (
  findstr /C:"\"license\"" templates\rigTemplate\pack.json >nul
  if errorlevel 1 (
    echo FAIL: missing pack.json license field ^(templates\rigTemplate\pack.json^)
    set FAIL=1
  )
)
if exist packs\rigPlotFinders\pack.json (
  findstr /C:"GPL-2.0-or-later" packs\rigPlotFinders\pack.json >nul
  if errorlevel 1 (
    echo FAIL: rigPlotFinders must declare GPL-2.0-or-later ^(vendors Potrace^)
    set FAIL=1
  )
)
for /d %%D in (examples\*) do (
  if exist "%%D\app.json" (
    findstr /C:"\"license\"" "%%D\app.json" >nul
    if errorlevel 1 (
      echo FAIL: missing app.json license field ^(%%D\app.json^)
      set FAIL=1
    )
  )
)
if exist templates\app\app.json (
  findstr /C:"\"license\"" templates\app\app.json >nul
  if errorlevel 1 (
    echo FAIL: missing app.json license field ^(templates\app\app.json^)
    set FAIL=1
  )
)
if exist templates\rigTemplate\examples\demo\app.json (
  findstr /C:"\"license\"" templates\rigTemplate\examples\demo\app.json >nul
  if errorlevel 1 (
    echo FAIL: missing app.json license field ^(templates\rigTemplate\examples\demo\app.json^)
    set FAIL=1
  )
)
if exist tools\contract_smoke\app.json (
  findstr /C:"\"license\"" tools\contract_smoke\app.json >nul
  if errorlevel 1 (
    echo FAIL: missing app.json license field ^(tools\contract_smoke\app.json^)
    set FAIL=1
  )
)
for /d %%D in (packs\*) do (
  for /d %%H in ("%%D\examples\*") do (
    if exist "%%H\app.json" (
      findstr /C:"\"license\"" "%%H\app.json" >nul
      if errorlevel 1 (
        echo FAIL: missing app.json license field ^(%%H\app.json^)
        set FAIL=1
      )
    )
  )
)
if exist packs\rigPlotFinders\examples\finders\app.json (
  findstr /C:"GPL-2.0-or-later" packs\rigPlotFinders\examples\finders\app.json >nul
  if errorlevel 1 (
    echo FAIL: rigPlotFinders example app.json must declare GPL-2.0-or-later
    set FAIL=1
  )
)

REM --- Pack ctors must not re-author pack.json identity / deps ---
if exist templates\rigTemplate\src (
  findstr /S /M /R /C:"setDescription[ ]*(" /C:"setLicense[ ]*(" /C:"setUrl[ ]*(" /C:"addDependency[ ]*(" templates\rigTemplate\src\*.cpp >nul 2>nul
  if not errorlevel 1 (
    echo FAIL: pack ctor must not set identity/deps ^(use pack.json^): templates\rigTemplate\src
    set FAIL=1
  )
)
for /d %%D in (packs\*) do (
  if exist "%%D\src" if exist "%%D\pack.json" (
    findstr /S /M /R /C:"setDescription[ ]*(" /C:"setLicense[ ]*(" /C:"setUrl[ ]*(" /C:"addDependency[ ]*(" "%%D\src\*.cpp" >nul 2>nul
    if not errorlevel 1 (
      echo FAIL: pack ctor must not set identity/deps ^(use pack.json^): %%D\src
      set FAIL=1
    )
  )
)

REM --- Every pack: at least one example with img\preview.png + README embed ---
for /d %%D in (packs\*) do (
  if exist "%%D\pack.json" (
    set "example_ok="
    for /d %%H in ("%%D\examples\*") do (
      if exist "%%H\CMakeLists.txt" if exist "%%H\img\preview.png" set "example_ok=1"
    )
    if not defined example_ok (
      echo FAIL: pack %%~nxD needs examples\^<name^>\ with CMakeLists.txt and img\preview.png
      set FAIL=1
    )
    if exist "%%D\README.md" (
      findstr /C:"preview.png" "%%D\README.md" >nul
      if errorlevel 1 (
        echo FAIL: pack %%~nxD README.md must embed preview.png
        set FAIL=1
      )
    )
  )
)

if not "%FAIL%"=="0" (
  echo.
  echo Ten Commandments invariant check failed. See docs\contract\commandments.md
  exit /b 1
)

echo OK: check-invariants ^(Ten Commandments machine gates^)
endlocal
