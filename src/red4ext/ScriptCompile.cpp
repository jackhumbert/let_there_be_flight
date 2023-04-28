#include <RED4ext/RED4ext.hpp>
#include "Utils/FlightModule.hpp"
#include "stdafx.hpp"
#include "Addresses.hpp"
#include <stdlib.h>

struct StringThing {
  uint32_t length;
  uint32_t unk;
  wchar_t* str;
};

char dest[0x1000];

REGISTER_FLIGHT_HOOK(uint64_t __fastcall, ExecuteCommand, void *scriptCompilation, 
    RED4ext::CString * command, StringThing *args, RED4ext::CString * currentDirectoryThing, char a5) {
  spdlog::info("command: {}", command->c_str());
  spdlog::info("argsLength: {}, {}, args:", args->length, args->unk);
  wcstombs_s(nullptr, dest, args->str, 0x1000);
  spdlog::info("{}", dest);
  spdlog::info("currentDirectoryThing: {}", currentDirectoryThing->c_str());

  strcat_s(dest, R"( -compile "C:\Program Files (x86)\Steam\steamapps\common\Cyberpunk 2077\red4ext\plugins\let_there_be_flight\")");
  args->length = strlen(dest);
  args->unk = strlen(dest);
  mbstowcs(args->str, dest, 0x1000);

  spdlog::info("argsLength: {}, {}, args:", args->length, args->unk);
  wcstombs_s(nullptr, dest, args->str, 0x1000);
  spdlog::info("{}", dest);

  // __debugbreak();
  return ExecuteCommand_Original(scriptCompilation, command, args, currentDirectoryThing, a5);
}
