#pragma once

// LTBF R41 (macOS): unchanged copy of the jackhumbert fork file (deps/red4ext.sdk). On macOS each "VFT hash + Windows slot" pair
// resolves through ltbf_mac_vft_slot (macos/MacShims.cpp).

#ifdef RED4EXT_STATIC_LIB
#include <RED4ext/Scripting/Natives/entIComponent.hpp>
#endif

namespace RED4ext
{
namespace ent
{

RED4EXT_INLINE bool IComponent::sub_110(CName propertyName, void* out) {
    static UniRelocFunc<decltype(&IComponent::sub_110)> func(VFT, 0x110);
    return func(this, propertyName, out);
}

RED4EXT_INLINE bool IComponent::sub_118(CName propertyName, void* out) {
    static UniRelocFunc<decltype(&IComponent::sub_118)> func(VFT, 0x118);
    return func(this, propertyName, out);
}

#ifdef __APPLE__
RED4EXT_INLINE Handle<game::PersistentState> IComponent::sub_150() {
    static UniRelocFunc<decltype(&IComponent::sub_150)> func(VFT, 0x150);
    return func(this);
}
#else
RED4EXT_INLINE Handle<game::PersistentState>* IComponent::sub_150(Handle<game::PersistentState>* a1) {
    static UniRelocFunc<decltype(&IComponent::sub_150)> func(VFT, 0x150);
    return func(this, a1);
}
#endif

RED4EXT_INLINE bool IComponent::sub_158() {
    static UniRelocFunc<decltype(&IComponent::sub_158)> func(VFT, 0x158);
    return func(this);
}

RED4EXT_INLINE bool IComponent::sub_160() {
    static UniRelocFunc<decltype(&IComponent::sub_160)> func(VFT, 0x160);
    return func(this);
}

RED4EXT_INLINE uint64_t IComponent::Initialize(CompInit* a1) {
    static UniRelocFunc<decltype(&IComponent::Initialize)> func(VFT, 0x170);
    return func(this, a1);
}

RED4EXT_INLINE void IComponent::Uninitialize(ScriptGameInstance* a1) {
    static UniRelocFunc<decltype(&IComponent::Uninitialize)> func(VFT, 0x178);
    return func(this, a1);
}

RED4EXT_INLINE void IComponent::OnAttach(void* a1) {
    static UniRelocFunc<decltype(&IComponent::OnAttach)> func(VFT, 0x180);
    return func(this, a1);
}

RED4EXT_INLINE void IComponent::OnDetach(void *a1) {
    static UniRelocFunc<decltype(&IComponent::OnDetach)> func(VFT, 0x190);
    return func(this, a1);
}

RED4EXT_INLINE uint64_t IComponent::OnRenderSelection(uint64_t a1) {
    static UniRelocFunc<decltype(&IComponent::OnRenderSelection)> func(VFT, 0x1F8);
    return func(this, a1);
}

RED4EXT_INLINE const char * IComponent::GetBucketString() {
    static UniRelocFunc<decltype(&IComponent::GetBucketString)> func(VFT, 0x220);
    return func(this);
}

RED4EXT_INLINE CClass* IComponent::GetReplicatedStateClass() {
    static UniRelocFunc<decltype(&IComponent::GetReplicatedStateClass)> func(VFT, 0x228);
    return func(this);
}

} // namespace ent
} // namespace RED4ext
