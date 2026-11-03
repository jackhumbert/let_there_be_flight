// LTBF R42a (macOS): a stand-in dependency plugin for deps_test.cpp. Only Query(), reporting STUB_MAJOR.STUB_MINOR.STUB_PATCH.
#include <RED4ext/Api/SemVer.hpp>
#include <RED4ext/Api/v0/PluginInfo.hpp>

extern "C" __attribute__((visibility("default"))) void Query(RED4ext::v0::PluginInfo* aInfo)
{
    aInfo->version = RED4EXT_SEMVER(STUB_MAJOR, STUB_MINOR, STUB_PATCH);
}
