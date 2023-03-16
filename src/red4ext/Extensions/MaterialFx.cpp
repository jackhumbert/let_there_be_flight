#pragma once

#include "MaterialFx.hpp"

void MaterialResource::OnRegister(Descriptor *aType) {
  aType->flags.hasNoDefaultObjectSerialization = true;
}

void MaterialResource::OnDescribe(Descriptor *aType, RED4ext::CRTTISystem * rtti) {
  DESCRIBE_PROPERTY(MaterialResource, ResRef, skidMarks);
  DESCRIBE_PROPERTY(MaterialResource, ResRef, tireTracks);
  DESCRIBE_PROPERTY(MaterialResource, bool, loaded);
}

void MaterialCondition::OnRegister(Descriptor *aType) {
  aType->flags.hasNoDefaultObjectSerialization = true;
}

void MaterialCondition::OnDescribe(Descriptor *aType, RED4ext::CRTTISystem * rtti) {
  DESCRIBE_PROPERTY(MaterialCondition, MaterialResource, particle);
  DESCRIBE_PROPERTY(MaterialCondition, MaterialResource, decal);
}

void MaterialFx::OnRegister(Descriptor *aType) {
  aType->flags.hasNoDefaultObjectSerialization = true;
}

void MaterialFx::OnDescribe(Descriptor *aType, RED4ext::CRTTISystem * rtti) {
  DESCRIBE_PROPERTY(MaterialFx, MaterialCondition, normal);
  DESCRIBE_PROPERTY(MaterialFx, MaterialCondition, wet);
  DESCRIBE_PROPERTY(MaterialFx, MaterialCondition, rain);
}