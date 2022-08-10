#pragma once

#include "Common.hpp"
#include "RTTIRegistrar.hpp"

#include <RED4ext/RTTISystem.hpp>

namespace Engine
{
template<typename T>
requires std::is_enum_v<T>
class RTTIEnumSeq : RED4ext::CEnum, detail::TypeDescriptor<T>
{
public:
    RTTIEnumSeq() : CEnum(0ull, sizeof(T), {})
    {
    }

    void SetName(const char* aName)
    {
        RED4ext::CEnum::name = RED4ext::CNamePool::Add(aName);
    }

    void SetFlags(const RED4ext::CEnum::Flags& aFlags)
    {
        RED4ext::CEnum::flags = aFlags;
    }

    inline static void Register()
    {
        RED4ext::RTTIRegistrator::Add(&OnRegisterRTTI, &OnPostRegisterRTTI);
    }

private:
    static void OnRegisterRTTI()
    {
        auto* type = new RTTIEnumSeq<T>();

        // T::OnRegister();

        if (type->name.IsNone())
            type->SetName(detail::ExtractShortTypeName<T>());

        auto* rtti = RED4ext::CRTTISystem::Get();
        rtti->RegisterType(type);

        detail::TypeDescriptor<T>::s_type = type;
    }

    static void OnPostRegisterRTTI()
    {
    }

    inline static RTTIRegistrar s_registrar{ &OnRegisterRTTI, &OnPostRegisterRTTI }; // NOLINT(cert-err58-cpp)
};
}
