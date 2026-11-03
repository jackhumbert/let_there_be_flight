#include "RTTIRegistrar.hpp"

namespace
{
#ifdef __APPLE__
// macOS: function-local statics. Registrars are static members constructed during static initialization of other files, which
// may run before this file's globals exist (the first registrar would push into an unconstructed vector).
std::vector<Engine::RTTIRegistrar*>& Pending()
{
    static std::vector<Engine::RTTIRegistrar*> s_list;
    return s_list;
}

std::vector<Engine::RTTIRegistrar*>& All()
{
    static std::vector<Engine::RTTIRegistrar*> s_all;
    return s_all;
}
#else
std::vector<Engine::RTTIRegistrar*> s_pending;
#endif
}

Engine::RTTIRegistrar::RTTIRegistrar(CallbackFunc aRegFunc, CallbackFunc aBuildFunc)
    : m_registered(false)
    , m_regFunc(aRegFunc)
    , m_buildFunc(aBuildFunc)
{
#ifdef __APPLE__
    Pending().push_back(this);
    All().push_back(this);
#else
    s_pending.push_back(this);
#endif
}

void Engine::RTTIRegistrar::Register()
{
    if (!m_registered)
    {
        RED4ext::RTTIRegistrator::Add(m_regFunc, m_buildFunc);
        m_registered = true;
    }
}

void Engine::RTTIRegistrar::RegisterPending()
{
#ifdef __APPLE__
    for (const auto& pending : Pending())
        pending->Register();

    Pending().clear();
#else
    for (const auto& pending : s_pending)
        pending->Register();

    s_pending.clear();
#endif
}
#ifdef __APPLE__

size_t Engine::RTTIRegistrar::RunRegisterPhase()
{
    size_t ran = 0;
    for (auto* registrar : All())
    {
        if (!registrar->m_ranRegister)
        {
            registrar->m_ranRegister = true;
            registrar->m_registered = true;
            if (registrar->m_regFunc)
                registrar->m_regFunc();
            ++ran;
        }
    }
    return ran;
}

size_t Engine::RTTIRegistrar::RunBuildPhase()
{
    size_t ran = 0;
    for (auto* registrar : All())
    {
        if (registrar->m_ranRegister && !registrar->m_ranBuild)
        {
            registrar->m_ranBuild = true;
            if (registrar->m_buildFunc)
                registrar->m_buildFunc();
            ++ran;
        }
    }
    return ran;
}

size_t Engine::RTTIRegistrar::Count()
{
    return All().size();
}
#endif
