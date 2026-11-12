# Build cook.exe against a PhysX 3.4 source checkout whose PhysXCooking project is built
# (see README.md). Run from any shell; it sets up MSVC itself.
param(
    [string]$PhysX = $env:LTBF_PHYSX_SRC,
    [string]$Out
)
if (-not $PhysX) { throw '-PhysX is not set: pass the PhysX 3.4 source checkout, or set the environment variable LTBF_PHYSX_SRC.' }
if (-not $Out) {
    $work = if ($env:LTBF_WORK) { $env:LTBF_WORK } else { Join-Path $PSScriptRoot '..\..\..\build\tools_work' }
    $Out = Join-Path $work 'tools\physx_cook'
}
$vcvars = $null
foreach ($root in @(${env:ProgramFiles(x86)}, ${env:ProgramFiles})) {
    foreach ($edition in 'BuildTools', 'Community', 'Professional', 'Enterprise') {
        $candidate = Join-Path $root "Microsoft Visual Studio\2022\$edition\VC\Auxiliary\Build\vcvars64.bat"
        if (-not $vcvars -and (Test-Path $candidate)) { $vcvars = $candidate }
    }
}
if (-not $vcvars) { throw 'No Visual Studio 2022 (vcvars64.bat) found under the Program Files folders.' }
New-Item -ItemType Directory -Force $Out | Out-Null
$src = Join-Path $PSScriptRoot 'cook.cpp'
$inc = "/I`"$PhysX\PhysX_3.4\Include`" /I`"$PhysX\PxShared\include`""
$libs = "`"$PhysX\PhysX_3.4\Lib\vc15win64\PhysX3Cooking_x64.lib`" `"$PhysX\PhysX_3.4\Lib\vc15win64\PhysX3Common_x64.lib`" `"$PhysX\PxShared\lib\vc15win64\PxFoundation_x64.lib`""
cmd /c "`"$vcvars`" >nul && cl /nologo /EHsc /O2 /MT /std:c++17 /DNDEBUG $inc `"$src`" /Fe`"$Out\cook.exe`" /Fo`"$Out\cook.obj`" /link $libs"
foreach ($d in @("$PhysX\PhysX_3.4\bin\vc15win64\PhysX3Cooking_x64.dll", "$PhysX\PhysX_3.4\bin\vc15win64\PhysX3Common_x64.dll", "$PhysX\PxShared\bin\vc15win64\PxFoundation_x64.dll")) {
    Copy-Item $d $Out
}
