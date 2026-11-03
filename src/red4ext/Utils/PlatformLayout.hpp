#pragma once

// LTBF R41: fields the macOS SDK's generated classes do not declare (they carry only the reflected members). On Windows these are
// the jackhumbert fork's members; on macOS they are read at their offsets (macos/MacLayout.hpp).

#include <RED4ext/Handle.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/IComponent.hpp>
#include <type_traits>

template<typename T>
inline T *LtbfPtr(T *aPointer) {
  return aPointer;
}

template<typename T>
inline T *LtbfPtr(const RED4ext::Handle<T> &aHandle) {
  return aHandle.GetPtr();
}

#ifdef __APPLE__
#include "MacLayout.hpp"
// ent::SlotComponent.slotIndexLookup, HashMap<CName, int32_t> at 0x150 (R41b: GetSlotIndex 0xC463C8 reads it inline)
#define LTBF_SLOT_INDEX_LOOKUP(sc)                                                                                              \
  (::LtbfMac::Layout::At<RED4ext::HashMap<RED4ext::CName, int32_t>>(LtbfPtr(sc), ::LtbfMac::Layout::SlotComponent_slotIndexLookup))
// ent::VisualControllerComponent.resourcePaths, DynArray<ResourcePath> at 0xC8 (inferred, see MacLayout.hpp)
#define LTBF_VCC_RESOURCE_PATHS(vcc)                                                                                            \
  (::LtbfMac::Layout::At<RED4ext::DynArray<RED4ext::ResourcePath>>(LtbfPtr(vcc),                                                \
                                                                   ::LtbfMac::Layout::VisualControllerComponent_resourcePaths))
#else
#define LTBF_SLOT_INDEX_LOOKUP(sc) ((sc)->slotIndexLookup)
#define LTBF_VCC_RESOURCE_PATHS(vcc) ((vcc)->resourcePaths)
#endif

#ifdef __APPLE__
// LTBF R41: the macOS SDK's Handle<T> has both a const and a non-const conversion to Handle<U>, so constructing a
// Handle<ent::IComponent> in place from a Handle<Derived> (DynArray::EmplaceBack) is ambiguous for clang. Same object, explicit type.
template<typename T>
inline const RED4ext::Handle<RED4ext::ent::IComponent> &LtbfAsComponentHandle(const RED4ext::Handle<T> &aHandle) {
  static_assert(std::is_base_of_v<RED4ext::ent::IComponent, T>, "not a component");
  return *reinterpret_cast<const RED4ext::Handle<RED4ext::ent::IComponent> *>(&aHandle);
}
#define LTBF_AS_COMPONENT_HANDLE(handle) LtbfAsComponentHandle(handle)
#else
#define LTBF_AS_COMPONENT_HANDLE(handle) (handle)
#endif
