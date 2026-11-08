#pragma once

// The Autopilot as a C++ flight mode (docs/design/native-ports.md, M6 step 6; docs/design/autopilot.md): a native
// FlightMode around the pure Navigation::Autopilot, under the class name the script mode had, so every reader keeps
// working (FlightController, FlightSummonSystem, AutoDriveController, VehicleFlight, the HUD, Audio/Inputs/Gather,
// cp2077_tools' ltbf_bridge and ltbf_draw).
//
// Each frame (FlightMode's NativeUpdate) it gathers what Step reads, as tools/flightsim/Trip.cpp does offline: the
// component's stats (FlightStats::Of), its input fields as FlightComponent wrote them this frame (before anything here
// writes them), isPlayerMounted, collisionTimer, thrusterTensor, the world inertia (FlightComponent's
// GetWorldInertiaRow), FlightSettings' values, the standard factors (the code's own, Flight::StandardParams) and the
// tracked destination (the redscript TrackedDestination). It writes the telemetry row, calls Step, writes force and torque, the component's input
// fields the output asks for (the thrusters' look, the engine sound, the force tail's damping), and turns the
// output's events into the redscript glue below. A frame the player has taken over is flown with
// Flight::AttitudeLift and the component's pitch and roll PIDs, as the script's UpdateWithNormalLift did.
//
// Redscript glue the C++ calls by name on this object (FlightModeAutopilot.reds; found once, warned once if missing):
// TrackedDestination, AutodriveEngaged (facts and vanilla's autodrive panel), PublishProgress, PublishFreeRoamUI,
// ShowNotification, ExitAutopilot, RequestFlightOff.
//
// Fields script reads keep the script's names and types. The tuning (AutopilotParams, every field by its name) and the
// state's numbers and flags are RTTI properties straight over the C++ members (DescribeFields), so a write from script
// (LtbfBridge) reaches the next frame. The Vector4 and String fields are copies: written after each call into the
// Autopilot, and the three a summon writes (externalDestination, padLandingOffset, landingKeepClear) read back before
// the next one when script changed them.

#include "Flight/Mode.hpp"
#include "Navigation/Autopilot.hpp"
#include "Navigation/FlightVehicleModel.hpp"
#include "Navigation/FlightWorld.hpp"

#include <RED4ext/Scripting/Natives/Generated/Vector4.hpp>
#include <RedLib.hpp>

#include <cstdint>

struct FlightModeAutopilot : FlightMode {
  FlightModeAutopilot();

  void OnInitializeNative() override;
  bool IsEnabledNative() override { return enabled; }
  void OnActivateNative() override;
  void OnDeactivateNative() override;
  void OnTeleportedNative() override;
  void OnUpdateNative(float aTimeDelta) override;
  RED4ext::CString DescriptionNative() override { return "Autopilot"; }

  // --- for script ---
  // the route's points (0 when there is none), a point and its distance along the route (zeros out of range)
  int32_t RouteSize();
  RED4ext::Vector4 RoutePoint(int32_t aIndex);
  float RouteDistance(int32_t aIndex);
  float RouteLength();
  // the route has a trajectory (the script's IsDefined(trajectory) && trajectory.IsValid()), and its planned speed at
  // route distance aS (m/s; 0 without one)
  bool HasTrajectory();
  float PlannedSpeed(float aS);
  // FindGroundSpot for script (FlightModeAutopilot.reds wraps it with the out parameter): a ground spot near aTarget
  // with room to land, aKeepRadius (2D) from aKeepClear; W 1 when found, all 0 when not
  RED4ext::Vector4 GroundSpot(RED4ext::Vector4 aTarget, RED4ext::Vector4 aKeepClear, float aKeepRadius);
  // a tap of the autodrive key while engaged (FlightController.TapModeSwitch)
  void ToggleFreeRoam();
  // the route's trajectory built again for the tuning as it is now (LtbfBridge after changing it)
  void BuildTrajectory();
  // the car's limits filled again, and the model of them Vehicle returns (LtbfBridge)
  void RefreshVehicle();
  RED4ext::Handle<FlightVehicleModel> Vehicle();
  // the horizontal unit vector of v, zero if v is vertical (FlightSummonSystem)
  static RED4ext::Vector4 Flat(RED4ext::Vector4 aV);

  // --- C++ state ---
  Navigation::AutopilotParams params;
  Navigation::Rng rng;
  Navigation::FlightWorld world;
  Navigation::Autopilot ap;

  // the Mod Settings switch, read by IsEnabled (Flight::AutopilotPlayer().enabled when the mode is set up)
  bool enabled = true;
  // the Standard lift factor flown this frame (Audio/Inputs/Gather reads it with the commands)
  float standardModeLiftFactor = 25.0f;

  // copies of the Autopilot's String and Vector4 fields, for script
  RED4ext::CString status;
  RED4ext::Vector4 externalDestination{}, padLandingOffset{}, landingKeepClear{};
  RED4ext::Vector4 destination{}, holdPosition{}, routeGoal{}, landingSpot{}, landingHeading0{}, landingHeading{};
  RED4ext::Vector4 lastPosition{}, freeRoamCentre{}, joinPoint{}, gateEntry{}, routeGateEntry{}, landSlewHeading{};
  RED4ext::Vector4 thrustCmd{}, closestNow{}, pathCurvature{}, accelCmd{}, headingRef{}, attAccel{}, attRef{};
  RED4ext::Vector4 targetRight{}, targetForward{}, targetUp{}, omegaRef{};

  // the tuning, the state's numbers and flags, and the surge hold's and plan-ahead's tunables as properties over the
  // C++ members, by name (called from RTTI_DEFINE_CLASS)
  static void DescribeFields(RED4ext::CClass *aType);

private:
  struct Frame;
  bool Gather(Navigation::AutopilotInput &aIn, Frame &aFrame);
  void ApplySettings(bool aForce);
  void PushScriptWrites();
  void PullState();
  void Glue(const Navigation::AutopilotEvents &aEvents, bool aMounted);
  void WriteTelemetry(const Navigation::AutopilotInput &aIn, float aCollisionTimer);
  void CloseTelemetry();

  // the summon's three fields as last copied out, to tell a script write from the Autopilot's own value
  RED4ext::Vector4 shadowExternal{}, shadowPadOffset{}, shadowKeepClear{};
  uint32_t settingsSeen = 0, developerSeen = 0;
  bool settingsApplied = false;
  int32_t telemetryFile = 0;
  float telemetryClock = 0.0f;
  RED4ext::Handle<FlightVehicleModel> vehicleModel;

  RTTI_IMPL_TYPEINFO(FlightModeAutopilot);
  RTTI_IMPL_ALLOCATOR();
};

RTTI_DEFINE_CLASS(FlightModeAutopilot, {
  RTTI_PARENT(FlightMode);
  RTTI_PROPERTY(enabled);
  RTTI_PROPERTY(standardModeLiftFactor);
  RTTI_PROPERTY(status);
  RTTI_PROPERTY(externalDestination);
  RTTI_PROPERTY(padLandingOffset);
  RTTI_PROPERTY(landingKeepClear);
  RTTI_PROPERTY(destination);
  RTTI_PROPERTY(holdPosition);
  RTTI_PROPERTY(routeGoal);
  RTTI_PROPERTY(landingSpot);
  RTTI_PROPERTY(landingHeading0);
  RTTI_PROPERTY(landingHeading);
  RTTI_PROPERTY(lastPosition);
  RTTI_PROPERTY(freeRoamCentre);
  RTTI_PROPERTY(joinPoint);
  RTTI_PROPERTY(gateEntry);
  RTTI_PROPERTY(routeGateEntry);
  RTTI_PROPERTY(landSlewHeading);
  RTTI_PROPERTY(thrustCmd);
  RTTI_PROPERTY(closestNow);
  RTTI_PROPERTY(pathCurvature);
  RTTI_PROPERTY(accelCmd);
  RTTI_PROPERTY(headingRef);
  RTTI_PROPERTY(attAccel);
  RTTI_PROPERTY(attRef);
  RTTI_PROPERTY(targetRight);
  RTTI_PROPERTY(targetForward);
  RTTI_PROPERTY(targetUp);
  RTTI_PROPERTY(omegaRef);
  FlightModeAutopilot::DescribeFields(type);
  RTTI_METHOD(RouteSize);
  RTTI_METHOD(RoutePoint);
  RTTI_METHOD(RouteDistance);
  RTTI_METHOD(RouteLength);
  RTTI_METHOD(HasTrajectory);
  RTTI_METHOD(PlannedSpeed);
  RTTI_METHOD(GroundSpot);
  RTTI_METHOD(ToggleFreeRoam);
  RTTI_METHOD(BuildTrajectory);
  RTTI_METHOD(RefreshVehicle);
  RTTI_METHOD(Vehicle);
  RTTI_METHOD(Flat);
});
