#include "FlightModule.hpp"
#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/Entity.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/EntityTemplate.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/IComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/IPlacedComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/MeshComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/ITransformBinding.hpp>
#include <RED4ext/Scripting/Natives/Generated/WorldWidgetComponent.hpp>
#include <RED4ext/Scripting/Natives/Generated/ent/HardTransformBinding.hpp>
#include <RED4ext/Scripting/Natives/Generated/world/ui/MeshTargetBinding.hpp>
#include "LoadResRef.hpp"

void EntityAddComponent(RED4ext::IScriptable *aContext, RED4ext::CStackFrame *aFrame, bool *aOut, int64_t a4) {
  auto ent = reinterpret_cast<RED4ext::ent::Entity *>(aContext);

  RED4ext::Handle<RED4ext::ent::IComponent> value;
  RED4ext::GetParameter(aFrame, &value);
  aFrame->code++; // skip ParamEnd
  // auto handle = RED4ext::Handle<RED4ext::ent::IComponent>(value);
  // auto handle = *(RED4ext::Handle<RED4ext::ent::IComponent> *)&value;
  // ent->components.EmplaceBack(handle);
  ent->components.EmplaceBack(value);

  if (aOut) {
    *aOut = true;
  }
}

void EntityAddWorldWidgetComponent(RED4ext::IScriptable *aContext, RED4ext::CStackFrame *aFrame, bool *aOut, int64_t a4) {
  auto ent = reinterpret_cast<RED4ext::ent::Entity *>(aContext);

  aFrame->code++; // skip ParamEnd

  auto rtti = RED4ext::CRTTISystem::Get();


  // MeshComponent
  auto mc = (RED4ext::ent::MeshComponent*)rtti->GetClass("entMeshComponent")->AllocInstance();
  
  RED4ext::CName mesh =
      "base\\environment\\decoration\\advertising\\holograms\\common\\common_holograms_transparent_a_w500_h150.mesh";
  mc->mesh.ref = mesh;
  mc->name = "radio_screen";
  mc->meshAppearance = "screen_ui";
  mc->localTransform.Position.x.Bits = 216270;
  mc->visualScale.X = 10.0;
  mc->visualScale.X = 10.0;
  mc->visualScale.X = 10.0;

  auto mcHandle = RED4ext::Handle<RED4ext::ent::MeshComponent>(mc);
  ent->components.EmplaceBack(mcHandle);

  // WorldWidgetComponent
  auto wwc = (RED4ext::WorldWidgetComponent *)rtti->GetClass("WorldWidgetComponent")->AllocInstance();

  wwc->name = "radio_ui";

  RED4ext::CName fc = "base\\gameplay\\gui\\world\\radio\\radio_ui.inkwidget";
  wwc->widgetResource.ref = fc;

  auto mtb = (RED4ext::world::ui::MeshTargetBinding *)rtti->GetClass("worlduiMeshTargetBinding")->AllocInstance();
  mtb->bindName = "radio_screen";
  wwc->meshTargetBinding = RED4ext::Handle<RED4ext::world::ui::MeshTargetBinding>(mtb);

  auto wwcHandle = RED4ext::Handle<RED4ext::WorldWidgetComponent>(wwc);
  ent->components.EmplaceBack(wwcHandle);

  if (aOut) {
    *aOut = true;
  }
}

//base\vehicles\common\gameplay\flight_components.ent

//void EntityAddFlightComponent(RED4ext::IScriptable *aContext, RED4ext::CStackFrame *aFrame, bool *aOut, int64_t a4) {
//  auto ent = reinterpret_cast<RED4ext::ent::Entity *>(aContext);
//
//
//  RED4ext::CName *name;
//  RED4ext::GetParameter(aFrame, &name);
//  aFrame->code++; // skip ParamEnd
//
//  if (name != 0) {
//    auto resHandle = new RED4ext::ResourceHandle<RED4ext::ent::EntityTemplate>();
//    RED4ext::CName fc = "base\\vehicles\\common\\gameplay\\flight_components.ent";
//    LoadResRef<RED4ext::ent::EntityTemplate>((uint64_t *)&fc, resHandle, true);
//
//    auto et = resHandle->wrapper->resource.GetPtr();
//    et->sub_28(true);
//
//    auto handle = RED4ext::Handle<RED4ext::ent::IComponent>(value);
//    ent->components.EmplaceBack(handle);
//
//    if (aOut) {
//      *aOut = true;
//    }
//  }
//}

void IPlacedComponentUpdateHardTransformBinding(RED4ext::IScriptable *aContext, RED4ext::CStackFrame *aFrame, bool *aOut, int64_t a4) {
  auto ipc = reinterpret_cast<RED4ext::ent::IPlacedComponent *>(aContext);

  RED4ext::CName bindName;
  RED4ext::CName slotName;
  RED4ext::GetParameter(aFrame, &bindName);
  RED4ext::GetParameter(aFrame, &slotName);
  aFrame->code++; // skip ParamEnd

  auto rtti = RED4ext::CRTTISystem::Get();
  auto transform = ipc->parentTransform.GetPtr();
  if (transform->GetNativeType() == rtti->GetClass("entHardTransformBinding")) {
    auto ht = (RED4ext::ent::HardTransformBinding *)(transform);
    ht->bindName = bindName;
    ht->slotName = slotName;
    if (aOut) {
      *aOut = true;
    }
  } else {
    if (aOut) {
      *aOut = false;
    }
  }

}

struct EntityAddComponentModule : FlightModule {
  void PostRegisterTypes() {
    auto rtti = RED4ext::CRTTISystem::Get();
    auto ent = rtti->GetClass("entEntity");
    ent->RegisterFunction(
        RED4ext::CClassFunction::Create(ent, "AddComponent", "AddComponent", &EntityAddComponent, {.isNative = true}));
    ent->RegisterFunction(
        RED4ext::CClassFunction::Create(ent, "AddWorldWidgetComponent", "AddWorldWidgetComponent", &EntityAddWorldWidgetComponent, {.isNative = true}));

    auto ipc = rtti->GetClass("entIPlacedComponent");
    ipc->RegisterFunction(RED4ext::CClassFunction::Create(ipc, "UpdateHardTransformBinding", "UpdateHardTransformBinding",
                                                          &IPlacedComponentUpdateHardTransformBinding, {.isNative = true}));
  }
};

REGISTER_FLIGHT_MODULE(EntityAddComponentModule);