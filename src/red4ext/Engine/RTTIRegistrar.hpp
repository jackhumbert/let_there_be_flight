#pragma once

#include "Common.hpp"

#include <RED4ext/RTTISystem.hpp>

namespace Engine
{
/**
 * The registrar queues callbacks to register all at once on demand.
 * This serves two purposes:
 * - Auto discovery of used RTTI descriptors.
 * - Postpone until RTTI system is ready.
 */
class RTTIRegistrar
{
public:
    using CallbackFunc = RED4ext::RTTIRegistrator::CallbackFunc;

    RTTIRegistrar(CallbackFunc aRegFunc, CallbackFunc aBuildFunc);

    void Register();
    static void RegisterPending();

#ifdef __APPLE__
    // LTBF R41: the game never runs RTTI register callbacks on macOS, so ltbf_registerNatives runs the registrars directly: every
    // register function first, then every build (post-register) function. Each runs once. Returns how many ran.
    static size_t RunRegisterPhase();
    static size_t RunBuildPhase();
    static size_t Count();
#endif

private:
    bool m_registered;
    CallbackFunc m_regFunc;
    CallbackFunc m_buildFunc;
#ifdef __APPLE__
    bool m_ranRegister = false;
    bool m_ranBuild = false;
#endif
};
}

#ifdef __APPLE__
// LTBF R41: the registrar of an Engine RTTI class is an `inline static` member of a class template (RTTIClassDescriptor<T>::s_registrar
// and friends). MSVC instantiates it eagerly; clang only when something odr-uses it, so a class that no C++ code names would never
// register. Taking the address of the class's RegisterRTTI (which uses s_registrar) in a kept variable forces it. Windows: empty.
#define LTBF_RTTI_CAT2(a, b) a##b
#define LTBF_RTTI_CAT(a, b) LTBF_RTTI_CAT2(a, b)
#define LTBF_FORCE_RTTI_REGISTRAR(...) \
    [[maybe_unused]] __attribute__((used)) static void (*const LTBF_RTTI_CAT(ltbf_force_rtti_, __COUNTER__))() = &__VA_ARGS__::RegisterRTTI;
#else
#define LTBF_FORCE_RTTI_REGISTRAR(...)
#endif
namespace Engine
{
}
