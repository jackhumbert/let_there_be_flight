#pragma once
#include "Utils/FlightModule.hpp"
#include <RedLib.hpp>

struct FlightLog : RED4ext::IScriptable {
  static void Info(RED4ext::CString value);
  static void Warn(RED4ext::CString value);
  static void Error(RED4ext::CString value);
  static void Probe(RED4ext::Handle<RED4ext::IScriptable> image, RED4ext::red::ResourceReferenceScriptToken value);

  RTTI_IMPL_TYPEINFO(FlightLog);
  RTTI_IMPL_ALLOCATOR();
};

RTTI_DEFINE_CLASS(FlightLog, {
  RTTI_METHOD(Info);
  RTTI_METHOD(Warn);
  RTTI_METHOD(Error);
  RTTI_METHOD(Probe);
});