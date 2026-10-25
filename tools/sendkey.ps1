# Send key presses to the Cyberpunk 2077 window as hardware scancodes (the game ignores virtual-key events).
#
#   powershell -File tools/sendkey.ps1 -Keys space [-HoldMs 80]
#   powershell -File tools/sendkey.ps1 -Keys f,w -HoldMs 2000     (hold several keys together)
#
# Brings the game window to the front first; input only reaches the foreground window.
param(
    [Parameter(Mandatory = $true)][string[]]$Keys,
    [int]$HoldMs = 80
)
$ErrorActionPreference = 'Stop'
Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class LtbfKeys {
    [StructLayout(LayoutKind.Sequential)] struct KEYBDINPUT { public ushort wVk, wScan; public uint dwFlags, time; public IntPtr extra; }
    [StructLayout(LayoutKind.Explicit, Size = 40)] struct INPUT { [FieldOffset(0)] public uint type; [FieldOffset(8)] public KEYBDINPUT ki; }
    [DllImport("user32.dll")] static extern uint SendInput(uint n, INPUT[] inputs, int size);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
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
}
$proc = Get-Process Cyberpunk2077 -ErrorAction SilentlyContinue | Where-Object { $_.MainWindowHandle -ne 0 } | Select-Object -First 1
if (-not $proc) { Write-Error 'Cyberpunk2077 window not found' }
[LtbfKeys]::SetForegroundWindow($proc.MainWindowHandle) | Out-Null
Start-Sleep -Milliseconds 200
$parsed = foreach ($k in $Keys) {
    $c = $codes[$k.ToLower()]
    if ($null -eq $c) { Write-Error "unknown key '$k'" }
    if ($c -is [string]) { [pscustomobject]@{ scan = [Convert]::ToUInt16($c.Substring(1), 16); ext = $true } }
    else { [pscustomobject]@{ scan = [uint16]$c; ext = $false } }
}
foreach ($p in $parsed) { [LtbfKeys]::Send($p.scan, $p.ext, $false) }
Start-Sleep -Milliseconds $HoldMs
foreach ($p in $parsed) { [LtbfKeys]::Send($p.scan, $p.ext, $true) }
Write-Output "sent $($Keys -join '+') for ${HoldMs}ms"
