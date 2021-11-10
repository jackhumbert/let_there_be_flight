//#include <iostream>
//
//#include <RED4ext/RED4ext.hpp>
//#include <RED4ext/RTTITypes.hpp>
//
//#include "stdafx.hpp"
//
//namespace FlightStats_Record {
//
//    struct FlightStats_Record : RED4ext::gamedataTweakDBRecord
//    {
//        RED4ext::CClass* GetNativeType();
//        float mass;
//    };
//
//    RED4ext::TTypedClass<FlightStats_Record> flightStats_RecordCls("gamedataFlightStats_Record");
//
//    RED4ext::CClass* FlightStats_Record::GetNativeType()
//    {
//        return &flightStats_RecordCls;
//    }
//
//    void RegisterTypes() {
//        flightStats_RecordCls.flags = { .isNative = true, .isImportOnly = true };
//        RED4ext::CRTTISystem::Get()->RegisterType(&flightStats_RecordCls, 0x34222AB7); // murmur 3 of FlightStats with 0x5EEDBA5E seed
//    }
//
//    void RegisterFunctions() {
//        auto rtti = RED4ext::CRTTISystem::Get();
//        auto scriptable = rtti->GetClass("gamedataTweakDBRecord");
//        flightStats_RecordCls.parent = scriptable;
//
//        rtti->RegisterScriptName("gamedataFlightStats_Record", "FlightStats_Record");
//    }
//}
