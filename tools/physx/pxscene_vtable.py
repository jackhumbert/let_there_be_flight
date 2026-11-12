"""PxScene's vtable in the game's PhysX3_x64.dll (PhysX 3.4.2, the game's own build): which slot
raycast, sweep and overlap take, found from the DLL itself so it can be checked again after a game
patch. Result for the 2.5 DLL: the vtable at rva 0x20caf0, raycast slot 94 (byte offset 0x2f0).

NpScene's RTTI type descriptor survives but nothing references it, so the vtable is found from the
API's error messages, which the DLL keeps ("PxScene::release(): ...", "PxScene::fetchQueries: ...").
Each message is traced to the code that loads it (lea rip-relative), the function holding that
code (.pdata, chained unwind info followed to the primary function) and the .rdata slot pointing at
that function. release() is PxScene's first pure virtual; the slot before it is the scalar deleting
destructor (the virtual ~PxScene() declared first), which is where the table starts. In PxScene.h
(3.4) the scene query calls run fetchQueries, raycast, sweep, overlap, getSceneQueryStaticTimestamp,
so raycast is the slot after fetchQueries; the neighbours are disassembled to check (raycast takes
its distance in xmm3, getSceneQueryStaticTimestamp returns one field).

    python tools/physx/pxscene_vtable.py [<game>/bin/x64/PhysX3_x64.dll]
"""

import bisect
import os
import re
import struct
import sys

import capstone
import pefile

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from common import paths  # noqa: E402

DLL = None  # the game bin/x64/PhysX3_x64.dll, set when used


class Image:
    def __init__(self, path):
        self.pe = pefile.PE(path)
        self.base = self.pe.OPTIONAL_HEADER.ImageBase
        self.data = self.pe.get_memory_mapped_image()
        self.text = next(s for s in self.pe.sections if s.Name.rstrip(b"\0") == b".text")
        self.functions = sorted((e.struct.BeginAddress, e.struct.EndAddress, e.struct.UnwindData)
                                for e in self.pe.DIRECTORY_ENTRY_EXCEPTION)
        self.md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_64)

    def u64(self, rva):
        return struct.unpack_from("<Q", self.data, rva)[0]

    def in_text(self, rva):
        return self.text.VirtualAddress <= rva < self.text.VirtualAddress + self.text.Misc_VirtualSize

    def function_of(self, rva):
        """The primary function holding rva (chained unwind info followed back)."""
        i = bisect.bisect_right(self.functions, (rva, 1 << 32, 0)) - 1
        if i < 0 or not (self.functions[i][0] <= rva < self.functions[i][1]):
            return None
        begin, _, unwind = self.functions[i]
        for _ in range(16):
            if not (self.data[unwind] >> 3) & 4:  # UNW_FLAG_CHAININFO
                return begin
            chained = unwind + 4 + 2 * ((self.data[unwind + 2] + 1) & ~1)
            begin, _, unwind = struct.unpack_from("<III", self.data, chained)
        return begin

    def code(self, rva, n=6):
        return "; ".join(f"{i.mnemonic} {i.op_str}" for i in list(self.md.disasm(self.data[rva:rva + 64], self.base + rva))[:n])

    def message_function(self, prefix):
        """The functions loading a message that starts with prefix."""
        out = set()
        t0 = self.text.VirtualAddress
        code = self.data[t0:t0 + self.text.Misc_VirtualSize]
        targets = {m.start() for m in re.finditer(re.escape(prefix.encode()), self.data)}
        # REX.W lea reg, [rip + disp32]
        for m in re.finditer(rb"[\x48\x4c]\x8d[\x05\x0d\x15\x1d\x25\x2d\x35\x3d]", code):
            at = t0 + m.start()
            if at + 7 + struct.unpack_from("<i", self.data, at + 3)[0] in targets:
                f = self.function_of(at)
                if f is not None:
                    out.add(f)
        return out

    def slots_pointing_at(self, rva):
        return [m.start() for m in re.finditer(re.escape(struct.pack("<Q", self.base + rva)), self.data)]


def main():
    img = Image(sys.argv[1] if len(sys.argv) > 1 else os.path.join(paths.GAME, "bin", "x64", "PhysX3_x64.dll"))
    release = [s for f in img.message_function("PxScene::release()") for s in img.slots_pointing_at(f)]
    assert len(release) == 1, f"release() slots: {release}"
    start = release[0] - 8
    dtor = img.u64(start) - img.base
    print(f"slot 0, the destructor? rva {dtor:#x}: {img.code(dtor, 8)}")
    print(f"slot 1, release: {img.code(img.u64(release[0]) - img.base, 4)}")
    fetch = [s for f in img.message_function("PxScene::fetchQueries") for s in img.slots_pointing_at(f)]
    assert len(fetch) == 1, f"fetchQueries slots: {fetch}"
    k = (fetch[0] - start) // 8
    for i, name in ((k, "fetchQueries"), (k + 1, "raycast"), (k + 2, "sweep"), (k + 3, "overlap"),
                    (k + 4, "getSceneQueryStaticTimestamp")):
        f = img.u64(start + 8 * i) - img.base
        print(f"slot {i:3d} (offset {8 * i:#5x}) {name:30s} rva {f:#x}: {img.code(f, 4)}")
    print(f"\nPxScene vtable at rva {start:#x}; raycast slot {k + 1}, byte offset {8 * (k + 1):#x}")


if __name__ == "__main__":
    main()
