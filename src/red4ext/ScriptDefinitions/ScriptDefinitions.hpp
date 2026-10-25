#pragma once
#include <RED4ext/Common.hpp>
#include <RED4ext/RTTITypes.hpp>

// The game's in-memory script definitions (what a script bundle is loaded into), game 2.5.
// Ported from mod_settings (src/red4ext/ScriptDefinitions), where the names come from SHA-256
// confirmed symbols and the layouts from the 2.5 constructors, Save/Load and the
// CScriptedDataEnvironment Add*/Find* functions. Members named unk* are never read or written
// by Load/Save. Only the parts LTBF reads are kept here; see mod_settings for the notes.

struct IScriptDataSaver;
struct IScriptDataLoader;
struct CScriptedDataClass;
struct CScriptedDataProperty;
struct CScriptedDataTypeRef;
struct CScriptedDataFunction;
struct CScriptedDataFileInfo;
struct CScriptedDataEnum;
struct CScriptedDataBitfield;

struct IScriptDataObjectFlags {
  uint8_t isExternal : 1; // resolves to a definition that already exists (native, or another bundle)
  uint8_t hasUnk28 : 1;
  uint8_t hasUnk30 : 1;
  uint8_t unk08 : 1;
  uint8_t unk10 : 4;
};
RED4EXT_ASSERT_SIZE(IScriptDataObjectFlags, 0x1);

struct IScriptDataObject {
  enum class EMetaType : uint8_t {
    TypeRef = 0x0,
    Class = 0x1,
    Constant = 0x2,
    Enum = 0x3,
    Bitfield = 0x4,
    Function = 0x5,
    FunctionParam = 0x6,
    FunctionLocal = 0x7,
    Property = 0x8,
    FileInfo = 0x9,
  };

  enum class EVisibility : uint8_t {
    Public = 0x0,
    Protected = 0x1,
    Private = 0x2,
  };

  virtual const RED4ext::Memory::IAllocator &GetMemoryPool() const = 0; // 00
  virtual ~IScriptDataObject() = default;                               // 08
  virtual EMetaType GetMetaType() const = 0;                            // 10
  virtual EVisibility GetVisibility() const = 0;                        // 18
  virtual const IScriptDataObject *GetParent() const = 0;               // 20
  virtual const IScriptDataObject *GetBase() const = 0;                 // 28
  virtual bool IsImported() const = 0;                                  // 30
  virtual void Save(IScriptDataSaver &aSaver) const = 0;                // 38
  virtual void Load(IScriptDataLoader &aLoader) = 0;                    // 40

  RED4ext::CName name;                     // 08
  uint32_t unk10;                          // 10
  uint8_t unk14;                           // 14
  uint64_t unk18;                          // 18
  IScriptDataObjectFlags objectFlags;      // 20 - added in 2.5
  RED4ext::CName unk28;                    // 28 - added in 2.5
  RED4ext::DynArray<RED4ext::CName> unk30; // 30 - added in 2.5
};
RED4EXT_ASSERT_SIZE(IScriptDataObject, 0x40);
RED4EXT_ASSERT_OFFSET(IScriptDataObject, name, 0x08);
RED4EXT_ASSERT_OFFSET(IScriptDataObject, objectFlags, 0x20);

struct CScriptedDataTypeRef : IScriptDataObject {
  enum class EKind : uint32_t {
    Named = 0x0,
    Class = 0x1,
    Handle = 0x2,
    WeakHandle = 0x3,
    DynArray = 0x4,
    StaticArray = 0x5,
    Reference = 0x6,
  };

  RED4ext::CBaseRTTIType *rttiType;   // 40
  const IScriptDataObject *innerType; // 48
  uint32_t staticArraySize;           // 50
  EKind kind;                         // 54
};
RED4EXT_ASSERT_SIZE(CScriptedDataTypeRef, 0x58);
RED4EXT_ASSERT_OFFSET(CScriptedDataTypeRef, kind, 0x54);

// In-memory bit order (Save reorders them into the bundle's flags word).
struct CScriptedDataPropertyFlags {
  uint16_t isImported : 1; // redscript: is_native
  uint16_t isEditable : 1;
  uint16_t isInline : 1;
  uint16_t isConst : 1;
  uint16_t isReplicated : 1;
  uint16_t isInstanceEditable : 1;
  uint16_t isPersistent : 1;
  uint16_t isTestOnly : 1;
  uint16_t isBrowsable : 1;
  uint16_t unk200 : 1;
  uint16_t unk400 : 1;
  uint16_t hasHint : 1;
  uint16_t unk1000 : 4;
};
RED4EXT_ASSERT_SIZE(CScriptedDataPropertyFlags, 0x2);

struct CScriptedDataProperty : IScriptDataObject {
  RED4ext::CProperty *rttiProperty;                              // 40
  const IScriptDataObject *parent;                               // 48 - the owning class
  CScriptedDataPropertyFlags flags;                              // 50
  RED4ext::DynArray<RED4ext::CString> defaultNames;              // 58
  RED4ext::DynArray<RED4ext::CString> defaultValues;             // 68
  uint32_t unk78;                                                // 78
  uint32_t unk7C;                                                // 7C
  EVisibility visibility;                                        // 80
  RED4ext::HashMap<RED4ext::CName, RED4ext::CString> attributes; // 88 - redscript @runtimeProperty
  const CScriptedDataTypeRef *type;                              // B8
  RED4ext::CName hint;                                           // C0
};
RED4EXT_ASSERT_SIZE(CScriptedDataProperty, 0xC8);
RED4EXT_ASSERT_OFFSET(CScriptedDataProperty, flags, 0x50);
RED4EXT_ASSERT_OFFSET(CScriptedDataProperty, attributes, 0x88);
RED4EXT_ASSERT_OFFSET(CScriptedDataProperty, type, 0xB8);

struct CScriptedDataClassFlags {
  uint8_t isImported : 1;
  uint8_t isImportOnly : 1;
  uint8_t isAbstract : 1;
  uint8_t isFinal : 1;
  uint8_t isStruct : 1;
  uint8_t hasAttributes : 1;
  uint8_t unk40 : 1;
  uint8_t unk80 : 1;
};
RED4EXT_ASSERT_SIZE(CScriptedDataClassFlags, 0x1);

struct CScriptedDataClass : IScriptDataObject {
  RED4ext::CClass *rttiType;                                     // 40
  const CScriptedDataClass *base;                                // 48
  RED4ext::DynArray<CScriptedDataProperty *> properties;         // 50
  RED4ext::DynArray<CScriptedDataProperty *> overrides;          // 60
  RED4ext::DynArray<void *> unk70;                               // 70 - added in 2.5, runtime only
  RED4ext::DynArray<void *> unk80;                               // 80 - added in 2.5, runtime only
  RED4ext::DynArray<CScriptedDataFunction *> functions;          // 90
  RED4ext::HashMap<RED4ext::CName, RED4ext::CString> attributes; // A0
  EVisibility visibility;                                        // D0
  CScriptedDataClassFlags flags;                                 // D1
};
RED4EXT_ASSERT_SIZE(CScriptedDataClass, 0xD8);
RED4EXT_ASSERT_OFFSET(CScriptedDataClass, properties, 0x50);
RED4EXT_ASSERT_OFFSET(CScriptedDataClass, flags, 0xD1);

struct CScriptedDataEnvironment {
  bool unk00;                                                                // 00
  RED4ext::HashMap<RED4ext::CName, IScriptDataObject *> namedTypes;          // 08
  RED4ext::HashMap<RED4ext::CName, CScriptedDataFunction *> globalFunctions; // 38
  RED4ext::HashMap<RED4ext::CName, CScriptedDataTypeRef *> typeRefsByName;   // 68
  RED4ext::DynArray<CScriptedDataFileInfo *> files;                          // 98
  RED4ext::DynArray<CScriptedDataFunction *> functions;                      // A8
  RED4ext::DynArray<CScriptedDataEnum *> enums;                              // B8
  RED4ext::DynArray<CScriptedDataBitfield *> bitfields;                      // C8
  RED4ext::DynArray<CScriptedDataClass *> classes;                           // D8
  RED4ext::DynArray<CScriptedDataTypeRef *> typeRefs;                        // E8
};
RED4EXT_ASSERT_OFFSET(CScriptedDataEnvironment, classes, 0xD8);
RED4EXT_ASSERT_OFFSET(CScriptedDataEnvironment, typeRefs, 0xE8);
