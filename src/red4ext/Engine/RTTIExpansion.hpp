#pragma once

#include "Common.hpp"
#include "RTTIRegistrar.hpp"

#include <RED4ext/RTTISystem.hpp>

namespace Engine
{
template<class T, RED4ext::CName N>
class RTTIClassExpander : public RED4ext::CClass
{
public:
    template<typename R, typename TR>
    RED4ext::CClassStaticFunction* AddFunction(RTTIStaticFunction<R, TR> aFunc, const char* aName,
                                               RED4ext::CBaseFunction::Flags aFlags = {})
    {
        const auto* ptr = reinterpret_cast<RED4ext::ScriptingFunction_t<R>>(aFunc);
        auto* func = RED4ext::CClassStaticFunction::Create(this, aName, aName, ptr, aFlags);

        RED4ext::CClass::RegisterFunction(func);

        return func;
    }

    template<class C, typename R, typename TR>
    requires detail::IsMemberCallCompatible<T, C>
    RED4ext::CClassStaticFunction* AddFunction(RTTIMemberFunction<C, R, TR> aFunc, const char* aName,
                                               RED4ext::CBaseFunction::Flags aFlags = {})
    {
        const auto* ptr = reinterpret_cast<RED4ext::ScriptingFunction_t<R>>(aFunc);
        auto* func = RED4ext::CClassFunction::Create(this, aName, aName, ptr, aFlags);

        RED4ext::CClass::RegisterFunction(func);

        return func;
    }

    template<auto AFunc>
    requires detail::IsFunctionPtr<decltype(AFunc)>
    RED4ext::CClassFunction* AddFunction(const char* aName, RED4ext::CBaseFunction::Flags aFlags = {})
    {
        const auto* ptr = WrapScriptableFunction<AFunc>();

        RED4ext::CClassFunction* func;
        if constexpr (detail::IsMemberFunctionPtr<decltype(AFunc)>)
        {
            static_assert(
                detail::IsScripable<typename detail::FunctionPtr<decltype(AFunc)>::context_type>,
                "Only IScriptable classes can have member methods.");

            func = RED4ext::CClassFunction::Create(this, aName, aName, ptr, aFlags);
        }
        else
        {
            func = RED4ext::CClassStaticFunction::Create(this, aName, aName, ptr, aFlags);
        }

        DescribeScriptableFunction(func, AFunc);

        RED4ext::CClass::RegisterFunction(func);

        return func;
    }

    inline static void RegisterRTTI()
    {
        s_registrar.Register();
    }

private:
    static void OnRegisterRTTI() {}

    static void OnPostRegisterRTTI()
    {
        using FinalDescriptor = typename T::Descriptor;

        auto* rtti = RED4ext::CRTTISystem::Get();
        auto* type = rtti->GetClass(N);

        if (type)
        {
            T::OnExpand(reinterpret_cast<FinalDescriptor*>(type), rtti);
        }
    }

    inline static RTTIRegistrar s_registrar{ &OnRegisterRTTI, &OnPostRegisterRTTI }; // NOLINT(cert-err58-cpp)
};

template<class T, typename B = void, RED4ext::CName N = B::NAME>
class RTTIExpansion : public B
{
public:
    using Descriptor = RTTIClassExpander<T, N>;

    inline static void RegisterRTTI()
    {
        Descriptor::RegisterRTTI();
    }

private:
    friend Descriptor;

    static void OnExpand(Descriptor* aType, RED4ext::CRTTISystem* aRtti) {}
};
}
