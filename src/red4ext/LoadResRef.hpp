#include <RED4ext/NativeTypes.hpp>

template <typename T>
RED4ext::UniversalRelocFunc<RED4ext::ResourceToken<T> *(*)(
    RED4ext::ResourcePath *, 
    RED4ext::SharedPtr<RED4ext::ResourceToken<T>> *wrapper, 
    bool sync)>
        LoadResRef(1157708450);