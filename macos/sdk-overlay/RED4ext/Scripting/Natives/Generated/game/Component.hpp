#pragma once

// clang-format off

// LTBF R41 (macOS): the jackhumbert fork's Generated/game/Component.hpp (deps/red4ext.sdk) on top of the macOS SDK. It replaces
// the macOS SDK's data-only gameComponent for this plugin (FlightComponent overrides its virtuals). gameComponent is
// IComponent + PSInterface: PSInterface's vptr at 0x90, persistentState at 0x98, size 0xA8 on both platforms. The base
// implementations that call the game ("VFT hash + Windows slot") resolve on macOS through ltbf_mac_vft_slot (macos/MacShims.cpp).
// Changes from the fork: IGameInstance and Object are forward declared; sub_238 reads the scripted-class flag the same way.

#include <cstdint>
#include <RED4ext/Common.hpp>
#include <RED4ext/Handle.hpp>
#include <RED4ext/RTTITypes.hpp>
#include <RED4ext/Relocation.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/IComponent.hpp>
#include <RED4ext/Scripting/Natives/gamePSInterface.hpp>

namespace RED4ext
{
struct IGameInstance;
namespace game
{
struct PersistentState;
struct Object;

struct Component : ent::IComponent, PSInterface
{
    static constexpr const char* NAME = "gameComponent";
    static constexpr const char* ALIAS = "GameComponent";
    static constexpr const uint32_t VFT = 331746781;

// overridden virtuals

    inline virtual Memory::IAllocator* GetAllocator() override {
        static UniRelocFunc<decltype(&Component::GetAllocator)> call(VFT, 0x10);
        return call(this);
    }

#ifdef __APPLE__
    // LTBF R42f (macOS): by-value return through x8, see entIComponent.hpp.
    inline virtual Handle<game::PersistentState> sub_150() override {
        static UniRelocFunc<decltype(&Component::sub_150)> call(VFT, 0x150);
        return call(this);
    }
#else
    inline virtual Handle<game::PersistentState>* sub_150(Handle<game::PersistentState>* a1) override {
        static UniRelocFunc<decltype(&Component::sub_150)> call(VFT, 0x150);
        return call(this, a1);
    }
#endif

    inline virtual void OnAttach(void* a1) override {
        static UniRelocFunc<decltype(&Component::OnAttach)> call(VFT, 0x180);
        return call(this, a1);
    }

    inline virtual void OnDetach(void* a1) override {
        static UniRelocFunc<decltype(&Component::OnDetach)> call(VFT, 0x190);
        return call(this, a1);
    }

    // checks to make sure entity is gameObject
    inline virtual void sub_218(void* a1) override {
        static UniRelocFunc<decltype(&Component::sub_218)> call(VFT, 0x218);
        return call(this, a1);
    }

    // new virtuals

    // sets the persisent state
    inline virtual void sub_230(Handle<game::PersistentState>* a1) {
        static UniRelocFunc<decltype(&Component::sub_230)> call(VFT, 0x230);
        return call(this, a1);
    }

    // is Scripted Class / has scripted callbacks?
    inline virtual bool sub_238(Handle<Object>*) {
        return this->GetType()->flags.isScriptedClass;
    }

    inline virtual void OnGameAttach(IGameInstance* a1) { } // 240
    inline virtual void OnGameDetach(IGameInstance* a1) { } // 248
    inline virtual void OnEditorAttach() { }                // 250
    inline virtual void OnEditorDetach() { }                // 258
};
static_assert(sizeof(Component) == 0xA8, "gameComponent size");
} // namespace game
using gameComponent = game::Component;
using GameComponent = game::Component;
} // namespace RED4ext

// clang-format on
