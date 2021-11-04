#pragma once

#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Scripting/IScriptable.hpp>
#include "FmodHelper.hpp"

namespace FlightAudio {
    void RegisterTypes();
    void RegisterFunctions();
    void Load();
    void Unload();
}
