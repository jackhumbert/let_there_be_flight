#pragma once

// LTBF R42d (macOS): copied unchanged from cp2077-codeware/macos/inc (Codeware R16b); macos/sdk-overlay comes before the SDK on
// the include path. Without it FlightSystem::IsSavingLocked wrote through a null aLock when the game collected the save locks
// (Boot B4: SIGSEGV at 0x0 in let_there_be_flight.dylib, called from the game's walker 0x3314DDC).
// R16b. Codeware macOS copy of the SDK header (macos/inc comes first on the include path). Differences from
// the vendored RED4ext.SDK header, all under __APPLE__ and marked R16b:
//   IsSavingLocked: the Windows SDK models the returned SaveLock as a hidden first argument
//   (void* IsSavingLocked(SaveLock* aLock, bool a2)). On macOS arm64 the game returns it by value through the
//   indirect-result register x8 and passes the bool in x1, so the SDK signature made Codeware's systems write
//   through a null aLock when the game collected the save locks (Phase 6 crash after fast travel).
//   SDK PR candidate: a platform-neutral signature, SaveLock IsSavingLocked(bool a2).

#include <RED4ext/Common.hpp>
#include <RED4ext/JobQueue.hpp>
#include <RED4ext/Scripting/Natives/IUpdatableSystem.hpp>

namespace RED4ext
{
struct GameInstance;

namespace world
{
struct RuntimeScene;
}

namespace game
{
struct SaveLock;

struct IGameSystem : IUpdatableSystem
{
    static constexpr const char* NAME = "gameIGameSystem";
    static constexpr const char* ALIAS = "IGameSystem";

    virtual void OnWorldAttached(world::RuntimeScene* aScene);                                                // 110
    virtual void OnBeforeWorldDetach(world::RuntimeScene* aScene);                                            // 118
    virtual void OnWorldDetached(world::RuntimeScene* aScene);                                                // 120
    virtual void OnAfterWorldDetach();                                                                        // 128
    virtual uint32_t OnBeforeGameSave(const JobGroup& aJobGroup, void* a2);                                   // 130
    virtual void OnGameSave(void* aStream);                                                                   // 138
    virtual void OnAfterGameSave();                                                                           // 140
    virtual void OnGameLoad(const JobGroup& aJobGroup, bool& aSuccess, void* aStream);                        // 148
    virtual bool OnGameRestored();                                                                            // 150
    virtual void OnGamePrepared();                                                                            // 158
    virtual void OnGamePaused();                                                                              // 160
    virtual void OnGameResumed();                                                                             // 168
#if defined(__APPLE__)
    virtual SaveLock IsSavingLocked(bool a2);                                                                 // 170 (Mac 0x178), R16b
#else
    virtual void* IsSavingLocked(SaveLock* aLock, bool a2);                                                   // 170
#endif
    virtual void OnStreamingWorldLoaded(world::RuntimeScene* aScene, uint64_t a2, const JobGroup& aJobGroup); // 178
    virtual void sub_180();                                                                                   // 180
    virtual void sub_188();                                                                                   // 188
    virtual void sub_190(void* a1);                                                                           // 190
    virtual void sub_198();                                                                                   // 198
    virtual void OnInitialize(const JobHandle& aJob);                                                         // 1A0
    virtual void OnUninitialize();                                                                            // 1A8

    GameInstance* gameInstance; // 40
};
RED4EXT_ASSERT_SIZE(IGameSystem, 0x48);
} // namespace game
} // namespace RED4ext

#ifdef RED4EXT_HEADER_ONLY
#include <RED4ext/Scripting/Natives/gameIGameSystem-inl.hpp>
#endif
