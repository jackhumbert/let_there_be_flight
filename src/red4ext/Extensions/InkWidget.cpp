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
    auto effect = reinterpret_cast<RED4ext::ink::IEffect*>(RED4ext::CRTTISystem::Get()->GetClass(typeName)->CreateInstance());
    effect->effectName = effectName;
    this->effects.EmplaceBack(RED4ext::Handle<RED4ext::ink::IEffect>(effect));
  }

  bool SetBlurDimension(RED4ext::CName effectName, RED4ext::ink::EBlurDimension blurDimension) {
    bool found = false;
    for (auto& effect : this->effects) {
      if (effect->GetEffectType() == RED4ext::ink::EffectType::BoxBlur && effect->effectName == effectName) {
        auto blurEffect = reinterpret_cast<RED4ext::ink::BoxBlurEffect*>(&effect);
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