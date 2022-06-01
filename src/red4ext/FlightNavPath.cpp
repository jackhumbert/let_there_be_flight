#include "FlightModule.hpp"
#include "Utils.hpp"
#include <RED4ext/RED4ext.hpp>
#include <spdlog/spdlog.h>
#include <RED4ext/Scripting/Natives/Generated/game/mappins/QuestMappin.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/mappins/PointOfInterestMappin.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/ui/MinimapContainerController.hpp>

struct FlightNavPath : RED4ext::IScriptable {
  RED4ext::CClass *GetNativeType();
  static FlightNavPath *GetInstance();
};

RED4ext::TTypedClass<FlightNavPath> cls("FlightNavPath");

RED4ext::CClass *FlightNavPath::GetNativeType() { return &cls; }

RED4ext::Handle<FlightNavPath> handle;

FlightNavPath *FlightNavPath::GetInstance() {
  if (!handle.instance) {
    spdlog::info("[RED4ext] New FlightNavPath Instance");
    auto instance = reinterpret_cast<FlightNavPath *>(cls.AllocInstance());
    handle = RED4ext::Handle<FlightNavPath>(instance);
  }

  return (FlightNavPath *)handle.instance;
}

void GetInstanceScripts(RED4ext::IScriptable *aContext, RED4ext::CStackFrame *aFrame,
                        RED4ext::Handle<FlightNavPath> *aOut, int64_t a4) {
  aFrame->code++;

  if (!handle.instance) {
    spdlog::info("[RED4ext] New FlightNavPath Instance");
    auto instance = reinterpret_cast<FlightNavPath *>(cls.AllocInstance());
    handle = RED4ext::Handle<FlightNavPath>(instance);
  }

  if (aOut) {
    handle.refCount->IncRef();
    *aOut = RED4ext::Handle<FlightNavPath>(handle);
  }
}

// 48 8B C4 48 89 48 08 55 41 55 48 8D 68 A8 48 81 EC 48 01 00 00 48 89 58 10 0F 57 C0 48 89 70 E8
constexpr uintptr_t UpdateNavPathAddr = 0x140000C00 + 0x255D180 - RED4ext::Addresses::ImageBase;
void UpdateNavPath(RED4ext::game::ui::MinimapContainerController *, __int64,  unsigned __int8,
  RED4ext::ink::WidgetReference *);
decltype(&UpdateNavPath) UpdateNavPath_Original;

void UpdateNavPath(RED4ext::game::ui::MinimapContainerController* mmcc, __int64 a2, unsigned __int8 questOrPOI,
  RED4ext::ink::WidgetReference* widgetRef) {
  UpdateNavPath_Original(mmcc, a2, questOrPOI, widgetRef);

  auto rtti = RED4ext::CRTTISystem::Get();
  if (mmcc->GetType() == rtti->GetClass("gameuiMinimapContainerController")) {
    auto fnp = FlightNavPath::GetInstance();
    auto args = RED4ext::CStackType(rtti->GetType("Int32"), &questOrPOI);
    auto stack = RED4ext::CStack(fnp, &args, 1, nullptr, 0);
    cls.GetFunction("Update")->Execute(&stack);
  }
}

void GetQuestMappin(RED4ext::IScriptable *aContext, RED4ext::CStackFrame *aFrame,
                    RED4ext::Handle<RED4ext::game::mappins::QuestMappin> *aOut, int64_t a4) {
  aFrame->code++; // skip ParamEnd

  if (aOut) {
    auto ms = reinterpret_cast<RED4ext::game::ui::MinimapContainerController *>(aContext);
    *aOut = ms->questMappin;
  }
}

void GetPOIMappin(RED4ext::IScriptable *aContext, RED4ext::CStackFrame *aFrame,
                    RED4ext::Handle<RED4ext::game::mappins::IMappin> *aOut, int64_t a4) {
  aFrame->code++; // skip ParamEnd

  if (aOut) {
    auto ms = reinterpret_cast<RED4ext::game::ui::MinimapContainerController *>(aContext);
    *aOut = ms->poiMappin;
  }
}

struct FlightNavPathModule : FlightModule {
  void Load(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) {
    aSdk->hooking->Attach(aHandle, RED4EXT_OFFSET_TO_ADDR(UpdateNavPathAddr), &UpdateNavPath,
                          reinterpret_cast<void **>(&UpdateNavPath_Original));
  }

  void Unload(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) {
    aSdk->hooking->Detach(aHandle, RED4EXT_OFFSET_TO_ADDR(UpdateNavPathAddr));
  }

  void RegisterTypes() {
    auto rtti = RED4ext::CRTTISystem::Get();
    auto scriptable = rtti->GetClass("IScriptable");
    cls.parent = scriptable;
    cls.flags = {.isNative = true};
    RED4ext::CRTTISystem::Get()->RegisterType(&cls);
  }

  void PostRegisterTypes() {
    auto rtti = RED4ext::CRTTISystem::Get();

    auto getInstance = RED4ext::CClassStaticFunction::Create(&cls, "GetInstance", "GetInstance", &GetInstanceScripts,
                                                             {.isNative = true, .isStatic = true});
    cls.RegisterFunction(getInstance);

    // expose the minimap members to the scripts
    auto ms = rtti->GetClass("gameuiMinimapContainerController");
    ms->props.PushBack(RED4ext::CProperty::Create(rtti->GetType("array:Vector4"), "questPoints", nullptr, 0x1E0));
    ms->props.PushBack(RED4ext::CProperty::Create(rtti->GetType("array:Vector4"), "poiPoints", nullptr, 0x208));

    auto getQuestMappin =
        RED4ext::CClassFunction::Create(ms, "GetQuestMappin", "GetQuestMappin", &GetQuestMappin, {.isNative = true});
    ms->RegisterFunction(getQuestMappin);
    auto getPOIMappin =
        RED4ext::CClassFunction::Create(ms, "GetPOIMappin", "GetPOIMappin", &GetPOIMappin, {.isNative = true});
    ms->RegisterFunction(getPOIMappin);
  }
};

 //REGISTER_FLIGHT_MODULE(FlightNavPathModule);