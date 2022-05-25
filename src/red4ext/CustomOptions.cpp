#include "FlightModule.hpp"
#include <RED4ext/RED4ext.hpp>
#include <spdlog/spdlog.h>
#include "Utils.hpp"

struct OptionStructUnk18 {
  void *unk00;
  uint64_t unk08;
  uint64_t unk10;
  void* unk18;
  void* unk20;
};

#pragma pack(1)
struct OptionStruct {
  uint32_t version;
  uint32_t version2;
  uint64_t unk08;
  //uint16_t version3;
  void * unk10;
  OptionStructUnk18 *unk18;
  void *unk20;
  void *unk28;
  uint64_t unk30;
  uint64_t unk38;
  uint64_t unk40;
  uint64_t unk48;
  uint32_t unk50;
  uint32_t unk54;
  uint64_t unk58;
};
RED4EXT_ASSERT_OFFSET(OptionStruct, unk10, 0x10);

struct OptionData {
  char *contentStart;
  char *content;
  char *contentEnd;
  uint64_t length;
};

// 40 53 48 83 EC 30 48 8B 05 D3 08 04 02 48 8B DA 4C 8B C1 48 8D 54 24 50 45 33 C9 48 8B C8 4C 8B
constexpr uintptr_t LoadFileAddr = 0x140000C00 + 0x2B93E30 - RED4ext::Addresses::ImageBase;
char __fastcall LoadFile(RED4ext::CString *path, RED4ext::CString *contents) {
  RED4ext::RelocFunc<char (*)(RED4ext::CString * a1, RED4ext::CString * a2)> func(LoadFileAddr);
  return func(path, contents);
}

// 48 89 5C 24 10 48 89 74 24 18 55 57 41 56 48 8B EC 48 83 EC 60 48 8B 41 20 45 33 F6 66 0F 6F 0D 2C 6F 4E 00 0F 57 C0 48 89 45 C0 48 8B DA 4C 89
constexpr uintptr_t ParseJsonFileAddr = 0x140000C00 + 0x2BA7730 - RED4ext::Addresses::ImageBase;
OptionStruct *__fastcall ParseJsonFile(OptionStruct *a1, OptionData **a2) {
  RED4ext::RelocFunc<OptionStruct *(*)(OptionStruct *a1, OptionData **a2)> func(
      ParseJsonFileAddr);
  return func(a1, a2);
}

// 48 89 5C 24 10 55 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 80 48 81 EC 80 01 00 00 48 8D 35 B9
constexpr uintptr_t OptionsRead1Addr = 0x140000C00 + 0x2BABF90 - RED4ext::Addresses::ImageBase;
char __fastcall OptionsRead1(uintptr_t a1, RED4ext::CString *filePath, OptionStruct *three) {
  RED4ext::RelocFunc<char (*)(uintptr_t a1, RED4ext::CString * filePath, OptionStruct * three)> func(
      OptionsRead1Addr);
  return func(a1, filePath, three);
 }

// 44 88 4C 24 20 48 89 54 24 10 48 89 4C 24 08 55 53 56 57 41 54 41 55 41 56 41 57 48 8D AC 24 D8
constexpr uintptr_t OptionsRead2Addr = 0x140000C00 + 0x2BA99D0 - RED4ext::Addresses::ImageBase;
 char __fastcall OptionsRead2(uintptr_t a1, RED4ext::CString *filePath, OptionStruct *three, char zero_then_one) {
  RED4ext::RelocFunc<char (*)(uintptr_t a1, RED4ext::CString * filePath, OptionStruct * three, char zero_then_one)>
      func(OptionsRead2Addr);
  return func(a1, filePath, three, zero_then_one);
}

// 48 89 5C 24 08 48 89 74  24 10 48 89 7C 24 18 55 41 54 41 55 41 56 41 57 48 8D 6C 24 A0 48 81 EC 60 01 00 00 4C 8B E9
// 4D 8B F8 48 8D 4D 40 4C 8B
char __fastcall OptionsLoad(uintptr_t rcx0, RED4ext::CString *commonFilePath, RED4ext::CString *platformFilePath);
constexpr uintptr_t OptionsLoadAddr = 0x140000C00 + 0x2BACB40 - RED4ext::Addresses::ImageBase;
decltype(&OptionsLoad) OptionsLoad_Original;

char __fastcall OptionsLoad(uintptr_t rcx0, RED4ext::CString *common, RED4ext::CString *platform) { 
  auto value = OptionsLoad_Original(rcx0, common, platform);
  auto allocator = new RED4ext::Memory::DefaultAllocator();/*
  const char *flightOptions =
      (Utils::GetRootDir() / "r6" / "config" / "settings" / "flight_control.json").string().c_str();*/
  const char *flightOptions = "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Cyberpunk 2077\\r6\\config\\settings\\flight_control.json";
  OptionStruct optionStruct;
  OptionStructUnk18 unk18;
  memset(&optionStruct, 0, 0x60);
  memset(&unk18, 0, 0x28);
  unk18.unk08 = 0x10000;
  optionStruct.unk10 = &unk18;
  optionStruct.unk18 = &unk18;

  //optionStruct.version = 3;
  //optionStruct.version2 = 3;
  optionStruct.unk48 = 0x400;

  RED4ext::CString filePath = RED4ext::CString(flightOptions);
  //filePath.length = strlen(flightOptions);
  //filePath.allocator = allocator;

  RED4ext::CString contents;
  contents.text.str[0] = 0;
  contents.length = 0;
  contents.allocator = 0;

  if (!LoadFile(&filePath, &contents)) {
    spdlog::error("Failed to load flight control config file");
  }

  OptionData *data = new OptionData();
  auto content = contents.text.ptr;
  auto length = contents.Length();
  auto end = &content[length];
  data->contentStart = content;
  data->content = content;
  data->length = length;
  data->contentEnd = end;

  if (content != &content[length]) {
    if (*content == -17)
      data->contentStart = ++content;
    if (content != end) {
      if (*content == -69)
        data->contentStart = ++content;
      if (content != end && *content == -65)
        data->contentStart = content + 1;
    }
  }
  //optionStruct.unk08 = (uint64_t)(content + 16);
  //optionStruct.version3 = 3;

  ParseJsonFile(&optionStruct, &data);

  //cs.text.ptr = const_cast<char *>(&flightOptions);
  if (OptionsRead1(rcx0, &filePath, &optionStruct)) {
    spdlog::info("Flight options read 1");
    if (OptionsRead2(rcx0, &filePath, &optionStruct, 0)) {
      spdlog::info("Flight options read 2");
    }
  }
  return value;
}

struct CustomOptionsModule : FlightModule {
  void Load(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) {
    aSdk->hooking->Attach(aHandle, RED4EXT_OFFSET_TO_ADDR(OptionsLoadAddr), &OptionsLoad,
                          reinterpret_cast<void **>(&OptionsLoad_Original));
  }
  void Unload(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) {
    aSdk->hooking->Detach(aHandle, RED4EXT_OFFSET_TO_ADDR(OptionsLoadAddr));
  }
};

REGISTER_FLIGHT_MODULE(CustomOptionsModule);