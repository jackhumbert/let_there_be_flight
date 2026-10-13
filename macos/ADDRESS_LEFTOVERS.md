# R41b: address leftovers of Let There Be Flight and Mod Settings

Read-only pass, 2026-10-12. No game launch, no write probes, no Frida, no sub-agents, nothing downloaded, `engine/tools/scc` not run. Every address below was read from the two game binaries (Mac `Cyberpunk2077` Steam 2.3.1, Windows `Cyberpunk2077.exe`). It settles the UNSURE items of `let_there_be_flight/macos/ADDRESS_SURVEY.md` (R40).

- RVA conventions as in R40: Mac RVA = address - 0x100000000; Windows RVA = `0001:` offset + 0x1000. "Win" and "Mac" columns are RVAs unless a column says offset.
- Every Mac function RVA given as a MAPPED target is a function start (LC_FUNCTION_STARTS) unless the row says otherwise.
- "Rating" is the `check-rows.py` heuristic verdict from `tools/addr-match/score.py <win> <mac>`. It sees field offsets, strings, call counts and size, not wrappers or inlining, so tiny or shifted pairs rate WEAK or MISMATCH and carry a written reason in `check-rows.reviewed.tsv`.
- Evidence rows are committed in `tools/addr-match/{check-rows.reviewed.tsv,check-vtable.evidence.tsv,check-offsets.evidence.tsv}` (commit 7333004, local, not pushed). `check-rows.py` gives the same report before and after (rows 158, vtable 19, offsets 62, props 13; exit 0).

## Counts

| Task | Items | MAPPED | REPLACEMENT-CONFIRMED | INLINED | UNSURE |
|---|---|---|---|---|---|
| 1. UNSURE offsets (survey rows) | 8 | 8 (19 field values: 15 same as Windows, 4 shifted) | 0 | 0 | 0 |
| 2. CRUID counter | 1 | 1 (global + accessor; counted again in task 3) | 0 | 0 | 0 |
| 3. LTBF direct calls | 13 | 5 (Unlock, GetProxy, GetSlotIndex, EndActions, CRUID) | 7 (GetLocalSlotTransformFromIndex, createHandleWithRadius, 4 CreateCRTTI*, LoadResRef) | 1 (GetMatrixFromOrientation, math replacement) | 0 |
| 3. Mod Settings calls | 3 | 2 (GetMappingCollection, ClearRuntimeData) | 0 | 1 (CalcHash, math replacement) | 0 |
| 4. UNSURE vtable slots | 2 | 2 (`sub_238`; the gameComponent and entIComponent base vtables) | 0 | 0 | 0 |
| 5. ApplyOverrides body | 1 | CONFIRMED real entry | | | |
| 6. SkipKey | 1 | MAPPED but UNREACHABLE (0 references; predicate inlined in 0x48038B8) | | | |

New findings that are not in the 38 R40 offsets: 2 corrections to R40 (TPPCameraComponent.vehicle, GetProxy alternative), 1 new layout mismatch (ColliderSphere), 5 verified Mod Settings structure rows.

## Values the build tracks need

| What | Windows | Mac |
|---|---|---|
| BikePhysics `turnRate` (WheeledPhysics member) offset | 0xF4 | **0xDC** |
| BikePhysics `tiltControlEnabled` offset | 0xE20 | **0xE08** |
| BikePhysics `negTurnRate`, `turnInput` offsets | 0xE18, 0xE1C | **0xE00, 0xE04** |
| PhysicsData `vehicle` (BaseObject*), `centerOfMass`, `worldPosition` | 0x180, 0x1D4, 0x170 | same |
| TPPCameraComponent `cameraPitch`, `cameraDirection` (Vector3), `drivingDirectionCompensationSpeedCoef`, `...AngleSmooth` | 0x4D8, 0x4DC, 0x578, 0x580 | same |
| TPPCameraComponent `vehicle` Handle: instance (R40 said 0x328 / 0x320, both were the refcount slot) | 0x320 | **0x318** (refcount 0x320) |
| FPPCameraComponent `yawOffset`, `animFeature` | 0x42C, 0x360 | same |
| `Effects.vehicle`; ChassisComponent `proxyID`, `sharedMutex`; ProxyHelper mutex pointer | 0, 0x160, 0x16C, 0x18 | same |
| MaterialFx `extra_byte_for_fun` (MaterialResource +0x11, padding) | +0x11 | same (padding) |
| ColliderSphere `radius`, `sizeof` | 0x90, 0xA0 | **0x88, 0x90** |
| BaseObject `actionInterface` | 0x3B8 | same |
| CRUID counter global (RVA) | .data 0x3485608 | **0x8DC7EB0** (`int64_t`, in `__DATA` .bss) |
| CRUID::Next accessor | inline `lock xadd` | **0x91248C**, `uint64_t()` in x0 |
| ProxyHelper::Unlock | 0x1A5EA4 | **0x4096010** |
| ProxyID::GetProxy | 0x2A22B8 | **0x405A798** (`void*(uint32_t id)`) |
| SlotComponent::GetSlotIndex | 0x1C68B4 | **0xC463C8** (`int(SlotComponent*, CName)`) |
| SlotComponent slot world transform (the callee of GetLocalSlotTransformFromIndex) | 0x1D81E8 | **0xC47020** |
| ActionInterface::EndActions | 0x8630A0 | **0x345D760** (`void(ActionInterface*)`) |
| UserMapping::GetMappingCollection | 0x4A60D0 | **0x1D796B4** |
| UserMapping::ClearRuntimeDataOnRawMappings | 0x4A6A54 | **0x1D7923C** |
| ApplyOverrides body (entry hook target) | 0x4A61C4 | **0x1D792E4**: real entry, `void(UserMapping*)` |
| SkipKey | 0x21F01C0 | 0x48026F0 has **no reference**; hook never fires; inline copy in 0x48038B8 |
| game::Component `sub_238` | slot 0x238 | slot **0x240** (0x1F29BE8) |
| gameComponent own vptr (fork VFT 331746781); entIComponent own vptr (1586431952) | 0x2ADF728; 0x2AC03F0 | **0x6FB1820**; **0x6ED3698** |

## 1. Struct offsets (the 8 UNSURE survey rows)

Each row: a Windows function that loads or stores the field, its Mac twin, and both offsets. Rating is for the function pair. Fork field names are from `deps/red4ext.sdk`.

| Field | Win | Mac | Status | Evidence | Rating |
|---|---|---|---|---|---|
| WheeledPhysics.turnRate (used by the bike hook) | 0xF4 | **0xDC** | MAPPED, shifted | BikeSuspension_AnimationUpdate Win 0x400F60 = Mac 0x12D2738. Win `movss xmm0,[rcx+0xf4]; xorps sign; movss [rcx+0xe18],xmm0`; Mac `ldr s1,[x0,#0xdc]; fneg s1,s1; str s1,[x0,#0xe00]`. Fork order: numDriveWheels 0xE0 ... frontBackBias 0xF0, turnRate 0xF4. | OK (4/16 offsets, calls 7/8) |
| BikePhysics.negTurnRate, turnInput (same function) | 0xE18, 0xE1C | **0xE00, 0xE04** | MAPPED, shifted | Same pair; turnInput = clamp(`[vehicle+0x278]`, -1, 1); bikeObject Win 0xDE0 = Mac 0xDC8, so the bike block moves by -0x18. | as above |
| BikePhysics.tiltControlEnabled | 0xE20 | **0xE08** | MAPPED, shifted | UpdateTilt: Bike vtable Win 0x2B40A18 slot 0x180 = 0x261E7D0, Mac 0x6F15BB0 slot 0x188 = 0x12D2F80. Win `cmp byte [rcx+0xe20],0; cmp byte [rcx+0x20],0; mov rcx,[rcx+0xde0]; mov dl,1; call 0x2E5C04`; Mac `ldrb w8,[x0,#0xe08]; ldrb w8,[x0,#0x18]; ldr x0,[x0,#0xdc8]; mov w1,#1; bl 0x12B3604`. Fork: negTurnRate 0xE18, turnInput 0xE1C, tiltControlEnabled 0xE20. | MISMATCH by heuristic (offsets 0/6, all shifted); reviewed by the register-for-register match |
| PhysicsData.vehicle (BaseObject*, back pointer) | 0x180 | 0x180 | MAPPED, same | SetCenterOfMass Win 0x1CF6300 = Mac 0x1359D98. Win `mov [rcx+0x1d4],eax` ... `mov rcx,[rdi+0x180]; lea rdx,[rsp+0x50]; call 0x2E547C` (chassis weak handle from `[vehicle+0x2E8/0x2F0]`) `; cmp byte [rdi+0x1c4],0`; Mac `stur x9,[x8,#0x84]` (x8 = x0+0x150), `ldr x8,[x0,#0x180]; ldr x20,[x8,#0x2e8]; ldr x9,[x8,#0x2f0]`, `ldrb w8,[x19,#0x1c4]`. The bike tilt calls it on both sides (Win 0x1CF6DA0, Mac 0x12D3310). | OK (offsets 3/15, calls 6/16: Mac inlines the callees) |
| PhysicsData.centerOfMass | 0x1D4 | 0x1D4 | MAPPED, same | Same pair: Win stores `[rcx+0x1d4/0x1d8/0x1dc]` from the Vector3 argument; Mac `str s0,[x0,#0x1dc]` and the `stur` above. | as above |
| PhysicsData.worldPosition (Vector4; R40's "centre of mass at 0x170") | 0x170 | 0x170 | MAPPED, same | Same pair, Mac `str d1,[x0,#0x170]; st1 {v1.s}[2],[x0+0x178]`. Computed from the transform at 0x150/0x160 and centerOfMass. | as above |
| PhysicsData.usesInertiaTensor | 0x1C4 | 0x1C4 | MAPPED, same | Same pair. | |
| TPPCameraComponent.cameraPitch | 0x4D8 | 0x4D8 | MAPPED, same | UpdatePitch Win 0x70CA10 = Mac 0x12080F0: the float is read, clamped between the slope limits (0x54C/0x550 on ground, 0x554/0x558 in air) and stored back at +0x4D8 on both. | STRONG (R40 rating, 18/29) |
| TPPCameraComponent.cameraDirection (Vector3, LTBF writes it in the same hook) | 0x4DC | 0x4DC | MAPPED, same | Same pair: Win stores `[r9+0x30..0x38]` at 0x4DC/0x4E0/0x4E4; Mac `add x8,x0,#0x4dc; str d4,[x8]; st1 {v4.s}[2],[x0+0x4e4]`. | |
| TPPCameraComponent.drivingDirectionCompensationSpeedCoef | 0x578 | 0x578 | MAPPED, same | GetYaw Win 0x70AE10 = Mac 0x12073C8 read SpeedMin 0x570, SpeedMax 0x574, SpeedCoef 0x578 (mulss), Angle 0x57C, AngleSmooth 0x580, AngularVelocityMin 0x584 in the same order, plus 0x408, 0x448, 0x480, 0x500 and the store at 0x50C. | STRONG (11/19) |
| TPPCameraComponent.drivingDirectionCompensationAngleSmooth | 0x580 | 0x580 | MAPPED, same | Same pair. | |
| FPPCameraComponent.yawOffset | 0x42C | 0x42C | MAPPED, same | FPPCameraUpdate Win 0x4E1638 = Mac 0x357DD5C: Win `lea rdx,[rcx+0x42c]; lea r8,[rcx+0x430]; call 0x4E2968` (stores the action values `YawInput` and `PitchInput`); Mac adds x22 = this+0x42C and x21 = this+0x430 and calls GetActionValue (0xB98500) with the `YawInput` and `PitchInput` CNames. The Mac reflection dump keeps headingLocked 0x3F6, sensitivityMultX 0x498, timeDilationCurveName 0x4B0. The earlier "yawOffset in TPPCameraComponent" in R40 was this FPP field. | OK (R40: 11/28) |
| FPPCameraComponent.animFeature (Handle instance; LTBF sets `vehicleProceduralCameraWeight`) | 0x360 | 0x360 | MAPPED, same | Win `mov rax,[rdi+0x360]; movss [rax+0x78]` = Mac `ldr x8,[x19,#0x360]; stp s0,s0,[x8,#0x78]`. Offsets inside the anim feature were not checked. | |
| Effects.vehicle (BaseObject*) | 0 | 0 | MAPPED, same | WheelEffectUpdate Win 0x576FE4 `mov rax,[r14]; mov rbx,[rax+0x2c8]` = Mac 0x133BD08 `ldr x8,[x20]; ldr x22,[x8,#0x2c8]`. | STRONG (37/64) |
| ChassisComponent.proxyID | 0x160 | 0x160 | MAPPED, same | Win 0x667EC8 = Mac 0x1238B5C (chassis proxy update): `mov edx,[rcx+0x160]` = `ldr w0,[x0,#0x160]`; also `[rbx+0x50]`, localTransform `0xC0/0xC4/0xC8`. The Mac reflection dump keeps collisionResource 0x128, optionalPlayerOnly... 0x140, size 0x190. | OK (4/15, calls 10/18) |
| ChassisComponent.sharedMutex | 0x16C | 0x16C | MAPPED, same | Same pair: Win `lea rcx,[rbx+0x16c]` (lock), Mac `add x2,x19,#0x16c` passed to the ProxyHelper constructor. | |
| MaterialResource.extra_byte_for_fun (LTBF's flag byte) | +0x11 | +0x11 | MAPPED, same: padding | MaterialFx = 3 conditions x 0x30 (normal 0, wet 0x30, rain 0x60), each: skidMarks 8, tireTracks 8, loaded +0x10, 7 bytes of padding, decal +0x18. Win condition selector 0xD06AF4 / 0x1FA10FA (`cmp byte [rcx+0x40]` -> +0x30, `cmp byte [rcx+0x70]` -> +0x60) = Mac SkidMarkEffectStart 0x1340F28 (`ldrb [x22,#0x40]; add x8,x22,#0x30; ldrb [x22,#0x70]; add x8,x22,#0x60`, decal loaded 0x58/0x88). | OK (R40) |
| physics::ProxyHelper.mutex (SharedSpinLock*) | +0x18 | +0x18 | MAPPED, same | Win ProxyHelper::Unlock 0x1A5EA4 `mov rdx,[rcx+0x18]` = Mac 0x4096010 `ldr x8,[x0,#0x18]`. | |

### New layout findings from this pass

| Field | Win | Mac | Status | Evidence |
|---|---|---|---|---|
| TPPCameraComponent.vehicle (Handle&lt;BaseObject&gt;), CORRECTS R40 | instance 0x320, refcount 0x328 | instance **0x318**, refcount 0x320 | MAPPED, shifted | GetLocationFromOffset Win 0x70C4A8 = Mac 0x12048E8. Win `mov rax,[rcx+0x328]` (refcount check, `lock cmpxchg [rdx],ecx`), `mov [rsp+0x58],rax`, `mov rax,[rbx+0x320]`, `mov [rsp+0x50],rax`; Mac `ldr x8,[x0,#0x320]` (refcount, `casal`), `ldr x22,[x20,#0x320]`, `ldr x21,[x20,#0x318]`, `stp x21,x22`, `cbz x21`. R40 recorded 0x328 -> 0x320, which are the refcount slots; the **instance pointer the hooks read** is at Mac 0x318. Fields from 0x330 on are the same as Windows. Affects the GetLocationFromOffset, UpdatePitch and GetYaw hooks. |
| physics::ColliderSphere.radius, sizeof | 0x90, 0xA0 | **0x88, 0x90** | MAPPED, shifted | Mac reflection dump (`layout/reflection-2026-10-08.tsv`): physicsColliderSphere size 144, radius 136. physicsICollider is unchanged (localToBody 0x30, material 0x50, tag 0x68, filterData 0x70, volumeModifier 0x80, size 0x90), so only the sphere's float moved into the base's tail padding. LTBF writes `material`, `localToBody` (same offsets) and passes the radius to createHandleWithRadius (see 3). |
| ent::SlotComponent.slotIndexLookup (HashMap&lt;CName,int&gt;), slots | 0x150, 0x120 | same | verified | GetSlotIndex and the slot transform (3). Size 0x1A0 on both sides (dump: size 416, slots 288, fallbackSlots 304). |
| vehicle::BaseObject.actionInterface | 0x3B8 | 0x3B8 | verified | EndActions callers (3). |
| Mod Settings structures | see the Mod Settings section | | verified | UserMapping.rawMappings Map at +0x98, ContextManager.userMapping +8, actions +0x28, Mapping stride 0x30, RawMappingCollection stride 0x18 with needs_update at +0x10: all the same as Windows. |

Implication for R41: the fork's structs cannot be used on Mac as they are (R40 blocker 1). Use explicit offsets from the table above for the fields LTBF touches. Hooks that only pass a struct pointer on are not affected.

## 2. CRUID counter

| Item | Win | Mac | Status | Evidence | Rating |
|---|---|---|---|---|---|
| CRUID counter global (hash 2027097961) | .data 0x3485608 | **0x8DC7EB0** (`int64_t`, `__DATA` .bss, zero at start) | MAPPED | The only three Mac functions that reference the global are the accessors below. | n/a (data) |
| CRUID::Next accessor | inline in every user | **0x91248C**, `uint64_t()` | MAPPED | `adrp x8,0x8DC7000; add x8,x8,#0xeb0; mov w9,#1; ldaddal x9,x8,[x8]; lsl x8,x8,#2; add x0,x8,#5; ret` returns `(old + 1) * 4 \| 1`, the same value as Windows (`lock xadd [3485608],1; inc; shl 2; or 1`). 0x9124A8 and 0x9124DC are the same code storing the result to `[x0]`. | n/a |

Users, paired by structure (MEDIUM): Win 0x682C5C (stride 0x108 elements) = Mac 0x1AB2690 (`mov w28,#0x108`, `bl 0x91248C`); Win 0x1EDEB8A block (stores the CRUID, `mov [rsp+0x58],4`, `[rdi+0x50]`) = Mac 0x1B5E89C (`str x0,[sp,#0x90]`, `mov w8,#4`, `ldr x0,[x20,#0x50]`); Win 0x2413764 (assign a CRUID to each element whose `[+0x60]` is 0) = Mac 0xCB5918 / 0xCBC4BC (the Mac invalid CRUID is the constant 0 from 0x9124D4).

For the build: replace `CRUID::Next()` with a call of 0x91248C (a plain function, no hook), or on the global with `((__atomic_fetch_add(counter, 1, __ATOMIC_SEQ_CST) + 1) * 4) | 1`. The game's counter and LTBF's must be the same variable, which both do.

## 3. LTBF direct calls (13)

| Hash | Name | Win | Mac | Status | Evidence | ABI notes and rating |
|---|---|---|---|---|---|---|
| 1189351231 | ProxyHelper::Unlock | 0x1A5EA4 | **0x4096010** | MAPPED | 14 / 20 byte leaves, 20 callers each. Win `mov rdx,[rcx+0x18]; xor eax,eax; test; xchg byte [rdx],al`; Mac `ldr x8,[x0,#0x18]; cbz; mov w9,#-0x80; ldclralb w9,w8,[x8]` (the Mac game lock protocol: exclusive = byte 0 -> 0x80, unlock clears bit 7). | `void(ProxyHelper*)`. OK. Note: Mac 0x4095F28 is Create + store mutex + **Lock** in one call, so do not use it where LTBF locks explicitly; use Create 0x4095DF8 (already mapped) and store the mutex pointer yourself. |
| 1570510966 | physics::ProxyID::GetProxy | 0x2A22B8 | **0x405A798** | MAPPED | Win loads the manager global 0x3467A50, calls the handle-returning lookup 0x23F3FC, takes the handle's pointer, releases the handle; Mac loads 0x90DF0B0, calls 0x4055F44 (the 0x23F3FC twin: 32 callers each, the same index/generation/bitmap tests, out handle in x8), takes `handle.ptr`, releases it (inlined), returns it. | `void*(uint32_t id)`. WEAK (tiny), reviewed. **R40's alternative was wrong:** `ProxyHelper.proxy` is a different object (the pool item from 0x4056330 = Win 0x23F398, `pool[+0x140 + idx*8]`); GetProxy returns the manager slot's handle target, which LTBF casts to `PhysicalSystemProxy*`. |
| 688524786 | ent::SlotComponent::GetSlotIndex | 0x1C68B4 | **0xC463C8** | MAPPED | Win `add rcx,0x150; call 0x1C68E4` (HashMap&lt;CName,int&gt; find); Mac does the lookup inline: size `[+0x158]`, buckets `[+0x15C]`, index table `[+0x150]`, nodes `[+0x160]`, stride `[+0x16C]`, value at node+0x10, `mov w0,#-1` when missing. 79 Mac callers vs 80 Windows. | `int32_t(SlotComponent*, CName)`, CName as a 64-bit hash in x1. WEAK (the Win body is a wrapper), reviewed. |
| 2420184407 | ent::SlotComponent::GetLocalSlotTransformFromIndex | 0x4E9790 | no standalone Mac copy | REPLACEMENT-CONFIRMED (composite) | Win body = `GetSlotGlobalTransform(this, idx, this+0xE0, &tmp)` (Win 0x1D81E8) then the world-to-local conversion 0x4E9804. The first callee has a verified Mac twin **0xC47020** (STRONG, offsets 5/6: `tbnz w1,#31`; `ldr w8,[x0,#0x12c]`; slots `[+0x120]` stride 0x30; flags `[+0x88]` bit 1; `[+0x140]` stride 0x28; 66 Mac callers vs 65 Windows); 5 Windows callers have no Mac function of this shape (the conversion is inlined). | Mac `bool(SlotComponent*, int32_t idx, const WorldTransform* parent = this+0xE0, WorldTransform* out)`: x0..x3; an invalid index copies `*parent` to `*out` and returns true, as on Windows. Replacement in the plugin: `local.position = rotate(conj(parent.orientation), (slot.position - parent.position) * 2^-17)` (positions are int32 fixed point, 2^-17 = 0.0000076293945), `local.orientation = normalize(conj(parent.orientation) * slot.orientation)` (formula read from Win 0x4E9804; LTBF uses only `position`). |
| 1320231254 | physics::ColliderSphere::createHandleWithRadius | 0x210D844 | no function | REPLACEMENT-CONFIRMED | Win: allocate 0xA0 bytes, construct the sphere, set the radius, make a Handle. Mac class: `physicsColliderSphere` size 0x90, `radius` a reflected Float at 0x88, ICollider part unchanged (dump). | Create with RTTI (`CClass::CreateInstance` of `physicsColliderSphere`) and set `radius` through the reflected property, not through the fork's offset 0x90. Then `material`, `localToBody` (same offsets) and `CreatePxShape` (vtable slot 0xF0 on Mac) as now. |
| 61607255 | action::ActionInterface::EndActions | 0x8630A0 | **0x345D760** | MAPPED | Two spin locks at `this+0xE8` / `+0xE9` taken alternately around four DynArray&lt;ActionBase*&gt; ((+8,+0x14), (+0x30,+0x3C), (+0x18,+0x24), (+0x40,+0x4C)), the `[action+0x28]==3` test, array clear. Four callers each, all pair up: Win 0x43D7A0 = Mac 0x12AAB7C, Win 0x9CEB54 = Mac 0x12B98E4, Win 0x2043460 = Mac 0x12A6988 (tail call, `add x0,x0,#0x3b8`), Win 0x191B40 = Mac 0x377D5BC (`+0x2F8`, another owner). | `void(ActionInterface*)`, `this` = vehicle `BaseObject + 0x3B8` on both sides. OK. The quickhack scripts (Effectors.reds, Actions.reds) call `EndActions`, so it should not be dropped. |
| 1404578149, 682896186, 1992632754, 2481073725 | CreateCRTTI Array / WeakHandle / Handle / RaRef TypeFromClass | 0x1749FC, 0x173A08, 0x174298, 0xC4B510 | no standalone function; name-based lookup | REPLACEMENT-CONFIRMED | The game creates these types by name. Mac `CRTTISystem::GetType(CName)` is vtable 0x6FFBD70 slot 0 = 0x21927A8: lookup 0x2192D08, name remap, then **0x2192F00**, which walks the prefix-factory table at `rtti+0x140` (stride 0x18, filled by 0x2191AA4 = Win 0x207EAA0, STRONG with 8 strings; `array:` factory 0x21959B8, `handle:` 0x2195C80, `whandle:` 0x2195E00, `raRef:` 0x2195D80, also `rRef:`, `ptr:`, `static:`, `script_ref:`), calls the factory with the remaining name and the RTTI system, then **registers the result** (0x21920B4). Windows has the same table (0x207EAA0 registers the same prefixes with factories 0x174454, 0x17459C, 0x174510, 0x2078234). Codeware already calls `rtti->GetType(CName("array:gamedataDistrict"))` on Mac (MacR17.cpp:141). | `rtti->GetType(CName("handle:" + inner))` replaces `CreateCRTTIHandleTypeFromClass + RegisterType`; `ref` -> `handle:`, `wref` -> `whandle:`, `array` -> `array:`, `raRef` -> `raRef:`, inner = the class's RTTI name (nested names such as `array:handle:Foo` recurse). **Drop the explicit `RegisterType`** on Mac: GetType registers (a second registration is the only risk). |
| 1157708450 | LoadResRef&lt;T&gt; | 0x233B68 | no function; SDK wrappers | REPLACEMENT-CONFIRMED | Win 0x233B68 -> 0x233C08 builds a ResourceRequest (path, defaults, sync flag) and calls `ResourceLoader::IssueLoadingRequest` (Win 0x1DE50C = Mac **0x21B71CC**, row 2365013187). The by-path variant (Win 0x2253C8 = Mac **0x21B7130**, row 1250309504) builds the same default request and calls the same function. LTBF's only live call (FlightSystem::OnGameLoad, `sync=false`) is the default. | Mac SDK: `ResourceLoader::Get()->LoadAsync<T>(path)` returns `SharedPtr<ResourceToken<T>>` (result through x8; wrapper already in `ResourceLoader.hpp`); assign it to the `ResourceReference` token. No new row. |
| 2124484623 | GetMatrixFromOrientation (Quaternion to Matrix) | 0x1DA684 | none | INLINED + REPLACEMENT-CONFIRMED | 52 Windows callers; no Mac function with callers of that order exists (61 candidates of the right size and instruction mix have 0-4 callers), so the Mac compiler inlined it. Only use: FlightSystem::PrePhysics builds the FMOD listener matrix. | Reimplement from Win 0x1DA684: normalize q (zero stays zero); rows `X = [1-2(y²+z²), 2(xy+zw), 2(xz-yw), 0]`, `Y = [2(xy-zw), 1-2(x²+z²), 2(yz+xw), 0]`, `Z = [2(xz+yw), 2(yz-xw), 1-2(x²+y²), 0]`, `W = (0,0,0,1)`; LTBF then sets W.XYZ from the position. Unit-test against the identity and a 90 degree yaw. |
| 2027097961 | CRUID counter | .data 0x3485608 | **0x8DC7EB0**, accessor **0x91248C** | MAPPED | See section 2. | |

## 3b. Mod Settings direct calls (3)

| Hash | Name | Win | Mac | Status | Evidence | ABI notes and rating |
|---|---|---|---|---|---|---|
| 3702071356 | UserMapping::GetMappingCollection | 0x4A60D0 | **0x1D796B4** | MAPPED | Sort the raw map if its dirty bit (`[+0xB8]` bit 0) is set, binary search the CName keys at `[+0x98]` (count `[+0xA4]`), return `values[+0xA8] + idx*0x18` or null. Callers: the game's override function Win 0x4A5ED0 = Mac 0x1D5C02C and Win 0x2A5B180 = Mac 0x1D5C2DC (both STRONG, offsets 8/15), the third Windows caller 0xE65DE8 and the Mac callers 0x1D5C588 / 0x1D5CB6C were not paired. | `RawMappingCollection*(UserMapping*, CName)`: this in x0, the CName as a 64-bit hash in x1. OK (1/5, calls 2/3). |
| 4196866905 | UserMapping::ClearRuntimeDataOnRawMappings | 0x4A6A54 | **0x1D7923C** | MAPPED | Sort if dirty, then clear the byte at `+0x10` of every 0x18-byte value (`values[+0xA8]`, count `[+0xA4]`); Mac vectorizes the loop. It is the first call of the same game function on both sides (Win `[rcx+8]` = userMapping, Mac `ldr x0,[x21,#8]`). | `void(UserMapping*)`. Heuristic MISMATCH (vectorized loop), reviewed. 2 callers on each side. |
| 2583235069 | CalcHash(const CString&) | 0x5C0BA0 | no function (inlined) | INLINED + REPLACEMENT-CONFIRMED | Windows has only 2 callers; the one that matters is the bindings XML loader Win 0x5C14E8 (`Missing root node 'bindings' in %hs`) = Mac **0x1D83A68** (STRONG, 21 shared strings). Mac inlines the hash at 0x1D853B4..0x1D853C4 and 0x1D853F4..0x1D85404: `ldrsb w9,[x22],#1; sub w9,w9,w8; add w8,w9,w8,lsl #5` = `h = c + 31*h` over signed chars from h = 0, 32-bit wrap, same as Windows (`imul r8d,r8d,0x1f; movsx ecx,byte; add`). The result goes to the mapping's `overridableUI` (Mapping +0x24). | Reimplement `uint32_t CalcHash(const CString&)` = that loop. The Mac CString layout is the Windows one: length and flags in `[+0x14]` (`ands w9,w8,#0x3fffffff`), heap pointer at `[+0]` when `[+0x14] >= 0x40000000` (`lsr w8,w8,#0x1e` set), inline buffer otherwise; use the SDK's `CString::c_str()` and `Length()`. |

All three Mod Settings calls therefore work without a new hook. `kMacOverrideCallsReady` can turn true once the structure checks below hold.

### Structures the override loop reads (all verified the same as Windows)

| Field | Offset (Win = Mac) | Evidence |
|---|---|---|
| UserMapping.rawMappings: Map&lt;CName, RawMappingCollection&gt; | keys `[+0x98]`, capacity `[+0xA0]`, size `[+0xA4]`, values `[+0xA8]`, values size `[+0xB4]`, flags `[+0xB8]` bit 0 = needs sort | ApplyOverrides body Win 0x4A61C4 vs Mac 0x1D792E4; the GetMappingCollection / Clear twins |
| RawMappingCollection | `mappings` DynArray (data `[+0]`, size `[+0xC]`), `needs_update` byte `[+0x10]`, stride 0x18 | Clear twin |
| Mapping (stride 0x30) | type +0, key16 +2, name +8, presetName +0x10, val +0x18, thresholds +0x1C/+0x20, overridableUI +0x24, +0x28 | AddMapping Win 0x4A65A0 = Mac 0x1D78074 (OK, 7/16), same loads in the body |
| ContextManager | userMapping `[+8]`, actions Map at `+0x28` (keys `[+0x28]`, size `[+0x34]`, values `[+0x38]`, flags `[+0x48]`), Action.data.mapNames data `[+8]`, size `[+0x14]` | the game's override function Win 0x4A5ED0 = Mac 0x1D5C02C (STRONG) |
| UserMapping.contextManager `[+0x48]` | **not verified on Mac** | see UNSURE below |

## 4. Vtable slots (the 2 UNSURE rows)

| Slot | Win | Mac | Status | Evidence | Rating |
|---|---|---|---|---|---|
| game::Component `sub_238` (FlightComponent overrides it) | 0x238 -> 0x1B4D28 | **0x240 -> 0x1F29BE8** | MAPPED (HIGH) | Win `sub rsp,0x28; mov rax,[rcx]; call [rax+8]; mov eax,[rax+0x70]; shr eax,2; and al,1` = `GetType()->flags.isScriptedClass`. Mac `ldr x8,[x0]; ldr x8,[x8,#8]; blr x8; ldrb w8,[x0,#0x70]; ubfx w0,w8,#2,#1`. The same function is in all 8 Mac component vtables that do not override it (the R40 candidates). The four new virtuals after it are nullsubs on both sides: Win 0x240-0x258 (`ret 0`, 0x14A700) = Mac 0x248-0x260 (`ret`, 0x1F29C0C / 10 / 14 / 18). `bool(Component*, Handle<Object>*)`, return in w0. | OK (1/2, calls 1/1) |
| entIComponent / gameComponent base vtables (the fork's "VFT hash + slot" base calls) | 0x2AC03F0 (entIComponent), 0x2ADF728 (gameComponent) | **0x6ED3698**, **0x6FB1820** | MAPPED (HIGH) | R40 had 8 candidate vtables each and could not name the class's own. gameComponent: its constructor Mac **0x1F2A148** stores 0x6FB1820 at `[this]` (and 0x6FB1A98 = +0x278 at `[this+0x90]` for PSInterface) after calling the entIComponent constructor 0xCA5E34; slot 0 = 0x1F29204 (returns the class pointer from 0x900A170), slot 8 = 0x1F291E8, right after the type-name getter 0x1F2918C that references the string `gameComponent`. entIComponent: slot 0 = 0xCA56C8, slot 8 = 0xCA56AC, next to the type-name function 0xCA5650 (`entIComponent`). In 0x6FB1820: OnAttach slot 0x188 = **0x1F296D8**, OnDetach 0x198 = **0x1F298B0**, sub_150 0x158 = 0x1F295E4, sub_218 0x220 = 0x1F29AA0, sub_230 0x238 = 0x1F29648 (derived vtables that override them differ, for example 0x6F16698). entIComponent slots (Win + 8): 0x118 0xCA63F4, 0x120 0xCA64D8, 0x158 0xCA6E44, 0x180 0xCA619C, 0x188 0xCA627C, 0x198 0xCA62AC, 0x200 0xCA6D68, 0x230 0xCA62C4. | STRONG for the string-bearing function pairs; slot lists are exact reads |

For R41: the fork's `UniRelocFunc(VFT, slot)` becomes "vtable RVA + Mac slot" with 0x6FB1820 for hash 331746781 and 0x6ED3698 for 1586431952 (Mac slot = Windows slot + 8 after the first two entries, as R40 found). IScriptable (1180173983) has three Mac vtables with the same slot 0/8 pair (0x7000F00, 0x71124A8, 0x7271638), so its own is not singled out; LTBF does not use it. IGameSystem stays ALREADY.

## 5. Mod Settings ApplyOverrides body: verdict

**The body 0x1D792E4 is a real function entry and is suitable for an entry hook.**

- It is in LC_FUNCTION_STARTS ([0x1D792E4, 0x1D795A8), 708 bytes) and starts with a normal AArch64 frame (`sub sp,sp,#0x80; stp x26,x25; stp x24,x23; stp x22,x21; stp x20,x19; stp x29,x30; add x29,sp,#0x70`), no PC-relative instruction in the first four words.
- It is the twin of Win 0x4A61C4 (rated OK, offsets 5/24): both copy the raw map at `+0x98`, reset the key map and the button / axis tables, then re-add every raw mapping through AddMapping (Win 0x4A65A0 = Mac 0x1D78074).
- References: one caller, the tail jump `b 0x1D792E4` at 0x1D5C778 inside 0x1D5C774 (`ldr x0,[x0,#8]; b`). That function's only caller is the tail jump in the stub 0x1D735A4 (`ldr x0,[x0,#0x10]; b 0x1D5C774`), which the Manager vtable 0x6F94398 holds at slot 0x258 (data pointer at 0x6F945F0). Windows is the same chain (vtable 0x2B1B170 slot 0x250 -> 14-byte thunk 0x1868498 -> jmp 0x4A61C4).
- Signature: `void(UserMapping*)`, x0 = the UserMapping (the stub loads `Manager->contextManager`, then `ContextManager->userMapping`). The Manager pointer is **not** available in the body, so the Manager-only side effects (`overrideSet`, `lastOverrideStaus`) cannot be set from there (R41c already skips them).
- Options if the Manager is needed: (a) accept the loss; (b) replace the vtable pointer at 0x6F945F0 (read-only data after fixups; it needs a protection change and is a write into game memory, which the plan does not allow without a separate decision); (c) hook the 8-byte stub, which is too small for a normal trampoline. (a) is the recommended path.
- The hook will fire exactly where the Windows thunk hook does (the single call path), so the call count in Stage 4 should equal the number of settings applications.

## 6. Mod Settings SkipKey: verdict

**Mac 0x48026F0 is a correct twin of the predicate but has no reference of any kind, so a hook on it installs and never fires.**

- Exhaustive scan of the Mac text and all data segments for the target 0x48026F0: no `bl` / `b`, no `b.cond`, `cbz` / `cbnz`, `tbz` / `tbnz`, no `adrp` + `add` / `ldr`, no `adr` or literal load, and no pointer to it in any data segment (chained-fixup aware); it is not in a vtable. (Ten `adrp` instructions target its page; each is followed by an `add` of a different low part, for example 0x27C, 0x2CC, 0xC68, 0xF20, never 0x6F0: neighbouring callbacks.)
- The single Windows caller is Win 0x21EFD24 (`call 0x21F01C0` at 0x21EFD9A). Its Mac twin is **0x48038B8** (rated OK; the same `[obj+0xCC]` flag, the same `0xE6` / `0xEC` / `0xED` handling, the same CName constant 0xD0B5B54D42F2CB83 loaded with `movk`). Clang inlined the whole predicate at 0x48039A4..0x4803AE0: key - 0x88 < 0x14 (then `<= 5` maps to 0x1B, else exit), key - 0x5B < 2 or key - 0xFF < 2 exit.
- 0x48038B8 takes `(this, key*, type*, float*)` through pointers on Mac (`ldr w9,[x1]; ldr w8,[x2]; ldr s0,[x3]`), where Windows 0x21EFD24 gets the key, type and float in registers (edx, r8d, xmm3); it has no direct callers (it is reached through a callback pointer).
- The Mod Settings intent (do not skip the gamepad range IK_Pad_First..+0x13) cannot be reached with an entry hook: the predicate is in the middle of 0x48038B8. Options: (a) drop the hook on the Mac and log once (the cost is that gamepad buttons stay unbindable in the Mod Settings key UI); (b) hook 0x48038B8 and re-implement it (about 560 bytes with internal calls), which is not worth it. Recommended: (a); R41c should not register the hook at 0x48026F0 or list it in its `hook-offsets.txt` (a never-firing INSTALLED line would mislead the Stage 4 check).

## 7. R40 corrections

| R40 text | Correction |
|---|---|
| TPPCameraComponent vehicle handle Win 0x328, Mac 0x320 | those are the refcount slots; the instance is Win 0x320, Mac **0x318** |
| ProxyID::GetProxy alternative "helper.proxy is the proxy" | wrong object; use Mac 0x405A798 |
| ProxyHelper::Unlock "no standalone Mac copy found" | exists: Mac 0x4096010 |
| TPPCameraComponent "yawOffset" | it is an FPPCameraComponent field (0x42C, same) |
| PhysicsData "centre of mass used for torque 0x170" | that is `worldPosition` (Vector4); the fork's `centerOfMass` Vector3 is at 0x1D4 (both platforms) |
| SkipKey "check INSTALLED and the call count in Stage 4" | answered here: unreachable, the call count would be 0 |
| `EndActions` "can be dropped" | it is used by the quickhack scripts; a Mac function exists (0x345D760) |

## 8. Still UNSURE, with options

1. **UserMapping.contextManager `[+0x48]` on Mac** (Mod Settings reads `userMapping->contextManager->actions`). The ContextManager-side offsets are verified; the `+0x48` back pointer was not found in any twin function. Option: in R41c, check once at runtime that `userMapping->contextManager && userMapping->contextManager->userMapping == userMapping` and skip the feature with a log line otherwise. Not a write probe.
2. **GetLocalSlotTransformFromIndex has no standalone Mac copy.** The replacement uses two verified Mac pieces (GetSlotIndex 0xC463C8, slot world transform 0xC47020) plus the world-to-local math read from Win 0x4E9804; the orientation operand order is unverified (LTBF uses only `position`).
3. **Manager pointer** for ApplyOverrides (section 5) and the loss of the gamepad-range SkipKey change (section 6): accepted losses for now.
4. **FPP AnimFeature layout** (`vehicleProceduralCameraWeight` at +0xEC on Windows) was not checked on Mac; it is outside the survey rows. Option: read it from the reflection dump once the class is registered.
5. **CRUID users** are paired by structure, MEDIUM; the global and the accessor are exact.
6. **GetMatrixFromOrientation formula** was read from one Windows function and should be unit-tested.

## 9. Reproduction

- Scripts and notes are in the session scratchpad (`.../scratchpad/r41b/`, not committed): `common.py` (loads the R40 helpers), `wpat.py` / `wfind.py` (Windows byte patterns and displacement sets), `cruid*.py`, `skipkey*.py`, `slot*.py`, `ms*.py`, `vs*.py`, `endact*.py`, `calchash*.py`. They import `tools/addr-match/rx.py` and the R40 helpers (`wl.py`, `vt.py`, `macindex.py`).
- Typical checks: `./rx w <rva>` / `./rx m <rva>`, `enc.py ldrx:0x180 ldrx:0x2f0 ...`, `score.py <win> <mac>`, the Mac reflection dump `layout/reflection-2026-10-08.tsv`.
