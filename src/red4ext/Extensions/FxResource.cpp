#include "Utils/FlightModule.hpp"
#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/FxResource.hpp>
#include <RED4ext/Scripting/Natives/Generated/red/ResourceReferenceScriptToken.hpp>
#include "LoadResRef.hpp"

void CastResRefToFxResource(RED4ext::IScriptable *aContext, RED4ext::CStackFrame *aFrame,
                            RED4ext::game::FxResource *aOut, int64_t a4) {
  RED4ext::red::ResourceReferenceScriptToken value;
  RED4ext::GetParameter(aFrame, &value);
  aFrame->code++; // skip ParamEnd

  //auto resHandle = new RED4ext::ResourceHandle<RED4ext::world::Effect>();
  //if (value.resource.path.hash != 0) {
    //RED4ext::CName fc = value.resource.path.hash;
    //value.resource.Resolve();
    //LoadResRef<RED4ext::world::Effect>((uint64_t *)&fc, resHandle, true);
  //}

  if (aOut) {
    aOut->effect.path = value.resource.path;
    //aOut->effect.Resolve();
  }
}

struct FxResourceModule : FlightModule {
  void PostRegisterTypes() {
    auto rtti = RED4ext::CRTTISystem::Get();
    auto f = RED4ext::CGlobalFunction::Create("Cast;ResRef;FxResource", "Cast;ResRef;FxResource", &CastResRefToFxResource);
    rtti->RegisterFunction(f);
  }
};

REGISTER_FLIGHT_MODULE(FxResourceModule);