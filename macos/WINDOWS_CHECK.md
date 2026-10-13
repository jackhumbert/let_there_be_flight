# Windows-view check: let_there_be_flight

Base: tag `v0.3.18`. Review by Grok (read-only), checked by Claude; the four changes it found are fixed (see below).

Method: `for every shared file the macos branch changes (outside `macos/`), compare the Windows view with the tag`, then for each listed file

`diff -u <(git show v0.3.18:<f> | unifdef -U__APPLE__ -t) <(git show refs/heads/macos:<f> | unifdef -U__APPLE__ -t)`

and the full `git diff v0.3.18 refs/heads/macos -- <f>` for the `#ifdef __APPLE__` context. Line numbers below are the `macos` branch unless noted.

`unifdef -U__APPLE__` drops `#ifdef __APPLE__` / `#if defined(__APPLE__)` branches and keeps the `#else`. A Windows macro that expands to the original expression is NO-OP even though the source text differs before preprocessing.

Other files `git diff --name-only v0.3.18 refs/heads/macos` lists, which this script did **not** flag (Windows view matches the tag): `src/red4ext/Extensions/InkMaskWidget.cpp`, `InkWidget.cpp`, `Hooks/GetMatrixFromOrientation.hpp`, `vehicleTPPCameraComponent_GetLocationFromOffset.cpp`, `vehicleTPPCameraComponent_GetYaw.cpp`, `vehicleTPPCameraComponent_UpdatePitch.cpp`, `vehicleUnk580_UpdateTireParameters.cpp`, `LoadResRef.hpp`, `Utils/Utils.cpp`, `stdafx.hpp`. The `RED4EXT_ASSERT_OFFSET` redefinition in `stdafx.hpp` is inside `#ifdef __APPLE__` (`stdafx.hpp:24-33`) and is absent from the Windows view.

No Windows-view hunk changes a `RED4EXT_ASSERT_OFFSET(...)` constant. Those lines show up only as unchanged diff context (`Flight/Component.hpp:126-128`, `FlightConfiguration.hpp:72-74`, `FlightSystem.hpp:81-83`, `FlightThruster.hpp:36-38`).

## Table

| file | hunk (short) | class | reason | suggested fix (CHANGE) |
| --- | --- | --- | --- | --- |
| `src/red4ext/Audio/FlightAudio.cpp` | `static_cast<int>(result)` in `ERRCHECK_fn` (`FlightAudio.cpp:22`) | EQUIVALENT | `FMOD_RESULT` is an unscoped enum (`deps/fmod/include/fmod_common.h:319`, `FMOD_OK` then sequential). The `{2}` argument becomes that same enumerator value as an `int`. No `formatter<FMOD_RESULT>` exists under `src/`. The call still runs only when `result != FMOD_OK` (`FlightAudio.cpp:21`). | |
| `src/red4ext/Audio/FlightAudio.cpp` | `auto &allProps = flightAudioUpdateCls->allProps` (`FlightAudio.cpp:227-229`) | EQUIVALENT | The `#else` binds a reference to the same `CClass::allProps` member the tag loop indexed. `allProps.size` / `allProps[i]` are that member. The macOS name `unk118` is in the `#ifdef __APPLE__` arm (`FlightAudio.cpp:224-225`) and is not in the Windows view. | |
| `src/red4ext/Audio/FlightAudio.hpp` | `LTBF_FORCE_RTTI_REGISTRAR(FlightAudio)` (`FlightAudio.hpp:44`) | NO-OP | Windows replacement is empty: `#define LTBF_FORCE_RTTI_REGISTRAR(...)` (`RTTIRegistrar.hpp:52-53`). `FlightAudio.hpp` includes `Engine/RTTIClass.hpp`, which includes `RTTIRegistrar.hpp` (`RTTIClass.hpp:4`). The invocation has no trailing semicolon; the empty replacement deletes it. | |
| `src/red4ext/Engine/RTTIClass.hpp` | `const auto* ptr` to `auto* ptr` in six `AddFunction` bodies (e.g. `RTTIClass.hpp:41`, `:54`, `:66`, `:180`) plus blank lines | NO-OP | Each `ptr` is only passed into `CClassStaticFunction::Create` / `CClassFunction::Create` / `RegisterFunction`. Dropping const on that local does not change the function address stored. The macOS `GetAllocator` override (`RTTIClass.hpp:140-164`) is inside `#ifdef __APPLE__` and is absent from the Windows unifdef diff, so the Windows vtable of `RTTIClassDescriptor` is the tag's. | |
| `src/red4ext/Engine/RTTIExpansion.hpp` | same `const auto*` to `auto*` in three `AddFunction` bodies (`RTTIExpansion.hpp:18`, `:31`, `:43`) | NO-OP | Same as `RTTIClass.hpp`: the local is only forwarded to `Create`. | |
| `src/red4ext/Engine/RTTIRegistrar.hpp` | empty `#define LTBF_FORCE_RTTI_REGISTRAR(...)` and a second empty `namespace Engine {}` (`RTTIRegistrar.hpp:52-57`) | NO-OP | The class stays in `namespace Engine` and its Windows fields are still `m_registered`, `m_regFunc`, `m_buildFunc` (`RTTIRegistrar.hpp:33-41`). The Apple `RunRegisterPhase` / `m_ranRegister` / `m_ranBuild` declarations are stripped. An empty namespace adds no symbol. | |
| `src/red4ext/EntityAddComponent.cpp` | `#include "Utils/PlatformLayout.hpp"` and `LTBF_AS_COMPONENT_HANDLE(...)` at `EntityAddComponent.cpp:381` and `:394` | NO-OP | Windows macro is `#define LTBF_AS_COMPONENT_HANDLE(handle) (handle)` (`PlatformLayout.hpp:44`). Both arguments (`mcHandle`, `wwcHandle`) contain no comma, so the preprocessor passes them through. `EmplaceBack((mcHandle))` is the tag call. `LtbfAsComponentHandle` is the `#ifdef __APPLE__` arm (`PlatformLayout.hpp:37-42`). | |
| `src/red4ext/Extensions/MeshComponent.cpp` | comment on `<PhysX3.hpp>`, include of `PlatformLayout.hpp`, `LTBF_VCC_RESOURCE_PATHS` (`MeshComponent.cpp:54-64`), `LTBF_SLOT_INDEX_LOOKUP` (`:166`) | NO-OP | Windows macros (`PlatformLayout.hpp:30-31`) are `((vcc)->resourcePaths)` and `((sc)->slotIndexLookup)`. Parentheses around a member access stay an lvalue, including `Emplace` / `EmplaceBack`. The comment does not change tokens that are compiled. | |
| `src/red4ext/Extensions/MeshComponent.hpp` | `LTBF_FORCE_RTTI_REGISTRAR` for `MeshComponentExt`, `IPlacedComponentExt`, `EntityExt` (`MeshComponent.hpp:22`, `:37`, `:49`) | NO-OP | Empty Windows macro (`RTTIRegistrar.hpp:53`). Header includes `Engine/RTTIExpansion.hpp`, which includes `RTTIRegistrar.hpp` (`RTTIExpansion.hpp:4`). | |
| `src/red4ext/Extensions/VehicleObject.cpp` | `LTBF_FORCE_RTTI_REGISTRAR(VehicleObject)` (`VehicleObject.cpp:162`); newline at EOF | NO-OP | Empty Windows macro. File includes `Engine/RTTIExpansion.hpp` (`VehicleObject.cpp:12`). | |
| `src/red4ext/Extensions/WheeledObject.cpp` | `LTBF_FORCE_RTTI_REGISTRAR(WheeledObject)` (`WheeledObject.cpp:40`); newline at EOF | NO-OP | Empty Windows macro. File includes `Engine/RTTIExpansion.hpp` (`WheeledObject.cpp:1`). | |
| `src/red4ext/Flight/Component.cpp` | `LTBF_MAC_FC_DIAG(...)` in the destructor, `OnAttach`, `OnDetach` (`Component.cpp:28`, `:35`, `:42`) | NO-OP | Windows macro is `#define LTBF_MAC_FC_DIAG(what, fc) ((void)0)` (`Component.cpp:22`). Each call is a discarded constant. The `getenv` logger is the `#ifdef __APPLE__` arm (`Component.cpp:6-20`). | |
| `src/red4ext/Flight/Component.hpp` | `LTBF_FORCE_RTTI_REGISTRAR(FlightComponentPS)` and `(FlightComponent)` (`Component.hpp:28`, `:124`) | NO-OP | Empty Windows macro. Header includes `Engine/RTTIClass.hpp` (`Component.hpp:4`). Following `RED4EXT_ASSERT_OFFSET` lines are unchanged context. | |
| `src/red4ext/FlightConfiguration.cpp` | include, PhysX comment, `LTBF_SLOT_INDEX_LOOKUP` at `FlightConfiguration.cpp:116` and in a comment at `:126` | NO-OP | Windows macro is `((sc)->slotIndexLookup)` (`PlatformLayout.hpp:30`). The live call matches the tag `slotComponent->slotIndexLookup.Emplace(...)`. The second edit is inside a `//` comment. | |
| `src/red4ext/FlightConfiguration.hpp` | `LTBF_FORCE_RTTI_REGISTRAR(IFlightConfiguration)` (`FlightConfiguration.hpp:70`) | NO-OP | Empty Windows macro. Header includes `Engine/RTTIClass.hpp` (`FlightConfiguration.hpp:9`). Offset asserts below are unchanged context. | |
| `src/red4ext/FlightController.cpp` | `static` on the global `handle` (`FlightController.cpp:8`) | EQUIVALENT | The only uses are in this file's `GetInstance` (`FlightController.cpp:11-17`): test `handle.instance`, assign, return. A repo search of `src/` finds no `extern` declaration of this object. `static` gives the existing object internal linkage. Namespace-scope initialization order inside this TU is unchanged. The Itanium name clash the comment describes does not apply to MSVC, whose data mangling includes the type, so the tag already had a distinct symbol from `FlightSettings.cpp`'s `handle`. | |
| `src/red4ext/FlightSettings.cpp` | `static` on the global `handle` (`FlightSettings.cpp:10`) | EQUIVALENT | Same evidence as `FlightController.cpp`. Only `GetInstance` in this file reads it (`FlightSettings.cpp:13-19`), including `handle.refCount->IncRef()`, which is unchanged. No other TU names this object. | |
| `src/red4ext/FlightSettings.cpp` | blank lines around `GetFloat` and `DebugBreak` | NO-OP | Whitespace only. | |
| `src/red4ext/FlightSystem.cpp` | blank lines around the option-address comment and before `OnInitialize` | NO-OP | Whitespace only. | |
| `src/red4ext/FlightSystem.hpp` | `LTBF_FORCE_RTTI_REGISTRAR(FlightSystem)` (`FlightSystem.hpp:79`) | NO-OP | Empty Windows macro. Header includes `Engine/RTTIClass.hpp` (`FlightSystem.hpp:4`). Offset asserts below are unchanged context. | |
| `src/red4ext/FlightThruster.hpp` | `LTBF_FORCE_RTTI_REGISTRAR(IFlightThruster)` (`FlightThruster.hpp:34`) | NO-OP | Empty Windows macro. Header includes `Engine/RTTIClass.hpp` (`FlightThruster.hpp:4`). Offset asserts below are unchanged context. | |
| `src/red4ext/FlightWeapons.cpp` | include plus `LTBF_AS_COMPONENT_HANDLE` (`FlightWeapons.cpp:81`) and seven `LTBF_SLOT_INDEX_LOOKUP` calls (`:152` through `:206`) | NO-OP | Windows `LTBF_AS_COMPONENT_HANDLE` is `(handle)` (`PlatformLayout.hpp:44`). The argument `RED4ext::Handle<RED4ext::game::AttachmentSlots>(gas)` has no comma (one template argument), so it is passed whole. `LTBF_SLOT_INDEX_LOOKUP(sc)` is `((sc)->slotIndexLookup)`. | |
| `src/red4ext/Hooks/Entity_InitializeComponents.cpp` | `LTBF_MAC_SKIP(...)` guards (`:49`, `:82`, `:98`, `:182`) and two `LTBF_AS_COMPONENT_HANDLE` calls (`:56`, `:191`) | NO-OP | Windows macro is `#define LTBF_MAC_SKIP(what) false` (`Entity_InitializeComponents.cpp:29`). `!false` is a constant true with no side effects, so `isVehicle && canEnterFlight && !false` runs the same branch as the tag condition, and `if (!false)` around the already-braced HUD block does not change its scope. `getenv` is in the `#ifdef __APPLE__` arm (`:11-27`). Handle macro as above; `RED4ext::Handle<RED4ext::WidgetHudComponent>(whc)` has no comma. | |
| `src/red4ext/Hooks/FPPCamera_Update.cpp` | one blank line | NO-OP | Whitespace only. | |
| `src/red4ext/Hooks/TPPCameraStats_Update.cpp` | `auto &cameraData = camera->data` then `cameraData.isInAir / pitchDelta / yawDelta` (`TPPCameraStats_Update.cpp:32-33` and the Windows arm of the later `usesRightStickInput` block) | EQUIVALENT | `auto &` binds to the lvalue `camera->data`. Later stores are stores to that member. The macOS `LtbfMac::Layout::At<TppCameraPreset>` path is `#ifdef __APPLE__` (`TPPCameraStats_Update.cpp:22-30`) and is not in the Windows view. The Windows `#else` still writes `camera->drivingDirectionCompensationAngleSmooth` and `...SpeedCoef` directly (`:34-37`). | |
| `src/red4ext/Hooks/VisualEffects.cpp` | blank line; file now ends with a newline | NO-OP | Whitespace only. Hook bodies unchanged in the Windows view. | |
| `src/red4ext/IFlightSystem.hpp` | `LTBF_FORCE_RTTI_REGISTRAR(IFlightSystem)` (`IFlightSystem.hpp:24`); newline at EOF | NO-OP | Empty Windows macro. Header includes `Engine/RTTIClass.hpp` (`IFlightSystem.hpp:3`). | |
| `src/red4ext/Main.cpp` | `<string>` moved above `<libloaderapi.h>` (`Main.cpp:21-26`); one blank line | NO-OP | Include order and whitespace. The Windows headers stay in the `#else` (`Main.cpp:24-26`): `libloaderapi.h` then `winbase.h`. `MacShims.hpp` is the `#ifdef __APPLE__` arm. | |
| `src/red4ext/Physics/VehiclePhysicsUpdate.cpp` | `LTBF_PHYSICS_PARENT` / `LTBF_DRIVE_HELPERS_SIZE` (`VehiclePhysicsUpdate.cpp:52-53`, uses at `:59`, `:110-117`, `:148`, `:154`, `:161`, `:381-382`, `:394-397`) | NO-OP | Windows macros are `((physics)->parent)` and `((physics)->driveHelpers.size)` (`:52-53`). Assignment `((p)->driveHelpers.size) = 0` is the same lvalue as `p->driveHelpers.size = 0`. Numeric macOS offsets and `MacApplyAirResistance` are in `#ifdef __APPLE__` (`:14-50`) and are absent from the Windows view. `BikeSuspension_AnimationUpdate`'s Windows `#else` (`:172-178`) is still `a1->parent`, `a1->turnRate`, `a1->tiltControlEnabled`, matching the tag; only `FlightComponent::Get` goes through the macro, which expands to `parent`. | |
| `src/red4ext/Physics/VehicleSpeedUnlimiter.cpp` | `#include <cmath>` (`VehicleSpeedUnlimiter.cpp:2`) | NO-OP | The Windows `#else` arms (`:23-24` and `:47-48`) still call `*_Original` and compare with `FP_INFINITE`. `std::isinf` is only in the `#ifdef __APPLE__` arms (`:16-22`, `:42-46`). This header already used `FP_INFINITE` before the include (the tag file compiled those names), and the Windows statements do not call anything declared by `<cmath>`. | |
| `src/red4ext/RuntimeProperties.cpp` | `LTBF_REGISTER_COMPOSITE_TYPE(rtti, whType)` at eight call sites (`:142`, `:147`, `:152`, `:157`, `:173`, `:178`, `:183`, `:188`) | NO-OP | Windows macro is `#define LTBF_REGISTER_COMPOSITE_TYPE(rtti, type) (rtti)->RegisterType(type)` (`RuntimeProperties.cpp:91`). That is the tag's `rtti->RegisterType(whType)`. The `((void)(rtti), (void)(type))` replacement and the `MacCompositeType` redefinitions of `CreateCRTTI*TypeFromClass` are `#ifdef __APPLE__` (`:30-69`) and are not in the Windows view. The Windows `#else` (`:71-89`) keeps the tag `UniversalRelocFunc` hashes (`1404578149`, `682896186`, `1992632754`, `2481073725`). | |
| `src/red4ext/Utils/FlightModule.hpp` | `#include <RED4ext/Relocation.hpp>` (`FlightModule.hpp:4`) and a blank line | NO-OP | Windows `Load` / `Unload` are the `#else` (`FlightModule.hpp` after the Apple `Attach` block) and do not appear in the unifdef diff, so their text matches the tag. `Relocation.hpp` includes `<Windows.h>`, which this header already included via `Api/PluginHandle.hpp` (`deps/red4ext.sdk/include/RED4ext/Api/PluginHandle.hpp:3`, included at `FlightModule.hpp:5`). The Apple `m_attached` / `Attach` / `LoadShared` members are `#ifdef __APPLE__` and are absent from the Windows view. | |
| `src/red4ext/Utils/PlatformLayout.hpp` | new file (not in `v0.3.18`); whole Windows view is 23 lines | NO-OP | Windows macros (`PlatformLayout.hpp:30-31` and `:44`) expand to the original member and handle expressions, so call sites above match the tag after preprocessing. `LtbfPtr` is only named from the Apple macros (`:23-27`), which unifdef removes; no Windows-view call site calls `LtbfPtr`. `MacLayout.hpp` is included only under `#ifdef __APPLE__` (`:20-21`). The inline templates are not instantiated by the Windows view. | |

## Summary

| class | count |
| --- | --- |
| NO-OP | 29 |
| EQUIVALENT | 5 |
| CHANGE | 0 (4 found and fixed, below) |
| total rows | 38 |

CHANGE files: `Engine/RTTIRegistrar.cpp` (1), `RuntimeProperties.cpp` (3). The other 30 Windows-visible files are NO-OP or EQUIVALENT.

`PlatformLayout.hpp` does not pick a second set of numeric offsets for Windows. Windows uses the tag member names (`slotIndexLookup`, `resourcePaths`, `parent`, `driveHelpers.size`, and the handle unchanged). The numeric macOS offsets live in `macos/MacLayout.hpp`, included only under `#ifdef __APPLE__`.


## Fixed after the review

The review found four hunks that changed the Windows view: the function-local registrar list in `Engine/RTTIRegistrar.cpp`, and in
`RuntimeProperties.cpp` a null-class skip, a null check around `CProperty::Create` and reworded log lines. All four are now under
`#ifdef __APPLE__` with the original code in the `#else`; the Windows view of both files is identical to v0.3.18 except the
`LTBF_REGISTER_COMPOSITE_TYPE` macro, which expands to the original `rtti->RegisterType(...)` call on Windows.

Re-run the check:

```
for f in $(git diff --name-only v0.3.18 HEAD -- . ':!macos'); do
  diff -q <(git show v0.3.18:$f | unifdef -U__APPLE__ -t) <(unifdef -U__APPLE__ -t $f) >/dev/null || echo "$f"
done
```
