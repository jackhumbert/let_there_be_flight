"""Exception address and likely call chain from a Windows minidump, as IDB addresses.

No debugger needed: reads the dump's exception stream (code, faulting address, thread id, the
thread's CONTEXT) and module list, then scans the crashing thread's stack memory for values that
point into Cyberpunk2077.exe (candidate return addresses; a heuristic, not an unwinder).
Addresses are rebased to the IDB's image base (0x140000000) for IDA lookups.

  python tools/minidump_addrs.py <Cyberpunk2077.dmp> [--module Cyberpunk2077.exe] [--max 40]
"""
import argparse
import struct

IDB_BASE = 0x140000000
EXCEPTION_STREAM, MODULE_LIST, THREAD_LIST, MEMORY_LIST, MEMORY64_LIST = 6, 4, 3, 5, 9


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("dump")
    ap.add_argument("--module", default="Cyberpunk2077.exe")
    ap.add_argument("--max", type=int, default=40)
    args = ap.parse_args()
    d = open(args.dump, "rb").read()
    sig, ver, nstreams, dir_rva = struct.unpack_from("<4sIII", d, 0)
    assert sig == b"MDMP", sig
    streams = {}
    for i in range(nstreams):
        t, size, rva = struct.unpack_from("<III", d, dir_rva + 12 * i)
        streams[t] = (size, rva)

    def utf16(rva):
        n = struct.unpack_from("<I", d, rva)[0]
        return d[rva + 4: rva + 4 + n].decode("utf-16-le")

    modules = []
    size, rva = streams[MODULE_LIST]
    n = struct.unpack_from("<I", d, rva)[0]
    for i in range(n):
        base, msize, _, _, name_rva = struct.unpack_from("<QIIII", d, rva + 4 + 108 * i)
        modules.append((base, msize, utf16(name_rva)))
    exe = [m for m in modules if m[2].lower().endswith(args.module.lower())][0]

    def where(addr):
        for base, msize, name in modules:
            if base <= addr < base + msize:
                short = name.split("\\")[-1]
                if short.lower() == args.module.lower():
                    return f"{short}+{addr - base:#x} (IDB {IDB_BASE + addr - base:#x})"
                return f"{short}+{addr - base:#x}"
        return None

    size, rva = streams[EXCEPTION_STREAM]
    tid = struct.unpack_from("<I", d, rva)[0]
    code, flags, rec, addr, nparams = struct.unpack_from("<IIQQI", d, rva + 8)
    params = struct.unpack_from("<15Q", d, rva + 8 + 32)
    ctx_size, ctx_rva = struct.unpack_from("<II", d, rva + 8 + 152)
    print(f"exception {code:#010x} at {addr:#x} = {where(addr)}; thread {tid}")
    if code == 0xC0000005:
        print(f"  access violation: {'write' if params[0] == 1 else 'read' if params[0] == 0 else params[0]} "
              f"of {params[1]:#x}")
    # x64 CONTEXT: Rax at 0x78 ... Rip at 0xF8, Rsp at 0x98
    regs = ["rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"]
    vals = struct.unpack_from("<16Q", d, ctx_rva + 0x78)
    rip = struct.unpack_from("<Q", d, ctx_rva + 0xF8)[0]
    print("  rip", hex(rip), where(rip) or "")
    print("  " + " ".join(f"{r}={v:#x}" for r, v in zip(regs, vals)))
    rsp = vals[4]

    # crashing thread's stack memory: thread list gives its stack range
    stack = None
    size, rva = streams[THREAD_LIST]
    n = struct.unpack_from("<I", d, rva)[0]
    for i in range(n):
        t_id, _, _, _, teb, start, msize, mrva, _, _ = struct.unpack_from("<IIIIQQIIII", d, rva + 4 + 48 * i)
        if t_id == tid:
            stack = (start, d[mrva: mrva + msize])
    if stack:
        start, mem = stack
        off = max(0, rsp - start)
        hits = []
        for o in range(off - off % 8, len(mem) - 8, 8):
            v = struct.unpack_from("<Q", mem, o)[0]
            if exe[0] <= v < exe[0] + exe[1]:
                hits.append((start + o, v))
            if len(hits) >= args.max:
                break
        print(f"stack scan from rsp, {len(hits)} values pointing into {args.module}:")
        for sp, v in hits:
            print(f"  [{sp:#x}] {where(v)}")


if __name__ == "__main__":
    main()
