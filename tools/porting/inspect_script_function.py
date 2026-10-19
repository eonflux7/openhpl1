#!/usr/bin/env python3
"""The code of a script function of the original games.

    python3 tools/porting/inspect_script_function.py bp SetWidgetBool [| python3 tools/porting/compact_disassembly.py]

A script function's own code has no symbol: it sits at the end of the symbol
before its `_Generic` wrapper. This prints from the last function prologue
before the wrapper up to the wrapper, annotated like disassemble_game.py.
"""
import re, sys
import disassemble_game

binary = disassemble_game.Binary(sys.argv[1])
wrapper_label = "<GenericScript::%s_Generic(asIScriptGeneric*)" % sys.argv[2]
instructions = []
for line in open(binary.disassembly_path):
    if re.match(r"[0-9a-f]+ <", line):
        if wrapper_label in line:
            break
        continue
    if line.strip():
        instructions.append(line.rstrip())
else:
    sys.exit("no such script function")
instructions = instructions[-3000:]
prologue_index = max(i for i in range(len(instructions) - 1) if "push   %ebp" in instructions[i] and "mov    %esp,%ebp" in instructions[i + 1])
print("==== " + sys.argv[2])
for line in instructions[prologue_index:]:
    print(binary.annotate(line))
