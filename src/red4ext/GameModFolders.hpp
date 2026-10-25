#pragma once

// GameModFolders.hpp - load a RED4ext plugin's archives and ArchiveXL-style configs from its own folder,
// using the loader built into Cyberpunk 2077 2.5+ (no ArchiveXL needed).
//
// 2.5 asks gamemod::GameModService for the list of installed mod.io mod folders, and that one list feeds:
//   - res::ResourceGameDepot::OpenModArchives: mounts every *.archive in each folder (archive set "mod")
//   - App::ExtensionLoader::Configure: loads every *.json in each folder as an ArchiveXL-style config
//     (rapidjson; same keys as ArchiveXL's YAML: localization/onscreens, streaming/sectors/nodeDeletions, ...)
//   - the TweakDB loader (sub_142120C50 in 2.5): every *.json under the folder whose path contains "tweaks",
//     in TweakXL's YAML schema written as JSON ($type, $base, $value, $props: "AutoFlats", field directives
//     as { "!append": [...] }). No $instances templating; ExtraFlats.json / InheritanceMap.json are special.
// Register() hooks the function that builds that list and appends your folder to it.
//
// Header-only, depends only on RED4ext's plugin API. It is meant to be copied into other mods
// unchanged. Every copy installs its own hook on the same function, and the hooks chain through
// RED4ext. They cooperate because every copy follows the same rules:
//   1. Call the original first, then append. Never remove, reorder or replace entries.
//   2. Build and append entries only with the game's own functions (red::AbsolutePath::CreateDirPath,
//      red::DynArray<red::AbsolutePath>::PushBack), so the allocator and layout are always the game's.
//   3. Skip a folder that is already in the list (case-insensitive, either slash), so two mods that
//      register the same folder do not mount it twice.
//   4. The original rebuilds the list from scratch on every call, so appending on every call is correct.
//      Do not cache the list or append only once.
// If you change the behavior, bump kVersion and keep the rules above.
//
// Usage, from your plugin's Main (EMainReason::Load), before the game initializes its resource depot:
//
//   if (!GameModFolders::Register(aSdk, aHandle, pluginDir)) {
//     // game older than 2.5, or the function moved: fall back to ArchiveXL::RegisterArchive
//   }
//   ...
//   GameModFolders::Unregister(aSdk, aHandle);   // EMainReason::Unload
//
// pluginDir holds your .archive and your config as <name>.json (ArchiveXL's .xl keys, in JSON).
// Every *.json in the folder is offered to the config loader, so keep unrelated JSON files elsewhere.
//
// Found with the 2.5 IDB (FileVersion 3.0.5813403). Hashes are the game's address-database hashes.

#include <RED4ext/Api/PluginHandle.hpp>
#include <RED4ext/Api/Sdk.hpp>

#include <Windows.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace GameModFolders
{
inline constexpr uint32_t kVersion = 1;

namespace Detail
{
// gamemod::GameModService vfunc +0x170 (0x142302B10 in 2.5): fills a DynArray<red::AbsolutePath> with the
// installed mod.io mod folders. Unnamed; void (GameModService* this, DynArray<AbsolutePath>* out).
inline constexpr uint32_t kGetModFolders = 3981123713;
// ?CreateDirPath@AbsolutePath@red@@SA?AV12@VStringView@2@@Z
inline constexpr uint32_t kCreateDirPath = 1439044616;
// ?PushBack@?$DynArray@VAbsolutePath@red@@@red@@QEAAX$$QEAVAbsolutePath@2@@Z
inline constexpr uint32_t kPushBack = 3438811507;
// ?Execute@?$NonTrivialDestructorExecutor@VAbsolutePath@red@@@policies@red@@SAXPEAVAbsolutePath@3@I@Z
inline constexpr uint32_t kDestruct = 1377641636;

// red::AbsolutePath is a red::String: 0x14 bytes inline text, or a heap pointer when either of the
// top two bits of info is set (the pointer is stored in the first 8 bytes of text); length is
// info & 0x3FFFFFFF.
struct Path
{
    char text[0x14];
    uint32_t info;
    void* allocator;
};
static_assert(sizeof(Path) == 0x20);
static_assert(offsetof(Path, info) == 0x14);

struct StringView
{
    const char* data;
    uint32_t length;
};

struct PathArray
{
    Path* entries;
    uint32_t capacity;
    uint32_t size;
};

using GetModFolders_t = void (*)(void* aService, PathArray* aOut);
using CreateDirPath_t = Path* (*)(Path* aOut, StringView* aPath);
using PushBack_t = void (*)(PathArray* aArray, Path* aPath);
using Destruct_t = void (*)(Path* aPaths, uint32_t aCount);

struct State
{
    std::vector<std::string> folders;
    uintptr_t target = 0;
    GetModFolders_t original = nullptr;
    CreateDirPath_t createDirPath = nullptr;
    PushBack_t pushBack = nullptr;
    Destruct_t destruct = nullptr;
};

inline State& GetState()
{
    static State s_state;
    return s_state;
}

// RED4ext's resolver, called directly: RED4ext::UniversalRelocBase::Resolve terminates the game on a
// missing hash, and Register() should fail softly on games without the built-in loader.
inline uintptr_t Resolve(uint32_t aHash)
{
    using Resolve_t = uintptr_t (*)(uint32_t);
    static const auto s_resolve = []() -> Resolve_t {
        const auto module = GetModuleHandleW(L"RED4ext.dll");
        return module ? reinterpret_cast<Resolve_t>(GetProcAddress(module, "RED4ext_ResolveAddress")) : nullptr;
    }();
    return s_resolve ? s_resolve(aHash) : 0;
}

inline std::string_view View(const Path& aPath)
{
    const char* text = aPath.text;
    if (aPath.info & 0xC0000000)
        std::memcpy(&text, aPath.text, sizeof(text));
    return {text, aPath.info & 0x3FFFFFFF};
}

inline bool SameFolder(std::string_view aLeft, std::string_view aRight)
{
    const auto trim = [](std::string_view s) {
        while (!s.empty() && (s.back() == '\\' || s.back() == '/'))
            s.remove_suffix(1);
        return s;
    };
    const auto fold = [](char c) {
        if (c == '/')
            return '\\';
        if (c >= 'A' && c <= 'Z')
            return static_cast<char>(c - 'A' + 'a');
        return c;
    };
    aLeft = trim(aLeft);
    aRight = trim(aRight);
    if (aLeft.size() != aRight.size())
        return false;
    for (size_t i = 0; i < aLeft.size(); ++i)
    {
        if (fold(aLeft[i]) != fold(aRight[i]))
            return false;
    }
    return true;
}

// Backslashes and a trailing separator, the way the game builds mod folder paths itself.
inline std::string Normalize(const std::filesystem::path& aFolder)
{
    const auto u8 = aFolder.lexically_normal().u8string();
    std::string folder(u8.begin(), u8.end());
    for (auto& c : folder)
    {
        if (c == '/')
            c = '\\';
    }
    if (!folder.empty() && folder.back() != '\\')
        folder.push_back('\\');
    return folder;
}

inline void GetModFolders(void* aService, PathArray* aOut)
{
    auto& state = GetState();
    state.original(aService, aOut);
    if (!aOut)
        return;

    for (const auto& folder : state.folders)
    {
        bool present = false;
        for (uint32_t i = 0; i < aOut->size && !present; ++i)
            present = SameFolder(View(aOut->entries[i]), folder);
        if (present)
            continue;

        Path path{};
        StringView view{folder.data(), static_cast<uint32_t>(folder.size())};
        state.createDirPath(&path, &view);
        state.pushBack(aOut, &path);
        state.destruct(&path, 1);
    }
}
} // namespace Detail

// Adds aFolder to the game's mod folder list. Call from EMainReason::Load; may be called more than once.
// Returns false (and changes nothing) if this game version has no built-in loader or the hook failed.
inline bool Register(const RED4ext::Sdk* aSdk, RED4ext::PluginHandle aHandle, const std::filesystem::path& aFolder)
{
    auto& state = Detail::GetState();
    if (!state.target)
    {
        const auto target = Detail::Resolve(Detail::kGetModFolders);
        const auto createDirPath = Detail::Resolve(Detail::kCreateDirPath);
        const auto pushBack = Detail::Resolve(Detail::kPushBack);
        const auto destruct = Detail::Resolve(Detail::kDestruct);
        if (!target || !createDirPath || !pushBack || !destruct)
            return false;

        state.createDirPath = reinterpret_cast<Detail::CreateDirPath_t>(createDirPath);
        state.pushBack = reinterpret_cast<Detail::PushBack_t>(pushBack);
        state.destruct = reinterpret_cast<Detail::Destruct_t>(destruct);
        if (!aSdk->hooking->Attach(aHandle, reinterpret_cast<void*>(target),
                                   reinterpret_cast<void*>(&Detail::GetModFolders),
                                   reinterpret_cast<void**>(&state.original)))
            return false;
        state.target = target;
    }

    auto folder = Detail::Normalize(aFolder);
    for (const auto& existing : state.folders)
    {
        if (Detail::SameFolder(existing, folder))
            return true;
    }
    state.folders.push_back(std::move(folder));
    return true;
}

// Removes this plugin's hook. Call from EMainReason::Unload.
inline void Unregister(const RED4ext::Sdk* aSdk, RED4ext::PluginHandle aHandle)
{
    auto& state = Detail::GetState();
    if (state.target)
        aSdk->hooking->Detach(aHandle, reinterpret_cast<void*>(state.target));
    state = {};
}
} // namespace GameModFolders
