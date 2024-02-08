#include <RED4ext/Addresses-Zoltan.hpp>
#include <RED4ext/NativeTypes.hpp>
#include "Addresses.hpp"

/// @hash 1157708450
constexpr uintptr_t LoadResRefT = LoadResRefT_Addr;

template <typename T>
RED4ext::RelocFunc<RED4ext::ResourceToken<T> *(*)(RED4ext::ResourcePath *, RED4ext::SharedPtr<RED4ext::ResourceToken<T>> *wrapper, bool sync)>
    LoadResRef(LoadResRefT_Addr);