#include "ModSettingsVariable.hpp"

void ModSettingsVariable::UpdateValues() {
  for (auto &listener : listeners) {
    if (!listener.Expired()) {
      auto instance = reinterpret_cast<RED4ext::IScriptable*>(listener.instance);
      //auto classType = h->GetType();
      auto classType = RED4ext::CRTTISystem::Get()->GetClass(className);
      auto valuePtr = settingsVar->GetValuePtr();
      for (auto i = 0; i < classType->propertiesWithDefaults.size; i++) {
        if (classType->propertiesWithDefaults[i] == settingsVar->name) {
          uint64_t addr = classType->defaultValues[i]->type;
          auto propType =  reinterpret_cast<RED4ext::CBaseRTTIType *>(addr & 0xFFFFFFFFFFFFFFF8);
          propType->Assign(&classType->defaultValues[i]->value, valuePtr);
        }
      }
      auto prop = classType->propertiesByName.Get(settingsVar->name);
      if (prop) {
        (*prop)->SetValue(instance, valuePtr);
      }
      // might be nice to support an update function as well
      // RED4ext::CClassFunction *update;
      // for (auto func : classType->funcs) {
      //  if (func->shortName == "OnModSettingsUpdate") {
      //    update = func;
      //  }
      //}
      // if (update) {
      //  RED4ext::CStackType args[2];
      //  args[0] = RED4ext::CStackType(RED4ext::CRTTISystem::Get()->GetType("CName"), &settingsVar->name);
      //  args[1] = RED4ext::CStackType(RED4ext::CRTTISystem::Get()->GetType("Float"), settingsVar->GetValuePtr());
      //  auto stack = RED4ext::CStack(h.GetPtr(), args, 2);
      //  update->Execute(&stack);
      //}
    }
  }
}

RED4ext::user::RuntimeSettingsVar *ModSettingsVariable::CreateSettingVarFromBool(ScriptProperty *prop) {
  // auto rtm = (new RED4ext::Memory::DefaultAllocator())->AllocAligned(sizeof(RED4ext::user::RuntimeSettingsVarBool),
  // 8); memset(rtm.memory, 0, sizeof(RED4ext::user::RuntimeSettingsVarBool)); auto boolVar = new
  // RED4ext::user::RuntimeSettingsVarBool(*(RED4ext::user::RuntimeSettingsVarBool *)rtm.memory);
  auto boolVar = new RED4ext::user::RuntimeSettingsVarBool();

  boolVar->type = RED4ext::user::EConfigVarType::Bool;

  //auto defaultValue = false;
  //if (prop->defaultValues.size) {
  //  auto cstr = prop->defaultValues[0].c_str();
  //  char *p;
  //  auto value = strtoul(cstr, &p, 10);
  //  if (*p == 0) {
  //    defaultValue = value;
  //  }
  //}

  //boolVar->valueInput = defaultValue;
  //boolVar->defaultValue = defaultValue;
  //boolVar->valueValidated = defaultValue;
  //boolVar->valueWrittenToFile = defaultValue;

  return boolVar;
}

RED4ext::user::RuntimeSettingsVar *ModSettingsVariable::CreateSettingVarFromInt(ScriptProperty *prop) {
  // auto rtm = (new RED4ext::Memory::DefaultAllocator())->AllocAligned(sizeof(RED4ext::user::RuntimeSettingsVarInt),
  // 8); memset(rtm.memory, 0, sizeof(RED4ext::user::RuntimeSettingsVarInt)); auto intVar = new
  // RED4ext::user::RuntimeSettingsVarInt(*(RED4ext::user::RuntimeSettingsVarInt *)rtm.memory);
  auto intVar = new RED4ext::user::RuntimeSettingsVarInt();

  intVar->type = RED4ext::user::EConfigVarType::Int;

  //auto defaultValue = 0;
  //if (prop->defaultValues.size) {
  //  auto cstr = prop->defaultValues[0].c_str();
  //  char *p;
  //  auto value = strtoul(cstr, &p, 10);
  //  if (*p == 0) {
  //    defaultValue = value;
  //  }
  //}

  //intVar->valueInput = defaultValue;
  //intVar->defaultValue = defaultValue;
  //intVar->valueValidated = defaultValue;
  //intVar->valueWrittenToFile = defaultValue;

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

RED4ext::user::RuntimeSettingsVar *ModSettingsVariable::CreateSettingVarFromFloat(ScriptProperty *prop) {
  // auto rtm = (new RED4ext::Memory::DefaultAllocator())->AllocAligned(sizeof(RED4ext::user::RuntimeSettingsVarFloat),
  // 8); memset(rtm.memory, 0, sizeof(RED4ext::user::RuntimeSettingsVarFloat)); auto floatVar = new
  // RED4ext::user::RuntimeSettingsVarFloat(*(RED4ext::user::RuntimeSettingsVarFloat *)rtm.memory);
  auto floatVar = new RED4ext::user::RuntimeSettingsVarFloat();

  floatVar->type = RED4ext::user::EConfigVarType::Float;

  //auto defaultValue = 0.0;
  //if (prop->defaultValues.size) {
  //  std::string valueStr(prop->defaultValues[0].c_str());
  //  defaultValue = std::stof(valueStr);
  //}

  //floatVar->valueInput = defaultValue;
  //floatVar->defaultValue = defaultValue;
  //floatVar->valueValidated = defaultValue;
  //floatVar->valueWrittenToFile = defaultValue;

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

RED4ext::user::RuntimeSettingsVar *ModSettingsVariable::CreateSettingVarFromEnum(ScriptProperty *prop) {
  // auto rtm = (new
  // RED4ext::Memory::DefaultAllocator())->AllocAligned(sizeof(RED4ext::user::RuntimeSettingsVarIntList), 8);
  // memset(rtm.memory, 0, sizeof(RED4ext::user::RuntimeSettingsVarIntList));
  // auto intVar = new RED4ext::user::RuntimeSettingsVarIntList(*(RED4ext::user::RuntimeSettingsVarIntList
  // *)rtm.memory);
  auto intVar = new RED4ext::user::RuntimeSettingsVarIntList();
  intVar->type = RED4ext::user::EConfigVarType::IntList;

  auto e = (RED4ext::CEnum *)RED4ext::CRTTISystem::Get()->GetType(prop->type->name);
  if (e) {
    for (const auto &value : e->hashList) {
      intVar->displayValues.EmplaceBack(value);
    }
    for (const auto &value : e->valueList) {
      intVar->values.EmplaceBack((int32_t)value);
    }
  }

  //uint32_t defaultValue = 0;
  //if (prop->defaultValues.size) {
  //  if (e) {
  //    e->FromString(&defaultValue, prop->defaultValues[0]);
  //  } else {
  //    auto cstr = prop->defaultValues[0].c_str();
  //    char *p;
  //    auto value = strtoul(cstr, &p, 10);
  //    if (*p == 0) {
  //      defaultValue = value;
  //    }
  //  }
  //}

  //intVar->valueInput = defaultValue;
  //intVar->defaultValue = defaultValue;
  //intVar->valueValidated = defaultValue;
  //intVar->valueWrittenToFile = defaultValue;

  intVar->bitfield.listHasDisplayValues = true;

  return intVar;
}
