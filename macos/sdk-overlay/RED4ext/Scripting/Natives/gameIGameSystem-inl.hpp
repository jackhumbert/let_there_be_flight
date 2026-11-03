#pragma once

#ifdef RED4EXT_STATIC_LIB
#include <RED4ext/Scripting/Natives/gameIGameSystem.hpp>
#endif

#include <RED4ext/Scripting/Natives/Generated/game/SaveLock.hpp>
#include <RED4ext/Scripting/Natives/Generated/world/RuntimeScene.hpp>

RED4EXT_INLINE void RED4ext::game::IGameSystem::OnWorldAttached(RED4ext::world::RuntimeScene* aScene)
{
    RED4EXT_UNUSED_PARAMETER(aScene);
}

RED4EXT_INLINE void RED4ext::game::IGameSystem::OnBeforeWorldDetach(RED4ext::world::RuntimeScene* aScene)
{
    RED4EXT_UNUSED_PARAMETER(aScene);
}

RED4EXT_INLINE void RED4ext::game::IGameSystem::OnWorldDetached(RED4ext::world::RuntimeScene* aScene)
{
    RED4EXT_UNUSED_PARAMETER(aScene);
}

RED4EXT_INLINE void RED4ext::game::IGameSystem::OnAfterWorldDetach()
{
}

RED4EXT_INLINE uint32_t RED4ext::game::IGameSystem::OnBeforeGameSave(const RED4ext::JobGroup& aJobGroup, void* a2)
{
    RED4EXT_UNUSED_PARAMETER(aJobGroup);
    RED4EXT_UNUSED_PARAMETER(a2);

    return 0;
}

RED4EXT_INLINE void RED4ext::game::IGameSystem::OnGameSave(void* aStream)
{
    RED4EXT_UNUSED_PARAMETER(aStream);
}

RED4EXT_INLINE void RED4ext::game::IGameSystem::OnAfterGameSave()
{
}

RED4EXT_INLINE void RED4ext::game::IGameSystem::OnGameLoad(const RED4ext::JobGroup& aJobGroup, bool& aSuccess,
                                                           void* aStream)
{
    RED4EXT_UNUSED_PARAMETER(aJobGroup);
    RED4EXT_UNUSED_PARAMETER(aSuccess);
    RED4EXT_UNUSED_PARAMETER(aStream);
}

RED4EXT_INLINE bool RED4ext::game::IGameSystem::OnGameRestored()
{
    return true;
}

RED4EXT_INLINE void RED4ext::game::IGameSystem::OnGamePrepared()
{
}

RED4EXT_INLINE void RED4ext::game::IGameSystem::OnGamePaused()
{
}

RED4EXT_INLINE void RED4ext::game::IGameSystem::OnGameResumed()
{
}

#if defined(__APPLE__)
// R16b. Returns an empty SaveLock (reason Nothing, empty description) by value, in the caller's x8 buffer.
RED4EXT_INLINE RED4ext::game::SaveLock RED4ext::game::IGameSystem::IsSavingLocked(bool a2)
{
    RED4EXT_UNUSED_PARAMETER(a2);

    return RED4ext::game::SaveLock{};
}
#else
RED4EXT_INLINE void* RED4ext::game::IGameSystem::IsSavingLocked(RED4ext::game::SaveLock* aLock, bool a2)
{
    RED4EXT_UNUSED_PARAMETER(a2);

    *aLock = {};
    return aLock;
}
#endif

RED4EXT_INLINE void RED4ext::game::IGameSystem::OnStreamingWorldLoaded(RED4ext::world::RuntimeScene* aScene,
                                                                       uint64_t a2, const RED4ext::JobGroup& aJobGroup)
{
    RED4EXT_UNUSED_PARAMETER(aScene);
    RED4EXT_UNUSED_PARAMETER(a2);
    RED4EXT_UNUSED_PARAMETER(aJobGroup);
}

RED4EXT_INLINE void RED4ext::game::IGameSystem::sub_180()
{
}

RED4EXT_INLINE void RED4ext::game::IGameSystem::sub_188()
{
}

RED4EXT_INLINE void RED4ext::game::IGameSystem::sub_190(void* a1)
{
    RED4EXT_UNUSED_PARAMETER(a1);
}

RED4EXT_INLINE void RED4ext::game::IGameSystem::sub_198()
{
}

RED4EXT_INLINE void RED4ext::game::IGameSystem::OnInitialize(const RED4ext::JobHandle& aJob)
{
    RED4EXT_UNUSED_PARAMETER(aJob);
}

RED4EXT_INLINE void RED4ext::game::IGameSystem::OnUninitialize()
{
}
