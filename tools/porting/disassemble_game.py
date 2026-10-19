#!/usr/bin/env python3
"""Annotated disassembly of functions of the original games' binaries.

    python3 tools/porting/disassemble_game.py bp 'cGameLever::Update'      # one function
    python3 tools/porting/disassemble_game.py bp -l 'cGameLever'           # list functions (name, size)
    python3 tools/porting/disassemble_game.py bp -c 'cGameLever::Update'   # only calls, compares and constants

bp, rq and ov are blackplague.bin, requiem.bin and Overture's penumbra.bin.
The name is a regular expression matched against the demangled name.
Addresses in the read-only data are shown as the string or float there.
The disassembly is cached in build/porting/.
"""
import os, re, struct, subprocess, sys

STEAM = os.path.expanduser("~/.local/share/Steam/steamapps/common/")
BINS = {
    "bp": STEAM + "Penumbra Black Plague/blackplague.bin",
    "rq": STEAM + "Penumbra Black Plague/requiem.bin",
    "ov": STEAM + "Penumbra Overture/penumbra.bin",
}
CACHE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "build", "porting")


class Binary:
    def __init__(self, tag):
        self.path = BINS[tag]
        self.data = open(self.path, "rb").read()
        self.sections = []
        out = subprocess.run(["readelf", "-S", "-W", self.path], capture_output=True, text=True).stdout
        for line in out.splitlines():
            m = re.match(r"\s*\[\s*\d+\]\s+(\S+)\s+\S+\s+([0-9a-f]+)\s+([0-9a-f]+)\s+([0-9a-f]+)", line)
            if m:
                self.sections.append((m.group(1), int(m.group(2), 16), int(m.group(3), 16), int(m.group(4), 16)))
        os.makedirs(CACHE, exist_ok=True)
        self.disassembly_path = os.path.join(CACHE, tag + ".dis")
        if not os.path.exists(self.disassembly_path):
            with open(self.disassembly_path, "w") as f:
                subprocess.run(["objdump", "-d", "-C", "--no-show-raw-insn", self.path], stdout=f)

    def read(self, addr, size):
        for name, start, offset, length in self.sections:
            if name in (".rodata", ".data") and start <= addr < start + length:
                return self.data[offset + addr - start: offset + addr - start + size]
        return None

    def describe(self, addr, hint=""):
        raw = self.read(addr, 256)
        if raw is None:
            return None
        if hint == "float":
            return "%g" % struct.unpack("<f", raw[:4])[0]
        if hint == "double":
            return "%g" % struct.unpack("<d", raw[:8])[0]
        # wchar_t strings are four bytes a character; read them first, as
        # L"EFX:" would otherwise pass for the narrow string "E"
        chars = []
        for i in range(0, len(raw) - 3, 4):
            c = struct.unpack("<I", raw[i:i + 4])[0]
            if c == 0:
                break
            if not (32 <= c < 127 or c in (9, 10)):
                chars = None
                break
            chars.append(chr(c))
        if chars and len(chars) >= 2:
            return 'L"%s"' % "".join(chars).replace("\n", "\\n")
        end = raw.find(b"\0")
        if end >= 0 and all(32 <= c < 127 or c in (9, 10) for c in raw[:end]):
            # Empty and one character strings are worth showing too
            return '"%s"' % raw[:end].decode().replace("\n", "\\n")
        if chars:
            return 'L"%s"' % "".join(chars)
        return "float %g" % struct.unpack("<f", raw[:4])[0]

    def functions(self, pattern):
        rx = re.compile(pattern)
        cur, lines, show = None, [], False
        for line in open(self.disassembly_path):
            m = re.match(r"([0-9a-f]+) <(.*)>:$", line)
            if m:
                if show:
                    yield cur, lines
                cur = re.sub(r"@@Base$", "", m.group(2))
                lines, show = [], bool(rx.search(cur))
            elif show and line.strip():
                lines.append(line.rstrip())
        if show:
            yield cur, lines

    def annotate(self, line):
        notes = []
        float_hint = "float" if re.search(r"\bf\w*s\s", line) else "double" if re.search(r"\bf\w*l\s", line) else ""
        for m in re.finditer(r"(\$)?0x(8[5-6][0-9a-f]{5})\b", line):
            addr = int(m.group(2), 16)
            d = self.describe(addr, "" if m.group(1) else float_hint)
            if d:
                notes.append(d)
        line = re.sub(r"std::basic_string<char, std::char_traits<char>, std::allocator<char> >", "tString", line)
        line = re.sub(r"std::basic_string<wchar_t, std::char_traits<wchar_t>, std::allocator<wchar_t> >", "tWString", line)
        line = line.replace("@@Base", "")
        return line + ("    ; " + " | ".join(notes) if notes else "")


def main():
    args = sys.argv[1:]
    if len(args) < 2:
        sys.exit(__doc__)
    b = Binary(args[0])
    mode = args[1] if args[1] in ("-l", "-c") else ""
    pattern = args[2] if mode else args[1]
    for name, lines in b.functions(pattern):
        if mode == "-l":
            print("%6d  %s" % (len(lines), name))
            continue
        print("==== " + name)
        for line in lines:
            if mode == "-c" and not re.search(r"\bcall|\bcmp|\bucomi|\bfcom|\bfucom|\$0x8[5-6]|\bfld|\bj[a-z]+\s", line):
                continue
            print(b.annotate(line))


if __name__ == "__main__":
    main()
