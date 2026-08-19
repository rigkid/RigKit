# Retired. This script used to rewrite the first include block: it dropped
# non-include lines, alphabetized all <> includes, and put <imfilebrowser.h>
# before <imgui.h> (which requires ImGui types first).
#
# Tidy includes with clang-format only:
#   tools\format.bat
#   tools/format.sh
#
# Report-only scan (does not write):
#   powershell -File tools\scan-includes.ps1

Write-Error "tools/fix-includes.ps1 is retired. Use tools/format.bat (clang-format SortIncludes). See docs/includes.md."
exit 1
