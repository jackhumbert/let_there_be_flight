#include "FlightModule.hpp"
#include "ModSettings.hpp"
#include "stdafx.hpp"
#include <RED4ext/Scripting/Natives/Generated/user/SettingsVarFloat.hpp>

RED4ext::TTypedClass<ModSettings> modSettings("ModSettings");

RED4ext::CClass *ModSettings::GetNativeType() { return &modSettings; }

RED4ext::Handle<ModSettings> handle;

ModSettings *ModSettings::GetInstance() {
  if (!handle.instance) {
    spdlog::info("[RED4ext] New ModSettings Instance");
    auto instance = reinterpret_cast<ModSettings *>(modSettings.AllocInstance());
    handle = RED4ext::Handle<ModSettings>(instance);
  }

  return (ModSettings *)handle.instance;
}

void ModSettings::AddVariable(ModSettingsVariable *variable) { 
  auto self = ModSettings::GetInstance();

  if (!self->variables.size) {
    self->variables =
        RED4ext::DynArray<ModSettingsVariable *>(new RED4ext::Memory::DefaultAllocator());
  }

  self->variables.EmplaceBack(variable); 
  
  if (!self->variablesByMod.size) {
    self->variablesByMod = RED4ext::HashMap<RED4ext::CName, RED4ext::DynArray<ModSettingsVariable *>>(
        new RED4ext::Memory::DefaultAllocator);
  }

  auto modVars = self->variablesByMod.Get(variable->mod);
  if (modVars) {
    modVars->EmplaceBack(variable);
  } else {
    auto ra = RED4ext::DynArray<ModSettingsVariable *>(new RED4ext::Memory::DefaultAllocator);
    ra.EmplaceBack(variable);
    self->variablesByMod.Insert(variable->mod, ra);
  }

  if (!self->categoriesByMod.size) {
    self->categoriesByMod = RED4ext::HashMap<RED4ext::CName, RED4ext::DynArray<RED4ext::CName>>(
        new RED4ext::Memory::DefaultAllocator);
  }

  auto modCategories = self->categoriesByMod.Get(variable->mod);
  if (modCategories) {
    auto found = false;
    for (const auto &category : *modCategories) {
      found |= (variable->category == category);
    }
    if (!found && variable->category != "None") {
      modCategories->EmplaceBack(variable->category);
    }
  } else if (variable->category != "None") {
    auto ra = RED4ext::DynArray<RED4ext::CName>(new RED4ext::Memory::DefaultAllocator);
    ra.EmplaceBack(variable->category);
    self->categoriesByMod.Insert(variable->mod, ra);
  }
}

void GetInstanceScripts(RED4ext::IScriptable *aContext, RED4ext::CStackFrame *aFrame,
                        RED4ext::Handle<ModSettings> *aOut, int64_t a4) {
  aFrame->code++;

  auto h = ModSettings::GetInstance();

  if (aOut) {
    h->ref.refCount->IncRef();
    *aOut = RED4ext::Handle<ModSettings>(h);
  }
}

void GetModsScripts(RED4ext::IScriptable *aContext, RED4ext::CStackFrame *aFrame,
                    RED4ext::DynArray<RED4ext::CName> *aOut, int64_t a4) {
  aFrame->code++;

  if (aOut) {
    auto h = ModSettings::GetInstance();
    *aOut = RED4ext::DynArray<RED4ext::CName>(new RED4ext::Memory::DefaultAllocator);

    auto size = h->variablesByMod.size;
    for (uint32_t index = 0; index != size; ++index) {
      aOut->EmplaceBack(h->variablesByMod.nodeList.nodes[index].key);
    }
  }
}

void GetCategoriesScripts(RED4ext::IScriptable *aContext, RED4ext::CStackFrame *aFrame,
                    RED4ext::DynArray<RED4ext::CName> *aOut, int64_t a4) {
  RED4ext::CName mod;
  RED4ext::GetParameter(aFrame, &mod);
  aFrame->code++;

  if (aOut) {
    auto h = ModSettings::GetInstance();
    auto categories = h->categoriesByMod.Get(mod);
    if (categories) {
      *aOut = *categories;
    } else {
      *aOut = RED4ext::DynArray<RED4ext::CName>(new RED4ext::Memory::DefaultAllocator());
    }
  }
}

template <typename T> T *CreateWithVFT() { 
  auto inst = new T(); 
  *(uintptr_t *)inst = T::VFT + reinterpret_cast<uintptr_t>(GetModuleHandle(nullptr));
  return inst;
}

void GetVarsScripts(RED4ext::IScriptable *aContext, RED4ext::CStackFrame *aFrame,
                    RED4ext::DynArray<RED4ext::Handle<RED4ext::user::SettingsVar>> *aOut, int64_t a4) {
  RED4ext::CName mod;
  RED4ext::CName category;
  RED4ext::GetParameter(aFrame, &mod);
  RED4ext::GetParameter(aFrame, &category);
  aFrame->code++;

  if (aOut) {
    auto h = ModSettings::GetInstance();
    *aOut = RED4ext::DynArray<RED4ext::Handle<RED4ext::user::SettingsVar>>(new RED4ext::Memory::DefaultAllocator);
    auto modVars = h->variablesByMod.Get(mod);
    for (const auto &variable : *modVars) {
      if (variable->category != category)
        continue;
      auto configVar = (RED4ext::user::SettingsVarFloat*)RED4ext::CRTTISystem::Get()->GetClass("userSettingsVarFloat")->AllocInstance();
      configVar->runtimeVar = variable->settingsVar;
      auto h = RED4ext::Handle<RED4ext::user::SettingsVar>(configVar);
      h.refCount->IncRef();
      aOut->EmplaceBack(h);
    }
  }
}

RED4ext::TTypedClass<ModSettingsVariable> modSettingsVariable("ModSettingsVariable");

RED4ext::CClass *ModSettingsVariable::GetNativeType() { return &modSettingsVariable; }

struct ModSettingsModule : FlightModule {
  void RegisterTypes() {
    auto rtti = RED4ext::CRTTISystem::Get();
    auto scriptable = rtti->GetClass("IScriptable");
    modSettings.flags = {.isNative = true};
    modSettings.parent = scriptable;
    RED4ext::CRTTISystem::Get()->RegisterType(&modSettings);
    modSettingsVariable.flags = {.isNative = true};
    modSettingsVariable.parent = scriptable;
    RED4ext::CRTTISystem::Get()->RegisterType(&modSettingsVariable);
  };

  void PostRegisterTypes() {
    auto rtti = RED4ext::CRTTISystem::Get();
    auto getInstance = RED4ext::CClassStaticFunction::Create(&modSettings, "GetInstance", "GetInstance",
                                                             &GetInstanceScripts, {.isNative = true, .isStatic = true});
    modSettings.RegisterFunction(getInstance);
    auto getMods = RED4ext::CClassStaticFunction::Create(&modSettings, "GetMods", "GetMods", &GetModsScripts,
                                                         {.isNative = true, .isStatic = true});
    modSettings.RegisterFunction(getMods);
    auto getVars = RED4ext::CClassStaticFunction::Create(&modSettings, "GetVars", "GetVars", &GetVarsScripts,
                                                         {.isNative = true, .isStatic = true});
    modSettings.RegisterFunction(getVars);
    auto getCategories = RED4ext::CClassStaticFunction::Create(&modSettings, "GetCategories", "GetCategories", &GetCategoriesScripts,
                                                         {.isNative = true, .isStatic = true});
    modSettings.RegisterFunction(getCategories);
    modSettings.props.EmplaceBack(RED4ext::CProperty::Create(rtti->GetType("Int32"), "isAccessingModspace", nullptr,
                                                     offsetof(ModSettings, isAccessingModspace)));
  }
};

 REGISTER_FLIGHT_MODULE(ModSettingsModule);