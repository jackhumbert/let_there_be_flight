#pragma once

#include <type_traits>

#include <RED4ext/NativeTypes.hpp>
#include <RED4ext/RTTISystem.hpp>
#include <RED4ext/Scripting/Functions.hpp>
#include <RED4ext/Scripting/Utils.hpp>

namespace Engine
{
namespace detail
{
template<typename T>
class TypeDescriptor
{
public:
    inline static RED4ext::CBaseRTTIType* GetType()
    {
        return s_type;
    }

protected:
    inline static RED4ext::CBaseRTTIType* s_type;
};

template<typename T, typename B>
concept IsCompatible = std::is_same_v<B, T> or std::is_base_of_v<B, T>;

template<typename T>
concept IsScripable = IsCompatible<T, RED4ext::IScriptable>;

template<typename T>
concept IsTypeInfoOrVoid = IsCompatible<T, RED4ext::CBaseRTTIType> or std::is_same_v<void, T>;

template<typename T, typename U>
concept IsMemberCallCompatible = IsScripable<T> and IsCompatible<T, U> and sizeof(T) == sizeof(U);

template<typename T>
concept HasTypeGetter = requires(T)
{
    { T::GetRTTIType() } -> std::convertible_to<RED4ext::CClass*>;
};

template<typename T>
concept HasTypeNameGetter = requires(T)
{
    { T::GetRTTIName() } -> std::same_as<RED4ext::CName>;
};

template<typename T>
concept HasTypeNameConst = requires(T)
{
    { T::RTTIName } -> std::convertible_to<const char*>;
};

template<typename T>
concept HasTypeFlagsConst = requires(T)
{
    { T::RTTIFlags } -> std::convertible_to<RED4ext::CClass::Flags>;
};

template<typename T>
concept HasGeneratedNameConst = requires(T)
{
    { T::NAME } -> std::convertible_to<const char*>;
    { T::ALIAS } -> std::convertible_to<const char*>;
};

template<typename T>
struct Specialization : public std::false_type {};

template<template<typename> class G, typename A>
struct Specialization<G<A>> : public std::true_type
{
    using argument_type = A;
};

template<template<typename, typename...> class G, typename A, typename... Args>
struct Specialization<G<A, Args...>> : public std::true_type
{
    using argument_type = A;
};

template<typename T>
concept IsSpecialization = Specialization<T>::value;

template<typename T>
struct RedSpecialization : public std::false_type {};

template<typename A>
struct RedSpecialization<RED4ext::DynArray<A>> : Specialization<RED4ext::DynArray<A>>
{
    static constexpr auto prefix = "array:";
};

template<typename A>
struct RedSpecialization<RED4ext::Handle<A>> : Specialization<RED4ext::Handle<A>>
{
    static constexpr auto prefix = "handle:";
};

template<typename A>
struct RedSpecialization<RED4ext::WeakHandle<A>> : Specialization<RED4ext::WeakHandle<A>>
{
    static constexpr auto prefix = "whandle:";
};

template<typename A>
struct RedSpecialization<RED4ext::ScriptRef<A>> : Specialization<RED4ext::ScriptRef<A>>
{
    static constexpr auto prefix = "script_ref:";
};

template<typename T>
concept IsRedSpecialization = RedSpecialization<T>::value;

template<typename F>
struct FunctionPtr : public std::false_type {};

template<typename R, typename... Args>
struct FunctionPtr<R (*)(Args...)> : public std::true_type
{
    using context_type = void;
    using return_type = R;
    using arguments_type = std::tuple<std::remove_const_t<std::remove_reference_t<Args>>...>;
    template<size_t I>
    using argument_type = typename std::tuple_element<I, arguments_type>::type;
    using qualified_arguments_type = std::tuple<Args...>;
    template<size_t I>
    using qualified_argument_type = typename std::tuple_element<I, qualified_arguments_type>::type;
    static constexpr size_t arity = sizeof...(Args);
};

template<typename C, typename R, typename... Args>
struct FunctionPtr<R (C::*)(Args...)> : FunctionPtr<R (*)(Args...)>
{
    using context_type = C;
};

template<typename T>
concept IsFunctionPtr = FunctionPtr<T>::value;

template<typename T>
concept IsStaticFunctionPtr = FunctionPtr<T>::value && std::is_void_v<typename FunctionPtr<T>::context_type>;

template<typename T>
concept IsMemberFunctionPtr = FunctionPtr<T>::value && !std::is_void_v<typename FunctionPtr<T>::context_type>;

namespace TypeNameExtractor
{
    template<typename T>
    consteval const auto& GetSelfSignature()
    {
        #ifdef _MSC_VER
        return __FUNCSIG__;
        #else
        return __PRETTY_FUNCTION__;
        #endif
    }

    consteval std::pair<size_t, size_t> GetAbsoluteBounds()
    {
        constexpr auto test = "bool";
        constexpr auto len = std::char_traits<char>::length(test);

        const auto& str = GetSelfSignature<bool>();

        for (size_t i = 0; i < sizeof(str) - len; ++i)
        {
            if (std::char_traits<char>::compare(str + i, test, len) == 0)
                return { i, sizeof(str) - len - i };
        }

        return { 0, 0 };
    }

    template<typename T>
    consteval std::pair<size_t, size_t> GetTypeNameBounds()
    {
        const auto bounds = GetAbsoluteBounds();
        const auto& str = GetSelfSignature<T>();

        auto left = bounds.first;
        auto right = bounds.second;

        if (std::char_traits<char>::compare(str + left, "struct ", 7) == 0)
            left += 7;
        else if (std::char_traits<char>::compare(str + left, "class ", 6) == 0)
            left += 6;
        else if (std::char_traits<char>::compare(str + left, "enum ", 5) == 0)
            left += 5;

        for (auto i = sizeof(str) - right - 1; i > left; --i)
        {
            if (str[i] == '>')
                break;

            if (str[i] == ':')
            {
                left = i + 1;
                break;
            }
        }

        return { left, right };
    }

    template<typename T>
    consteval auto GeFullTypeNameArray()
    {
        using U = std::remove_pointer_t<std::remove_cvref_t<T>>;

        constexpr auto bounds = GetAbsoluteBounds();
        constexpr auto& str = GetSelfSignature<U>();
        constexpr auto len = sizeof(str) - bounds.first - bounds.second + 1;

        std::array<char, len> name{};

        for (auto i = 0; i < len - 1; ++i)
            name[i] = str[bounds.first + i];

        return name;
    }

    template<typename T>
    consteval auto GetShortTypeNameArray()
    {
        using U = std::remove_pointer_t<std::remove_cvref_t<T>>;

        constexpr auto bounds = GetTypeNameBounds<U>();
        constexpr auto& str = GetSelfSignature<U>();
        constexpr auto len = sizeof(str) - bounds.first - bounds.second + 1;

        std::array<char, len> name{};

        for (auto i = 0; i < len - 1; ++i)
            name[i] = str[bounds.first + i];

        return name;
    }
};

template<typename T>
const char* ExtractFullTypeName()
{
    static auto name = TypeNameExtractor::GeFullTypeNameArray<T>();
    return name.data();
}

template<typename T>
const char* ExtractShortTypeName()
{
    static auto name = TypeNameExtractor::GetShortTypeNameArray<T>();
    return name.data();
}

template<typename T, RED4ext::CName N>
struct TypeCase
{
    using type = T;
    static constexpr auto name = N;
};

template<typename T, typename Case>
consteval bool TryTypeCase(RED4ext::CName& name)
{
    if constexpr(std::is_same_v<T, typename Case::type>)
    {
        name = Case::name;
        return true;
    }

    return false;
}

template<typename T, typename... Cases>
consteval RED4ext::CName MatchTypeCase()
{
    RED4ext::CName name{};
    (TryTypeCase<T, Cases>(name) || ...);
    return name;
}

template<typename T, RED4ext::CName Prefix = 0xCBF29CE484222325>
consteval RED4ext::CName ResolveConstTypeName()
{
    // if constexpr (HasTypeNameConst<T>)
    //     return RED4ext::FNV1a64(T::RTTIName, Prefix);

    if constexpr (HasGeneratedNameConst<T>)
        return RED4ext::FNV1a64(T::NAME, Prefix);

    if constexpr (IsRedSpecialization<T>)
        return ResolveConstTypeName<typename RedSpecialization<T>::argument_type,
                                    RED4ext::FNV1a64(RedSpecialization<T>::prefix, Prefix)>();

    return MatchTypeCase<
        T,
        TypeCase<int8_t, RED4ext::FNV1a64("Int8", Prefix)>,
        TypeCase<uint8_t, RED4ext::FNV1a64("Uint8", Prefix)>,
        TypeCase<int16_t, RED4ext::FNV1a64("Int16", Prefix)>,
        TypeCase<uint16_t, RED4ext::FNV1a64("Uint16", Prefix)>,
        TypeCase<int32_t, RED4ext::FNV1a64("Int32", Prefix)>,
        TypeCase<uint32_t, RED4ext::FNV1a64("Uint32", Prefix)>,
        TypeCase<int64_t, RED4ext::FNV1a64("Int64", Prefix)>,
        TypeCase<uint64_t, RED4ext::FNV1a64("Uint64", Prefix)>,
        TypeCase<float, RED4ext::FNV1a64("Float", Prefix)>,
        TypeCase<double, RED4ext::FNV1a64("Double", Prefix)>,
        TypeCase<bool, RED4ext::FNV1a64("Bool", Prefix)>,
        TypeCase<RED4ext::CString, RED4ext::FNV1a64("String", Prefix)>,
        TypeCase<RED4ext::CName, RED4ext::FNV1a64("CName", Prefix)>,
        TypeCase<RED4ext::TweakDBID, RED4ext::FNV1a64("TweakDBID", Prefix)>,
        TypeCase<RED4ext::ItemID, RED4ext::FNV1a64("gameItemID", Prefix)>,
        TypeCase<RED4ext::NodeRef, RED4ext::FNV1a64("NodeRef", Prefix)>,
        TypeCase<RED4ext::Variant, RED4ext::FNV1a64("Variant", Prefix)>
    >();
}

template<typename T>
std::string BuildDynamicTypeName()
{
    if constexpr (HasTypeNameGetter<T>)
        return T::GetRTTIName().ToString();

    if constexpr (IsRedSpecialization<T>)
        return std::string(RedSpecialization<T>::prefix)
            .append(BuildDynamicTypeName<typename RedSpecialization<T>::argument_type>());

    auto type = TypeDescriptor<T>::GetType();
    if (type)
        return type->GetName().ToString();

    return {};
}

template<typename T>
constexpr RED4ext::CName ResolveTypeName()
{
    using U = std::remove_pointer_t<std::remove_cvref_t<T>>;

    constexpr auto name = ResolveConstTypeName<U>();

    if constexpr (!name.IsNone())
        return name;

    if constexpr (HasTypeNameGetter<U>)
        return U::GetRTTIName();

    auto type = TypeDescriptor<U>::GetType();
    if (type)
        return type->GetName().ToString();

    // To create a composite type (array, handle, etc.) it's enough to add
    // the full type name to the name pool. The RTTI system generates the
    // missing types dynamically if it can get the name as a string.
    return RED4ext::CNamePool::Add(BuildDynamicTypeName<U>().c_str());
}

template<typename T>
inline void ExtractArg(RED4ext::CStackFrame* aFrame, T* aArg)
{
    RED4ext::GetParameter(aFrame, aArg);
}

template<typename... Args, std::size_t... I>
inline void ExtractArgs(RED4ext::CStackFrame* aFrame, std::tuple<Args...>& aArgs, std::index_sequence<I...>)
{
    (ExtractArg(aFrame, &std::get<I>(aArgs)), ...);
}

template<typename... Args>
inline void ExtractArgs(RED4ext::CStackFrame* aFrame, std::tuple<Args...>& aArgs)
{
    ExtractArgs(aFrame, aArgs, std::make_index_sequence<sizeof...(Args)>());
    aFrame->code++;
}
}

template<typename R, typename RT>
requires detail::IsTypeInfoOrVoid<RT>
using RTTIStaticFunction = void (*)(RED4ext::IScriptable*, RED4ext::CStackFrame*, R*, RT*);

template<class C, typename R, typename RT>
requires detail::IsScripable<C> and detail::IsTypeInfoOrVoid<RT>
using RTTIMemberFunction = void (C::*)(RED4ext::CStackFrame*, R*, RT*);

template<typename R, typename... Args>
using StaticFunction = R (*)(Args...);

template<class C, typename R, typename... Args>
using MemberFunction = R (C::*)(Args...);

template<auto AFunc>
requires detail::IsFunctionPtr<decltype(AFunc)>
inline RED4ext::ScriptingFunction_t<void*> WrapScriptableFunction()
{
    using namespace detail;

    using F = decltype(AFunc);
    using C = typename FunctionPtr<F>::context_type;
    using R = typename FunctionPtr<F>::return_type;
    using Args = typename FunctionPtr<F>::arguments_type;

    static const auto s_func = AFunc;

    auto f = [](RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame,
                R* aRet, RED4ext::CBaseRTTIType* aRetType) -> void
    {
        Args args;
        ExtractArgs(aFrame, args);

        if constexpr (std::is_void_v<R>)
        {
            if constexpr (std::is_void_v<C>)
                std::apply(s_func, args);
            else
                std::apply(s_func, std::tuple_cat(std::make_tuple(reinterpret_cast<C*>(aContext)), args));
        }
        else
        {
            R ret;
            if constexpr (std::is_void_v<C>)
                ret = std::apply(s_func, args);
            else
                ret = std::apply(s_func, std::tuple_cat(std::make_tuple(reinterpret_cast<C*>(aContext)), args));

            if (aRet) {
              if (aRetType) {
                aRetType->Assign(aRet, &ret);
              } else {
                *aRet = ret;
              }
            }
        }
    };

    return reinterpret_cast<RED4ext::ScriptingFunction_t<void*>>(+f);
}

template<typename R, typename... Args>
void DescribeScriptableFunction(RED4ext::CBaseFunction* aScriptFunc, StaticFunction<R, Args...>)
{
    using namespace detail;

    (aScriptFunc->AddParam(ResolveTypeName<Args>(), "arg"), ...);

    if constexpr (!std::is_void_v<R>)
        aScriptFunc->SetReturnType(ResolveTypeName<R>());
}

template<typename C, typename R, typename... Args>
void DescribeScriptableFunction(RED4ext::CBaseFunction* aScriptFunc, MemberFunction<C, R, Args...>)
{
    using namespace detail;

    (aScriptFunc->AddParam(ResolveTypeName<Args>(), "arg"), ...);

    if constexpr (!std::is_void_v<R>)
        aScriptFunc->SetReturnType(ResolveTypeName<R>());
}
}
