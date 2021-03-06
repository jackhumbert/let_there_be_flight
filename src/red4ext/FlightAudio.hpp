#pragma once

#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Scripting/IScriptable.hpp>
#include "FmodHelper.hpp"

namespace FlightAudio {
    void RegisterTypes();
    void RegisterFunctions();
    bool Load(RED4ext::CGameApplication *aApp);
    bool Unload(RED4ext::CGameApplication* aApp);
}
