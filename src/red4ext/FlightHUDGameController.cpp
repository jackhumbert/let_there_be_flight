//#include <iostream>
//
//#include "FlightHUDGameController.hpp"
//#include <RED4ext/RED4ext.hpp>
//#include <RED4ext/Scripting/Natives/Generated/Vector4.hpp>
//#include <RED4ext/Scripting/Natives/ScriptGameInstance.hpp>
//#include <RED4ext/Scripting/Natives/Generated/game/ui/HUDGameController.hpp>
//#include <RED4ext/Scripting/IScriptable.hpp>
//#include <RED4ext/RTTITypes.hpp>
//
//#include "Utils.hpp"
//#include "stdafx.hpp"
//
//namespace FlightHUDGameController {
//
//    struct FlightHUDGameController : RED4ext::game::ui::HUDGameController
//    {
//        RED4ext::CClass* GetNativeType();
//    };
//
//    RED4ext::TTypedClass<FlightHUDGameController> flightHUDCls("FlightHUDGameController");
//
//    RED4ext::CClass* FlightHUDGameController::GetNativeType()
//    {
//        return &flightHUDCls;
//    }
//
//    void Load() {
//
//    }
//
//    void Unload() {
//
//    }
//
//    void RegisterTypes() {
//        flightHUDCls.flags = { .isNative = true };
//        RED4ext::CRTTISystem::Get()->RegisterType(&flightHUDCls);
//    }
//
//    void RegisterFunctions() {
//        auto rtti = RED4ext::CRTTISystem::Get();
//        auto parent = rtti->GetClass("gameuiHUDGameController");
//        flightHUDCls.parent = parent;
//
//    }
//}
