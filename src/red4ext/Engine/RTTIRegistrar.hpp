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

private:
    bool m_registered;
    CallbackFunc m_regFunc;
    CallbackFunc m_buildFunc;
};
}
