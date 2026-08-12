param(
	# Capture only these packs (by pack name). Default: every example below.
	[string[]]$Only = @(),
	# Leave the window at its native size and scale the shot down to the example
	# width afterwards. Needed on HiDPI: forcing the window to example pixels
	# leaves PrintWindow with the pre-resize surface.
	[switch]$Native
)

$ErrorActionPreference = 'Stop'

Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Text;
using System.Runtime.InteropServices;
public static class Win32 {
	public const uint SWP_SHOWWINDOW = 0x0040;
	public const int SW_RESTORE = 9;
	public const int PW_RENDERFULLCONTENT = 2;
	public static readonly IntPtr HWND_TOPMOST = new IntPtr(-1);
	public static readonly IntPtr HWND_NOTOPMOST = new IntPtr(-2);

	public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);
	[DllImport("user32.dll")] public static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);
	[DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr hWnd);
	[DllImport("user32.dll")] public static extern bool IsWindow(IntPtr hWnd);
	[DllImport("user32.dll")] public static extern int GetWindowText(IntPtr hWnd, StringBuilder lpString, int nMaxCount);
	[DllImport("user32.dll")] public static extern int GetWindowTextLength(IntPtr hWnd);
	[DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);
	[DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
	[DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);
	[DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr hWnd, IntPtr hWndInsertAfter, int X, int Y, int cx, int cy, uint uFlags);
	[DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr hwnd, IntPtr hdcBlt, int nFlags);
	[DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
	[DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);
	[DllImport("user32.dll")] public static extern IntPtr GetDC(IntPtr hWnd);
	[DllImport("user32.dll")] public static extern int ReleaseDC(IntPtr hWnd, IntPtr hDC);
	[DllImport("gdi32.dll")] public static extern IntPtr CreateCompatibleDC(IntPtr hdc);
	[DllImport("gdi32.dll")] public static extern IntPtr CreateCompatibleBitmap(IntPtr hdc, int nWidth, int nHeight);
	[DllImport("gdi32.dll")] public static extern IntPtr SelectObject(IntPtr hdc, IntPtr hgdiobj);
	[DllImport("gdi32.dll")] public static extern bool DeleteObject(IntPtr hObject);
	[DllImport("gdi32.dll")] public static extern bool DeleteDC(IntPtr hdc);

	[StructLayout(LayoutKind.Sequential)]
	public struct RECT { public int Left, Top, Right, Bottom; }
}
"@

# Without this the window rect comes back in virtualised coordinates and
# PrintWindow copies only the top-left corner of a HiDPI surface.
[void][Win32]::SetProcessDPIAware()

function Find-WindowForProcess {
	param([int]$ProcessId, [string]$TitleContains)
	$script:foundHwnd = [IntPtr]::Zero
	$want = $ProcessId
	$needle = $TitleContains
	$cb = [Win32+EnumWindowsProc]{
		param([IntPtr]$hWnd, [IntPtr]$lParam)
		if (-not [Win32]::IsWindowVisible($hWnd)) { return $true }
		$len = [Win32]::GetWindowTextLength($hWnd)
		if ($len -le 0) { return $true }
		$sb = New-Object System.Text.StringBuilder ($len + 1)
		[void][Win32]::GetWindowText($hWnd, $sb, $sb.Capacity)
		$title = $sb.ToString()
		$owner = [uint32]0
		[void][Win32]::GetWindowThreadProcessId($hWnd, [ref]$owner)
		if (($owner -eq $want) -and ($title -like "*$needle*")) {
			$script:foundHwnd = $hWnd
			return $false
		}
		return $true
	}
	[void][Win32]::EnumWindows($cb, [IntPtr]::Zero)
	return $script:foundHwnd
}

function Capture-Window {
	param(
		[IntPtr]$Hwnd,
		[string]$OutPath,
		[int]$Width,
		[int]$Height,
		[int]$ProcessId = 0,
		[string]$TitleContains = '',
		[int]$ScaleWidth = 0,
		[switch]$NoResize
	)

	if (-not [Win32]::IsWindow($Hwnd)) { throw "Invalid hwnd before capture" }
	[void][Win32]::ShowWindow($Hwnd, [Win32]::SW_RESTORE)
	$flags = [Win32]::SWP_SHOWWINDOW
	if ($NoResize) {
		# GLFW can recreate the HWND on external resize; host_shell dies if we force size.
		$flags = $flags -bor 0x0001 # SWP_NOSIZE
		[void][Win32]::SetWindowPos($Hwnd, [Win32]::HWND_TOPMOST, 16, 16, 0, 0, $flags)
	} else {
		[void][Win32]::SetWindowPos($Hwnd, [Win32]::HWND_TOPMOST, 16, 16, $Width, $Height, $flags)
	}
	[void][Win32]::SetForegroundWindow($Hwnd)
	Start-Sleep -Milliseconds 800

	# Re-resolve if the window was recreated.
	if (-not [Win32]::IsWindow($Hwnd) -and $ProcessId -gt 0 -and $TitleContains) {
		$Hwnd = Find-WindowForProcess -ProcessId $ProcessId -TitleContains $TitleContains
	}
	if (-not [Win32]::IsWindow($Hwnd)) { throw "hwnd died after position" }
	$wrect = New-Object Win32+RECT
	if (-not [Win32]::GetWindowRect($Hwnd, [ref]$wrect)) { throw "GetWindowRect failed" }
	$w = $wrect.Right - $wrect.Left
	$h = $wrect.Bottom - $wrect.Top
	if ($w -lt 200 -or $h -lt 200) { throw "Window too small ${w}x${h}" }

	$hdcScreen = [Win32]::GetDC([IntPtr]::Zero)
	$hdcMem = [Win32]::CreateCompatibleDC($hdcScreen)
	$hbmp = [Win32]::CreateCompatibleBitmap($hdcScreen, $w, $h)
	$old = [Win32]::SelectObject($hdcMem, $hbmp)
	try {
		$ok = [Win32]::PrintWindow($Hwnd, $hdcMem, [Win32]::PW_RENDERFULLCONTENT)
		if (-not $ok) { $ok = [Win32]::PrintWindow($Hwnd, $hdcMem, 0) }
		if (-not $ok) { throw "PrintWindow failed" }

		$bmp = [System.Drawing.Image]::FromHbitmap($hbmp)
		$shot = $bmp
		try {
			if ($ScaleWidth -gt 0 -and $bmp.Width -gt $ScaleWidth) {
				$scaled = New-Object System.Drawing.Bitmap($ScaleWidth, [int][Math]::Round($bmp.Height * $ScaleWidth / $bmp.Width))
				$g = [System.Drawing.Graphics]::FromImage($scaled)
				$g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
				$g.DrawImage($bmp, 0, 0, $scaled.Width, $scaled.Height)
				$g.Dispose()
				$shot = $scaled
			}
			$dir = Split-Path -Parent $OutPath
			if (-not (Test-Path $dir)) { New-Item -ItemType Directory -Force -Path $dir | Out-Null }
			$shot.Save($OutPath, [System.Drawing.Imaging.ImageFormat]::Png)
			Write-Host ("Saved {0} ({1} bytes, {2}x{3})" -f $OutPath, (Get-Item $OutPath).Length, $shot.Width, $shot.Height)
		} finally {
			if ($shot -ne $bmp) { $shot.Dispose() }
			$bmp.Dispose()
		}
	} finally {
		[void][Win32]::SelectObject($hdcMem, $old)
		[void][Win32]::DeleteObject($hbmp)
		[void][Win32]::DeleteDC($hdcMem)
		[void][Win32]::ReleaseDC([IntPtr]::Zero, $hdcScreen)
		if ([Win32]::IsWindow($Hwnd)) {
			[void][Win32]::SetWindowPos($Hwnd, [Win32]::HWND_NOTOPMOST, 16, 16, 0, 0, ([Win32]::SWP_SHOWWINDOW -bor 0x0001))
		}
	}
}

$examples = @(
	@{
		Exe = 'D:\repos\rigkit\packs\rigComponent\examples\creators\build\bin\creators\creators.exe'
		Title = 'rigComponent'
		Out = 'D:\repos\rigkit\packs\rigComponent\examples\creators\img\preview.png'
		WaitSec = 2.0
		W = 1024
		H = 768
	},
	@{
		Exe = 'D:\repos\rigkit\packs\rigSystems\examples\present\build\bin\present\present.exe'
		Title = 'rigSystems'
		Out = 'D:\repos\rigkit\packs\rigSystems\examples\present\img\preview.png'
		WaitSec = 2.5
		W = 1024
		H = 768
	},
	@{
		Exe = 'D:\repos\rigkit\packs\rigProject\examples\document\build\bin\document\document.exe'
		Title = 'rigProject'
		Out = 'D:\repos\rigkit\packs\rigProject\examples\document\img\preview.png'
		WaitSec = 2.0
		W = 1024
		H = 768
	},
	@{
		Exe = 'D:\repos\rigkit\packs\rigImGui\examples\host_shell\build\bin\host_shell\host_shell.exe'
		Title = 'rigImGui'
		Out = 'D:\repos\rigkit\packs\rigImGui\examples\host_shell\img\preview.png'
		WaitSec = 4.0
		W = 1280
		H = 720
		NoResize = $true
	}
)

if ($Only.Count -gt 0) {
	$examples = @($examples | Where-Object { $Only -contains $_.Title })
	if ($examples.Count -eq 0) { throw "No example matched -Only $($Only -join ',')" }
}

Get-Process creators, present, document, host_shell -ErrorAction SilentlyContinue |
	Stop-Process -Force -ErrorAction SilentlyContinue
Start-Sleep -Milliseconds 500

foreach ($h in $examples) {
	Write-Host "`n=== $($h.Title) ==="
	if (-not (Test-Path $h.Exe)) { throw "Missing $($h.Exe)" }
	$work = Split-Path -Parent $h.Exe
	$proc = Start-Process -FilePath $h.Exe -WorkingDirectory $work -PassThru
	try {
		$deadline = (Get-Date).AddSeconds(20)
		$hwnd = [IntPtr]::Zero
		while ((Get-Date) -lt $deadline) {
			Start-Sleep -Milliseconds 250
			if ($proc.HasExited) { throw "Process exited early code=$($proc.ExitCode)" }
			$hwnd = Find-WindowForProcess -ProcessId $proc.Id -TitleContains $h.Title
			if ($hwnd -ne [IntPtr]::Zero) { break }
		}
		if ($hwnd -eq [IntPtr]::Zero) { throw "Window not found for $($h.Title)" }
		Start-Sleep -Seconds $h.WaitSec
		$capArgs = @{
			Hwnd = $hwnd
			OutPath = $h.Out
			Width = $h.W
			Height = $h.H
			ProcessId = $proc.Id
			TitleContains = $h.Title
		}
		if ($h.NoResize) { $capArgs.NoResize = $true }
		if ($Native) {
			$capArgs.NoResize = $true
			$capArgs.ScaleWidth = $h.W
		}
		Capture-Window @capArgs
	} finally {
		if (-not $proc.HasExited) {
			Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
		}
		Start-Sleep -Milliseconds 400
	}
}

Write-Host "`nDone."
