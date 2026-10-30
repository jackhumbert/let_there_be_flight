# physx_cook

`cook.exe` cooks triangle meshes into PhysX 3.4 blobs ("NXS MESH" v15, BV4 midphase, no face
remap table): the format of the game's geometry cache and of `physicsColliderMesh.compiledGeometryBuffer`
in an entity. Used by `tools/world/proxy_colliders.py` through `tools/world/physx_cook.py`.

Re-cooking game meshes (decoded from the geometry cache) gives the same header and size; a few
bytes differ in the triangle order inside BV4 leaves, and the game's blobs can carry per-triangle
materials (flag 0x1), which we do not write.

## Building (once)

1. The PhysX 3.4 SDK source: `git clone --depth 1 https://github.com/NVIDIAGameWorks/PhysX-3.4.git`
   (kevin: `D:\cp2077_work\build\tools\physx-3.4-src`).
2. Newer MSVC has no `<typeinfo.h>`: in `PxShared\src\foundation\include\PsAllocator.h` include
   `<typeinfo>` instead.
3. Build the cooking library with the VS 2022 toolset, warnings not as errors:

   ```
   $env:_CL_ = '/WX-'
   MSBuild.exe PhysX_3.4\Source\compiler\vc15win64\PhysX.sln /t:PhysXCooking /p:Configuration=release /p:Platform=x64 /p:PlatformToolset=v143 /p:WindowsTargetPlatformVersion=10.0 /m
   ```

4. `powershell -File tools\physx_cook\build.ps1 [-PhysX <checkout>] [-Out <dir>]` builds `cook.exe`
   and copies the three DLLs next to it (default `D:\cp2077_work\build\tools\physx_cook`).
