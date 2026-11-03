#pragma once
// LTBF R41 (macOS): Input Loader is not a RED4ext plugin on the Mac. The inputs come from the Perl input loader
// (engine/tools/inputloader.pl) reading r6/input/ltbf-mac.xml, which the launcher runs. Add is a logged no-op.

#include <filesystem>
#include <RED4ext/Api/PluginHandle.hpp>
#include "MacShims.hpp"

class InputLoader
{
public:
    inline static bool Add(RED4ext::PluginHandle, std::filesystem::path aPath)
    {
        LtbfMac::Log("[ltbf] InputLoader::Add(%s) is a no-op on macOS (the launcher merges r6/input/ltbf-mac.xml)",
                     aPath.string().c_str());
        return true;
    }
};
