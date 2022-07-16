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