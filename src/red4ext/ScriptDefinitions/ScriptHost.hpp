#include <RED4ext/RED4ext.hpp>

struct ScriptFile {
  RED4ext::CName name;
  RED4ext::CString filename;
};

enum EBreakpointState : unsigned __int8 {
  Continue = 0x0,
  StepOver = 0x1,
  StepInto = 0x2,
  StepOut = 0x3,
  Pause = 0x4,
};

struct ScriptHost {
  void *vft1;
  void *vft2;
  RED4ext::DynArray<void*> unk10;
  uint32_t unk20;
  uint16_t unk24;
  uint16_t unk26;
  uint64_t unk28;
  RED4ext::DynArray<uint32_t> scriptFileIndexes;
  RED4ext::DynArray<ScriptFile *> scriptFiles;
  uint64_t unk50;
  uint32_t breakpointThread;
  EBreakpointState breakpointState;
  uint8_t unk5D;
  uint8_t unk5E;
  uint8_t unk5F;
  uint64_t unk60;
  RED4ext::HashMap<uint64_t, uint64_t> unk68;
  RED4ext::SharedMutex unk68MUTX;
  uint8_t unk99;
  uint8_t unk9A;
  uint8_t unk9B;
  uint8_t unk9C;
  uint8_t unk9D;
  uint8_t unk9E;
  uint8_t unk9F;
  void *psa;
};
