#include "FlightHelperWrapper.hpp"
#include "FlightModule.hpp"
#include "LoadResRef.hpp"
#include <spdlog/spdlog.h>
#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/Entity.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/HardTransformBinding.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/PlaceholderComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/BaseObject.hpp>
#include <RED4ext/Scripting/Natives/Generated/user/SettingsVar.hpp>
#include <RED4ext/Scripting/Natives/Generated/user/RuntimeSettings.hpp>
#include <RED4ext/Scripting/Natives/Generated/user/RuntimeSettingsGroup.hpp>
#include <RED4ext/Scripting/Natives/Generated/user/RuntimeSettingsVar.hpp>

#include <RED4ext/Addresses.hpp>
#include <RED4ext/NativeTypes.hpp>

struct NativeSettings : RED4ext::IScriptable {
  RED4ext::CClass *GetNativeType();

  static NativeSettings *GetInstance();
  RED4ext::user::RuntimeSettings *modSettings;
  bool isAccessingModspace;
};
RED4EXT_ASSERT_SIZE(NativeSettings, 0x49);
RED4EXT_ASSERT_OFFSET(NativeSettings, isAccessingModspace, 0x48);

RED4ext::TTypedClass<NativeSettings> cls("NativeSettings");

RED4ext::CClass *NativeSettings::GetNativeType() { return &cls; }

RED4ext::Handle<NativeSettings> handle;

NativeSettings *NativeSettings::GetInstance() {
  if (!handle.instance) {
    spdlog::info("[RED4ext] New NativeSettings Instance");
    auto instance = reinterpret_cast<NativeSettings *>(cls.AllocInstance());
    handle = RED4ext::Handle<NativeSettings>(instance);
  }

  return (NativeSettings *)handle.instance;
}

void GetInstanceScripts(RED4ext::IScriptable *aContext, RED4ext::CStackFrame *aFrame,
                        RED4ext::Handle<NativeSettings> *aOut, int64_t a4) {
  aFrame->code++;

  auto h = NativeSettings::GetInstance();

  if (aOut) {
    h->ref.refCount->IncRef();
    *aOut = RED4ext::Handle<NativeSettings>(h);
  }
}

struct SettingsDataReader {
  void *__vftable;
  uint32_t unk08;
  uint32_t unk0C;
  uint64_t unk10;
  uint64_t unk18;
  uint64_t unk20;
  uint64_t unk28;
  RED4ext::CString filename;
};

// 48 89 5C 24 08 48 89 6C 24 18 56 57 41 56 48 83 EC 40 48 8B F2 48 8D 4C 24 68 49 8B D0 41 8B E9
constexpr uintptr_t CreateReaderAddr = 0x2B92D20 + 0x140000C00 - RED4ext::Addresses::ImageBase;
RED4ext::RelocFunc<SettingsDataReader **(*)(uintptr_t *, SettingsDataReader **reader, RED4ext::CString* filename, char flags)>
  CreateReader(CreateReaderAddr);

// 40 53 48 83 EC 20 65 48 8B 04 25 58 00 00 00 8B 0D EB DF FF 01 BA 9C 07 00 00 48 8B 0C C8 8B 04
RED4ext::user::RuntimeSettings *__fastcall GetRuntimeSettings();
constexpr uintptr_t GetRuntimeSettingsAddr = 0x2B9A730 + 0x140000C00 - RED4ext::Addresses::ImageBase;
decltype(&GetRuntimeSettings) GetRuntimeSettings_Original;

RED4ext::user::RuntimeSettings *__fastcall GetRuntimeSettings() { 
  if (NativeSettings::GetInstance()->isAccessingModspace) {
    return NativeSettings::GetInstance()->modSettings;
  } else {
    return GetRuntimeSettings_Original();
  }
}


// 48 89 5C 24 08 48 89 74 24 10 48 89 7C 24 18 55 41 54 41 55 41 56 41 57 48 8D 6C 24 A0 48 81 EC - index 1
bool __fastcall ReadSettingsOptions(RED4ext::user::RuntimeSettings **rcx0, RED4ext::CString *a2, RED4ext::CString *a3);
constexpr uintptr_t ReadSettingsOptionsAddr = 0x2BACB40 + 0x140000C00 - RED4ext::Addresses::ImageBase;
decltype(&ReadSettingsOptions) ReadSettingsOptions_Original;

bool __fastcall ReadSettingsOptions(RED4ext::user::RuntimeSettings** settings, RED4ext::CString* a2, RED4ext::CString* a3) {
  auto nativeSettings = NativeSettings::GetInstance();
  nativeSettings->modSettings = new RED4ext::user::RuntimeSettings();
  nativeSettings->modSettings->root = "/";
  
  // create hashmap with the root entry present
  nativeSettings->modSettings->m_registry =
      RED4ext::HashMap<RED4ext::CName, RED4ext::user::RuntimeSettingsGroup *>((*settings)->m_registry.GetAllocator());
  
  auto root = new RED4ext::user::RuntimeSettingsGroup();
  root->name = "/";
  root->path = "/";
  nativeSettings->modSettings->m_registry.Emplace(RED4ext::CName("/"), root);

  // copy these mappings, since they'll be the same
  nativeSettings->modSettings->updatePolicyMapping = (*settings)->updatePolicyMapping;
  nativeSettings->modSettings->importPolicyMapping = (*settings)->importPolicyMapping;
  nativeSettings->modSettings->typeMapping = (*settings)->typeMapping;

  // setup empty arrays/hashmaps
  nativeSettings->modSettings->m_validators =
      RED4ext::HashMap<uint64_t, uint64_t>((*settings)->m_validators.GetAllocator());
  nativeSettings->modSettings->unk70 = RED4ext::DynArray<void *>((*settings)->unk70.GetAllocator());
  nativeSettings->modSettings->unk80 = RED4ext::HashMap<uint64_t, uint64_t>((*settings)->unk80.GetAllocator());
  nativeSettings->modSettings->unkB0 = RED4ext::DynArray<void *>((*settings)->unkB0.GetAllocator());
  nativeSettings->modSettings->immediateChanges =
      RED4ext::DynArray<RED4ext::user::RuntimeSettingsVar *>((*settings)->immediateChanges.GetAllocator());
  nativeSettings->modSettings->confirmChanges = RED4ext::DynArray<void *>((*settings)->confirmChanges.GetAllocator());
  nativeSettings->modSettings->restartChanges = RED4ext::DynArray<void *>((*settings)->restartChanges.GetAllocator());
  nativeSettings->modSettings->checkpointChanges =
      RED4ext::DynArray<void *>((*settings)->checkpointChanges.GetAllocator());
  nativeSettings->modSettings->data =
      RED4ext::DynArray<RED4ext::user::RuntimeSettingsVar *>((*settings)->data.GetAllocator());
  nativeSettings->modSettings->delayedChanges =
      RED4ext::DynArray<RED4ext::user::RuntimeSettingsVar *>((*settings)->delayedChanges.GetAllocator());
  nativeSettings->modSettings->unk220 = RED4ext::DynArray<void *>((*settings)->unk220.GetAllocator());
  nativeSettings->modSettings->unk230 = RED4ext::DynArray<void *>((*settings)->unk230.GetAllocator());

  nativeSettings->modSettings->unk266 = (*settings)->unk266;
  nativeSettings->modSettings->unk26C = (*settings)->unk26C;
  nativeSettings->modSettings->unk270 = (*settings)->unk270;
  nativeSettings->modSettings->unk274 = (*settings)->unk274;

  auto og = ReadSettingsOptions_Original(settings, a2, a3);

  const char *optionsDef =
      "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Cyberpunk 2077\\r6\\config\\settings\\mod_options.json";
  RED4ext::CString optionsDefPath = RED4ext::CString(optionsDef);
  const char *options = "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Cyberpunk "
                        "2077\\r6\\config\\settings\\mod_options_platform.json";
  RED4ext::CString optionsPath = RED4ext::CString(options);

  nativeSettings->isAccessingModspace = true;
  auto mod = ReadSettingsOptions_Original(&nativeSettings->modSettings, &optionsDefPath, &optionsPath);
  nativeSettings->isAccessingModspace = false;
  return og && mod; // ok to combine, i think
}

// 48 89 4C 24 08 55 57 41 55 48 8D 6C 24 B0 48 81 EC 50 01 00 00 48 8B FA 4C 8B E9 48 85 D2 75 0E
RED4ext::user::SettingsLoadStatus __fastcall ReadSettingsData(RED4ext::user::RuntimeSettings **settings,
                                                                  SettingsDataReader *reader);
constexpr uintptr_t ReadSettingsDataAddr = 0x2BAD480 + 0x140000C00 - RED4ext::Addresses::ImageBase;
decltype(&ReadSettingsData) ReadSettingsData_Original;

RED4ext::user::SettingsLoadStatus __fastcall ReadSettingsData(RED4ext::user::RuntimeSettings** settings,
  SettingsDataReader* reader)
{
  auto og = ReadSettingsData_Original(settings, reader);

  auto nativeSettings = NativeSettings::GetInstance();
  nativeSettings->isAccessingModspace = true;

  const char *data =
      "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Cyberpunk 2077\\r6\\config\\settings\\mod_data.json";

  auto dataC = RED4ext::CString(data);
  // nativeSettings->modSettings->version = (*settings)->version;

  auto modReader = new SettingsDataReader();

  CreateReader(nullptr, &modReader, &dataC, 0);

  nativeSettings->modSettings->unk261 = 0x00;
  nativeSettings->modSettings->unk262 = 0x00;
  nativeSettings->modSettings->unk263 = 0x00;
  nativeSettings->modSettings->unk264 = 0x02;

  // modReader->__vftable = reader->__vftable;
  // modReader->unk08 = 0x0000000A;
  // modReader->unk0C = 0x000000C3;
  // modReader->unk10 = 0x0000000000000000;
  // modReader->unk18 = 0x0000000000000000;
  // modReader->unk20 = 0xFFFFFFFFFFFFFFFF;
  // modReader->unk28 = 0x0000000000000000;
  // modReader->filename = RED4ext::CString(data);

  auto mod = ReadSettingsData_Original(&nativeSettings->modSettings, modReader);
  nativeSettings->isAccessingModspace = false;
  if (mod == RED4ext::user::SettingsLoadStatus::FileIsMissing) {
    spdlog::error("Couldn't find settings data file: {}", data);
  } else if (mod == RED4ext::user::SettingsLoadStatus::FileIsCorrupted) {
    spdlog::error("Unabled to read settings data file - may be corrupted: {}", data);
  }
  return og; // not ok to combine, will restore defaults if it fails
}

struct ConfigVarModule : FlightModule {
  void Load(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) {
    auto loadedRuntimeSettings = false;
    while (!loadedRuntimeSettings) {
      loadedRuntimeSettings =
          aSdk->hooking->Attach(aHandle, RED4EXT_OFFSET_TO_ADDR(GetRuntimeSettingsAddr), &GetRuntimeSettings,
                                reinterpret_cast<void **>(&GetRuntimeSettings_Original));
    }
    auto loadedOptions = false;
    while (!loadedOptions) {
      loadedOptions =
          aSdk->hooking->Attach(aHandle, RED4EXT_OFFSET_TO_ADDR(ReadSettingsOptionsAddr), &ReadSettingsOptions,
                                reinterpret_cast<void **>(&ReadSettingsOptions_Original));
    }
    auto loadedData = false;
    while (!loadedData) {
      loadedData = aSdk->hooking->Attach(aHandle, RED4EXT_OFFSET_TO_ADDR(ReadSettingsDataAddr), &ReadSettingsData,
                                     reinterpret_cast<void **>(&ReadSettingsData_Original));
    }
  }

  void RegisterTypes() {
    auto rtti = RED4ext::CRTTISystem::Get();
    auto scriptable = rtti->GetClass("IScriptable");
    cls.flags = {.isNative = true};
    cls.parent = scriptable;
    RED4ext::CRTTISystem::Get()->RegisterType(&cls);
  };

  void PostRegisterTypes() {
    auto rtti = RED4ext::CRTTISystem::Get();
    cls.props.EmplaceBack(RED4ext::CProperty::Create(rtti->GetType("Bool"), "isAccessingModspace", nullptr,
                                                     offsetof(NativeSettings, isAccessingModspace)));
    auto getInstance = RED4ext::CClassStaticFunction::Create(&cls, "GetInstance", "GetInstance", &GetInstanceScripts,
                                                             {.isNative = true, .isStatic = true});
    cls.RegisterFunction(getInstance);
  }

  void Unload(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) {
    aSdk->hooking->Detach(aHandle, RED4EXT_OFFSET_TO_ADDR(GetRuntimeSettingsAddr));
    aSdk->hooking->Detach(aHandle, RED4EXT_OFFSET_TO_ADDR(ReadSettingsOptionsAddr));
    aSdk->hooking->Detach(aHandle, RED4EXT_OFFSET_TO_ADDR(ReadSettingsDataAddr));
  }
};

REGISTER_FLIGHT_MODULE(ConfigVarModule);