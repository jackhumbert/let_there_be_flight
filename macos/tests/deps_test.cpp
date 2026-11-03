// LTBF R42a (macOS): offline test of the dependency check. RED4ext-macos loads the plugins in folder order and runs each Main right
// away, so at LTBF's Main a dependency in a later folder is not loaded yet (Boot B: ArchiveXL and mod_settings came after
// let_there_be_flight, Main returned false, RED4ext unloaded LTBF, and its scripts failed to link).
//
// Usage: deps_test <game root of a test tree> later|missing
//   later:   <root>/red4ext/plugins/{ArchiveXL,TweakXL,mod_settings}/<name>.dylib exist but are not loaded at Main. Main must return
//            true and count all three as found. Then ArchiveXL (1.27.4) and TweakXL (1.10.0, too old) are loaded and the late check
//            (ltbf_registerNatives) must find ArchiveXL only.
//   missing: no dependency dylibs at all. Main must still return true (macOS never unloads LTBF for a dependency); nothing found.
// Main runs with an empty Sdk (no hooking interface), so no hook is attached and nothing outside the test tree is touched.
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <dlfcn.h>
#include <string>

using Main_t = bool (*)(void*, uint8_t, const void*);
using Check_t = int (*)(int);

static int g_failures = 0;
static void Expect(bool aOk, const char* aWhat)
{
    std::printf("%s %s\n", aOk ? "ok  " : "FAIL", aWhat);
    if (!aOk)
        ++g_failures;
}

int main(int argc, char** argv)
{
    if (argc != 3)
    {
        std::printf("usage: deps_test <root> later|missing\n");
        return 2;
    }
    const std::string root = argv[1];
    const bool later = std::strcmp(argv[2], "later") == 0;
    const std::string plugins = root + "/red4ext/plugins/";

    void* ltbf = dlopen((plugins + "let_there_be_flight/let_there_be_flight.dylib").c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!ltbf)
    {
        std::printf("FAIL dlopen let_there_be_flight.dylib: %s\n", dlerror());
        return 1;
    }
    auto main = reinterpret_cast<Main_t>(dlsym(ltbf, "Main"));
    auto check = reinterpret_cast<Check_t>(dlsym(ltbf, "ltbf_mac_check_dependencies"));
    if (!main || !check)
    {
        std::printf("FAIL exports Main / ltbf_mac_check_dependencies missing\n");
        return 1;
    }

    static unsigned char sdk[128] = {}; // RED4ext::Sdk with every interface null
    const bool loaded = main(ltbf, 0 /* EMainReason::Load */, sdk);
    Expect(loaded, "Main(Load) returns true");

    const int early = check(0);
    if (later)
        Expect(early == 7, "at Main: ArchiveXL, TweakXL and mod_settings present but not loaded count as found (7)");
    else
        Expect(early == 0, "at Main: no dependency on disk, nothing found (0)");

    if (later)
    {
        Expect(dlopen((plugins + "ArchiveXL/ArchiveXL.dylib").c_str(), RTLD_NOW | RTLD_LOCAL) != nullptr, "load ArchiveXL stub 1.27.4");
        Expect(dlopen((plugins + "TweakXL/TweakXL.dylib").c_str(), RTLD_NOW | RTLD_LOCAL) != nullptr, "load TweakXL stub 1.10.0");
        const int late = check(1);
        Expect(late == 1, "at ltbf_registerNatives: ArchiveXL found; TweakXL too old and mod_settings not loaded are skipped (1)");
    }

    std::printf("deps_test %s: %s (%d failure(s))\n", argv[2], g_failures ? "FAILED" : "ok", g_failures);
    return g_failures ? 1 : 0;
}
