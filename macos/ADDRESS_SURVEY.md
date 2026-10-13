# Let There Be Flight and Mod Settings: Mac address and layout survey (R40, Stage 0)

Read-only survey, 2026-10-12. No game launch, no build, no probes. Every address below was read from the game binaries.

- Sources: `let_there_be_flight` v0.3.18 (7721e31, branch `macos`), `mod_settings` v0.2.21 (4231819, branch `macos`). The SDK is jackhumbert's fork (`deps/red4ext.sdk`, b5ea591, branch `new-types`), and `red_lib` is jackhumbert's `jack` branch (2d9bb93).
- Mac binary: `Cyberpunk 2077.app/Contents/MacOS/Cyberpunk2077` (Steam 2.3.1). Windows reference: the Windows `Cyberpunk2077.exe` of the same game build with `cyberpunk2077_addresses.json`.
- RVA conventions: Mac RVA = address - 0x100000000. Windows RVA = `0001:` offset + 0x1000.
- Inventory sources:
  - LTBF's `src/red4ext`, plus the fork headers and `red_lib` code LTBF actually calls.
  - A hash scan of the release DLL `let_there_be_flight.dll` (sha256 `1b47a2a0...4831f`, from the Nexus release zip). It looks for `mov r32, imm32` of a known address hash: 115 hashes.
  - Mod Settings has no release DLL here, so its inventory comes from source only.
- Tools: `tools/addr-match` (`rx`, `match2.py`, `match3.py`, `enc.py`, `riprefs.py`, the check-rows heuristics through `score`). The survey scripts are listed at the end.

## Method

1. **Vtables.** The 20,294 Windows vtables (`.rdata` LEA targets) were matched to the 11,134 Mac vtables (`__DATA_CONST` address points after a null typeinfo).
   - Matching used per-slot body features (field offsets, float constants, size, call count).
   - Override fingerprints settle which subclass is which: the slots where one class differs from its siblings.
2. **Call alignment.** For each matched pair of functions, the ordered call lists were aligned. A callee in the same position, with matching features, is a lead.
3. **Confirmation.** Every lead was confirmed by comparing the two bodies with `rx w` and `rx m`: same field offsets, constants, call shape and branch structure.
4. **Other anchors.** Where no caller pair existed, the anchors were:
   - encoding searches (`enc.py` style), and
   - 64-bit CName hash constants: `mov r64, imm64` on Windows, `movz/movk` on Mac.
5. **Rating.** Each row got the `check-rows.py` heuristic rating (`win_features`, `mac_features`, `verdict`). Rows the heuristic cannot rate get a written reason in `check-rows.reviewed.tsv`.

## Counts (Gate S)

| Group | Items | MAPPED | INLINED | UNSURE | ALREADY |
|---|---|---|---|---|---|
| LTBF hooks | 27 | 24 | 1 | 0 | 2 |
| Mod Settings hooks | 3 | 2 | 0 | 0 | 1 |
| LTBF direct calls (game functions LTBF calls) | 23 | 8 | 2 | 13 | 0 |
| Mod Settings direct calls | 3 | 0 | 0 | 3 | 0 |
| Vtable slots LTBF uses (calls, overrides, base calls) | 16 | 13 | 0 | 2 | 1 |
| Struct field offsets LTBF reads or writes | 38 | 30 located (18 same, 12 mismatch) | n/a | 8 | n/a |

The direct calls include PhysicsStructUpdate, which is also a hook (one row in each table). In the offsets row, "located" means a Windows and Mac function pair shows the offset on both sides; the mismatches are listed under layouts.

The release DLL also embeds SDK-core hashes from the fork. With our Mac SDK as the base (plan Stage 1):
- 50 are ALREADY (Mac SDK rows or handlers, or Codeware rows);
- 9 are Mac traps, the same as in Codeware (`MacTrapTable.inc`, `NOT_YET.md`);
- 4 are fork-only VFT data hashes, used for base-class calls (see the vtable section);
- 3 are false positives (immediates with no source reference).

## The 5 core flight hooks

| Hook | Mac | Status | Note |
|---|---|---|---|
| VehicleHelperUpdate 3281786499 | 0x1391684 | MAPPED | WheeledPhysics vtable slot 0x60 (Windows 0x58). It is the same function in the Bike, Car and Tank vtables. The hook writes `driveHelpers.size`, which is at a different offset on Mac (see layouts). |
| ApplyForceAtPosition 611586815 | 0x1359264 | MAPPED | A standalone function with 7 call sites in 6 functions. Its torque half is inlined inside it on Mac, where Windows tail-calls ApplyTorqueAtPosition. Blocking it blocks both, as on Windows. |
| ApplyTorqueAtPosition 3303544265 | 0x13592EC | MAPPED | 5 call sites (FourWheelTorque 0x1398B58, 0x127BEE4 x3, 0x127DB48). On Mac its body is also inlined into 0x13593AC, the twin of Windows 0x336C14 ("apply angular impulse"). Clean extra entry: hook 0x13593AC too (callers RollFactorTorqueThing, which LTBF already skips in flight, and 0x135B22C). |
| VehicleUpdateOrientationWithPID 1414536155 | 0x12DC200 | MAPPED | Vehicle object vtable slot 0x2B0 (Windows 0x2A8), in the two Mac vtables whose override fingerprint matches the Windows Car and ArmedCar vtables exactly. |
| ProcessAirResistance 2526549425 | 0x1392998 | MAPPED | 5th call of the physics step 0x1391AE4, which is called from job body 0x139B574 (Windows 0xC6A0CC). ApplyAirResistance is inlined into it on Mac; LTBF's hook calls that function directly, see blockers. |

All five have a clean Mac entry, so none of the core hooks triggers the no-go rule.

## The 7 small targets (at most 0xB4 bytes on Windows)

| Target | Mac | Status |
|---|---|---|
| ApplyForceAtPosition 611586815 | 0x1359264 | MAPPED (standalone; torque part inlined inside it) |
| ApplyTorqueAtPosition 3303544265 | 0x13592EC | MAPPED (plus an inlined copy in 0x13593AC, which is hookable) |
| PhysicsUnkStructVelocityUpdate 4067628720 | 0x13598E0 | MAPPED (same 10000.0 limit, stores velocity at +0x18) |
| GetFxForMaterial 2081691750 | none | INLINED twice into WheelEffectUpdate 0x133BD08 (blocks at 0x133C534 and 0x133C67C) |
| ProcessAirResistance 2526549425 | 0x1392998 | MAPPED |
| TPPCameraStats_Update 4125170300 | 0x1208A04 | MAPPED (same copy of the camera data to camera+0x330..0x38A) |
| VehicleHelperUpdate 3281786499 | 0x1391684 | MAPPED |

## LTBF hooks (27)

The rating column is the `check-rows.py` verdict on the pair. Every Mac RVA is a function start (LC_FUNCTION_STARTS).

| Hash | Name | Win RVA | Mac RVA | Status | Rating | Evidence | ABI notes |
|---|---|---|---|---|---|---|---|
| 2879787320 | CarSuspension_AnimationUpdate | 0x2E017C | 0x12DE77C | MAPPED | OK (offsets 8/28, floats -8,-4,-1,1,4,8 shared) | 6th call of Car physics vslot W 0x40 (0x2DFDE0) = M 0x48 (0x12DEB04); both call lists align (vcalls 0x1A0/0x1A8/0x128 = Mac 0x1A8/0x1B0/0x130). | x0 = CarPhysics*, s0 = dt. The hook reads `parent`: layout mismatch. |
| 501486464 | UpdateVehicleCameraInput | 0x2E6010 | 0x12B1BF8 | MAPPED | STRONG (17/28) | Caller W 0x2E5368 = M 0x12B163C: 0x12B045C, 0x12B169C, the target, 0x12AF730, then tail call through vslot 0x388 (W 0x380). Two Mac inlined copies of the caller also call 0x12B1BF8. | x0 = BaseObject* |
| 3303544265 | vehiclePhysicsData_ApplyTorqueAtPosition | 0x336D44 | 0x13592EC | MAPPED | OK | (offset - CoM[+0x170]) x torque added to +0xC..+0x14 on both. Called by FourWheelTorque on both sides. | x0, x1, x2 pointers; same |
| 898639042 | ProcessScriptTypes | 0x3CCBA0 | 0x21FBF90 | ALREADY | n/a | Codeware `MacAddressResolver.cpp` row (ScriptValidator_Validate). | bool(validator, ScriptBundle*, report): same three pointers |
| 3191280029 | BikeSuspension_AnimationUpdate | 0x400F60 | 0x12D2738 | MAPPED | OK (floats -1,1) | 6th call of Bike vslot W 0x40 (0x400E88) = M 0x48 (0x12D2CE8). | x0, s0. Bike fields are shifted on Mac (layouts). |
| 2531201123 | FPPCameraUpdate | 0x4E1638 | 0x357DD5C | MAPPED | OK (11/28; same 0x4D8, 0x42C, 0x430, 0x43C) | Same entry test `[this+0x4D8]`, same stores; Mac inlines 0x4E2968 ('YawInput'). Callers: 4 on both sides, one through FPP vtable W 0x31D4AD0 slot 0x60 = M 0x72A8D78 slot 0x68. | x0 = this, s0..s4 = five floats, w1 = bool (the C++ prototype maps directly) |
| 2932936864 | WheelEffectUpdate | 0x576FE4 | 0x133BD08 | MAPPED | STRONG (37/64) | Called by W 0x67EC48 = M 0x133B5F4 (both end with 5 calls of W 0x577978 = M 0xB3A06C), reached from Car physics vslot W 0x20 = M 0x28. Mac inlines GetFxForMaterial twice. | x0 = Effects*, w1 = wheel index |
| 3590925758 | TireTrackEffectStart | 0x578094 | 0x134199C | MAPPED | OK | Called right after the second inlined GetFx when it finds an fx. Takes 8 arguments in x0..x7 exactly like LTBF's prototype (effects, unk30, fx, a4, a5, three bools). | x0..x7 |
| 4067628720 | PhysicsUnkStructVelocityUpdate | 0x6F4B08 | 0x13598E0 | MAPPED | OK | Same body: \|v\|^2 < 10000.0 and not NaN, then velocity stored at +0x18..+0x20. 5 call sites on both sides. | void on both. LTBF reads a `short` return (Windows leftover AX); on Mac x0 is still `this`, so recompute the test. |
| 593628708 | PhysicsStructUpdate | 0x6F4B68 | 0x1359958 | MAPPED | OK (9/19, float 10000.0) | RigidBody::Recalculate: same offsets (inverseMass +0x40, force +0, torque +0xC, velocity +0x18, angular velocity +0x24, inertia rows +0x110/+0x120/+0x130, orientation +0x160, output +0x30). Last call of the physics step on both sides. | void on both; same note as above |
| 611586815 | ApplyForceAtPosition | 0x6F4CAC | 0x1359264 | MAPPED | heuristic NO-BODY (Windows leaf without unwind data); reviewed | Body twin: force +0..+8, then torque about CoM +0x170 into +0xC..+0x14. Called from the per-wheel force function W 0x6F3D98 = M 0x1398D8C. | x0, x1, x2 |
| 1989218322 | RollFactorTorqueThing | 0x6F6CB4 | 0x130D970 | MAPPED | OK (10/25, floats 0.01, 0.25, 1) | Called by WheeledPhysics vslot W 0x188 (0x6F5554) = M 0x190 (0x13968C8) near the end, followed by PhysicsStructUpdate on both. Helper argument `a1[2]` (+0x10) is the physics pointer on both sides. | x0 = helper, s0 |
| 3281786499 | VehicleHelperUpdate | 0x6F8A64 | 0x1391684 | MAPPED | OK | Vtable slot W 0x58 = M 0x60 in Bike W 0x2B40A18 = M 0x6F15BB0, Car W 0x30DB880 = M 0x6F164C8, Tank W 0x2B41788 = M 0x6F24E68. Windows callee 0x6F8518 (wheels and drive helpers) is inlined on Mac. | x0, s0; `uintptr_t` return is a pass-through |
| 283779224 | GetLocationFromOffset | 0x70C4A8 | 0x12048E8 | MAPPED | OK (5/22) | Caller W 0x70B760 = M 0x1205394 (it also calls W 0x70BA54 = M 0x1205C60). | **Result through x8 on Mac** (`mov x19, x8` ... `str q0, [x19]`). Mac args: x0 = camera, x1 = preset. Windows: rcx = camera, rdx = out, r8 = preset. Needs an Apple prototype. |
| 2719293980 | UpdatePitch | 0x70CA10 | 0x12080F0 | MAPPED | STRONG (18/29) | Single caller W 0x70BA54 = M 0x1205C60 on both. | x0 = camera |
| 3490519617 | Entity_InitializeComponents | 0x8B0190 | 0xC985EC | ALREADY | n/a | Codeware `MacAddressResolver.cpp` row (Entity_Initialize). | void(Entity*, JobQueue&, request*): same three pointers |
| 2526549425 | ProcessAirResistance | 0x8B44E4 | 0x1392998 | MAPPED | heuristic MISMATCH; reviewed | Physics step M 0x1391AE4 calls the twins of W 0xC6A0CC's callees in the same order: 0x6F4DA8, 0x8B4070, 0x6F3C04 (= M 0x1392810), vcall 0x188 (= M blr 0x190), this function, 0x8B42BC, then PhysicsStructUpdate. Same 10000.0 test. ApplyAirResistance is inlined; ApplyLowSpeedResistances is a call to 0x1397A48. | x0, s0 |
| 2150896908 | VehicleDetachPart | 0x9272FC | 0x13057E0 | MAPPED | OK (8/19) | Found by `enc.py ldrb:0x240 ldrx:0x80 ldrx:0x70`: parts at [this+0x80], stride 0x258, state byte +0x240 set to 2, [this]->[+0xB8]->[+8]->[+0xA0], [this+0x70] 16-byte entries. All the same on both sides. | x0 = manager, w1 = index, w2 = bool |
| 2268865059 | FourWheelTorque | 0xB781AC | 0x1398B58 | MAPPED | OK | Called twice from W 0x6F3C04 = M 0x1392810 with (0,1) and (3,2). Ends with ApplyTorqueAtPosition and PhysicsStructUpdate on both. | x0, w1, w2, s0, x3 = transform (matches the prototype) |
| 2081691750 | GetFxForMaterial | 0xBD0A74 | none | INLINED | n/a | Inlined twice in M 0x133BD08: `ldrb [x20,#0x160]`, maps at +0x100/+0x168, fallback key [x20,#0x1C8], returns node+0x10. These are the Windows offsets. | Clean alternative: do the fx swap in the SkidMark and TireTrack start hooks, which get the fx in x2 (blockers). |
| 1554324956 | UpdateTireParameters | 0xC6F2DC | 0x128AE3C | MAPPED | STRONG (10/20) | Chain from vehicle vslot W 0x380 (0x2DE3EC) = M 0x388 (0x139E2E4) -> W 0x19DBC4 = M 0x128B23C -> W 0x19F0F4 = M 0x1289F34 -> target. | x0 |
| 2693082452 | SkidMarkEffectStart | 0xD06700 | 0x1340F28 | MAPPED | OK | Called right after the first inlined GetFx when it finds an fx. Prologue twin (+0x50, +0x20, +0x30, the same helper calls). | x0..x7 (8 arguments) |
| 2463305925 | GetGameSystemsDataHook | 0xDF2DE0 | 0x3317EF8 | MAPPED | OK | Systems table rows: columns 0..4 through the C2dArray getter 0x3EFF674 (W 0x3CA8F0), name, then 4 bools; 16-byte GameSystemData. Callers: 0x3318100 (inside W 0x1620AC = M 0x3314FA0, from vtable W 0x2B74EA8 slot 0x20 = M 0x6FB1FA0 slot 0x28) and 0x3317EA8. | x0 = out array; void on Mac (callers keep x19) |
| 4125170300 | TPPCameraStats_Update | 0x2049284 | 0x1208A04 | MAPPED | OK (12/33) | Same copy of data[0..0x5A] to camera+0x330..0x38A. Caller W 0x6FC320 = M 0x11EE4F8 (inside W 0x2DEFDC = M 0x12B2B44). | void on Mac; LTBF's `uintptr_t` return is a pass-through |
| 1414536155 | VehicleUpdateOrientationWithPID | 0x2610C64 | 0x12DC200 | MAPPED | OK (floats 1e-05, 0.05, 1, 4 shared) | Vehicle object vslot W 0x2A8 = M 0x2B0 in W 0x2B40FC8 = M 0x6F13A18 and W 0x2B413B0 = M 0x6F15FA8 (override fingerprints identical: slots 41, 103, 104, 106, 107, 113-119, 122). | x0, x1 = transform, s0, s1 |
| 1836326624 | AreaSpeedLimiter_Update | 0x261A278 | 0x1242F44 | MAPPED | STRONG (string 'default', 22/36) | Called 4 times from W 0x6FB430 = M 0x136EDBC (vcall 0x200 = Mac blr 0x208). | x0 |
| 4124321726 | GetYaw | 0x70AE10 | 0x12073C8 | MAPPED | STRONG (11/19, float 360) | Single caller W 0x70BA54 = M 0x1205C60. | x0, x1, x2, s0 |

## Mod Settings hooks (3)

| Hash | Name | Win RVA | Mac RVA | Status | Evidence and notes |
|---|---|---|---|---|---|
| 3298233196 | SkipKey | 0x21F01C0 | 0x48026F0 | MAPPED | Same test: key-0x88 < 0x14, key-0xFF < 2, key-0x5B < 2. It is the only copy of this check in the Mac text. **No static caller found** (no bl/b/adr/data reference), so the caller probably reaches it indirectly. Check INSTALLED and the call count in Stage 4. |
| 1773342692 | ApplyOverrides | 0x1868498 | 0x1D735A4 (8-byte thunk) | MAPPED | Input manager vtable W 0x2B1B170 slot 0x250 = M 0x6F94398 slot 0x258. On Windows it is a 14-byte thunk (`[rcx+0x10]` -> `[+8]` -> jmp 0x4A61C4). The Mac chain is 0x1D735A4 (`ldr x0,[x0,#0x10]; b`) -> 0x1D5C774 (`ldr x0,[x0,#8]; b`) -> body 0x1D792E4 (= W 0x4A61C4, same userMapping+0x98). The hook target is only 8 bytes: see blockers. |
| 3993832650 | ReleaseScriptData | 0x7B7C18 | 0x21ECFD8 | ALREADY | Codeware `MacAddressResolver.cpp` row (ScriptBundle_dtor). |

## LTBF direct calls (game functions LTBF calls itself)

| Hash | Name (source) | Win RVA | Mac RVA | Status | Evidence, or the clean alternative |
|---|---|---|---|---|---|
| 593628708 | vehicle::PhysicsStructUpdate (FourWheelTorque hook) | 0x6F4B68 | 0x1359958 | MAPPED | Same row as the hook. |
| 3489929719 | WheeledPhysics::ApplyAirResistance (fork StaticHook) | 0x8B469C | none | INLINED | Inside ProcessAirResistance 0x1392998 (0x13929F8-0x1392A94): drag [this+0xC80] (Windows +0xC90), factor -1.2, 0.01 threshold, AddForce 0x1359090 on physicsData, speed to [[vehicle+0x5D0]+0xEC]. See blockers. |
| 1695226878 | WheeledPhysics::ApplyLowSpeedResistances | 0x8B4584 | 0x1397A48 | MAPPED | The only callee of the inlined block; one caller on both sides. Rating OK. |
| 1210388152 | physics::ProxyHelper::Create | 0x2405EC | 0x4095DF8 | MAPPED | Same body: zero +0/+8, id at +0x10, manager global (Mac 0x90DF0B0), validity check, proxy -> [this], second lookup -> [this+8]. 109 Mac callers, 119 Windows callers. Rating OK. |
| 200045 | ProxyHelper::UpdateProxyCache | 0x240720 | 0x4095D54 | MAPPED | If [this+8]: call with [[this+8]] and clear [this+8]. Called right after Create in the per-wheel force function on both sides. Rating WEAK (tiny); reviewed. |
| 1189351231 | ProxyHelper::Unlock | 0x1A5EA4 | none found | UNSURE | 14-byte leaf (release of the spinlock byte at [this+0x18]); no standalone Mac copy found, probably inlined. Alternative: release `helper.mutex` with the Mac SDK SharedSpinLock. |
| 3124696465 | ProxyHelper::SetSimulationShape | 0x204ACA8 | 0x40953B4 | MAPPED | Same wrapper: shape flag 0x2A, [this], [this+8], [this+0x10] into the setter 0x4092A50. Rating OK. |
| 811668261 | ProxyHelper::SetIsQueryable | 0x204AD24 | 0x4095364 | MAPPED | Same wrapper with flag 0x29. Rating OK. Windows passes body index 0 and the Mac copy passes its argument; LTBF calls it with 0. |
| 3094614336 | physics::FilterData::LoadPreset | 0x7318C8 | 0x3FFFC20 | MAPPED | CName-hash anchor vote (28 votes) plus body: preset lookup, store at +0x50, tail call (W 0x246CA8). Rating OK. |
| 2725251676 | physics::FilterData::Init | 0x247148 | none | INLINED | Default constructor; the Mac FilterData vtable 0x729EFD0 is stored only by composite constructors. Alternative: Mac 0x3FFFC54 builds a FilterData from a preset name (x8 = result, x0 = CName), which is exactly LTBF's Init then LoadPreset("Vehicle Chassis"). Or construct through RTTI. |
| 1570510966 | physics::ProxyID::GetProxy | 0x2A22B8 | not found | UNSURE | Alternative: LTBF builds a ProxyHelper for the same id just before it, and `helper.proxy` ([helper+0], from 0x4056330) is the proxy. |
| 688524786 | ent::SlotComponent::GetSlotIndex | 0x1C68B4 | not found | UNSURE | 48-byte wrapper over the map at +0x150. Alternative: read the map in the plugin, or call the script natives. |
| 2420184407 | ent::SlotComponent::GetLocalSlotTransformFromIndex | 0x4E9790 | not found | UNSURE | Alternative: script natives (GetSlotTransform). |
| 1320231254 | physics::ColliderSphere::createHandleWithRadius | 0x210D844 | not found | UNSURE | One Windows caller. Alternative: RTTI CreateInstance of `physicsColliderSphere` (Mac CClass_CreateInstance is mapped) and set the radius. |
| 61607255 | action::ActionInterface::EndActions | 0x8630A0 | not found | UNSURE | Only behind the script helper `VehicleObject.EndActions`; can be dropped. |
| 2793083181 | vehicle::BaseObject::ForceEnablePhysics | 0x25F78A4 | 0x12AF1FC | MAPPED | Same body: [this+0x2C8] physics woken, loop over the 32 bits of [this+0x260] calling the toggle (W 0x5F7600 = M 0x12AF264, w2 = 1). Rating STRONG. |
| 1404578149 | CreateCRTTIArrayTypeFromClass | 0x1749FC | not found | UNSURE | Alternative (as RedLib and Codeware do on Mac): look up `array:<name>` through CRTTISystem. |
| 682896186 | CreateCRTTIWeakHandleTypeFromClass | 0x173A08 | not found | UNSURE | Alternative: `whandle:<name>` lookup. |
| 1992632754 | CreateCRTTIHandleTypeFromClass | 0x174298 | not found | UNSURE | Alternative: `handle:<name>` lookup. |
| 2481073725 | CreateCRTTIRaRefTypeFromClass | 0xC4B510 | not found | UNSURE | Alternative: `raRef:<name>` lookup. |
| 1157708450 | LoadResRef<T> | 0x233B68 | not found | UNSURE | Alternative: ResourceLoader rows already mapped on Mac (IssueLoadingRequestByPath 0x21B7130 and friends). |
| 2124484623 | GetMatrixFromOrientation | 0x1DA684 | not found | UNSURE | Pure quaternion-to-matrix math (489 bytes, no calls). Reimplement in the plugin. |
| 2027097961 | CRUID counter (data, `CRUID::Next`) | .data 0x3485608 | not found | UNSURE | Windows users 0x2413764, 0x1EDEB8A, 0x682C5C (`lock xadd`, then `*4 | 1`). Needed by the Entity_InitializeComponents hook and MeshComponent. |

Not used, so not mapped:
- GetPermanentBoolParameter 1497441376: `Hooks/StateMachine.hpp` is never included, and the hash is not in the DLL.
- The fork's `/// @hash` declarations that are not in the DLL (UnkD10::Reset and others).
- `RuntimeClass.hpp` (hard-coded Windows VFT 0x35E6D08 + 0xC00): never included.

## Mod Settings direct calls

| Hash | Name | Win RVA | Mac | Status | Alternative |
|---|---|---|---|---|---|
| 2583235069 | CalcHash(const CString&) | 0x5C0BA0 | not found | UNSURE | Pure function: h = h*31 + (signed char) over the CString bytes (length [+0x14] & 0x3FFFFFFF, heap pointer when the length is >= 0x40000000). Reimplement. |
| 3702071356 | UserMapping::GetMappingCollection | 0x4A60D0 | not found | UNSURE | HashMap find at userMapping+0x98; value stride 0x18, values at +0x10. Reimplement (check the Mac HashMap layout first). |
| 4196866905 | UserMapping::ClearRuntimeDataOnRawMappings | 0x4A6A54 | not found | UNSURE | Loop over the same map, clearing byte +0x10 of every 0x18-byte value. Reimplement. |

## SDK-core hashes in the release DLL (fork base)

These come from the fork's own headers, not from LTBF code. With our Mac SDK as the base they resolve the way Codeware does.

- **ALREADY (50).** The scan found 52 already-mapped hashes: 34 rows or handlers in `RED4ext.SDK-macos/include/RED4ext/Relocation-inl.hpp` and 18 rows in Codeware's `MacSdkRows.inc` and `MacAddressResolver.cpp`. Two of the Codeware rows are the LTBF hooks above, which leaves 50 SDK-core rows:
  - Memory_Vault and the five Vault functions;
  - the CClass vslot handlers;
  - Handle_ctor and Handle_DecWeakRef;
  - CString, CNamePool, DynArray_Realloc and CRTTISystem_Get;
  - the CClass, CBaseFunction and CStack rows;
  - the ISerializable defaults;
  - JobHandle_dtor, ResourceLoader, UpdateRegistrar_RegisterBucketUpdate and others.
- **Mac traps, as in Codeware (9).**
  - CBaseRTTIType_sub_80/88/90/98/A0 (2396396482, 2757309857, 364779332, 3832485534, 2930319133);
  - Memory_PoolStorage_OOM 3578730042;
  - Memory_Vault_Unk1 434901863;
  - ResourceToken_DestructUnk38 2508398574;
  - CGameEngine 2549221846 (data).

  These are reached only through the same SDK paths Codeware already lists in `NOT_YET.md`.
- **Fork-only VFT data (4).** IScriptable 1180173983, IComponent 1586431952, gameComponent 331746781 and IGameSystem 1854670959. The fork calls base-class virtuals as "VFT hash + slot". The Mac slots LTBF needs are in the vtable section.
- **False positives (3).** 1207963526, 4160752848 and 1913530389: no source reference.

## Vtable slots

Rule confirmed on the Mac binary:
- For ISerializable-derived classes the destructor is slot 0x18 on Windows and two slots (0x18, 0x20) on Mac. Slots below 0x18 are unchanged; slots from 0x20 on are Windows + 8.
- For plain C++ classes with the destructor first (vehicle physics, PhysX `PxBase` with its destructor at 0x18), the same shift applies after the destructor.

| Class::method | Windows slot | Mac slot | Mac function | Evidence |
|---|---|---|---|---|
| vehicle::WheeledPhysics::VehicleHelperUpdate (hook) | 0x58 | 0x60 | 0x1391684 | Bike, Car and Tank vtables (above). Same nullsub pattern at W 0x80/0x88/0x98/0xF0 = M 0x88/0x90/0xA0/0xF8. |
| vehicle::CarBaseObject::UpdateOrientationWithPID (hook) | 0x2A8 | 0x2B0 | 0x12DC200 | Override fingerprints (above). |
| vehicle::BaseObject vslot 0x380 (UpdateTireParameters chain) | 0x380 | 0x388 | 0x139E2E4 | Same in all Car-family vtables. |
| physx::PxShape::setSimulationFilterData (call) | 0xA0 | 0xA8 | 0x239CE8 | NpShape vtable 0x6E54808: getConcreteTypeName ("PxShape") at 0x08, isKindOf at 0x28 (Windows 0x20). The body writes the filter data from x1. |
| physx::PxShape::setQueryFilterData (call) | 0xB0 | 0xB8 | 0x239E34 | Copies 4 words from x1 to [this+0x50..0x5C]. Slot 0xB0 is the getter (result through x8). |
| physx::PxRigidActor::getNbShapes (call) | 0xC8 | 0xD0 | 0x2231F4 | NpRigidDynamic vtable 0x6E52158: `ldrh w0,[x0,#0x30]`. |
| physx::PxRigidActor::getShapes (call) | 0xD0 | 0xD8 | 0x2231FC | Same vtable: `add x0,x0,#0x28; b` to the shape list copier. |
| physics::ICollider::CreatePxShape (call) | 0xE8 | 0xF0 | 0x400CA08 | physicsColliderSphere vtable W 0x3012338 (from the RTTI constructor and from VFT hash 3014331173) = M 0x729FB50. The ISerializable defaults line up: W 0x2052B84/0x148940/0x711A14/0x1247898/0x4B0D9C = M 0x2186078/0x2185F34/0x21862CC/0x2186230/0x2186208. |
| ink::IEffect::GetEffectType (call) | 0xD8 | 0xE0 | 0x117EF84 | inkBoxBlurEffect vtable W 0x3174A20 = M 0x6F03F90: `mov al,4` vs `mov w0,#4` (BoxBlur = 4). |
| game::Component::OnAttach (base call from FlightComponent) | 0x180 | 0x188 | 0x1F296D8 | gameComponent W 0x2ADF728 against the 8 best Mac candidates: 4/8 agree, similarity 0.86 (the unshifted slot scores 0.09). |
| game::Component::OnDetach (base call) | 0x190 | 0x198 | 0x1F298B0 | 6/8 agree, similarity 0.81. |
| game::Component::GetAllocator (override) | 0x10 | 0x10 | class-specific | Below the destructor, so no shift (the unshifted slot is the best match, 0.49). |
| game::Component::sub_238 (override) | 0x238 | 0x240 | n/a | UNSURE: rule only (+8), not checked against a Mac body. FlightComponent's other overrides (OnEnabled, GetPSClass) follow declaration order; the overlay must keep the destructor where ISerializable has it. |
| IGameSystem overrides (FlightSystem) | n/a | n/a | Mac SDK | ALREADY: our Mac SDK's IGameSystem layout, as used by Codeware's systems. |
| input Manager ApplyOverrides (Mod Settings hook) | 0x250 | 0x258 | 0x1D735A4 | Input manager vtable W 0x2B1B170 = M 0x6F94398 (the neighbouring thunks line up). |
| entIComponent base slots 0x110-0x228 (fork VFT calls) | +0 | +8 | 0xCA63F4 ... | 8/8 Mac candidates agree on every slot at +8 (`vslots.txt`). LTBF itself only calls game::Component::OnAttach and OnDetach. UNSURE for the base implementation behind each slot: the exact base-class vtable is not singled out. |

## Struct layouts LTBF uses

"Win" is the game's Windows offset: the offset the fork must use for LTBF to work there. It is read from the Windows binary where possible and from the fork's comments otherwise. "Mac" is from the Mac twin of the same Windows function.

A layout probe compiled the fork headers with clang's Itanium layout (syntax only, no build). It fails the fork's own size asserts:
- `vehicle::Physics` (0xE0), `WheeledPhysics` (0xDE0), `BikePhysics` (0xE80), `TankPhysics` (0x3E0) and `UnkD10` (0x3410);
- so the fork's physics structs cannot be reused on Mac as they are.

| Type.field | Win | Mac | Status | Evidence (Windows function = Mac function) |
|---|---|---|---|---|
| vehicle::Physics.parent | 0x60 | 0x50 | MISMATCH | Physics::GetVehicle `mov rax,[rcx+0x60]` = M 0x1360EE8 `ldr x0,[x0,#0x50]`; ProcessAirResistance; PhysicsStructUpdate caller |
| vehicle::Physics flag (job body test) | 0x20 | 0x18 | MISMATCH | job body W 0xC6A0CC `cmp [rdi+0x20],0` = M 0x1391AE4 `ldrb w8,[x0,#0x18]` |
| vehicle::WheeledPhysics.insert2 (wheels, 0x190 stride) | 0x5E0 | 0x5D0 | MISMATCH | W 0x6F8518 `lea rax,[rcx+0x5e0]` = M 0x1391684 `add x20,x19,#0x5d0` |
| WheeledPhysics wheel count | 0xC20 | 0xC10 | MISMATCH | same pair |
| WheeledPhysics.driveHelpers (DynArray) | 0xDA8 (size 0xDB4) | 0xD98 (size 0xDA4) | MISMATCH | same pair. The fork's comment says 0xD68, which is stale. The VehicleHelperUpdate hook writes `.size`. |
| WheeledPhysics vehicle pointer used by the helper loop | 0x108 | 0xF0 | MISMATCH | same pair (`mov rdx,[rdi+0x108]` = `ldr x1,[x19,#0xf0]`) |
| WheeledPhysics air drag coefficient | 0xC90 | 0xC80 | MISMATCH | ApplyAirResistance W 0x8B469C = inlined block in M 0x1392998 |
| vehicle::BikePhysics vehicle pointer | 0xDE0 | 0xDC8 | MISMATCH | BikeSuspension W 0x400F60 = M 0x12D2738 |
| BikePhysics fields written by the bike update | 0xE18, 0xE1C | 0xE00, 0xE04 | MISMATCH | same pair. LTBF's `turnRate` and `tiltControlEnabled` sit in this shifted region; their exact Mac offsets are not located (UNSURE). |
| BikePhysics field read at the start | 0xF4 | 0xDC | MISMATCH | same pair |
| vehicle::BaseObject.physics | 0x2C8 | 0x2C8 | SAME | ForceEnablePhysics W 0x25F78A4 = M 0x12AF1FC |
| BaseObject.physicsData | 0x2D0 | 0x2D0 | SAME | getter M 0x12AA2C0 `ldr x0,[x0,#0x2d0]`; Windows `[rax+0x2d0]` everywhere |
| BaseObject.input (turnInput region) | 0x264 | 0x264 | SAME | Bike update reads [vehicle+0x264] on both |
| BaseObject toggle bitmask | 0x260 | 0x260 | SAME | ForceEnablePhysics pair |
| BaseObject fields 0x278 and 0x5D8 | same | same | SAME | Bike update pair |
| BaseObject field 0xBA8 (bike data block) | 0xBA8 | 0xB88 | MISMATCH | Bike update pair: BaseObject members past some point move on Mac |
| vehicle::PhysicsData (RigidBody) force, torque, velocity, angular velocity | 0x0, 0xC, 0x18, 0x24 | same | SAME | PhysicsStructUpdate, ApplyForce, ApplyTorque, the velocity setter and the getter M 0x1359A5C |
| PhysicsData.inverseMass | 0x40 | 0x40 | SAME | PhysicsStructUpdate |
| PhysicsData inertia rows, orientation, output | 0x110-0x13C, 0x160-0x16C, 0x30 | same | SAME | PhysicsStructUpdate |
| PhysicsData centre of mass used for torque | 0x170 | 0x170 | SAME | ApplyTorque and ApplyForce |
| PhysicsData world transform | 0x150-0x16F | same | SAME | M 0x1359D78 = Windows reads of [pd+0x150..0x160] |
| PhysicsData.vehicle (back pointer, used by the ApplyForce and ApplyTorque hooks) | fork n/a | not located | UNSURE | The Itanium probe gives 0x180 for the fork struct; no Mac access matched yet |
| PhysicsData.centerOfMass (fork field used for the TPP camera) | fork n/a | not located | UNSURE | Itanium probe 0x1D4 (differs from the 0x170 the torque code uses) |
| vehicle::TPPCameraComponent vehicle handle | 0x328 | 0x320 | MISMATCH | GetLocationFromOffset W 0x70C4A8 `[rcx+0x328]` = M 0x12048E8 `[x0,#0x320]` (the probe also gives 0x320 for Itanium) |
| TPPCameraComponent.data (CameraParentData copy) | 0x330..0x38A | same | SAME | TPPCameraStats_Update pair; LTBF writes `data.isInAir` inside it |
| TPPCameraComponent drivingDirectionCompensationAngleSmooth, SpeedCoef, cameraPitch, yawOffset | n/a | not located | UNSURE | Written by the TPPCameraStats and GetYaw hooks. Itanium probe: 0x564, 0x55C, 0x4D8. |
| game::FPPCameraComponent fields 0x3F5, 0x42C, 0x430, 0x43C, 0x4D8 | same | same | SAME | FPPCameraUpdate pair |
| vehicle::Effects GetFx fields 0x100, 0x160, 0x168, 0x1C8; wheel data at [this+0x30] (stride 0x80) | same | same | SAME | inlined GetFx in M 0x133BD08 |
| vehicle::Effects.vehicle | n/a | not located | UNSURE | Itanium probe 0x0 |
| VehicleDetachPart Manager: parts 0x80 (stride 0x258), part state 0x240, 0x70 entries | same | same | SAME | VehicleDetachPart pair |
| GameSystemData (name +0, 4 bools +8..+0xB, stride 0x10) | same | same | SAME | GetGameSystemsData pair |
| physics::FilterData preset handle | 0x50 | 0x50 | SAME | LoadPreset pair |
| physics::ProxyHelper (proxy +0, cache +8, id +0x10, lock +0x18) | same | +0/+8/+0x10 same | SAME (lock UNSURE) | Create and UpdateProxyCache pairs |
| DrivingForcesHelper physics pointer (`a1[2]`) | 0x10 | 0x10 | SAME | RollFactorTorqueThing pair |
| vehicle::ChassisComponent.proxyID / sharedMutex | 0x160 / n/a | not located | UNSURE | The probe gives 0x160 / 0x16C |
| MaterialFx `normal.particle.extra_byte_for_fun` (LTBF's own flag byte) | n/a | n/a | UNSURE | LTBF's own spare byte in a game struct; check that it is padding on Mac |

The input-manager structs that Mod Settings reads (Manager, ContextManager, UserMapping) were not surveyed beyond userMapping+0x98, which is the same on both sides (ApplyOverrides body). They are Stage 4 work.

## Blockers and options

1. **Physics struct layouts differ (blocker for a plain port).**
   - `vehicle::Physics`, `WheeledPhysics`, `BikePhysics` and part of `BaseObject` sit at other offsets on Mac (table above). The fork's structs do not even pass their own size asserts under Itanium layout.
   - Option A (recommended): Mac accessors with the verified offsets, Codeware-style OffsetPtr, for each field LTBF touches; the ones verified here are parent, driveHelpers, wheels and the bike block. Locate the remaining UNSURE fields in Stage 1 from these same function pairs.
   - Option B: Apple-only struct definitions with explicit padding (riskier).
2. **ApplyAirResistance is inlined.** LTBF calls it for speed² >= 10000, where the game skips it.
   - Option A: reimplement it in the plugin from the Mac inlined block. It is about 20 instructions: drag = coefficient [this+0xC80] x -1.2 x \|v\|^2, then AddForce 0x1359090, plus the speed store.
   - Option B: drop the extra high-speed drag. Gameplay effect: flying cars above 100 m/s get no air drag from this hook.
3. **GetFxForMaterial is inlined.**
   - Option A: move its logic into the SkidMarkEffectStart and TireTrackEffectStart hooks. They receive the fx in x2 and are only called when an fx was found, which LTBF assumes anyway.
   - Option B: drop the flight trail effects.
4. **GetLocationFromOffset returns through x8 on Mac.** It needs an Apple prototype: Vector4 result in x8, camera x0, preset x1. Not a blocker.
5. **Return values LTBF reads from void functions.** PhysicsStructUpdate and PhysicsUnkStructVelocityUpdate (`short`, compared to FP_INFINITE) read a leftover Windows register. On Mac, recompute the test (\|v\|^2 > 10000 or NaN) in the hook.
6. **ApplyTorqueAtPosition is also inlined in 0x13593AC.** Hook 0x13593AC as well (clean entry, 2 callers). Otherwise external angular impulses on the 0x135B22C path are not blocked in flight.
7. **Mod Settings ApplyOverrides is an 8-byte thunk on Mac.**
   - Option A: swap the vtable pointer at M 0x6F94398 + 0x258 (no code patch).
   - Option B: hook the body 0x1D792E4, which gets userMapping, not Manager, so it needs another route to the Manager.
   - Option C: check whether the hooking driver can patch an 8-byte function (one B to a near trampoline).
8. **SkipKey has no static caller on Mac.** The hook may never fire. Verify in Stage 4 (INSTALLED plus the call count).
9. **13 LTBF direct calls are UNSURE (no Mac function found).** Each has a clean replacement:
   - RTTI name lookups for the four CreateCRTTI*TypeFromClass;
   - the mapped ResourceLoader rows for LoadResRef;
   - local math for GetMatrixFromOrientation;
   - `helper.proxy` for GetProxy;
   - SharedSpinLock for Unlock;
   - script natives or map reads for the slot functions;
   - RTTI CreateInstance for the collider;
   - drop EndActions.

   Only the CRUID counter (data) has no replacement yet. Options: find the Mac global in Stage 1 (Windows users listed), or allocate IDs from a plugin counter in a range the game does not use.
10. **Mod Settings' three direct calls (CalcHash, GetMappingCollection, ClearRuntimeDataOnRawMappings)** are small enough to reimplement once the Mac HashMap layout at userMapping+0x98 is checked.

## Reproduction

Survey scripts (scratchpad, not committed): `wl.py`, `feat.py`, `vt.py`, `vtmatch.py`, `vtshow.py`, `calls.py`, `cand.py`, `who.py`, `macindex.py`, `gsearch.py`, `k64.py`, `stsearch.py`, `rtticls.py`, `dllscan.py`. They all import `tools/addr-match/rx.py` and the check-rows heuristics.

Evidence rows were appended to:
- `tools/addr-match/check-rows.reviewed.tsv` (`ltbf:` and `modsettings:` reasons);
- `check-vtable.evidence.tsv`;
- `check-offsets.evidence.tsv`.
