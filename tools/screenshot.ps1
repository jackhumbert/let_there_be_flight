# Capture the Cyberpunk 2077 window (borderless windowed) to a PNG, downscaled for review.
#
#   powershell -File tools/screenshot.ps1 -Out shot.png [-Width 1600]
#
# Brings the game window to the front first: CopyFromScreen reads the composited desktop, so the
# game must be visible. Exclusive fullscreen tends to capture black; use Borderless Windowed.
param(
    [Parameter(Mandatory = $true)][string]$Out,
    [int]$Width = 1600
)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class LtbfWin {
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
    [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int cmd);
    [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
}
'@
[LtbfWin]::SetProcessDPIAware() | Out-Null
$proc = Get-Process Cyberpunk2077 -ErrorAction SilentlyContinue | Where-Object { $_.MainWindowHandle -ne 0 } | Select-Object -First 1
if (-not $proc) { Write-Error 'Cyberpunk2077 window not found' }
$h = $proc.MainWindowHandle
[LtbfWin]::ShowWindow($h, 9) | Out-Null   # SW_RESTORE
[LtbfWin]::SetForegroundWindow($h) | Out-Null
Start-Sleep -Milliseconds 300
$r = New-Object LtbfWin+RECT
[LtbfWin]::GetWindowRect($h, [ref]$r) | Out-Null
$w = $r.Right - $r.Left; $hgt = $r.Bottom - $r.Top
$full = New-Object System.Drawing.Bitmap $w, $hgt
$g = [System.Drawing.Graphics]::FromImage($full)
$g.CopyFromScreen($r.Left, $r.Top, 0, 0, $full.Size)
$g.Dispose()
$scale = [Math]::Min(1.0, $Width / $w)
$sw = [int]($w * $scale); $sh = [int]($hgt * $scale)
$small = New-Object System.Drawing.Bitmap $sw, $sh
$g2 = [System.Drawing.Graphics]::FromImage($small)
$g2.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
$g2.DrawImage($full, 0, 0, $sw, $sh)
$g2.Dispose(); $full.Dispose()
$small.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)
$small.Dispose()
Write-Output "$Out ${w}x${hgt} -> ${sw}x${sh}"
