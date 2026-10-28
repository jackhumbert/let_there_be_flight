# Build cook.exe against a PhysX 3.4 source checkout whose PhysXCooking project is built
# (see README.md). Run from any shell; it sets up MSVC itself.
param(
    [string]$PhysX = 'D:\cp2077_work\build\tools\physx-3.4-src',
    [string]$Out = 'D:\cp2077_work\build\tools\physx_cook'
)
$vcvars = 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat'
if (-not (Test-Path $vcvars)) { $vcvars = 'C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat' }
New-Item -ItemType Directory -Force $Out | Out-Null
$src = Join-Path $PSScriptRoot 'cook.cpp'
$inc = "/I`"$PhysX\PhysX_3.4\Include`" /I`"$PhysX\PxShared\include`""
$libs = "`"$PhysX\PhysX_3.4\Lib\vc15win64\PhysX3Cooking_x64.lib`" `"$PhysX\PhysX_3.4\Lib\vc15win64\PhysX3Common_x64.lib`" `"$PhysX\PxShared\lib\vc15win64\PxFoundation_x64.lib`""
cmd /c "`"$vcvars`" >nul && cl /nologo /EHsc /O2 /MT /std:c++17 /DNDEBUG $inc `"$src`" /Fe`"$Out\cook.exe`" /Fo`"$Out\cook.obj`" /link $libs"
foreach ($d in @("$PhysX\PhysX_3.4\bin\vc15win64\PhysX3Cooking_x64.dll", "$PhysX\PhysX_3.4\bin\vc15win64\PhysX3Common_x64.dll", "$PhysX\PxShared\bin\vc15win64\PxFoundation_x64.dll")) {
    Copy-Item $d $Out
}
