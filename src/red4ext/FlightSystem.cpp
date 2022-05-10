#include <fmod_errors.h>

#include <RED4ext/RED4ext.hpp>
#include <RED4ext/RTTITypes.hpp>
#include <RED4ext/Scripting/Natives/Generated/Vector4.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/IGameSystem.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/IGameSystem.hpp>
#include <RED4ext/Addresses.hpp>
#include <RED4ext/Scripting/Natives/ScriptGameInstance.hpp>
#include <fmod.hpp>
#include <fmod_studio.hpp>
#include <iostream>

#include "FlightLog.hpp"
#include "Utils.hpp"
#include "stdafx.hpp"

using namespace RED4ext;

namespace FlightSystem {

struct UpdateCalls {
  void *(*update)(void *, float);
  void *(*copy1)(void **, void **);
  void *(*copy2)(void **, void **);
  game::IGameSystem *(*callback)();
};

struct UpdateCallStruct {
  game::IGameSystem *system;
  void *unk08;
  void *unk10;
  void *unk18;
  UpdateCalls *calls;
};

UpdateCalls FlightSystemUpdateCalls;

int32_t flightSystemUpdateRegister = 0;

struct FlightSystem : game::IGameSystem {
  CClass* GetNativeType();  // 00
  void* RegisterUpdate(uintptr_t lookup);  // 110
};

TTypedClass<FlightSystem> flightSystemCls("FlightSystem");

CClass* FlightSystem::GetNativeType() { return &flightSystemCls; }

void* FlightSystemUpdate(void *, float) { return NULL; }

void* FlightSystem::RegisterUpdate(uintptr_t lookup) {
	//RED4ext::CClass *cls;
	//RED4ext::UpdateCalls *result;
	//RED4ext::UpdateCallStruct ucs1; 
	//RED4ext::UpdateCallStruct ucs2;

	//if (flightSystemUpdateRegister >
	//    (NtCurrentTeb()->ThreadLocalStoragePointer +
	//     *(unsigned int *)(0x144B99330 - Addresses::ImageBase)) + 1948) {
	//  reinterpret_cast<void(*)(int32_t*)>(Addresses::Init_thread_header)(&flightSystemUpdateRegister);
	//    if (flightSystemUpdateRegister == -1) {
	//      FlightSystemUpdateCalls.callback = 0;
	//      FlightSystemUpdateCalls.update = &FlightSystemUpdate;
	//      FlightSystemUpdateCalls.copy1 =
	//          reinterpret_cast < void *(*)(void**, void**)>(Addresses::CopyInstance);
	//      FlightSystemUpdateCalls.copy2 =
	//          reinterpret_cast<void *(*)(void **, void **)>(Addresses::CopyInstance);
	//      reinterpret_cast<void(*)(int32_t*)>(Addresses::Init_thread_footer)(&flightSystemUpdateRegister);
	//    }
	//}
	//ucs1.system = this;
	//ucs1.calls = &FlightSystemUpdateCalls;
	//ucs2.calls = &FlightSystemUpdateCalls;
	//FlightSystemUpdateCalls.copy1((void **)&ucs2.system, (void **)&ucs1.system);
	//cls = this->GetType();
	//reinterpret_cast<void (*)(uintptr_t, uint8_t, CClass *, const char *, void *, uint32_t)>(
	//    Addresses::UpdateDefinition_CreateFromParent)(
	//    lookup, 3u, cls, "FlightSystem/Update", &ucs2, 0xA);
	//result = ucs1.calls; 
	//if (ucs1.calls && ucs1.calls->callback) {
	//    return ((void *(*)(RED4ext::UpdateCallStruct *))ucs1.calls->callback)(&ucs1);
	//}
	//return result;        
	spdlog::info("[Flight System] sub_110!");
	return 0;
}

void RegisterTypes() {
  flightSystemCls.flags = {.isNative = true};
  CRTTISystem::Get()->RegisterType(&flightSystemCls);
}

void RegisterFunctions() {
  auto rtti = CRTTISystem::Get();
  auto iGameSystem = rtti->GetClass("gameIGameSystem");
  flightSystemCls.parent = iGameSystem;

  CBaseFunction::Flags n_flags = {.isNative = true};
}

} // namespace FlightSystem