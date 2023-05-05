#include <RED4ext/RED4ext.hpp>
#include "Utils/FlightModule.hpp"
#include "stdafx.hpp"
#include "Addresses.hpp"
#include <stdlib.h>
#include "Utils/Utils.hpp"

// struct StringThing {
//   uint32_t length;
//   uint32_t unk;
//   wchar_t* str;
// };

// 1.6  RVA: 0xA743D0 / 10961872
// 1.62hf1  RVA: 0xA83A60 / 11024992
/// @pattern 48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 48 83 EC 20 41 0F B7 D8 0F B6 FA 48 8B F1 E8
// __int64 __fastcall sub_7FF75BF543D0(RED4ext::CBaseEngine *a1, unsigned __int8 a2, unsigned __int16 a3);

// // char dest[0x1000];

// REGISTER_FLIGHT_HOOK(uint64_t __fastcall, ExecuteCommand, void *scriptCompilation, 
//     RED4ext::CString * command, StringThing *args, RED4ext::CString * currentDirectory, char a5) {
      
//   spdlog::info("command: {}", command->c_str());
//   spdlog::info("argsLength: {}, {}, args:", args->length, args->unk);
//   // wcstombs_s(nullptr, dest, args->str, 0x1000);
//   spdlog::info(L"{}", args->str);
//   spdlog::info("currentDirectory: {}", currentDirectory->c_str());

//   // strcat_s(dest, R"( -compile "C:\Program Files (x86)\Steam\steamapps\common\Cyberpunk 2077\red4ext\plugins\let_there_be_flight\")");
//   // args->length = strlen(dest);
//   // args->unk = strlen(dest);
//   // mbstowcs(args->str, dest, 0x1000);

//   // spdlog::info("argsLength: {}, {}, args:", args->length, args->unk);
//   // wcstombs_s(nullptr, dest, args->str, 0x1000);
//   // spdlog::info("{}", dest);

//   spdlog::info("Updating compile command");
//   wchar_t * original = args->str;
//   wchar_t buffer[0x1000] = {0};
//   wcscpy_s(buffer, args->str);
//   auto paths = {
//     Utils::GetRootDir() / "red4ext" / "plugins" / "let_there_be_flight" / "let_there_be_flight.packed.reds",
//     Utils::GetRootDir() / "red4ext" / "plugins" / "let_there_be_flight" / "let_there_be_flight.module.reds"
//   };
//   for (auto& path : paths)
//   {
//     spdlog::info(L"Adding path: {}", path.wstring().c_str());
//     wsprintf(buffer, L"%s -compile \"%s\"", buffer, path.wstring().c_str());
//   }
  
//   args->str = buffer;
//   args->unk = args->length = wcslen(buffer);
  
//   spdlog::info("argsLength: {}, {}, args:", args->length, args->unk);
//   spdlog::info(L"{}", args->str);

//   auto result = ExecuteCommand_Original(scriptCompilation, command, args, currentDirectory, a5);

//   args->str = original;
//   args->unk = args->length = wcslen(original);

//   // __debugbreak();
//   return result;
// }
