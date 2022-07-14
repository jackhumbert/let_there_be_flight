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
#include <fstream>

struct NativeSettings : RED4ext::IScriptable {
  RED4ext::CClass *GetNativeType();

  static NativeSettings *GetInstance();
  RED4ext::user::RuntimeSettings *modSettings;
  int32_t isAccessingModspace;
};
RED4EXT_ASSERT_SIZE(NativeSettings, 0x4C);
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

struct FileFunctions;
struct FileReader;
struct FileWriter;

struct FileFunctions_vtbl {
  FileFunctions * ( __fastcall * DestroyFileFunctions)(FileFunctions *, char);
  FileReader ** (__fastcall * CreateReader)(FileFunctions *, FileReader **, RED4ext::CString *, char);
  FileWriter ** (__fastcall * CreateWriter)(FileFunctions *, FileWriter **, RED4ext::CString *, uint64_t);
};

struct FileFunctions {
  FileFunctions_vtbl * vft;
  RED4ext::CString cache;
  RED4ext::CString engine;
  RED4ext::CString r6;
};

struct FileHandler;

struct FileHandler_vtbl {
  RED4ext::Memory::IAllocator *(__fastcall * GetAllocator)(FileHandler *);
  FileHandler * (__fastcall *Close)(FileHandler *, uint64_t);
};

struct FileHandler {
  FileHandler_vtbl *vft;
  uint32_t unk08;
  uint32_t unk0C;
  uint64_t unk10;
  uint64_t unk18;
};

struct FileReader : FileHandler {
  uint64_t unk20;
  uint64_t unk28;
  RED4ext::CString filename;
};

struct FileWriter : FileHandler {
  FileReader* fileWriter;
  uint64_t unk28;
  uint64_t unk30;
  uint64_t unk38;
  uint64_t unk40;
  uint64_t unk48;
  uint64_t unk50;
  uint64_t unk58;
  uint64_t unk60;
  uint64_t unk68;
};

// 48 89 5C 24 08 48 89 6C 24 18 56 57 41 56 48 83 EC 40 48 8B F2 48 8D 4C 24 68 49 8B D0 41 8B E9
constexpr uintptr_t CreateReaderAddr = 0x2B92D20 + 0x140000C00 - RED4ext::Addresses::ImageBase;
RED4ext::RelocFunc<FileReader **(*)(FileFunctions *, FileReader **reader, RED4ext::CString *filename,
                                            char flags)>
  CreateReader(CreateReaderAddr);

// 48 89 5C 24 20 55 57 41 56 48 83 EC 30 48 8B 01 4C 8B F2 49 8B D0 41 8B E9 49 8B F8 FF 90 A0 00
constexpr uintptr_t CreateWriterAddr = 0x2B92EE0 + 0x140000C00 - RED4ext::Addresses::ImageBase;
RED4ext::RelocFunc<FileWriter **(*)(FileFunctions *, FileWriter **writer, RED4ext::CString *filename,
                                            uint64_t flags)>
  CreateWriter(CreateWriterAddr);

// 40 53 48 83 EC 20 65 48 8B 04 25 58 00 00 00 8B 0D EB DF FF 01 BA 9C 07 00 00 48 8B 0C C8 8B 04
RED4ext::user::RuntimeSettings *__fastcall GetRuntimeSettings();
constexpr uintptr_t GetRuntimeSettingsAddr = 0x2B9A730 + 0x140000C00 - RED4ext::Addresses::ImageBase;
decltype(&GetRuntimeSettings) GetRuntimeSettings_Original;

RED4ext::user::RuntimeSettings *__fastcall GetRuntimeSettings() { 
  if (NativeSettings::GetInstance()->isAccessingModspace > 0) {
    return NativeSettings::GetInstance()->modSettings;
  } else {
    return GetRuntimeSettings_Original();
  }
}

// 270
// 40 53 48 83 EC 20 65 48 8B 04 25 58 00 00 00 8B 0D EB DF FF 01 BA 9C 07 00 00 48 8B 0C C8 8B 04
// 0x00007FF7453DB330 - 0x7FF742840000

// 274
// 48 83 EC 28 65 48 8B 04 25 58 00 00 00 8B 0D 5D BC FF 01 BA 9C 07 00 00  48 8B 0C C8 8B 04 0A 39
// 0x00007FF7453DD6C0 - 0x7FF742840000

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
      RED4ext::HashMap<uint64_t, RED4ext::DynArray<void *>>((*settings)->m_validators.GetAllocator());
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
  nativeSettings->modSettings->unk220 = RED4ext::DynArray<RED4ext::user::RuntimeSettingsVar *>((*settings)->unk220.GetAllocator());
  nativeSettings->modSettings->unk230 = RED4ext::DynArray<void *>((*settings)->unk230.GetAllocator());

  nativeSettings->modSettings->unk266 = (*settings)->unk266; // set to 1 before load i think
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

  nativeSettings->isAccessingModspace++;
  auto mod = ReadSettingsOptions_Original(&nativeSettings->modSettings, &optionsDefPath, &optionsPath);
  nativeSettings->isAccessingModspace--;
  return og && mod; // ok to combine, i think
}

const uintptr_t FileFunctionsAddr = 0x7FF747415310 - 0x7FF742840000 + RED4ext::RelocBase::GetImageBase();

// 40 53 48 83 EC 20 48 8B D9 E8 F2 8D 00 00 48 8B CB E8 1A 08 00 00 48 8B CB E8 C2 02 00 00 48 8B
void * __fastcall ProcessSettingsData(RED4ext::user::RuntimeSettings *settings);
constexpr uintptr_t ProcessSettingsDataAddr = 0x2B9B8A0 + 0x140000C00 - RED4ext::Addresses::ImageBase;
decltype(&ProcessSettingsData) ProcessSettingsData_Original;

void * __fastcall ProcessSettingsData(RED4ext::user::RuntimeSettings *settings) {

  // need to pass over iscriptable/userSettings thing

  auto nativeSettings = NativeSettings::GetInstance();

  auto og = nullptr;

  if (nativeSettings->isAccessingModspace == 0) {
    auto og = ProcessSettingsData_Original(settings);
  }

  nativeSettings->isAccessingModspace++;
  auto mod = ProcessSettingsData_Original(nativeSettings->modSettings);
  nativeSettings->isAccessingModspace--;

  return og;
}

  // 48 89 4C 24 08 55 57 41 55 48 8D 6C 24 B0 48 81 EC 50 01 00 00 48 8B FA 4C 8B E9 48 85 D2 75 0E
RED4ext::user::SettingsLoadStatus __fastcall ReadSettingsData(RED4ext::user::RuntimeSettings **settings,
                                                                  FileReader *reader);
constexpr uintptr_t ReadSettingsDataAddr = 0x2BAD480 + 0x140000C00 - RED4ext::Addresses::ImageBase;
decltype(&ReadSettingsData) ReadSettingsData_Original;

RED4ext::user::SettingsLoadStatus __fastcall ReadSettingsData(RED4ext::user::RuntimeSettings** settings,
  FileReader* reader) {

  auto nativeSettings = NativeSettings::GetInstance();

  auto og = RED4ext::user::SettingsLoadStatus::Loaded;

  if (nativeSettings->isAccessingModspace == 0) {
    og = ReadSettingsData_Original(settings, reader);
  }

  nativeSettings->isAccessingModspace++;

  const char *data =
      "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Cyberpunk 2077\\r6\\config\\settings\\mod_data.json";
  auto dataC = RED4ext::CString(data);

  auto ff = *(FileFunctions **)(FileFunctionsAddr);
  auto modReader = new FileReader();
  //CreateReader(ff, &modReader, &dataC, 0);
  ff->vft->CreateReader(ff, &modReader, &dataC, 0);

  nativeSettings->modSettings->unk261 = 0x00;
  nativeSettings->modSettings->unk262 = 0x00;
  nativeSettings->modSettings->unk263 = 0x00;
  nativeSettings->modSettings->unk264 = 0x02;

  auto mod = ReadSettingsData_Original(&nativeSettings->modSettings, modReader);

  //if (modReader) {
  //  auto mem = (*(void *(__fastcall **)(FileReader *))(*(uint64_t *)modReader))(modReader);
  //  (*(void(__fastcall **)(FileReader *, uint64_t))(*(uint64_t *)modReader + 8i64))(modReader, 0i64);
  //  //free(mem);
  //}

  if (modReader) {
    auto mem = modReader->vft->GetAllocator(modReader);
    modReader->vft->Close(modReader, 0i64);
    mem->Free(modReader);
  }

  nativeSettings->isAccessingModspace--;
  if (mod == RED4ext::user::SettingsLoadStatus::FileIsMissing) {
    spdlog::error("Couldn't find settings data file: {}", data);
  } else if (mod == RED4ext::user::SettingsLoadStatus::FileIsCorrupted) {
    spdlog::error("Unabled to read settings data file - may be corrupted: {}", data);
  }
  return og; // not ok to combine, will restore defaults if it fails
}

struct ModSettingsWriter {
  virtual void sub_00() {}
  virtual void sub_08() {}
  virtual void WriteFile(char *contents, uint32_t length) {
    std::ofstream modfile;
    modfile.open(this->filename, std::ios::out);
    if (modfile) {
      modfile.write(contents, length);
      spdlog::info("Wrote {} to settings file: {}", length, this->filename);
      this->status = 0;
    } else {
      spdlog::error("Could not open file for writing: {}", this->filename);
      this->status = 0x80000;
    }
    modfile.close();
  }

  uint32_t status;
  const char *filename;
};

// 48 89 54 24 10 55 53 48 8D 6C 24 B1 48 81 EC E8 00 00 00 48 8B D9 48 85 D2 75 0C 33 C0 48 81 C4
uint64_t __fastcall WriteSettingsData(RED4ext::user::RuntimeSettings **settings, FileWriter *writer);
constexpr uintptr_t WriteSettingsDataAddr = 0x2BB36D0 + 0x140000C00 - RED4ext::Addresses::ImageBase;
decltype(&WriteSettingsData) WriteSettingsData_Original;

uint64_t __fastcall WriteSettingsData(RED4ext::user::RuntimeSettings **settings, FileWriter *writer) {
  auto nativeSettings = NativeSettings::GetInstance();

  // maybe these are data independent
   nativeSettings->modSettings->m_validators = (*settings)->m_validators;
  // nativeSettings->modSettings->unk70 = (*settings)->unk70;
  // nativeSettings->modSettings->unk80 = (*settings)->unk80;
  // nativeSettings->modSettings->unkB0 = (*settings)->unkB0;

  auto og = 0i64;

  if (nativeSettings->isAccessingModspace == 0) {
    og = WriteSettingsData_Original(settings, writer);
  }

  nativeSettings->isAccessingModspace++;

  nativeSettings->modSettings->unk263 = 0;

  const char *data =
      "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Cyberpunk 2077\\r6\\config\\settings\\mod_data.json";
  auto dataC = RED4ext::CString(data);
  //  nativeSettings->modSettings->version = (*settings)->version;

  // auto ff = new FileFunctions();
  // ff->vft = 0x00007FF745E27188;
  // const char *cache = "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Cyberpunk 2077\\cache\\";
  // ff->cache = RED4ext::CString(cache);
  // const char *engine = "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Cyberpunk 2077\\engine\\";
  // ff->engine = RED4ext::CString(engine);
  // const char *r6 = "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Cyberpunk 2077\\r6\\";
  // ff->r6 = RED4ext::CString(r6);
  auto ff = *(FileFunctions **)(FileFunctionsAddr);

   auto modWriter = new FileWriter();
  //auto modWriter = new ModSettingsWriter();
  //modWriter->filename =
      //"C:\\Program Files (x86)\\Steam\\steamapps\\common\\Cyberpunk 2077\\r6\\config\\settings\\mod_data.json";
  //CreateWriter(ff, &modWriter, &dataC, 1);

   ff->vft->CreateWriter(ff, &modWriter, &dataC, 1);
  //RED4ext::RelocFunc<FileWriter **(*)(FileFunctions *, FileWriter * *writer, RED4ext::CString * filename,
                                        //uint64_t flags)> CreateFileWriter(*(uint64_t*)(ff->vft + 0x10));
  //CreateFileWriter(ff, &modWriter, &dataC, 1);

  auto mod = WriteSettingsData_Original(&nativeSettings->modSettings, modWriter);

  //if (modWriter) {
  //  auto mem = (*(void *(__fastcall **)(FileWriter *))(*(uint64_t *)modWriter))(modWriter);
  //  (*(void(__fastcall **)(FileWriter *, uint64_t))(*(uint64_t *)modWriter + 8i64))(modWriter, 0i64);
  //  // free(mem);
  //}

  if (modWriter) {
    auto mem = modWriter->vft->GetAllocator(modWriter);
    modWriter->vft->Close(modWriter, 0i64);
    mem->Free(modWriter);
  }

  if (mod == 0) {
    spdlog::error("Unable to write settings file: {}", data);
  }
  nativeSettings->isAccessingModspace--;

  return og;
}

struct NativeSettingsModule : FlightModule {
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
    auto loadedProcess = false;
    while (!loadedProcess) {
      loadedProcess = aSdk->hooking->Attach(aHandle, RED4EXT_OFFSET_TO_ADDR(ProcessSettingsDataAddr), &ProcessSettingsData,
                                         reinterpret_cast<void **>(&ProcessSettingsData_Original));
    }
    auto loadedDataWriter = false;
    while (!loadedDataWriter) {
      loadedDataWriter = aSdk->hooking->Attach(aHandle, RED4EXT_OFFSET_TO_ADDR(WriteSettingsDataAddr), &WriteSettingsData,
                                         reinterpret_cast<void **>(&WriteSettingsData_Original));
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
    cls.props.EmplaceBack(RED4ext::CProperty::Create(rtti->GetType("Int32"), "isAccessingModspace", nullptr,
                                                     offsetof(NativeSettings, isAccessingModspace)));
    auto getInstance = RED4ext::CClassStaticFunction::Create(&cls, "GetInstance", "GetInstance", &GetInstanceScripts,
                                                             {.isNative = true, .isStatic = true});
    cls.RegisterFunction(getInstance);
  }

  void Unload(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) {
    aSdk->hooking->Detach(aHandle, RED4EXT_OFFSET_TO_ADDR(GetRuntimeSettingsAddr));
    aSdk->hooking->Detach(aHandle, RED4EXT_OFFSET_TO_ADDR(ReadSettingsOptionsAddr));
    aSdk->hooking->Detach(aHandle, RED4EXT_OFFSET_TO_ADDR(ReadSettingsDataAddr));
    aSdk->hooking->Detach(aHandle, RED4EXT_OFFSET_TO_ADDR(ProcessSettingsDataAddr));
    aSdk->hooking->Detach(aHandle, RED4EXT_OFFSET_TO_ADDR(WriteSettingsDataAddr));
  }
};

//REGISTER_FLIGHT_MODULE(NativeSettingsModule);