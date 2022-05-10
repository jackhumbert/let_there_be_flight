#include "FlightHelper.hpp"
#include <RED4ext/Common.hpp>
#include "stdafx.hpp"
#include "FlightController.hpp"
#include "FlightEvents.hpp"

namespace vehicle {
namespace flight {

//// 48 89 5C 24 08 57 48 83  EC 30 48 8B D9 48 8B FA  48 8B 0D 81 86 60 02 4C 8B C2 48 83 E1 F8 48 8D
//constexpr uintptr_t VehiclePhysicsPoolThing = 0x14136BFE0 - RED4ext::Addresses::ImageBase;
//
//// 48 8D 05 B1 2E 70 01 48 89 01 48 8B C1 C3
//constexpr uintptr_t InitializeDriveHelper = 0x141D2FEE0 - RED4ext::Addresses::ImageBase;

// 48 89 5C 24 08 48 89 6C  24 10 48 89 74 24 18 48
// 89 7C 24 20 41 54 41 56  41 57 48 83 EC 30 48 8B
// F1 48 8B EA 8B 49 0C 48  8B FA 45 33 E4 4C 8B 06
// 8B 56 08 8D 59 01 49 2B  E8 44 8D 3C CD 00 00 00
// 00 4C 8B F5 49 C1 FE 03  3B DA 76 3D 41 B8 01 00
// 00 00 8B C2 D1 E8 41 3B  D0 41 0F 46 C0 03 D0 3B
// DA 77 EF 85 C9 48 8D 05  B4 84 A2 FE 41 B9 08 00
// 00 00 48 8B CE 49 0F 44  C4 45 8B C1 48 89 44 24
// 20 E8 3A 12 49 FE 4C 8B  06 89 5E 0C 49 3B EF 73
// 0A 4B 8B 04 F0 4F 89 24  F0 EB 06 48 8B 07 4C 89
// 27 49 89 44 D8 F8 48 8B  5C 24 50 48 8B 6C 24 58
// 48 8B 74 24 60 48 8B 7C  24 68 48 83 C4 30 41 5F
// 41 5E 41 5C C3 CC CC CC  CC CC CC CC CC CC CC CC
constexpr uintptr_t AddDriveHelperToArray = 0x141D14080 - RED4ext::Addresses::ImageBase;

//// 0F B6 81 56 01 00 00 2C  06 3C 01 76 0C 48 81 C1  D8 00 00 00 E9 77 89 B8 01 C3 CC CC CC CC CC CC
//constexpr uintptr_t EntityQueueEvent = 0x1410391D0 - RED4ext::Addresses::ImageBase;

// struct FlightHelperCreationStruct {
//  FlightHelper *pointer;
//  uint64_t size;
//};

RED4ext::TTypedClass<Helper> helperCls("vehicleFlightHelperRuntime");

RED4ext::CClass *Helper::GetNativeType() { return &helperCls; }

void Helper::RegisterTypes() {
  //helperCls.flags = {.isNative = true};
  //RED4ext::CRTTISystem::Get()->RegisterType(&helperCls);
}

void Helper::RegisterFunctions() {
  //auto rtti = RED4ext::CRTTISystem::Get();
  //auto redEvent = rtti->GetClass("vehicleDriveHelper");
  //helperCls.parent = redEvent;

  //auto vdtpe = rtti->GetClass("vehicleFlightHelper");
  //vdtpe->props.PushBack(RED4ext::CProperty::Create(rtti->GetType("Vector4"), "force", nullptr, 0x08));
  //vdtpe->props.PushBack(RED4ext::CProperty::Create(rtti->GetType("Vector4"), "torque", nullptr, 0x18));
}

RED4ext::Handle<HelperWrapper> Helper::AddToDriverHelpers(RED4ext::DynArray<uintptr_t> *ra) { //, RED4ext::ScriptInstance fc) {
  // FlightHelperCreationStruct cs;
  // FlightHelperCreationStruct * csp;

  // RED4ext::RelocFunc<FlightHelperCreationStruct *(*)(FlightHelperCreationStruct*, unsigned __int64 a2)> poolThing(
  //    VehiclePhysicsPoolThing);
  // csp = poolThing(&cs, 0x8);
  // memset(csp->pointer, 0, csp->size);

  // RED4ext::RelocFunc<FlightHelper (*)(FlightHelper * a1)> initDriveHelper(InitializeDriveHelper);
  // initDriveHelper(csp->pointer);

  Helper *h = new Helper();
  h->wrapper = new HelperWrapper();

  h->wrapper->force.X = 0.0;
  h->wrapper->force.Y = 0.0;
  h->wrapper->force.Z = 0.0;

  h->wrapper->torque.X = 0.0;
  h->wrapper->torque.Y = 0.0;
  h->wrapper->torque.Z = 0.0;

  auto hwh = RED4ext::Handle<HelperWrapper>(h->wrapper);

  RED4ext::RelocFunc<void *(*)(RED4ext::DynArray<uintptr_t> *, void *)> addToDriveHelper(AddDriveHelperToArray);
  addToDriveHelper(ra, &h);
  spdlog::info("[flightHelper] Added to driveHelpers");

  return hwh;
}

void Helper::PhysicsUpdate(RED4ext::vehicle::BaseObject *vehicle, float timeDelta) {
  // if (vehicle->isOnGround) {
  //}
  // if (timeDelta > 0.0) {
  // spdlog::info("in the physics update! dT: {:01.6f}", deltaTime); // -4 sometimes

  //auto fh = reinterpret_cast<Helper *>(helper);

  vehicle->physicsStruct->force.X += this->wrapper->force.X;
  vehicle->physicsStruct->force.Y += this->wrapper->force.Y;
  vehicle->physicsStruct->force.Z += this->wrapper->force.Z;

  vehicle->physicsStruct->torque.X += this->wrapper->torque.X;
  vehicle->physicsStruct->torque.Y += this->wrapper->torque.Y;
  vehicle->physicsStruct->torque.Z += this->wrapper->torque.Z;

  this->wrapper->force.X = 0.0;
  this->wrapper->force.Y = 0.0;
  this->wrapper->force.Z = 0.0;

  this->wrapper->torque.X = 0.0;
  this->wrapper->torque.Y = 0.0;
  this->wrapper->torque.Z = 0.0;

  //auto rtti = RED4ext::CRTTISystem::Get();
  //auto FlightController = rtti->GetClass("FlightController");
  //auto PhysicsUpdate = FlightController->GetFunction("PhysicsUpdate");
  //// bool valid;
  //RED4ext::ExecuteFunction(FlightController, PhysicsUpdate, nullptr, nullptr);

  // RED4ext::StackArgs_t args;
  // args.emplace_back(nullptr, &timeDelta);
  // void *out;
  // RED4ext::ExecuteFunction(FlightController, PhysicsUpdate, &out, args);

  // auto e = new vehicle::flight::PhysicsUpdateEvent();
  ////e->vehicle = vehicle;
  // e->timeDelta = timeDelta;

  // RED4ext::RelocFunc<void (*)(RED4ext::ent::Entity *, RED4ext::red::Event *)> QueueEvent(EntityQueueEvent);
  // QueueEvent(vehicle, e);

  //}

  // auto stack = new RED4ext::CStack(a1->flightController, &deltaTime, 1, nullptr, 0i64);

  // PhysicsUpdate->Execute(stack);
  // bool valid;
  // RED4ext::ExecuteFunction(a1->flightController, PhysicsUpdate, &valid, &deltaTime);
}

} // namespace flight
} // namespace vehicle