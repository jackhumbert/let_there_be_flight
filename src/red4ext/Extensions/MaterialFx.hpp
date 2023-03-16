#pragma once

#include "stdafx.hpp"
#include <RED4ext/NativeTypes.hpp>
#include <RED4ext/RED4ext.hpp>
#include "Engine/RTTIClass.hpp"
#include <RED4ext\Scripting\Natives\Generated\red\ResourceReferenceScriptToken.hpp>

class MaterialResource : Engine::RTTIStruct<MaterialResource> {
public:
  RED4ext::ResRef skidMarks;
  RED4ext::ResRef tireTracks;
  bool loaded;
private:
  friend Descriptor;
  static void OnRegister(Descriptor *aType);
  static void OnDescribe(Descriptor *aType, RED4ext::CRTTISystem * rtti);
};

class MaterialCondition : Engine::RTTIStruct<MaterialCondition> {
public:
  MaterialResource particle;
  MaterialResource decal;
private:
  friend Descriptor;
  static void OnRegister(Descriptor *aType);
  static void OnDescribe(Descriptor *aType, RED4ext::CRTTISystem * rtti);
};

class MaterialFx : Engine::RTTIStruct<MaterialFx> {
public:
  MaterialCondition normal;
  MaterialCondition wet;
  MaterialCondition rain;
private:
  friend Descriptor;
  static void OnRegister(Descriptor *aType);
  static void OnDescribe(Descriptor *aType, RED4ext::CRTTISystem * rtti);
};