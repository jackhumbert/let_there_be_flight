#include "Addresses.hpp"
#include "FlightModule.hpp"

// Vehicle Speed Unlimiter

struct GetLocalizedTextFixer : FlightModule {
  void Load(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle);
  void Unload(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle);
  static RED4ext::CString *__fastcall LookupLocalizedString(__int64, RED4ext::CString *result, char *locKey);
};

decltype(&GetLocalizedTextFixer::LookupLocalizedString) LookupLocalizedString_Original;

RED4ext::CString *__fastcall GetLocalizedTextFixer::LookupLocalizedString(__int64 db, RED4ext::CString *result,
                                                                        char *locKey) {
  auto og = LookupLocalizedString_Original(db, result, locKey);
  if (result->length == 0) {
    result = new RED4ext::CString(locKey);
  }
  return result;
}

void GetLocalizedTextFixer::Load(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) {
  while (!aSdk->hooking->Attach(aHandle, RED4EXT_OFFSET_TO_ADDR(LookupLocalizedStringAddr),
                                &LookupLocalizedString, reinterpret_cast<void **>(&LookupLocalizedString_Original)))
    ;
}

void GetLocalizedTextFixer::Unload(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) {
  aSdk->hooking->Detach(aHandle, RED4EXT_OFFSET_TO_ADDR(PhysicsStructUpdateAddr));
  aSdk->hooking->Detach(aHandle, RED4EXT_OFFSET_TO_ADDR(PhysicsUnkStructVelocityUpdateAddr));
}

REGISTER_FLIGHT_MODULE(GetLocalizedTextFixer);