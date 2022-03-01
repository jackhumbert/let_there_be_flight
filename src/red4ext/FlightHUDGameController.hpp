#pragma once

#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Scripting/IScriptable.hpp>

namespace FlightHUDGameController {
    void RegisterTypes();
    void RegisterFunctions();
    void Load();
    void Unload();
}
