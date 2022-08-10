#pragma once

#include "Common.hpp"
#include "RTTIRegistrar.hpp"

#include <RED4ext/RTTISystem.hpp>
#include <RED4ext/Scripting/IScriptable.hpp>

namespace Engine
{
template<class T>
class RTTIClassDescriptor : public RED4ext::TTypedClass<T>, public detail::TypeDescriptor<T>
{
public:
    RTTIClassDescriptor() : RED4ext::TTypedClass<T>(0ull) {}

    void SetName(const char* aName)
    {
        RED4ext::CClass::name = RED4ext::CNamePool::Add(aName);
    }

    void SetFlags(const RED4ext::CClass::Flags& aFlags)
    {
        RED4ext::CClass::flags = aFlags;
    }

    template<typename R, typename TR>
    RED4ext::CClassStaticFunction* AddFunction(RTTIStaticFunction<R, TR> aFunc, const char* aName,
                                               RED4ext::CBaseFunction::Flags aFlags = {})
    {
        const auto* ptr = reinterpret_cast<RED4ext::ScriptingFunction_t<R*>>(aFunc);
        auto* func = RED4ext::CClassStaticFunction::Create(this, aName, aName, ptr, aFlags);

        RED4ext::CClass::RegisterFunction(func);

        return func;
    }

    template<class C, typename R, typename TR>
    requires std::is_same_v<C, T>
    RED4ext::CClassStaticFunction* AddFunction(RTTIMemberFunction<C, R, TR> aFunc, const char* aName,
                                               RED4ext::CBaseFunction::Flags aFlags = {})
    {
        const auto* ptr = reinterpret_cast<RED4ext::ScriptingFunction_t<R*>>(aFunc);
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
            func = RED4ext::CClassFunction::Create(this, aName, aName, ptr, aFlags);
        else
            func = RED4ext::CClassStaticFunction::Create(this, aName, aName, ptr, aFlags);

        DescribeScriptableFunction(func, AFunc);

        RED4ext::CClass::RegisterFunction(func);

        return func;
    }

    inline static void RegisterRTTI()
    {
        s_registrar.Register();
    }

    inline static RED4ext::CClass* Get()
    {
        return s_class;
    }

private:
    static void OnRegisterRTTI()
    {
        using FinalDescriptor = typename T::Descriptor;

        auto* desc = new FinalDescriptor();

        // if constexpr (detail::HasTypeNameConst<T>)
        //     desc->SetName(T::RTTIName);
        //
        // if constexpr (detail::HasTypeFlagsConst<T>)
        //     desc->SetFlags(T::RTTIFlags);

        T::OnRegister(desc);

        if (desc->name.IsNone())
            desc->SetName(detail::ExtractShortTypeName<T>());

        auto* rtti = RED4ext::CRTTISystem::Get();
        rtti->RegisterType(desc);

        s_class = desc;
        detail::TypeDescriptor<T>::s_type = desc;
    }

    static void OnPostRegisterRTTI()
    {
        using FinalDescriptor = typename T::Descriptor;
        using ParentType = typename T::Parent;

        auto* rtti = RED4ext::CRTTISystem::Get();

        // Auto-fill parent
        if constexpr (detail::HasTypeGetter<ParentType>)
            s_class->parent = ParentType::GetRTTIType();
        else if constexpr (detail::HasGeneratedNameConst<ParentType>)
            s_class->parent = rtti->GetClass(ParentType::NAME);
        else
            s_class->parent = rtti->GetClass("IScriptable");

        auto* desc = reinterpret_cast<FinalDescriptor*>(s_class);

        T::OnDescribe(desc, rtti);

        // Force native flag
        s_class->flags.isNative = true;
    }

    inline static RED4ext::CClass* s_class;
    inline static RTTIRegistrar s_registrar{ &OnRegisterRTTI, &OnPostRegisterRTTI }; // NOLINT(cert-err58-cpp)
};

template<class T, class P = RED4ext::IScriptable>
requires detail::IsScripable<P>
class RTTIClass : public P
{
public:
    using Descriptor = RTTIClassDescriptor<T>;
    using Parent = P;
    using Flags = RED4ext::CClass::Flags;

    RTTIClass() = default;

    RED4ext::CClass* GetNativeType() override
    {
        return Descriptor::Get();
    }

    inline static RED4ext::Handle<T> NewInstance(RED4ext::CClass* aDerived = nullptr)
    {
        // 1. Internal Handle
        // All IScriptable instances inherit WeakHandle .ref field from ISerializable.
        // This field is required for instance to be recognized by scripting engine.
        // Without this field initialized the scripted `this` will always be null.
        // When instantiated from scripts with `new Class()` the .ref is initialized
        // using the Handle constructed and returned by the `new` operator.
        // 2. Internal Class Pointer
        // Also IScriptable instances must have .unk30 set to the RTTI class instance.
        // This property is used by the engine when accessing class members.

        // TODO: Move to RED4ext::Handle? Or RED4ext::CClass::CreateHandle?

        // Resolve the type
        auto type = Descriptor::Get();

        if (aDerived && aDerived->IsA(type))
            type = aDerived;

        // Allocate and construct the instance
        auto instance = type->AllocInstance();
        type->ConstructCls(instance);

        // Construct the handle
        auto scriptable = reinterpret_cast<T*>(instance);
        auto handle = RED4ext::Handle<T>(scriptable);

        // Assign the handle to the instance
        scriptable->ref = RED4ext::WeakHandle(*reinterpret_cast<RED4ext::Handle<RED4ext::ISerializable>*>(&handle));

        // Assign the type to the instance
        scriptable->unk30 = type;

        return std::move(handle);
    }

    inline static RED4ext::CClass* GetRTTIType()
    {
        return Descriptor::Get();
    }

    inline static RED4ext::CName GetRTTIName()
    {
        return Descriptor::Get()->GetName();
    }

    inline static void RegisterRTTI()
    {
        Descriptor::RegisterRTTI();
    }

private:
    friend Descriptor;

    static void OnRegister(Descriptor* aType) {}
    static void OnDescribe(Descriptor* aType, RED4ext::CRTTISystem* aRtti) {}
};
}
