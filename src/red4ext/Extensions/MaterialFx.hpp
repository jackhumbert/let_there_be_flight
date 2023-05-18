#pragma once

#include "stdafx.hpp"
#include <RED4ext/NativeTypes.hpp>
#include <RED4ext/Common.hpp>
#include <RedLib.hpp>
#include <RED4ext\Scripting\Natives\Generated\red\ResourceReferenceScriptToken.hpp>
#include <RED4ext/Scripting/Natives/Generated/vehicle/BaseObject.hpp>

using namespace RED4ext::vehicle;

//struct MaterialResource {
//  Red::ResRef skidMarks;
//  Red::ResRef tireTracks;
//  bool loaded;
//};
//
//struct MaterialCondition {
//  MaterialResource particle;
//  MaterialResource decal;
//};
//
//struct MaterialFx {
//  MaterialCondition normal;
//  MaterialCondition wet;
//  MaterialCondition rain;
//};

RTTI_DEFINE_CLASS(MaterialResource, {
  RTTI_PROPERTY(skidMarks);
  RTTI_PROPERTY(tireTracks);
  RTTI_PROPERTY(loaded);
  type->flags.hasNoDefaultObjectSerialization = true;
});

RTTI_DEFINE_CLASS(MaterialCondition, {
  RTTI_PROPERTY(particle);
  RTTI_PROPERTY(decal);
  type->flags.hasNoDefaultObjectSerialization = true;
});

RTTI_DEFINE_CLASS(MaterialFx, {
  RTTI_PROPERTY(normal);
  RTTI_PROPERTY(wet);
  RTTI_PROPERTY(rain);
  type->flags.hasNoDefaultObjectSerialization = true;
});