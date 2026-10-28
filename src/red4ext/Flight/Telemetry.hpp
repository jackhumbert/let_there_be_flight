#pragma once
// Per-frame telemetry to a CSV file, from script: Begin opens red4ext/logs/ltbf_<name>_<time>.csv
// with a header line, Row appends one line of numbers, End closes it. A file of its own, so a
// frame's worth of numbers needs no string building in script and does not bury the plugin log.
#include <RED4ext/Scripting/IScriptable.hpp>
#include <RedLib.hpp>

struct FlightTelemetry : RED4ext::IScriptable {
  static bool Begin(RED4ext::CString aName, RED4ext::CString aHeader);
  static bool IsOpen();
  static void Row(RED4ext::DynArray<float> aValues);
  static void End();

  RTTI_IMPL_TYPEINFO(FlightTelemetry);
  RTTI_IMPL_ALLOCATOR();
};

RTTI_DEFINE_CLASS(FlightTelemetry, {
  RTTI_METHOD(Begin);
  RTTI_METHOD(IsOpen);
  RTTI_METHOD(Row);
  RTTI_METHOD(End);
});
