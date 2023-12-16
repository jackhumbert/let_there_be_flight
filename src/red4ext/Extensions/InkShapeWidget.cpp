#include "Utils/FlightModule.hpp"
#include "LoadResRef.hpp"
#include <RED4ext/Common.hpp>
#include <RED4ext/RTTISystem.hpp>
#include <RED4ext/Scripting/Natives/Generated/red/ResourceReferenceScriptToken.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/ShapeWidget.hpp>
#include <RedLib.hpp>

struct InkShapeWidget : RED4ext::ink::ShapeWidget {
  void SetShapeResource(RED4ext::ResRef& value) {
    this->shapeResource.path = value.resource.path;
  }
};

RTTI_EXPAND_CLASS(RED4ext::ink::ShapeWidget, {
    RTTI_METHOD_FQN(InkShapeWidget::SetShapeResource);
});