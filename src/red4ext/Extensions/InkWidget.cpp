#include "Utils/FlightModule.hpp"
#include "LoadResRef.hpp"
#include <RED4ext/Common.hpp>
#include <RED4ext/RTTISystem.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/BoxBlurEffect.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/EBlurDimension.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/Widget.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/IEffect.hpp>
#include <RedLib.hpp>

struct InkWidget : RED4ext::ink::Widget {
  void CreateEffect(RED4ext::CName typeName, RED4ext::CName effectName) {
    // could be script class instead
    auto effect = reinterpret_cast<RED4ext::ink::IEffect*>(RED4ext::CRTTISystem::Get()->GetClass(typeName)->CreateInstance(true));
    effect->effectName = effectName;
    this->effects.EmplaceBack(RED4ext::Handle<RED4ext::ink::IEffect>(effect));
  }

  bool SetBlurDimension(RED4ext::CName effectName, RED4ext::ink::EBlurDimension blurDimension) {
    bool found = false;
    for (auto& effect : this->effects) {
#ifdef __APPLE__
      // LTBF R41: ink::IEffect::GetEffectType is vtable slot 0xE0 on macOS (0xD8 on Windows, R40) and the macOS SDK declares no
      // virtuals for it; the class test below is the same check (only inkBoxBlurEffect returns BoxBlur). The Windows line also
      // casts the Handle's own address to the effect, which writes into the array; macOS uses the instance.
      static auto *s_boxBlur = RED4ext::CRTTISystem::Get()->GetClass("inkBoxBlurEffect");
      if (effect && effect->GetType() == s_boxBlur && effect->effectName == effectName) {
        auto blurEffect = reinterpret_cast<RED4ext::ink::BoxBlurEffect*>(effect.GetPtr());
#else
      if (effect->GetEffectType() == RED4ext::ink::EffectType::BoxBlur && effect->effectName == effectName) {
        auto blurEffect = reinterpret_cast<RED4ext::ink::BoxBlurEffect*>(&effect);
#endif
        blurEffect->blurDimension = blurDimension;
        found = true;
      }
    }
    return found;
  }
};

RTTI_EXPAND_CLASS(RED4ext::ink::Widget, {
    RTTI_METHOD_FQN(InkWidget::CreateEffect);
    RTTI_METHOD_FQN(InkWidget::SetBlurDimension);
});