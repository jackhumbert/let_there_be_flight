#pragma once
#include <RED4ext/RED4ext.hpp>

enum EDefinitionType : __int8 {
 Type = 0x0,
 Class = 0x1,
 Constant = 0x2,
 Enum = 0x3,
 Bitfield = 0x4,
 Function = 0x5,
 Parameter = 0x6,
 LocalVar = 0x7,
 Property = 0x8,
 SourceFile = 0x9,
};

enum EScriptType {
  Simple = 0x0,
  Cls = 0x1,
  Handle = 0x2,
  Unk3 = 0x3,
  Array = 0x4,
};

enum EVisibility : __int8 {
  Public = 0x0,
  Protected = 0x1,
  Private = 0x2,
};

struct ScriptClass;

struct ScriptDefinition {
  virtual RED4ext::Memory::IAllocator * dstr() = 0;
  virtual void sub_08() = 0;
  virtual EDefinitionType GetDefinitionType() = 0;
  virtual EVisibility GetVisibility() = 0;
  virtual ScriptClass  GetParent() = 0;
  virtual void sub_28() = 0;
  virtual bool IsNative() = 0;

  RED4ext::CName name;
  uint64_t unk10;
};

struct ScriptType : ScriptDefinition {
  RED4ext::CBaseRTTIType *rttiType;
  ScriptClass *innerType;
  uint32_t unk28;
  EScriptType type;
};

enum ScriptPropertyFlags : __int16 {
 prop_isNative = 0x1,
 prop_isEditable = 0x2,
 prop_isInline = 0x4,
 prop_isConst = 0x8,
 prop_isReplicated = 0x10,
 prop_hasHint = 0x20,
 prop_isInstanceEditable = 0x40,
 prop_hasDefault = 0x80,
 prop_isPersistent = 0x100,
 prop_isTestOnly = 0x200,
 prop_isMutable = 0x400,
 prop_isBrowsable = 0x800,
};

struct ScriptProperty : ScriptDefinition {
  RED4ext::CProperty *rttiProperty;
  ScriptDefinition *parent;
  ScriptPropertyFlags flags;
  uint16_t unk2A[3];
  RED4ext::DynArray<RED4ext::CString> defaultClasss;
  RED4ext::DynArray<RED4ext::CString> defaultValues;
  uint64_t unk50;
  EVisibility unk58;
  uint8_t unk59[7];
  RED4ext::HashMap<RED4ext::CName, RED4ext::CString> runtimeProperties;
  ScriptType *type;
};
RED4EXT_ASSERT_SIZE(ScriptProperty, 0x98);
RED4EXT_ASSERT_OFFSET(ScriptProperty, runtimeProperties, 0x60);

enum ScriptClassFlags : __int32 {
  cls_isNative = 0x1,
  cls_isAbstract = 0x2,
  cls_isFinal = 0x4,
  cls_isStruct = 0x8,
  cls_hasFunctions = 0x10,
  cls_hasFields = 0x20,
  cls_isImportOnly = 0x40,
  cls_isTestOnly = 0x80,
  cls_hasOverrides = 0x100,
  cls_unk200 = 0x200,
  cls_unk400 = 0x400,
};
struct ScriptClass : ScriptDefinition {
  RED4ext::CClass *rttiType;
  ScriptClass *parent;
  RED4ext::DynArray<ScriptProperty*> properties;
  RED4ext::DynArray<void*> overrides;
  RED4ext::DynArray<void*> functions;
  __unaligned __declspec(align(1))  RED4ext::HashMap<RED4ext::CName, RED4ext::CString> unk58;
  uint8_t visibility;
  uint8_t unk89;
  uint8_t unk8A;
  uint8_t unk8B;
  ScriptClassFlags flags;
};
RED4EXT_ASSERT_SIZE(ScriptClass, 0x90);
//char (*__kaboom)[offsetof(ScriptClass, rttiType)] = 1;