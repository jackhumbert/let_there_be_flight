#pragma once
// Per-frame telemetry to CSV files, from script: Begin opens red4ext/logs/ltbf_<name>_<time>.csv with
// a header line and returns its handle (0 if it could not), Row appends one line of numbers, End
// closes it. Files of their own, so a frame's worth of numbers needs no string building in script
// and does not bury the plugin log; several can be open at once (the Autopilot's, a probe's).
#include <RED4ext/Scripting/IScriptable.hpp>
#include <RedLib.hpp>

struct FlightTelemetry : RED4ext::IScriptable {
  static int32_t Begin(RED4ext::CString aName, RED4ext::CString aHeader);
  static bool IsOpen(int32_t aFile);
  static void Row(int32_t aFile, RED4ext::DynArray<float> aValues);
  static void End(int32_t aFile);

  RTTI_IMPL_TYPEINFO(FlightTelemetry);
  RTTI_IMPL_ALLOCATOR();
};

RTTI_DEFINE_CLASS(FlightTelemetry, {
  RTTI_METHOD(Begin);
  RTTI_METHOD(IsOpen);
  RTTI_METHOD(Row);
  RTTI_METHOD(End);
});
