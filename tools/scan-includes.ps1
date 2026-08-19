param(
	[string]$Root = (Split-Path -Parent $PSScriptRoot)
)

$exclude = @('\third_party\', '\build\', '\.git\')
$files = Get-ChildItem -Path "$Root\src", "$Root\examples", "$Root\packs", "$Root\templates" -Recurse -Include *.cpp,*.h,*.hpp -File -ErrorAction SilentlyContinue |
	Where-Object { $p = $_.FullName; -not ($exclude | Where-Object { $p -like "*$_*" }) }

function Get-IncludeBlocks([string]$Path) {
	$lines = Get-Content -LiteralPath $Path
	$blocks = @()
	$cur = @()
	$started = $false
	foreach ($line in $lines) {
		if ($line -match '^\s*#include\s+') {
			$started = $true
			$cur += $line.Trim()
		} elseif ($started -and $line -match '^\s*$') {
			if ($cur.Count -gt 0) { $blocks += ,@($cur); $cur = @() }
		} elseif ($started -and $line -match '^\s*(#pragma|/\*|//|\*)') {
			continue
		} elseif ($started) {
			if ($cur.Count -gt 0) { $blocks += ,@($cur) }
			break
		}
	}
	if ($cur.Count -gt 0) { $blocks += ,@($cur) }
	return $blocks
}

$issues = @()
foreach ($f in $files) {
	$blocks = Get-IncludeBlocks $f.FullName
	if ($blocks.Count -eq 0) { continue }
	$fileIssues = @()

	if ($f.Extension -eq '.cpp') {
		$base = $f.BaseName
		$firstBlock = $blocks[0]
		$ownPattern = '^#include\s+"' + [regex]::Escape($base) + '\.(h|hpp)"'
		$ownOk = ($firstBlock.Count -eq 1) -and ($firstBlock[0] -match $ownPattern)
		if (-not $ownOk) { $fileIssues += 'cpp-own-header' }
	}

	foreach ($i in 0..($blocks.Count - 1)) {
		$b = $blocks[$i]
		$quoted = @($b | Where-Object { $_ -match '#include\s+"' })
		$system = @($b | Where-Object { $_ -match '#include\s+<' })
		if ($quoted.Count -gt 0 -and $system.Count -gt 0) {
			$fileIssues += "mixed-block-$i"
		}
		if ($i -lt ($blocks.Count - 1)) {
			$next = $blocks[$i + 1]
			$nextQuoted = @($next | Where-Object { $_ -match '#include\s+"' })
			if ($system.Count -gt 0 -and $nextQuoted.Count -gt 0) {
				$fileIssues += 'system-before-project'
			}
		}
	}

	foreach ($b in $blocks) {
		$quoted = @()
		foreach ($line in $b) {
			if ($line -match '#include\s+"([^"]+)"') { $quoted += $matches[1] }
		}
		if ($quoted.Count -lt 2) { continue }
		$seenShort = $false
		foreach ($inc in $quoted) {
			if ($inc -match '^(core|ecs|rendering|packs)/') {
				if ($seenShort) { $fileIssues += 'host-after-pack'; break }
			} else {
				$seenShort = $true
			}
		}
	}

	if ($fileIssues.Count -gt 0) {
		$rel = $f.FullName.Substring($Root.Length + 1)
		$issues += [pscustomobject]@{ File = $rel; Issues = ($fileIssues -join ', ') }
	}
}

$issues | Sort-Object File | Format-Table -AutoSize
Write-Output "TOTAL: $($issues.Count) files with issues"
