#include "Utils/FlightModule.hpp"
#include "LoadResRef.hpp"
#include <RED4ext/Common.hpp>
#include <RED4ext/RTTISystem.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/MaskWidget.hpp>
#include <RED4ext/Scripting/Natives/Generated/red/ResourceReferenceScriptToken.hpp>
#include <RED4ext/Scripting/Utils.hpp>
#include <RedLib.hpp>

struct InkMaskWidget : RED4ext::ink::MaskWidget {
  bool SetAtlasResource(RED4ext::ResRef value) {
    auto valid = false;
#ifdef __APPLE__
    // LTBF R41: the SDK's ExecuteFunction(class name, ...) asks CGameEngine for a game-system context, and CGameEngine is not mapped
    // on macOS. IsValid is a static function, so it runs without an instance; the argument is the ResRef itself (the variadic
    // overload would pass the address of the pointer).
    static auto *s_isValid = []() -> RED4ext::CBaseFunction * {
      auto cls = RED4ext::CRTTISystem::Get()->GetClass("redResourceReferenceScriptToken");
      return cls ? cls->GetFunction("IsValid") : nullptr;
    }();
    if (s_isValid) {
      RED4ext::StackArgs_t args;
      args.emplace_back(nullptr, &value);
      RED4ext::ExecuteFunction(RED4ext::ScriptInstance(nullptr), s_isValid, &valid, args);
    }
#else
    RED4ext::ExecuteFunction("redResourceReferenceScriptToken", "IsValid", &valid, &value);
#endif
    if (valid) {
      this->textureAtlas.path = value.resource.path;
    }
    return valid;
  }
};

RTTI_EXPAND_CLASS(RED4ext::ink::MaskWidget, {
    RTTI_METHOD_FQN(InkMaskWidget::SetAtlasResource);
});