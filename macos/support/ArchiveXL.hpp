#pragma once
// LTBF R41 (macOS): copy of the macOS ArchiveXL fork's support/red4ext/ArchiveXL.hpp with the module path through dladdr
// (the fork itself is not edited). The registration is the same RTTI call into the ArchiveXL facade class.

#include <cstdio>
#include <filesystem>
#include <vector>

#include <dlfcn.h>
#include <RED4ext/Api/PluginHandle.hpp>

#include <RED4ext/Callback.hpp>
#include <RED4ext/Memory/Allocators.hpp>
#include <RED4ext/RTTISystem.hpp>

class ArchiveXL
{
public:
    inline static bool RegisterArchive(std::filesystem::path aPath)
    {
        std::error_code error;

        if (!std::filesystem::exists(aPath, error) || !std::filesystem::is_regular_file(aPath, error))
        {
            return false;
        }

        return RegisterPathOrQueue(std::move(aPath));
    }

    inline static bool RegisterArchive(HMODULE aHandle, std::filesystem::path aPath)
    {
        if (aPath.is_relative())
        {
            aPath = GetModulePath(aHandle) / aPath;
        }

        return RegisterArchive(std::move(aPath));
    }

    inline static bool RegisterArchives(std::filesystem::path aPath)
    {
        std::error_code error;

        if (!std::filesystem::exists(aPath, error) || !std::filesystem::is_directory(aPath, error))
        {
            return false;
        }

        return RegisterPathOrQueue(std::move(aPath));
    }

    inline static bool RegisterArchives(HMODULE aHandle, std::filesystem::path aPath)
    {
        if (aPath.is_relative())
        {
            aPath = GetModulePath(aHandle) / aPath;
        }

        return RegisterArchives(std::move(aPath));
    }

private:
    ArchiveXL() = default;

    inline static bool Initialize()
    {
        if (s_facade)
            return true;

        // macOS: LTBF's Memory::Vault is a stand-in with the RTTI and script pools only (MacVault.cpp), so the engine pool the
        // Windows test looks for is never in it. ltbf_registerNatives calls this after the game's RTTI system exists and the
        // ArchiveXL natives are registered, so the RTTI system is the readiness test here (same change as Mod Settings' overlay).
        if (!RED4ext::CRTTISystem::Get())
            return false;

        auto facade = RED4ext::CRTTISystem::Get()->GetClass("ArchiveXL");

        if (!facade || !facade->GetFunction("Version"))
            return false;

        s_facade = facade;
        return true;
    }

    inline static bool RegisterPathOrQueue(std::filesystem::path aPath)
    {
        if (!Initialize())
        {
            s_paths.push_back(std::move(aPath));

            if (s_paths.size() == 1)
            {
                RegisterPendingPaths();
            }

            return true;
        }

        return RegisterPath(aPath);
    }

    inline static bool RegisterPath(const std::filesystem::path& aPath)
    {
        bool success;
        RED4ext::CString path(aPath.string());

        auto rtti = RED4ext::CRTTISystem::Get();
        RED4ext::CStackType arg(rtti->GetType("String"), &path);
        RED4ext::CStackType result(rtti->GetType("Bool"), &success);
        RED4ext::CStack stack(nullptr, &arg, 1, &result);

        std::error_code error;
        RED4ext::CName method(std::filesystem::is_directory(aPath, error) ? "RegisterDir" : "RegisterArchive");

        s_facade->GetFunction(method)->Execute(&stack);

        return success;
    }

    static inline void RegisterPendingPaths()
    {
        if (Initialize())
        {
            for (const auto& path : s_paths)
            {
                RegisterPath(path);
            }

            s_paths.clear();
        }
        else
        {
            // macOS: the game never runs post-register callbacks, so a queued path would never be registered; say so.
            std::fprintf(stderr, "[ltbf] ArchiveXL is not ready (class ArchiveXL with Version not registered): %zu path(s) stay queued\n",
                         s_paths.size());
            RED4ext::CRTTISystem::Get()->AddPostRegisterCallback(&RegisterPendingPaths);
        }
    }

    inline static std::filesystem::path GetModulePath(HMODULE)
    {
        // macOS: the plugin calling this (these headers are inline in it), found with dladdr.
        Dl_info info{};
        if (dladdr(reinterpret_cast<const void*>(&GetModulePath), &info) && info.dli_fname)
        {
            return std::filesystem::path(info.dli_fname).parent_path();
        }
        return {};
    }

    inline static RED4ext::CClass* s_facade;
    inline static std::vector<std::filesystem::path> s_paths;
};
