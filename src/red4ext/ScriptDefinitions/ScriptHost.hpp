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
  const uintptr_t VFT_RVA = 0x30E74C0;

  void sub_00(); // empty
  void sub_08(); // load
  void sub_10();
  void sub_18();
  void sub_20();

  // something with files
  void sub_28();

  void sub_30(); // empty

  // if unk20 == 0
  void sub_38();

  // sets unk24
  void sub_40();

  // gets unk24
  void sub_48();

  // something with (),
  void sub_50(RED4ext::CString *);

  // something else with (),
  void sub_58(bool *, RED4ext::CString *);

  void sub_60();
  void sub_68();
  void sub_70();
  void sub_78();
  void sub_80();
  void sub_88();
  void sub_90();
  void sub_98();

  void *vft2;
  RED4ext::DynArray<void*> unk10;
  uint32_t unk20; // state?
  uint16_t unk24;
  uint16_t unk26;
  uint64_t unk28;
  RED4ext::Map<uint32_t, ScriptFile *> files;
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
