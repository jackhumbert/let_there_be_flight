#include "FlightController.hpp"
#include "FlightModule.hpp"

#include "Utils.hpp"
#include "stdafx.hpp"

namespace FlightController {

RED4ext::TTypedClass<FlightController> cls("FlightController");

RED4ext::CClass *FlightController::GetNativeType() { return &cls; }

RED4ext::Handle<FlightController> handle;

FlightController* FlightController::GetInstance() { 
  if (!handle.instance) {
    spdlog::info("[RED4ext] New FlightController Instance");
    auto instance = reinterpret_cast<FlightController *>(cls.AllocInstance());
    handle = RED4ext::Handle<FlightController>(instance);
  }
  
  return (FlightController*)handle.instance;
}

void GetInstanceScripts(RED4ext::IScriptable *aContext, RED4ext::CStackFrame *aFrame, RED4ext::Handle<FlightController>* aOut, int64_t a4) {
  aFrame->code++;

  if (!handle.instance) {
    spdlog::info("[RED4ext] New FlightController Instance");
    auto instance = reinterpret_cast<FlightController *>(cls.AllocInstance());
    handle = RED4ext::Handle<FlightController>(instance);
    //handle.instance = (FlightController *)cls.AllocInstance();
    //handle.refCount = new RED4ext::RefCnt();
  }

  if (aOut) {
    //handle.refCount->IncWeakRef();
    //aOut->instance = handle.instance;
    //aOut->refCount = handle.refCount;
    handle.refCount->IncRef();
    *aOut = RED4ext::Handle<FlightController>(handle);
  }
}


struct Module : FlightModule {
  void RegisterTypes() {
    cls.flags = {.isNative = true};
    RED4ext::CRTTISystem::Get()->RegisterType(&cls);
  }

  // void PhysicsUpdate(float timeDelta) {

  //}

  void PostRegisterTypes() {
    auto rtti = RED4ext::CRTTISystem::Get();
    auto scriptable = rtti->GetClass("IScriptable");
    cls.parent = scriptable;

    auto getInstance = RED4ext::CClassStaticFunction::Create(&cls, "GetInstance", "GetInstance",
                                                       &GetInstanceScripts, {.isNative = true, .isStatic = true});
    cls.RegisterFunction(getInstance);

    cls.props.PushBack(RED4ext::CProperty::Create(rtti->GetType("Bool"), "enabled", nullptr, 0x40));
    cls.props.PushBack(RED4ext::CProperty::Create(rtti->GetType("Bool"), "active", nullptr, 0x41));


    // auto physicsUpdate =
    // RED4ext::CClassStaticFunction(cls, "PhysicsUpdate", "PhysicsUpdate", &PhysicsUpdate, {});
    // auto PhysicsUpdate =
    // RED4ext::CClassFunction::Create(cls, "PhysicsUpdate", "PhysicsUpdate", NULL, {});
    // cls.RegisterFunction(physicsUpdate);
  }
};

REGISTER_FLIGHT_MODULE(Module);

} // namespace FlightController
