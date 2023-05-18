#pragma once

#include <iostream>

#include <RED4ext/InstanceType.hpp>
#include <RED4ext/Common.hpp>
#include <RED4ext/RTTITypes.hpp>
#include <RED4ext/Scripting/IScriptable.hpp>
#include <RED4ext/Scripting/Natives/Generated/Vector4.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/ImageWidgetReference.hpp>
#include <RED4ext/Scripting/Natives/Generated/red/ResourceReferenceScriptToken.hpp>
#include <RED4ext/Scripting/Natives/ScriptGameInstance.hpp>

#include "Log.hpp"
#include "stdafx.hpp"

void FlightLog::Info(const RED4ext::CString value) {
  spdlog::info(value.c_str());
}

void FlightLog::Warn(const RED4ext::CString value) {
  spdlog::warn(value.c_str());
}

void FlightLog::Error(const RED4ext::CString value) {
  spdlog::error(value.c_str());
}

void FlightLog::Probe(RED4ext::Handle<RED4ext::IScriptable> image, RED4ext::red::ResourceReferenceScriptToken value) {

}
