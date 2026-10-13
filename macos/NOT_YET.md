# Let There Be Flight on macOS: what is not there yet, and what is fail-safe

R41 build, 2026-10-12. Addresses come from `macos/ADDRESS_SURVEY.md` (R40) and `macos/ADDRESS_LEFTOVERS.md` (R41b). Nothing here was
tested in the game yet (Boot A is next).

## 1. Address hashes the dylib embeds that the Mac table does not resolve

`hashscan.py` on `let_there_be_flight.dylib` (R42b build) finds 143 hash values; 122 resolve through the generated `Relocation-inl.hpp` (the Mac
SDK rows, Codeware's `MacSdkRows.inc` rows, `macos/LtbfSdkRows.inc`, or one of the SDK's own stand-ins). The other 21:

| Hash | What | Why it is fail-safe |
|---|---|---|
| 0x19EC1367 (434901863) | SDK `Memory_Vault_Unk1` | Only in the `.cold` out-of-memory paths of the SDK allocators (`Allocator<Pool*>::sub_28`). Same as Codeware and ArchiveXL. The trap stub logs and returns 0. |
| 0xD54F163A (3578730042) | SDK `Memory_PoolStorage_OOM` | Same: the allocators' `OOM()` cold path only. |
| 1180173983, 1586431952, 331746781 | the fork's VFT hashes of IScriptable, entIComponent, gameComponent | Not resolved through the table. `ltbf_mac_vft_slot` (`macos/MacShims.cpp`) reads the class's own macOS vtable instead (entIComponent 0x6ED3698, gameComponent 0x6FB1820, R41b), Windows slot + 8 from 0x20 on; IScriptable slot 0xD8 is Codeware's row `IScriptable_sub_D8`. |
| the rest (16 in the R42b build; the values change from build to build) | none | False positives of the literal-word scan: instruction words (`bl`, `b`, `tbz`, `movz` encodings) and string bytes (`0x65725F74` is "t_re"). None is a `movz`/`movk` immediate; only the five rows above are. |

An address hash with no Mac row resolves to `ltbf_mac_trap_for_hash`: it logs the hash once, and the stub it returns logs its first
call and returns 0. A hook whose hash has no row is not attached (log line, feature skipped).

## 2. Features that are off on macOS

| Feature | Why | What it needs |
|---|---|---|
| Flight trail effects (tire tracks / skid marks replaced by the flight effects) | `GetFxForMaterial` (2081691750) is inlined on macOS. The Skid/TireTrack start hooks only restart an effect when GetFxForMaterial flagged a mode change, and they do not get the material name the replacement is looked up by. None of the three hooks is registered on macOS. | A hook inside the inlined block, or the material name from the caller of the start functions. |
| Thruster colliders (`IFlightConfiguration::AddColliders` / `RemoveColliders`) | Skipped with a one-time log line. R41b located every game piece (chassis proxy id 0x160 and lock 0x16C, ProxyID::GetProxy, ProxyHelper Create / Unlock, GetSlotIndex, the slot transform, ColliderSphere through RTTI with `radius` at 0x88), but the PhysX calls (`attachShape`, `detachShape`, `release`, the filter data setters) go through `deps/physx/PhysX3.hpp`, which is Windows-only (LoadLibraryA) and whose vtable slots are not ported. | The PhysX vtable slots on macOS (R40 has getNbShapes 0xD0, getShapes 0xD8, setSimulationFilterData 0xA8, setQueryFilterData 0xB8; attachShape/detachShape/release are not checked), then the R41b pieces. |
| FPP camera weight while flying (`AnimFeature_FPPCamera.vehicleProceduralCameraWeight`) | The offset inside the anim feature is not checked on macOS (`LTBF_MAC_TODO_R41b_FPPAnimFeature_vehicleProceduralCameraWeight`). LTBF sets the value through the class's reflected property instead; if the property is not found it logs once and skips. | The offset, if the property route turns out slow. |
| Input Loader | Not a plugin on macOS. `InputLoader::Add` is a logged no-op; the launcher's input loader installs `r6/input/ltbf-mac.xml` (the zip's `inputs.xml`). | Nothing in the plugin. |
| FMOD Studio live update | Not enabled on macOS (it opens a TCP listener and macOS asks whether the game may accept connections). | Nothing. |
| Cleanup of old Windows install files (`r6/scripts/let_there_be_flight`, `archive/pc/mod/let_there_be_flight.*`, ...) | Skipped: no Mac install put them there, and the plugin does not delete files in the game folder. | Nothing. |

## 3. Struct fields read or written through the fork's offsets, not checked against a macOS function

These are the jackhumbert fork's offsets. They pass the fork's offset asserts under clang, and the classes around them have the same
reflected layout on macOS, but no macOS function was read for them. Boot A and the first flight test will show.

| Field | Offset | Used by |
|---|---|---|
| `vehicle::BaseObject.worldTransform` | 0x340 | GetLocationFromOffset (centre the TPP camera on the mass) |
| `vehicle::BaseObject.airControl` | 0x5E0 | `VehicleObject.TurnOffAirControl` (writes the AirControl PID and axis fields; the AirControl struct is the fork's) |
| `vehicle::BaseObject.weapons`, `numWeapons` (Weapon stride 0x68) | 0xB00, 0xB10 | weapon natives (`GetWeapons`, `GetWeaponPlaceholderOrientation`) |
| `vehicle::BaseObject.effects`, `acoustics`, `collisions` | 0x5D8, 0x5E8, 0x5D0 | not read by LTBF directly; `collisions` (0x5D0) matches the macOS air-drag block (`[vehicle+0x5D0]+0xEC`) |
| `vehicle::PhysicsData.momentOfInertiaScale`, `localInertiaTensor`, `worldInertiaTensor`, `unk1B0` (gravity flag) | 0x1C8, 0x50, 0x90, 0x1B0 | `VehicleObject` natives (`GetMomentOfInertiaScale`, `GetInertiaTensor`, `GetGlobalInertiaTensor`, `EnableGravity`, `HasGravity`) |
| `VehiclePhysicsInsert2.dampedSpringForce` inside a wheel entry | fork offset (the array base 0x5D0, stride 0x190 and count 0xC10 are R40's) | `WheeledObject.GetDampedSpringForce` |
| `ent::VisualControllerComponent.resourcePaths` | 0xC8 (inferred: same class size 0x128 and same reflected offsets around it) | `EntityExt.AddComponent` (mesh components) |
| `vehicle::Acoustics.vehicle` | 0x138 (both twins of UpdateTireParameters read the same `[acoustics+0x414]` / `+0x41A`) | UpdateTireParameters |
| The fork's `TPPCameraComponent::CameraPreset` inside the `data` block (0x330 + isInAir 0x30, yawDelta 0x20, pitchDelta 0x24, isUsingMouse 0x28) and `TPPCameraUpdate.locationFromOffset` (+0x30) | as listed | TPP camera hooks. The block offset (0x330) and the camera fields are R41b's; the inner layout is the fork's. |

Checked in R41 itself (macOS disassembly):
- `vehicle::Effects::Unk40`: isUnderwater +0x7C, wheelLatSlip +0x74, wheelLongSlip +0x78 (WheelEffectUpdate 0x133BD48 / 0x133BD98), as in the fork.
- `TireParameterUpdate` (acoustics +0, variables +0x10, surface value +0x18) and `TireUpdate` (skidValue +0x1C, skidValue2 +0x20), as in the fork (UpdateTireParameters 0x128AE3C, which also takes a float in s0: the Mac hook declares and forwards it).
- `BaseObject.input.vehicleCameraReset` = BaseObject + 0x29D (UpdateVehicleCameraInput 0x12B1BF8 writes 0x29B..0x29D).
- ApplyAirResistance's inlined block (0x13929F8-0x1392A94): reimplemented in `Physics/VehiclePhysicsUpdate.cpp` with the same threshold, flag byte, factor and store.

## 4. Things that work differently from Windows on purpose

- Registration: the game never runs RTTI register callbacks or RED4ext game-state callbacks on macOS. `Main(Load)` only checks the
  dependencies and attaches the hooks on targets no other plugin hooks. `ltbf_registerNatives` (called by the launcher's NATREG step
  after every plugin's Main) registers the types (FlightModule RegisterTypes, the 12 Engine RTTI classes, RedLib's queue), then the
  post-registration, the TweakXL tweaks and the ArchiveXL archive, loads FMOD, and attaches the two hooks shared with Codeware.
- The two hooks on Codeware's targets (Entity_Initialize 0xC985EC, ScriptValidator_Validate 0x21FBF90) join Codeware's hook chain
  (`codeware_mac_hook_chain_attach`); if Codeware is loaded and refuses, the hook is REFUSED and its feature is skipped; a direct
  attach happens only when Codeware is not loaded. They are not in `macos/hook-offsets.txt` (Codeware's list has them).
- Dependencies (R42a): RED4ext-macos loads the plugins in folder order and runs each Main right away, so ArchiveXL, TweakXL or
  mod_settings can be present but not loaded yet at LTBF's Main. There a dependency whose dylib exists counts as found ("present,
  loads later"); nothing is dlopened out of order. Main never returns false on macOS because of a dependency: the offline script
  cache contains LTBF's scripts either way, and an unloaded LTBF means failed script links and a crash (Boot B). The real check
  (loaded, `Query()` version) runs in `ltbf_registerNatives`: a missing or too old TweakXL skips only the tweaks, ArchiveXL only
  the archive; the natives and types are always registered. Mod Settings is optional and only logged.
  Offline test: `macos/tests/deps_test.cpp` (via `run.sh`).
- Native script properties (R42b): the `@runtimeProperty("offset", ...)` properties of LTBF's classes get their RTTI type in the
  ScriptValidator hook (RuntimeProperties.cpp). On macOS the four CreateCRTTI*TypeFromClass calls are `CRTTISystem::GetType` by
  name, and that name must be in the CName pool (`CNamePool::Add`): GetType only creates a type that does not exist yet from the
  name's text. Every property whose class or type cannot be resolved, or whose CProperty cannot be created, is logged as an error
  with class, property and type.
- Archive (R42b): by NATREG ArchiveXL's depot is already initialized, so the packaging installs `let_there_be_flight.archive` and
  `.archive.xl` into `archive/Mac/mod/`, which ArchiveXL mounts at startup. When that file exists `ltbf_registerNatives` does not
  register the archive (one log line); otherwise it still tries `ArchiveXL::RegisterArchive`.
- FlightSettings setup (R42a): the Windows build runs it in the Running game state. RED4ext-macos runs no game-state callbacks, so
  the value maps are created in `ltbf_registerNatives` and the setup (the script `OnAttach`) runs once from
  `FlightSystem::OnInitialize`. If the game never initializes FlightSystem (the GetGameSystemsData hook not active), the setup does
  not run either and the settings stay at their defaults.
- Every other hook: one attach attempt, a log line (INSTALLED, CHAINED, NOT ACTIVE when the offset is not on the allowlist, REFUSED,
  FAILED). The Windows build retries forever.
- CRUID::Next calls the game's own accessor (0x91248C, the counter at 0x8DC7EB0).
- The four CreateCRTTI*TypeFromClass calls become `CRTTISystem::GetType("array:" / "handle:" / "whandle:" / "raRef:" + name)`, which
  registers the type itself (so the explicit RegisterType is skipped). LoadResRef becomes `ResourceLoader::LoadAsync<T>(path)`.
  GetMatrixFromOrientation is the R41b formula (`macos/tests/matrix_test.cpp`).
- `InkWidget.SetBlurDimension` tests the effect class instead of calling `GetEffectType` (vtable slot 0xE0 on macOS) and writes the
  blur dimension into the effect instance (the Windows line casts the handle's own address). `InkMaskWidget.SetAtlasResource` calls
  the static `IsValid` without the SDK's game-system context lookup (CGameEngine is not mapped on macOS).
- `LTBF_MAC_NO_AUDIO=ON` builds against `macos/fmod-stub` (silent stand-ins, nothing linked).

## 5. Before the first boot with audio

The FMOD libraries copied from the FMOD dmg carry the `com.apple.quarantine` flag. On 2026-10-12 a dlopen of the staged plugin in a
plain test process was refused ("library load disallowed by system policy") and the staged `libfmod.dylib` disappeared from the
folder right after (most likely the Gatekeeper "Not Opened" dialog's Move to Trash). It was copied again. Until the flag is cleared
on `ltbf/fmod-2.04.00` and the staged copies, the game would refuse the same load and LTBF (which links libfmod) would not load at
all. The no-audio build (`macos/build-noaudio`) has no FMOD dependency.

## R42f layout findings (2026-10-12)

- FIXED: `FlightSystem.playerComponent` is addressed by the scripts at the Windows offset 0xB8; libc++'s `std::unordered_map` (0x28) is
  smaller than MSVC's (0x40), so on macOS the member sat at 0xA0 and script writes landed past the end of the object (heap corruption in
  world). FlightSystem.hpp pads the map to 0x40 on Apple. The SDK defines `RED4EXT_ASSERT_OFFSET` as nothing under clang; stdafx.hpp
  now re-enables it for LTBF's own headers, so every script-visible offset of LTBF's classes is checked at compile time.
- OPEN: `vehicle::WheeledPhysics.insert2` compiles to 0x5D8 with a 0x188 stride; the macOS game uses 0x5D0 and 0x190. Only read by
  `WheeledObject.GetDampedSpringForce` (wrong value, no write). Needs the macOS layout of `VehiclePhysicsInsert2` and of the bytes
  between `insert1Count` and `insert2`.
- VehicleObject `acceleration`/`deceleration`: FIXED in R42g (below). `handbrake` 0x270 and `turnX` 0x27C: still OPEN (below).
  `isOnGround` 0x25C and `ignoreImpulses` 0x699 match the macOS SDK.
- Script writes to game classes checked against the macOS SDK layouts: MeshComponent visualScale/meshAppearance/motionBlurScale/LODMode
  and VehicleObject.ignoreImpulses match. PhysicalMeshComponent visibilityAnimationParam/startInactive differ (macOS 0x210/0x222 vs
  script 0x228/0x23A). No script uses them, but the game itself did: this was the in-world crash, FIXED in R42g (below).

## R42g finding (2026-10-12): duplicate runtime properties, the in-world crash

- FIXED (0d722d2): 13 of the scripts' `@runtimeProperty("offset", ...)` fields name properties the game class already reflects:
  - IComponent `name`/`appearanceName`;
  - MeshComponent `visualScale`/`objectTypeID`/`chunkMask`/`motionBlurScale`/`LODMode`/`order`;
  - PhysicalMeshComponent `visibilityAnimationParam`/`startInactive`;
  - VehicleObject `isOnGround`/`acceleration`/`deceleration`.

  Upstream's ProcessScriptTypes hook adds a second CProperty with the same name at the script's Windows offset. On Windows the offsets
  agree, so the duplicate does no harm. On macOS, PhysicalMeshComponent's two fields sit 0x18 lower, so the duplicates point 0x18
  bytes past the real fields, probably past the end of the object.

  Every in-world boot crashed while sectors streamed (8 in a row), always in pool allocator code. The cause is the game's own use of
  the class's properties, not the scripts. The exact game path was not traced. The most likely one: the game fills components from
  templates by property name, and the name now resolves to the duplicate.

  On macOS the hook now skips any property that the class or a parent already has, and the script binds to the game's property at the
  macOS offset. Each skip logs "the game class already has it". Boot G1 (world_smoke, full flight set) PASSED.
- FIXED by the same change: VehicleObject `acceleration`/`deceleration`. The game has them at macOS 0x264/0x268 (script 0x268/0x26C).
- RESOLVED (R43, same as Windows): the vehicle input block is identical on both platforms. The game's input writer (Windows
  0x2E54FC, macOS 0x12B169C) stores, by action name hash: 0x264 Accelerate, 0x268 Decelerate, 0x26C Handbrake, 0x270 StrafeY
  (0x274 StrafeX on the other path), 0x27C LeanFB, 0x280 RockFB. The game reflects `acceleration` 0x264 / `deceleration` 0x268,
  which is right (the jackhumbert SDK header has acceleration at 0x264 too). So the scripts' `@runtimeProperty` offsets
  (acceleration 0x268, deceleration 0x26C, handbrake 0x270, turnX 0x27C) are one slot off on Windows as well; that is an upstream
  detail, used only for flying vehicles the player does not drive (FlightComponent OnUpdate, the not-mounted branch). On macOS,
  acceleration/deceleration now read the game's own (correct) properties; `handbrake` and `turnX` keep the Windows offsets, so
  macOS behaves like Windows there. Mentioned in the upstream issue.
- `ignoreImpulses` 0x699 and `meshApperance` 0x190: same on both (code evidence: the tweak writer and the impulse applicator; two
  macOS loads next to chunkMask/motionBlurScale/LODMode and the Windows registrar).
- Not used by any script, so left as is: Entity `entityTags` 0x138 (no code walks it; neighbouring fields unshifted, UNSURE) and
  VehicleObject `tracePosition` (script 0xB04; the 12-byte trace vector is Windows 0xB14 / macOS 0xAF4 in this game build).
  Evidence: the input writer disassembled on both platforms and its action name hashes matched to the names in inputContexts.xml.
- WATCH: G1 logged 26 FlightComponent attach/detach pairs and 0 destructor calls (LTBF_MAC_DIAG=1). Either the game keeps despawned
  vehicles cached, or a strong reference leaks. It is not a crash. Check in R43 with a longer drive.

## R43 in-world flight test (2026-10-13)

Automated test (`ltbf_flight`): an Archer Hella spawned next to the player in Kabuki, the player mounted as driver, flight toggled with
the O key. Works: Flight_Toggle through the player state machine, Space (Lift) raises the car (1.6 to 2.2 m in 3 s), yaw (up to 167
degrees in 4 s), flight off lands the car, Mod Settings values reach the mod, no crash in 14 in-world boots.

- OPEN: forward thrust. In Hover & Fly mode LTBF produces the expected force along the car's forward vector (about 890 per frame for
  surge 1.0), but the car's speed levels off at about 0.47 to 0.49 m/s (1 to 2 m in 6 s), every run, at the test spot. Checked and not
  the cause: LTBF's collision damping (its timer is at the recovered value), the game's resistance pass (it does not change the
  velocity), the area speed limiter (LTBF skips it while flying), tire grip (the car hovers about 1 m up), the thruster share (100%),
  the flight mode (Drone mode just settles on the ground without lift input). Not yet known: whether this is the spawned test vehicle
  or the test spot, or a velocity clamp in the macOS physics path (LTBF_MAC_DIAG=1 logs `ForceSetLinearVelocity` callers; the samples
  so far are parked vehicles). Next: a hand test in an open area with an owned car, then a filtered log of the flying car's
  Recalculate / ForceSetLinearVelocity calls.
- Key bindings: LTBF's bindings use `overridableUI`, which needs settings entries the Mac input loader does not create; the Mac copy of
  the key file drops the attribute (see README). The keys are not rebindable in the Controls menu on macOS.
