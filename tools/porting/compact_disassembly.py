#!/usr/bin/env python3
"""Compact view of disassemble_game.py's output, for reading a function's logic.

    python3 tools/porting/disassemble_game.py bp 'cGameLever::Update' | python3 tools/porting/compact_disassembly.py

Addresses are kept only where something jumps to (as L<offset>), the
function prologue, padding and the exception landing pads are dropped, a
load followed by a store of the same register becomes one "dst = src" line,
long type names are shortened, and the old std::string's reference counting
and the arguments copied to the stack from registers are left out.
"""
import re, sys

TYPE_REPLACEMENTS = [(r"hpl::", ""), (r"std::string const&", "tString&"), (r"cVector3<float>", "cVector3f"),
         (r"cMatrix<float>", "cMatrixf"), (r"@plt", ""), (r"std::allocator<char> const&", "alloc"),
         (r"tString::basic_string", "tString"), (r"std::string::", "tString::")]
# The reference counting of the old std::string, and arguments put on the stack from registers
COMPILER_NOISE = re.compile(r"\$0x86ae3f0|lock xadd|_M_destroy|~basic_string|^(push|pop) %e(bp|bx|si|di)$|^mov %esp,%ebp$|^leave$")
STACK_ARGUMENT_STORE = re.compile(r"^(mov|lea) +[^,]*,(0x[0-9a-f]+)?\(%esp\)$")
PADDING = re.compile(r"^(nop|xchg +%ax,%ax|lea +0x0\(%e[sd]i(,%eiz,1)?\),%e[sd]i)$")


def emit_function(name, rows):
    if not rows:
        return
    base = rows[0][0]
    targets = set()
    for addr, ins in rows:
        m = re.match(r"j\w+ +([0-9a-f]+)", ins)
        if m:
            targets.add(int(m.group(1), 16))
    # Only what can be reached from the entry: the rest is unwinding code
    index = {a: k for k, (a, _) in enumerate(rows)}
    live, todo = set(), [0]
    while todo:
        k = todo.pop()
        while k < len(rows) and k not in live:
            live.add(k)
            ins = rows[k][1]
            m = re.match(r"(j\w+) +([0-9a-f]+)", ins)
            if m and int(m.group(2), 16) in index:
                todo.append(index[int(m.group(2), 16)])
            if ins.startswith(("jmp", "ret")) or "_Unwind_Resume" in ins or "__cxa_throw" in ins:
                break
            k += 1
    out, i = [], 0
    while i < len(rows):
        addr, ins = rows[i]
        if i not in live or COMPILER_NOISE.search(ins) or (STACK_ARGUMENT_STORE.match(ins) and "$" not in ins):
            i += 1
            continue
        label = "L%x: " % (addr - base) if addr in targets else "    "
        if PADDING.match(ins):
            i += 1
            continue
        m = re.match(r"mov[lb]? +(\S+\(%\w+\)|0x[0-9a-f]+),(%e?[abcd][xl])$", ins)
        if m and i + 1 < len(rows) and rows[i + 1][0] not in targets:
            n = re.match(r"mov[lb]? +(%e?[abcd][xl]),(\S+\(%\w+\))$", rows[i + 1][1].split(";")[0].strip())
            if n and n.group(1) == m.group(2):
                out.append(label + "%s = %s" % (n.group(2), m.group(1)))
                i += 2
                continue
        ins = re.sub(r"(j\w+) +([0-9a-f]+) <(.*)>$", lambda j: "%s L%x" % (j.group(1), int(j.group(2), 16) - base)
                     if int(j.group(2), 16) in index else "%s %s" % (j.group(1), j.group(3)), ins)
        ins = re.sub(r"call +[0-9a-f]+ <(.*)>$", r"call \1", ins)
        out.append(label + ins)
        i += 1
    print("==== " + name)
    skip = 0
    for line in out:
        if re.search(r"_Unwind_Resume", line):
            continue
        print(re.sub(r" {2,}", " ", line) if not line.startswith("L") else line)


name, rows = None, []
for line in sys.stdin:
    line = line.rstrip("\n")
    for a, b in TYPE_REPLACEMENTS:
        line = re.sub(a, b, line)
    if line.startswith("==== "):
        emit_function(name, rows)
        name, rows = line[5:], []
        continue
    m = re.match(r"\s*([0-9a-f]+):\t(.*)$", line)
    if m:
        rows.append((int(m.group(1), 16), re.sub(r"\s+", " ", m.group(2)).strip()))
emit_function(name, rows)
