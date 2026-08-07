param(
    [Parameter(Mandatory = $true)][string]$Exe,
    [Parameter(Mandatory = $true)][string]$OutPng,
    [int]$WaitSeconds = 8,
    [string]$Args = ''
)

Add-Type -AssemblyName System.Drawing

$src = @"
using System;
using System.Runtime.InteropServices;

public static class WinCap {
    [StructLayout(LayoutKind.Sequential)]
    public struct RECT { public int L, T, R, B; }

    [DllImport("user32.dll")]
    public static extern bool GetWindowRect(IntPtr hWnd, out RECT rect);

    [DllImport("user32.dll")]
    public static extern bool SetForegroundWindow(IntPtr hWnd);

    [DllImport("user32.dll")]
    public static extern bool SetProcessDPIAware();

    [DllImport("user32.dll")]
    public static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);

    [DllImport("user32.dll")]
    public static extern bool PrintWindow(IntPtr hWnd, IntPtr hdc, uint flags);
}
"@
Add-Type -TypeDefinition $src
[WinCap]::SetProcessDPIAware() | Out-Null

$workDir = Split-Path -Parent $Exe
if ($Args -ne '') {
    $p = Start-Process -FilePath $Exe -WorkingDirectory $workDir -ArgumentList $Args -PassThru
} else {
    $p = Start-Process -FilePath $Exe -WorkingDirectory $workDir -PassThru
}
Start-Sleep -Seconds $WaitSeconds

$p.Refresh()
$h = $p.MainWindowHandle
if ($h -eq [IntPtr]::Zero) {
    Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue
    Write-Error 'no main window handle'
    exit 1
}
[WinCap]::ShowWindow($h, 3) | Out-Null # SW_MAXIMIZE
[WinCap]::SetForegroundWindow($h) | Out-Null
Start-Sleep -Milliseconds 1500

$r = New-Object WinCap+RECT
[WinCap]::GetWindowRect($h, [ref]$r) | Out-Null
$w = $r.R - $r.L
$ht = $r.B - $r.T
Write-Host "window rect: $($r.L),$($r.T) ${w}x${ht}"
if ($w -le 0 -or $ht -le 0) {
    Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue
    Write-Error 'empty window rect'
    exit 1
}

$bmp = New-Object System.Drawing.Bitmap($w, $ht)
$g = [System.Drawing.Graphics]::FromImage($bmp)
# PrintWindow(PW_RENDERFULLCONTENT=2) captures the window surface even when
# occluded (GPU-rendered windows included). Fall back to a screen copy.
$hdc = $g.GetHdc()
$ok = [WinCap]::PrintWindow($h, $hdc, 2)
$g.ReleaseHdc($hdc)
if (-not $ok) {
    $g.CopyFromScreen($r.L, $r.T, 0, 0, $bmp.Size)
}
$bmp.Save($OutPng)
$g.Dispose()
$bmp.Dispose()
Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue
Write-Host "saved $OutPng"
