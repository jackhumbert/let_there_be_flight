#pragma once

// LTBF R41 (macOS): included by src/red4ext/stdafx.hpp (the precompiled header) on macOS only.
//
// The macOS SDK's Handle<T> and WeakHandle<T> check at instantiation that T is a complete ISerializable. LTBF creates handles to
// classes whose destructors destroy handles to these types, and the jackhumbert fork headers it was written against pulled their
// definitions in through longer include chains. ink::VirtualWindow is also needed before the SDK's inkHudWidgetSpawnEntry.hpp.

#include "LtbfMacGenerated.hpp"

#include <RED4ext/Scripting/Natives/Generated/AI/Archetype.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/ITransformBinding.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/SlotComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/EffectSet.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/PersistentState.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/VirtualWindow.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/WidgetLibraryResource.hpp>
#include <RED4ext/Scripting/Natives/CMesh.hpp>
#include <RED4ext/Scripting/Natives/Generated/CMaterialTemplate.hpp>
#include <RED4ext/Scripting/Natives/Generated/IDynamicTextureGenerator.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/EffectAction.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/EffectDurationModifier.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/EffectExecutor.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/EffectObjectFilter.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/EffectObjectProvider.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/EffectPostAction.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/EffectPreAction.hpp>
#include <RED4ext/Scripting/Natives/Generated/physics/FilterData.hpp>
#include <RED4ext/Scripting/Natives/Generated/world/ui/MeshTargetBinding.hpp>
#include <RED4ext/Scripting/Natives/Generated/CMaterialParameter.hpp>
