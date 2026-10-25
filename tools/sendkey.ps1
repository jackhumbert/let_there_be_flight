# Send key presses to the Cyberpunk 2077 window as hardware scancodes (the game ignores virtual-key events).
#
#   powershell -File tools/sendkey.ps1 -Keys space [-HoldMs 80]
#   powershell -File tools/sendkey.ps1 -Keys f,w -HoldMs 2000     (hold several keys together)
#
# Brings the game window to the front first; input only reaches the foreground window.
#   powershell -File tools/sendkey.ps1 -X 222 -Y 566                 (left click at screenshot.ps1 coordinates)
param(
    [string[]]$Keys = @(),
    [int]$HoldMs = 80,
    [int]$X = -1,
    [int]$Y = -1,
    [int]$ShotWidth = 1600   # width of the screenshot the -X/-Y coordinates come from
)
$ErrorActionPreference = 'Stop'
Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class LtbfKeys {
    [StructLayout(LayoutKind.Sequential)] struct KEYBDINPUT { public ushort wVk, wScan; public uint dwFlags, time; public IntPtr extra; }
    [StructLayout(LayoutKind.Sequential)] struct MOUSEINPUT { public int dx, dy; public uint mouseData, dwFlags, time; public IntPtr extra; }
    [StructLayout(LayoutKind.Explicit, Size = 40)] struct INPUT { [FieldOffset(0)] public uint type; [FieldOffset(8)] public KEYBDINPUT ki; [FieldOffset(8)] public MOUSEINPUT mi; }
    // lmb/rmb: MOUSEEVENTF_LEFTDOWN 0x2 / LEFTUP 0x4, RIGHTDOWN 0x8 / RIGHTUP 0x10
    public static void Mouse(bool right, bool up) {
        var i = new INPUT { type = 0 };
        i.mi.dwFlags = right ? (up ? 0x10u : 0x8u) : (up ? 0x4u : 0x2u);
        SendInput(1, new[] { i }, Marshal.SizeOf(typeof(INPUT)));
    }
    [DllImport("user32.dll")] static extern uint SendInput(uint n, INPUT[] inputs, int size);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
    [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
    [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
    const uint SCANCODE = 0x8, KEYUP = 0x2, EXTENDED = 0x1;
    public static void Send(ushort scan, bool extended, bool up) {
        var i = new INPUT { type = 1 };
        i.ki.wScan = scan;
        i.ki.dwFlags = SCANCODE | (up ? KEYUP : 0) | (extended ? EXTENDED : 0);
        SendInput(1, new[] { i }, Marshal.SizeOf(typeof(INPUT)));
    }
}
'@
# set-1 scancodes; extended keys flagged with 'e'
$codes = @{
    space = 0x39; enter = 0x1C; esc = 0x01; tab = 0x0F; lshift = 0x2A; lctrl = 0x1D; lalt = 0x38
    w = 0x11; a = 0x1E; s = 0x1F; d = 0x20; q = 0x10; e = 0x12; r = 0x13; f = 0x21; g = 0x22; h = 0x23
    j = 0x24; k = 0x25; l = 0x26; n = 0x31; m = 0x32; f5 = 0x3F
    x = 0x2D; c = 0x2E; v = 0x2F; b = 0x30; z = 0x2C; t = 0x14; y = 0x15; u = 0x16; i = 0x17; o = 0x18; p = 0x19
    '1' = 0x02; '2' = 0x03; '3' = 0x04; '4' = 0x05
    up = 'e48'; down = 'e50'; left = 'e4B'; right = 'e4D'
    lmb = 'mL'; rmb = 'mR'
}
$proc = Get-Process Cyberpunk2077 -ErrorAction SilentlyContinue | Where-Object { $_.MainWindowHandle -ne 0 } | Select-Object -First 1
if (-not $proc) { Write-Error 'Cyberpunk2077 window not found' }
[LtbfKeys]::SetProcessDPIAware() | Out-Null
[LtbfKeys]::SetForegroundWindow($proc.MainWindowHandle) | Out-Null
Start-Sleep -Milliseconds 200
if ($X -ge 0 -and $Y -ge 0) {
    $r = New-Object LtbfKeys+RECT
    [LtbfKeys]::GetWindowRect($proc.MainWindowHandle, [ref]$r) | Out-Null
    $scale = ($r.Right - $r.Left) / $ShotWidth
    [LtbfKeys]::SetCursorPos([int]($r.Left + $X * $scale), [int]($r.Top + $Y * $scale)) | Out-Null
    Start-Sleep -Milliseconds 150
    [LtbfKeys]::Mouse($false, $false); Start-Sleep -Milliseconds 80; [LtbfKeys]::Mouse($false, $true)
    Write-Output "clicked $X,$Y"
    if (-not $Keys) { return }
}
$parsed = foreach ($k in $Keys) {
    $c = $codes[$k.ToLower()]
    if ($null -eq $c) { Write-Error "unknown key '$k'" }
    if ($c -is [string] -and $c.StartsWith('m')) { [pscustomobject]@{ mouse = $true; right = ($c -eq 'mR') } }
    elseif ($c -is [string]) { [pscustomobject]@{ mouse = $false; scan = [Convert]::ToUInt16($c.Substring(1), 16); ext = $true } }
    else { [pscustomobject]@{ mouse = $false; scan = [uint16]$c; ext = $false } }
}
function Press($p, $up) { if ($p.mouse) { [LtbfKeys]::Mouse($p.right, $up) } else { [LtbfKeys]::Send($p.scan, $p.ext, $up) } }
foreach ($p in $parsed) { Press $p $false }
Start-Sleep -Milliseconds $HoldMs
foreach ($p in $parsed) { Press $p $true }
Write-Output "sent $($Keys -join '+') for ${HoldMs}ms"
