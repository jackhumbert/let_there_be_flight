#include "FlightModule.hpp"
#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Scripting/Natives/Generated/game/FxResource.hpp>
#include <RED4ext/Scripting/Natives/Generated/red/ResourceReferenceScriptToken.hpp>

void CastResRefToFxResource(RED4ext::IScriptable *aContext, RED4ext::CStackFrame *aFrame,
                            RED4ext::game::FxResource *aOut, int64_t a4) {
  RED4ext::red::ResourceReferenceScriptToken value;
  RED4ext::GetParameter(aFrame, &value);
  aFrame->code++; // skip ParamEnd

  if (aOut) {
    *aOut = *(RED4ext::game::FxResource*)&value.resource;
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