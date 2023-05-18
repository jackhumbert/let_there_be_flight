// #include "Addresses.hpp"
// #include "Utils/FlightModule.hpp"

// No more unlocalized text

//struct GetLocalizedTextFixer : FlightModule {
//  void Load(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle);
//  void Unload(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle);
//  static RED4ext::CString *__fastcall LookupLocalizedString(__int64, RED4ext::CString *result,
//                                                            RED4ext::CString *locKey);
//  static RED4ext::CString *__fastcall SetLocalizedTextString(__int64, RED4ext::CString *result,
//                                                            RED4ext::CString *locKey);
//};
//
//decltype(&GetLocalizedTextFixer::LookupLocalizedString) LookupLocalizedString_Original;
//
//RED4ext::CString *__fastcall GetLocalizedTextFixer::LookupLocalizedString(__int64 db, RED4ext::CString *result,
//                                                                          RED4ext::CString *locKey) {
//  auto og = LookupLocalizedString_Original(db, result, locKey);
//  if (og->length == 0) {
//    og = locKey;
//  }
//  return og;
//}
//
//decltype(&GetLocalizedTextFixer::SetLocalizedTextString) SetLocalizedTextString_Original;
//
//RED4ext::CString *__fastcall GetLocalizedTextFixer::SetLocalizedTextString(__int64 db, RED4ext::CString *result,
//                                                                          RED4ext::CString *locKey) {
//  auto og = SetLocalizedTextString_Original(db, result, locKey);
//  if (og->length == 0) {
//    og = locKey;
//  }
//  return og;
//}
//
//void GetLocalizedTextFixer::Load(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) {
//  while (!aSdk->hooking->Attach(aHandle, RED4EXT_OFFSET_TO_ADDR(LookupLocalizedStringAddr), &LookupLocalizedString,
//                                reinterpret_cast<void **>(&LookupLocalizedString_Original)))
//    while (!aSdk->hooking->Attach(aHandle, RED4EXT_OFFSET_TO_ADDR(SetLocalizedTextStringAddr), &SetLocalizedTextString,
//                                  reinterpret_cast<void **>(&SetLocalizedTextString_Original)))
//    ;
//}
//
//void GetLocalizedTextFixer::Unload(const RED4ext::Sdk *aSdk, RED4ext::PluginHandle aHandle) {
//  aSdk->hooking->Detach(aHandle, RED4EXT_OFFSET_TO_ADDR(LookupLocalizedStringAddr));
//  aSdk->hooking->Detach(aHandle, RED4EXT_OFFSET_TO_ADDR(SetLocalizedTextStringAddr));
//}
//
//REGISTER_FLIGHT_MODULE(GetLocalizedTextFixer);