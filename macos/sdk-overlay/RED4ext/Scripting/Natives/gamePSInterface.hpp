#pragma once

// LTBF R41 (macOS): the jackhumbert fork's gamePSInterface.hpp (deps/red4ext.sdk). gameComponent's second base: on macOS its vptr
// sits at 0x90 (IComponent's data size 0x8D, rounded to the pointer alignment), persistentState at 0x98, as on Windows.

#include <cstdint>
#include <RED4ext/Common.hpp>
#include <RED4ext/Handle.hpp>

namespace RED4ext
{
struct CClass;
namespace game
{
struct PersistentState;
struct PSInterface
{
    virtual ~PSInterface() = default;      // 00 (macOS: 00 and 08)
    virtual CClass* GetPSClass() = 0;      // 08 (macOS: 10)

    Handle<PersistentState> persistentState;
};
} // namespace game
} // namespace RED4ext
