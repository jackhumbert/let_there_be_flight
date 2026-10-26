# Record the game's monitor to an MP4. Fallback only: Xbox Game Bar (sendkey.ps1 -Keys lwin,lalt,r)
# gives better-looking video with audio; see "Driving the game" in CLAUDE.md.
#   powershell -File tools\record.ps1 -Out flight.mp4 -Seconds 60 [-Width 1280] [-Fps 30] [-Output 0] [-KeepFull]
#
# Capture: ffmpeg's Desktop Duplication grabber (ddagrab) on DXGI output -Output (0 = the monitor
# the game is on, here), encoded at full resolution with NVENC so the frames never leave the GPU.
# Then a second pass scales the clip to -Width with libx264 (-KeepFull keeps the 4K file too).
# The game is brought to the foreground first: unfocused it renders at about 10 fps (driver
# background frame cap), focused about 25 fps at 4K on the RTX 2070 Super.
# CPU paths are too slow at 4K: hwdownload + scale managed about 2 fps, and scale_d3d11 cannot
# create its texture from ddagrab frames (ffmpeg 9.0.2).
# ffmpeg: $env:FFMPEG or -Ffmpeg; defaults to the gyan.dev build under D:\cp2077_work\build\tools\ffmpeg.
param(
  [Parameter(Mandatory = $true)][string]$Out,
  [double]$Seconds = 60,
  [int]$Width = 1280,
  [int]$Fps = 30,
  [int]$Output = 0,
  [switch]$KeepFull,
  [string]$Ffmpeg = $(if ($env:FFMPEG) { $env:FFMPEG } else { 'D:\cp2077_work\build\tools\ffmpeg\ffmpeg-9.0.2-essentials_build\bin\ffmpeg.exe' })
)

Add-Type @'
using System; using System.Runtime.InteropServices;
public static class RecordFocus {
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int c);
}
'@
$game = Get-Process Cyberpunk2077 -ErrorAction SilentlyContinue | Select-Object -First 1
if ($game) {
  [RecordFocus]::ShowWindow($game.MainWindowHandle, 5) | Out-Null
  [RecordFocus]::SetForegroundWindow($game.MainWindowHandle) | Out-Null
} else {
  Write-Warning "Cyberpunk2077 is not running; recording the desktop anyway"
}

$full = [System.IO.Path]::GetFullPath($Out)
$dir = Split-Path -Parent $full
if (-not (Test-Path $dir)) { New-Item -ItemType Directory -Force $dir | Out-Null }
$raw = [System.IO.Path]::ChangeExtension($full, '.full.mp4')

& $Ffmpeg -hide_banner -loglevel error -y -f lavfi -i "ddagrab=output_idx=$($Output):framerate=$($Fps)" -t $Seconds `
  -c:v h264_nvenc -preset p4 -rc vbr -cq 21 -b:v 0 $raw
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

& $Ffmpeg -hide_banner -loglevel error -y -i $raw -vf "scale=$($Width):-2" `
  -c:v libx264 -preset medium -crf 23 -pix_fmt yuv420p -movflags +faststart $full
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
if (-not $KeepFull) { Remove-Item $raw }

$size = (Get-Item $full).Length / 1MB
"$full {0:N1} MB, {1} s" -f $size, $Seconds
