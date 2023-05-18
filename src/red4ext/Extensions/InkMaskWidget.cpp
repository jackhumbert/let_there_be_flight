#include "Utils/FlightModule.hpp"
#include "LoadResRef.hpp"
#include <RED4ext/Common.hpp>
#include <RED4ext/RTTISystem.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/MaskWidget.hpp>
#include <RED4ext/Scripting/Utils.hpp>
#include <RedLib.hpp>

struct InkMaskWidget : RED4ext::ink::MaskWidget {
  bool SetAtlasResource(RED4ext::ResRef value) {
    auto valid = false;
    RED4ext::ExecuteFunction("redResourceReferenceScriptToken", "IsValid", &valid, &value);
    if (valid) {
      this->textureAtlas.path = value.resource.path;
    }
    return valid;
  }
};

RTTI_EXPAND_CLASS(RED4ext::ink::MaskWidget, {
    RTTI_METHOD_FQN(InkMaskWidget::SetAtlasResource);
});