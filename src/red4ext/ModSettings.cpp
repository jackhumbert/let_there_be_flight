#include "FlightModule.hpp"
#include "ModSettings.hpp"
#include "stdafx.hpp"
#include <RED4ext/Scripting/Natives/Generated/user/SettingsVar.hpp>
#include <RED4ext/Scripting/Natives/Generated/user/SettingsVarBool.hpp>
#include <RED4ext/Scripting/Natives/Generated/user/SettingsVarFloat.hpp>
#include <RED4ext/Scripting/Natives/Generated/user/SettingsVarInt.hpp>
#include <RED4ext/Scripting/Natives/Generated/user/SettingsVarListInt.hpp>
#include "Utils.hpp"

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

RED4ext::user::RuntimeSettingsVar *ModSettings::CreateSettingVarFromBool(ScriptProperty *prop) {
  //auto rtm = (new RED4ext::Memory::DefaultAllocator())->AllocAligned(sizeof(RED4ext::user::RuntimeSettingsVarBool), 8);
  //memset(rtm.memory, 0, sizeof(RED4ext::user::RuntimeSettingsVarBool));
  //auto boolVar = new RED4ext::user::RuntimeSettingsVarBool(*(RED4ext::user::RuntimeSettingsVarBool *)rtm.memory);
  auto boolVar = new RED4ext::user::RuntimeSettingsVarBool();

  boolVar->type = RED4ext::user::EConfigVarType::Bool;

  auto defaultValue = false;
  if (prop->defaultValues.size) {
    auto cstr = prop->defaultValues[0].c_str();
    char *p;
    auto value = strtoul(cstr, &p, 10);
    if (*p == 0) {
      defaultValue = value;
    }
  }

  boolVar->valueInput = defaultValue;
  boolVar->defaultValue = defaultValue;
  boolVar->valueValidated = defaultValue;
  boolVar->valueWrittenToFile = defaultValue;

  return boolVar;
}

RED4ext::user::RuntimeSettingsVar *ModSettings::CreateSettingVarFromInt(ScriptProperty *prop) {
  //auto rtm = (new RED4ext::Memory::DefaultAllocator())->AllocAligned(sizeof(RED4ext::user::RuntimeSettingsVarInt), 8);
  //memset(rtm.memory, 0, sizeof(RED4ext::user::RuntimeSettingsVarInt));
  //auto intVar = new RED4ext::user::RuntimeSettingsVarInt(*(RED4ext::user::RuntimeSettingsVarInt *)rtm.memory);
  auto intVar = new RED4ext::user::RuntimeSettingsVarInt();

  intVar->type = RED4ext::user::EConfigVarType::Int;

  auto defaultValue = 0;
  if (prop->defaultValues.size) {
    auto cstr = prop->defaultValues[0].c_str();
    char *p;
    auto value = strtoul(cstr, &p, 10);
    if (*p == 0) {
      defaultValue = value;
    }
  }

  intVar->valueInput = defaultValue;
  intVar->defaultValue = defaultValue;
  intVar->valueValidated = defaultValue;
  intVar->valueWrittenToFile = defaultValue;

  auto step = prop->runtimeProperties.Get("ModSettings.step");
  if (step) {
    std::string valueStr(step->c_str());
    intVar->stepValue = std::stoi(valueStr);
  } else {
    intVar->stepValue = 1;
  }

  auto min = prop->runtimeProperties.Get("ModSettings.min");
  if (min) {
    std::string valueStr(min->c_str());
    intVar->minValue = std::stoi(valueStr);
  } else {
    intVar->minValue = 0;
  }

  auto max = prop->runtimeProperties.Get("ModSettings.max");
  if (max) {
    std::string valueStr(max->c_str());
    intVar->maxValue = std::stoi(valueStr);
  } else {
    intVar->maxValue = 10;
  }
  return intVar;
}

RED4ext::user::RuntimeSettingsVar *ModSettings::CreateSettingVarFromFloat(ScriptProperty *prop) {
  //auto rtm = (new RED4ext::Memory::DefaultAllocator())->AllocAligned(sizeof(RED4ext::user::RuntimeSettingsVarFloat), 8);
  //memset(rtm.memory, 0, sizeof(RED4ext::user::RuntimeSettingsVarFloat));
  //auto floatVar = new RED4ext::user::RuntimeSettingsVarFloat(*(RED4ext::user::RuntimeSettingsVarFloat *)rtm.memory);
  auto floatVar = new RED4ext::user::RuntimeSettingsVarFloat();

  floatVar->type = RED4ext::user::EConfigVarType::Float;

  auto defaultValue = 0.0;
  if (prop->defaultValues.size) {
    std::string valueStr(prop->defaultValues[0].c_str());
    defaultValue = std::stof(valueStr);
  }

  floatVar->valueInput = defaultValue;
  floatVar->defaultValue = defaultValue;
  floatVar->valueValidated = defaultValue;
  floatVar->valueWrittenToFile = defaultValue;

  auto step = prop->runtimeProperties.Get("ModSettings.step");
  if (step) {
    std::string valueStr(step->c_str());
    floatVar->stepValue = std::stof(valueStr);
  } else {
    floatVar->stepValue = 0.1;
  }

  auto min = prop->runtimeProperties.Get("ModSettings.min");
  if (min) {
    std::string valueStr(min->c_str());
    floatVar->minValue = std::stof(valueStr);
  } else {
    floatVar->minValue = 0.0;
  }

  auto max = prop->runtimeProperties.Get("ModSettings.max");
  if (max) {
    std::string valueStr(max->c_str());
    floatVar->maxValue = std::stof(valueStr);
  } else {
    floatVar->maxValue = 10.0;
  }
  return floatVar;
}

RED4ext::user::RuntimeSettingsVar *ModSettings::CreateSettingVarFromEnum(ScriptProperty *prop) {
  //auto rtm = (new RED4ext::Memory::DefaultAllocator())->AllocAligned(sizeof(RED4ext::user::RuntimeSettingsVarIntList), 8);
  //memset(rtm.memory, 0, sizeof(RED4ext::user::RuntimeSettingsVarIntList));
  //auto intVar = new RED4ext::user::RuntimeSettingsVarIntList(*(RED4ext::user::RuntimeSettingsVarIntList *)rtm.memory);
  auto intVar = new RED4ext::user::RuntimeSettingsVarIntList();
  intVar->type = RED4ext::user::EConfigVarType::IntList;

  auto e = (RED4ext::CEnum*)RED4ext::CRTTISystem::Get()->GetType(prop->type->name);
  if (e) {
    for (const auto &value : e->hashList) {
      intVar->displayValues.EmplaceBack(value);
    }
    for (const auto &value : e->valueList) {
      intVar->values.EmplaceBack((int32_t)value);
    }
  }

  uint32_t defaultValue = 0;
  if (prop->defaultValues.size) {
    if (e) {
      e->FromString(&defaultValue, prop->defaultValues[0]);
    } else {
      auto cstr = prop->defaultValues[0].c_str();
      char *p;
      auto value = strtoul(cstr, &p, 10);
      if (*p == 0) {
        defaultValue = value;
      }
    }
  }

  intVar->valueInput = defaultValue;
  intVar->defaultValue = defaultValue;
  intVar->valueValidated = defaultValue;
  intVar->valueWrittenToFile = defaultValue;

  intVar->bitfield.listHasDisplayValues = true;

  return intVar;
}

void ModSettings::AddVariable(ModSettingsVariable *variable) {
  auto self = ModSettings::GetInstance();
  
  std::shared_lock<RED4ext::SharedMutex> _(self->variables_lock);
  variable->listeners = RED4ext::DynArray<RED4ext::WeakHandle<RED4ext::IScriptable>>(new RED4ext::Memory::DefaultAllocator());

  if (!self->variables.size) {
    self->variables = RED4ext::DynArray<ModSettingsVariable *>(new RED4ext::Memory::DefaultAllocator());
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

  if (!self->variablesByClass.size) {
    self->variablesByClass = RED4ext::HashMap<RED4ext::CName, RED4ext::DynArray<ModSettingsVariable *>>(
        new RED4ext::Memory::DefaultAllocator);
  }

  auto classVars = self->variablesByClass.Get(variable->className);
  if (classVars) {
    classVars->EmplaceBack(variable);
  } else {
    auto ra = RED4ext::DynArray<ModSettingsVariable *>(new RED4ext::Memory::DefaultAllocator);
    ra.EmplaceBack(variable);
    self->variablesByClass.Insert(variable->className, ra);
  }

  if (variable->category != "None") {
    if (!self->categoriesByMod.size) {
      self->categoriesByMod =
          RED4ext::HashMap<RED4ext::CName, RED4ext::DynArray<RED4ext::CName>>(new RED4ext::Memory::DefaultAllocator);
    }

    auto modCategories = self->categoriesByMod.Get(variable->mod);
    if (modCategories) {
      auto found = false;
      for (const auto &category : *modCategories) {
        found |= (variable->category == category);
      }
      if (!found) {
        modCategories->EmplaceBack(variable->category);
      }
    } else {
      auto ra = RED4ext::DynArray<RED4ext::CName>(new RED4ext::Memory::DefaultAllocator);
      ra.EmplaceBack(variable->category);
      self->categoriesByMod.Insert(variable->mod, ra);
    }
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
      RED4ext::user::SettingsVar *configVar = NULL;
      switch (variable->settingsVar->type) {
      case RED4ext::user::EConfigVarType::Bool:
        configVar = (RED4ext::user::SettingsVarBool *)RED4ext::CRTTISystem::Get()
                        ->GetClass("userSettingsVarBool")
                        ->AllocInstance();
        break;
      case RED4ext::user::EConfigVarType::Float:
        configVar = (RED4ext::user::SettingsVarFloat *)RED4ext::CRTTISystem::Get()
                        ->GetClass("userSettingsVarFloat")
                        ->AllocInstance();
        break;
      case RED4ext::user::EConfigVarType::Int:
        configVar = (RED4ext::user::SettingsVarInt *)RED4ext::CRTTISystem::Get()
                        ->GetClass("userSettingsVarInt")
                        ->AllocInstance();
        break;
      case RED4ext::user::EConfigVarType::IntList:
        configVar = (RED4ext::user::SettingsVarListInt *)RED4ext::CRTTISystem::Get()
                        ->GetClass("userSettingsVarListInt")
                        ->AllocInstance();
        break;
      }
      if (configVar) {
        configVar->runtimeVar = variable->settingsVar;
        auto h = RED4ext::Handle<RED4ext::user::SettingsVar>(configVar);
        h.refCount->IncRef();
        aOut->EmplaceBack(h);
      }
    }
  }
}

void ModSettings::WriteToFile() {
  auto self = ModSettings::GetInstance();
  std::ofstream configFile(Utils::GetRootDir() / "red4ext" / "plugins" / "let_there_be_flight" / "modSettings.ini");
  if (configFile.is_open()) {
    for (const auto &node : self->variablesByClass) {
      configFile << "[" << node.key.ToString() << "]\n";
      for (const auto &variable : node.value) {
        if (variable->settingsVar->WasModifiedSinceLastSave()) {
          variable->UpdateValues();
          variable->settingsVar->ChangeWasWritten();
        }
        const char * value;
        auto str = RED4ext::CString::CString(new RED4ext::Memory::DefaultAllocator());
        //variable->settingsVar->GetValueToWrite(value);

        RED4ext::CRTTISystem::Get()->GetType(variable->typeName)->ToString(variable->settingsVar->GetValuePtr(), str);

        //switch (variable->settingsVar->type) {
        //case RED4ext::user::EConfigVarType::Bool: {
        //  auto settingsVar = (RED4ext::user::RuntimeSettingsVarBool *)variable->settingsVar;
        //  RED4ext::CRTTISystem::Get()->GetType(variable->typeName)->ToString(&settingsVar->valueValidated, str);
        //} break;
        //case RED4ext::user::EConfigVarType::Int: {
        //  auto settingsVar = (RED4ext::user::RuntimeSettingsVarInt *)variable->settingsVar;
        //  RED4ext::CRTTISystem::Get()->GetType(variable->typeName)->ToString(&settingsVar->valueValidated, str);
        //  break;
        //}
        //case RED4ext::user::EConfigVarType::IntList: {
        //  auto settingsVar = (RED4ext::user::RuntimeSettingsVarInt *)variable->settingsVar;
        //  RED4ext::CRTTISystem::Get()->GetType(variable->typeName)->ToString(&settingsVar->valueValidated, str);
        //} break;
        //case RED4ext::user::EConfigVarType::Float: {
        //  auto settingsVar = (RED4ext::user::RuntimeSettingsVarFloat *)variable->settingsVar;
        //  RED4ext::CRTTISystem::Get()->GetType(variable->typeName)->ToString(&settingsVar->valueValidated, str);
        //} break;
        //}

        value = str.c_str();

        configFile << variable->settingsVar->name.ToString() << " = " << value << "\n";
      }
      configFile << "\n";
    }
    configFile.close();
  }
}

#include <iostream>
#include "INIReader.h"

void ModSettings::ReadFromFile() {
  auto self = ModSettings::GetInstance();
  auto file = Utils::GetRootDir() / "red4ext" / "plugins" / "let_there_be_flight" / "modSettings.ini";
  INIReader reader(file.string());

  if (reader.ParseError() != 0) {
    return;
  }

  for (auto section : reader.Sections()) {
    auto variables = self->variablesByClass.Get(section.c_str());
    if (variables) {
      for (auto &variable : *variables) {
        uint64_t value = 0;
        void * value_p = &value;
        auto defaultStr = RED4ext::CString::CString(new RED4ext::Memory::DefaultAllocator());
        RED4ext::CRTTISystem::Get()
            ->GetType(variable->typeName)
            ->ToString(variable->settingsVar->GetValuePtr(), defaultStr);
        auto valueStr = reader.Get(section, variable->settingsVar->name.ToString(), defaultStr.c_str());
        auto cstr = RED4ext::CString(valueStr.c_str());
        RED4ext::CRTTISystem::Get()
            ->GetType(variable->typeName)
            ->FromString(value_p, cstr);

        variable->settingsVar->UpdateAll(value_p);
        variable->UpdateValues();

        //switch (var->settingsVar->type) {
        //case RED4ext::user::EConfigVarType::Bool: {
        //  auto settingsVar = (RED4ext::user::RuntimeSettingsVarBool *)var->settingsVar;
        //  value = reader.GetInteger(section, var->settingsVar->name.ToString(), settingsVar->defaultValue);
        //  value_p = &value;
        //}
        //  break;
        //case RED4ext::user::EConfigVarType::Int:
        //  {
        //  auto settingsVar = (RED4ext::user::RuntimeSettingsVarInt *)var->settingsVar;
        //  value = reader.GetInteger(section, var->settingsVar->name.ToString(), settingsVar->defaultValue);
        //  break;
        //  }
        //case RED4ext::user::EConfigVarType::IntList:
        //  {
        //  auto settingsVar = (RED4ext::user::RuntimeSettingsVarInt *)var->settingsVar;
        //  value = reader.GetInteger(section, var->settingsVar->name.ToString(), settingsVar->defaultValue);
        //  value_p = &value;
        //  }
        //  break;
        //case RED4ext::user::EConfigVarType::Float:
        //  {
        //  auto settingsVar = (RED4ext::user::RuntimeSettingsVarFloat *)var->settingsVar;
        //  value = reader.GetReal(section, var->settingsVar->name.ToString(), settingsVar->defaultValue);
        //  value_p = &value;
        //  }
        //  break;
        //}
      }
    }
  }


}

void AcceptChangesScripts(RED4ext::IScriptable *aContext, RED4ext::CStackFrame *aFrame, void *aOut, int64_t a4) {
  aFrame->code++;
  ModSettings::WriteToFile();
}

void RejectChangesScripts(RED4ext::IScriptable *aContext, RED4ext::CStackFrame *aFrame, void *aOut, int64_t a4) {
  aFrame->code++;
  auto ms = ModSettings::GetInstance();

  for (const auto &variable : ms->variables) {
    if (variable->settingsVar->HasChange()) {
      variable->settingsVar->RevertChange();
    }
  }
}

void RegisterListenerToClassScripts(RED4ext::IScriptable *aContext, RED4ext::CStackFrame *aFrame, void *aOut,
                                    int64_t a4) {
  RED4ext::Handle<RED4ext::IScriptable> instance;
  RED4ext::GetParameter(aFrame, &instance);
  aFrame->code++;

  if (instance) {
    auto ms = ModSettings::GetInstance();
    auto className = instance->GetType()->GetName();
    std::shared_lock<RED4ext::SharedMutex> _(ms->variables_lock);
    auto vars_p = ms->variablesByClass.Get(className);
    if (vars_p) {
      auto vars = *vars_p;
      for (auto i = 0; i < vars.size; i++) {
        std::shared_lock<RED4ext::SharedMutex> _(vars[i]->listeners_lock);
        vars[i]->listeners.EmplaceBack(RED4ext::WeakHandle(instance));
      }
    }
  }
}

void UnregisterListenerToClassScripts(RED4ext::IScriptable *aContext, RED4ext::CStackFrame *aFrame, void *aOut,
                                    int64_t a4) {
  RED4ext::Handle<RED4ext::IScriptable> handle;
  RED4ext::GetParameter(aFrame, &handle);
  aFrame->code++;

  if (handle) {
    auto ms = ModSettings::GetInstance();
    auto className = handle->GetType()->GetName();
    auto vars = ms->variablesByClass.Get(className);
    if (vars) {
      for (auto &var : *vars) {
        auto i = 0;
        for (const auto &listener : var->listeners) {
          if (listener.instance == handle.instance) {
            var->listeners.RemoveAt(i);
            break;
          }
          i++;
        }
      }
    }
  }
}

struct ModSettingsModule : FlightModule {
  void RegisterTypes() {
    auto rtti = RED4ext::CRTTISystem::Get();
    auto scriptable = rtti->GetClass("IScriptable");
    modSettings.flags = {.isNative = true};
    modSettings.parent = scriptable;
    RED4ext::CRTTISystem::Get()->RegisterType(&modSettings);
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
    auto acceptChanges = RED4ext::CClassStaticFunction::Create(&modSettings, "AcceptChanges", "AcceptChanges", &AcceptChangesScripts,
                                                         {.isNative = true, .isStatic = true});
    modSettings.RegisterFunction(acceptChanges);
    auto rejectChanges = RED4ext::CClassStaticFunction::Create(&modSettings, "RejectChanges", "RejectChanges", &RejectChangesScripts,
                                                         {.isNative = true, .isStatic = true});
    modSettings.RegisterFunction(rejectChanges);
    auto registerListenerToClass =
        RED4ext::CClassStaticFunction::Create(&modSettings, "RegisterListenerToClass", "RegisterListenerToClass",
                                              &RegisterListenerToClassScripts, {.isNative = true, .isStatic = true});
    modSettings.RegisterFunction(registerListenerToClass);
    auto unregisterListenerToClass =
        RED4ext::CClassStaticFunction::Create(&modSettings, "UnregisterListenerToClass", "UnregisterListenerToClass",
                                              &UnregisterListenerToClassScripts, {.isNative = true, .isStatic = true});
    modSettings.RegisterFunction(unregisterListenerToClass);

  }
};

 REGISTER_FLIGHT_MODULE(ModSettingsModule);