#pragma once

// FlightMode as a native class (docs/design/flight-modes.md, FlightMode, the native base): the base of every flight
// mode, C++ or redscript.
//
// A C++ mode is a subclass overriding the C++ virtuals below. Redscript reaches it through the base's own functions:
// FlightMode.reds' Initialize, IsEnabled, Activate, Deactivate, Teleported, Update and GetDescription call
// NativeInitialize, NativeIsEnabled, NativeActivate, NativeDeactivate, NativeTeleported, NativeUpdate and
// NativeDescription, which run the C++ step and then the redscript hook (OnInitialize, OnActivate, OnDeactivate,
// OnTeleported, OnUpdate) when the object's class overrides it, or the hook in place of the C++ answer (Enabled,
// Description). So a mod extends a C++ mode in redscript by overriding the hooks, and no redscript call has to find a
// C++ override (redscript calling a function that a native subclass overrides by name does not reach the override).
// The redscript modes that still override Update and the rest themselves work as before.
//
// FlightModeNativeProbe is the spike's mode: it counts its calls and gives no force but a 0.25 m/s2 sideways marker
// (in the list behind the developer setting nativeModeProbe; the car falls).

#include <RED4ext/Scripting/IScriptable.hpp>
#include <RED4ext/Scripting/Natives/Generated/Vector4.hpp>
#include <RedLib.hpp>

struct FlightMode : RED4ext::IScriptable {
  // the mode's output this frame, car frame (FlightMode.force and torque)
  RED4ext::Vector4 force{};
  RED4ext::Vector4 torque{};

  // a C++ mode's own steps
  // after FlightMode.Initialize has set the component (FlightComponent.OnGameAttach, before IsEnabled)
  virtual void OnInitializeNative() {}
  // whether the mode joins the car's mode list (its Mod Settings switch)
  virtual bool IsEnabledNative() { return true; }
  virtual void OnActivateNative() {}
  virtual void OnDeactivateNative() {}
  virtual void OnUpdateNative(float aTimeDelta) {}
  virtual RED4ext::CString DescriptionNative() { return ""; }
  // the car was teleported: by default the mode starts over where it is, through the object's Activate called by name
  // (a redscript mode's own Activate, or FlightMode's, which runs the C++ one), as FlightMode.reds' Teleported did
  virtual void OnTeleportedNative();

  // the C++ step, then the class's redscript hook if it overrides one
  void NativeInitialize();
  void NativeActivate();
  void NativeDeactivate();
  void NativeTeleported();
  void NativeUpdate(float aTimeDelta);
  // the class's Description (Enabled) hook if it overrides it, else the C++ one
  RED4ext::CString NativeDescription();
  bool NativeIsEnabled();

  // The mode list: TweakDB's FlightModes.classes (src/tweaks/flight_modes.yaml), and a new instance of a FlightMode
  // class by name, C++ or redscript, made as the flight configurations are (CreateInstance, ConstructCls); an empty
  // handle for a name that is not a concrete FlightMode class. The script initializes it.
  static RED4ext::DynArray<RED4ext::CName> ModeClasses();
  static RED4ext::Handle<FlightMode> Instantiate(RED4ext::CName aClass);

  RTTI_IMPL_TYPEINFO(FlightMode);
  RTTI_IMPL_ALLOCATOR();

private:
  // the class's override of a FlightMode hook, nullptr when it has none (looked up once per class)
  RED4ext::CClassFunction *Hook(int aIndex);
};

RTTI_DEFINE_CLASS(FlightMode, {
  RTTI_ABSTRACT();
  RTTI_PROPERTY(force);
  RTTI_PROPERTY(torque);
  RTTI_METHOD(NativeInitialize);
  RTTI_METHOD(NativeIsEnabled);
  RTTI_METHOD(NativeActivate);
  RTTI_METHOD(NativeDeactivate);
  RTTI_METHOD(NativeTeleported);
  RTTI_METHOD(NativeUpdate);
  RTTI_METHOD(NativeDescription);
  RTTI_METHOD(ModeClasses);
  RTTI_METHOD(Instantiate);
});

struct FlightModeNativeProbe : FlightMode {
  int32_t activations = 0;
  int32_t updates = 0;

  void OnActivateNative() override;
  void OnUpdateNative(float aTimeDelta) override;
  RED4ext::CString DescriptionNative() override { return "Native Probe"; }
  // "activations <n> updates <n>"
  RED4ext::CString Calls();

  RTTI_IMPL_TYPEINFO(FlightModeNativeProbe);
  RTTI_IMPL_ALLOCATOR();
};

RTTI_DEFINE_CLASS(FlightModeNativeProbe, {
  RTTI_PARENT(FlightMode);
  RTTI_METHOD(Calls);
});
