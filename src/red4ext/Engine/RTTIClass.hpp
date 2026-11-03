#pragma once

#include "Common.hpp"
#include "RTTIRegistrar.hpp"

#include <RED4ext/RTTISystem.hpp>
#include <RED4ext/Scripting/IScriptable.hpp>

#ifdef __APPLE__
namespace RED4ext::game
{
struct Component;
}
// macos/MacShims.cpp: the game's own gameComponent allocator (the one FlightComponent's GetAllocator returns).
extern "C" RED4ext::Memory::IAllocator* ltbf_mac_game_component_allocator();
#endif

namespace Engine
{

template<class T>
class RTTIStructDescriptor : public RED4ext::TTypedClass<T>, public detail::TypeDescriptor<T>
{
public:
  RTTIStructDescriptor() : RED4ext::TTypedClass<T>(0ull) {}

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
    auto* ptr = reinterpret_cast<RED4ext::ScriptingFunction_t<R*>>(aFunc);
    auto* func = RED4ext::CClassStaticFunction::Create(this, aName, aName, ptr, aFlags);

    RED4ext::CClass::RegisterFunction(func);

    return func;
  }

  template<class C, typename R, typename TR>
    requires std::is_same_v<C, T>
  RED4ext::CClassStaticFunction* AddFunction(RTTIMemberFunction<C, R, TR> aFunc, const char* aName,
                                             RED4ext::CBaseFunction::Flags aFlags = {})
  {
    auto* ptr = reinterpret_cast<RED4ext::ScriptingFunction_t<R*>>(aFunc);
    auto* func = RED4ext::CClassFunction::Create(this, aName, aName, ptr, aFlags);

    RED4ext::CClass::RegisterFunction(func);

    return func;
  }

  template<auto AFunc>
    requires detail::IsFunctionPtr<decltype(AFunc)>
  RED4ext::CClassFunction* AddFunction(const char* aName, RED4ext::CBaseFunction::Flags aFlags = {})
  {
    auto* ptr = WrapScriptableFunction<AFunc>();

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

//     if constexpr (detail::HasTypeNameConst<T>)
//         desc->SetName(T::RTTIName);
//
//     if constexpr (detail::HasTypeFlagsConst<T>)
//         desc->SetFlags(T::RTTIFlags);

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

    auto* rtti = RED4ext::CRTTISystem::Get();

    auto* desc = reinterpret_cast<FinalDescriptor*>(s_class);

    T::OnDescribe(desc, rtti);

    // Force native flag
    s_class->flags.isNative = true;
  }

  inline static RED4ext::CClass* s_class;
  inline static RTTIRegistrar s_registrar{ &OnRegisterRTTI, &OnPostRegisterRTTI }; // NOLINT(cert-err58-cpp)
};

template<class T>
class RTTIClassDescriptor : public RED4ext::TTypedClass<T>, public detail::TypeDescriptor<T>
{
public:
    RTTIClassDescriptor() : RED4ext::TTypedClass<T>(0ull) {}

#ifdef __APPLE__
    // LTBF R42g (macOS): allocate instances from the pool the game frees them to. TTypedClass::AllocMemory allocates through the
    // class's GetAllocator() (the SDK default is Memory::RTTIAllocator, PoolRTTI), but the game destroys an object through the
    // object's own GetAllocator() (vtable slot 0x10; game 0x102104940: GetAllocator, destructor, allocator->Free). That is
    // ISerializable's (Memory::ScriptAllocator, PoolScript) for every LTBF class except FlightComponent, whose game::Component
    // override returns the game's component allocator. On macOS those are different pools, so every freed LTBF object went onto
    // another pool's free list (Boots C to E1: zeroed vtables of live components, corrupt free lists). The class side now returns
    // exactly what the instance side returns, chosen by the class that declares the GetAllocator T inherits.
    RED4ext::Memory::IAllocator* GetAllocator() const override
    {
        using InstanceGetter = decltype(&T::GetAllocator);
        if constexpr (std::is_same_v<InstanceGetter, RED4ext::Memory::IAllocator* (RED4ext::ISerializable::*)()>)
        {
            return RED4ext::Memory::ScriptAllocator::Get();
        }
        else if constexpr (std::is_same_v<InstanceGetter, RED4ext::Memory::IAllocator* (RED4ext::game::Component::*)()>)
        {
            return ltbf_mac_game_component_allocator();
        }
        else
        {
            static_assert(sizeof(T) == 0, "LTBF macOS: a class with its own GetAllocator needs a matching class-side allocator here");
        }
    }
#endif

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
        auto* ptr = reinterpret_cast<RED4ext::ScriptingFunction_t<R*>>(aFunc);
        auto* func = RED4ext::CClassStaticFunction::Create(this, aName, aName, ptr, aFlags);

        RED4ext::CClass::RegisterFunction(func);

        return func;
    }

    template<class C, typename R, typename TR>
    requires std::is_same_v<C, T>
    RED4ext::CClassStaticFunction* AddFunction(RTTIMemberFunction<C, R, TR> aFunc, const char* aName,
                                               RED4ext::CBaseFunction::Flags aFlags = {})
    {
        auto* ptr = reinterpret_cast<RED4ext::ScriptingFunction_t<R*>>(aFunc);
        auto* func = RED4ext::CClassFunction::Create(this, aName, aName, ptr, aFlags);

        RED4ext::CClass::RegisterFunction(func);

        return func;
    }

    template<auto AFunc>
    requires detail::IsFunctionPtr<decltype(AFunc)>
    RED4ext::CClassFunction* AddFunction(const char* aName, RED4ext::CBaseFunction::Flags aFlags = {})
    {
        auto* ptr = WrapScriptableFunction<AFunc>();

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

template<class T>
class RTTIStruct
{
  public:
    using Descriptor = RTTIStructDescriptor<T>;
    using Flags = RED4ext::CClass::Flags;

    RTTIStruct() = default;

    inline static T* NewInstance(RED4ext::CClass* aDerived = nullptr)
    {
        // Resolve the type
        auto type = Descriptor::Get();

        if (aDerived && aDerived->IsA(type))
            type = aDerived;

        // Allocate and construct the instance
        auto instance = type->AllocInstance();
        type->ConstructCls(instance);

        // Construct the handle
        auto pointer = reinterpret_cast<T*>(instance);

        return std::move(pointer);
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

#define DESCRIBE_PROPERTY(cls, type, name) \
  aType->props.PushBack(RED4ext::CProperty::Create(rtti->GetType(#type ), #name , nullptr, offsetof(cls, name)))