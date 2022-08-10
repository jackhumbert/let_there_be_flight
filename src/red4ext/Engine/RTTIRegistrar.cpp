#include "RTTIRegistrar.hpp"

namespace
{
std::vector<Engine::RTTIRegistrar*> s_pending;
}

Engine::RTTIRegistrar::RTTIRegistrar(CallbackFunc aRegFunc, CallbackFunc aBuildFunc)
    : m_registered(false)
    , m_regFunc(aRegFunc)
    , m_buildFunc(aBuildFunc)
{
    s_pending.push_back(this);
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
    for (const auto& pending : s_pending)
        pending->Register();

    s_pending.clear();
}
