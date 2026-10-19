#!/usr/bin/env python3
"""Static check of the three games' level scripts against the game's script functions.

  tools/porting/check_scripts.py [overture|blackplague|requiem ...]

Reports, per script file:
  - callbacks named in a call (a string argument whose parameter is named
    *Func/*Callback/*Function) that no function of the script (or of the
    global script, for '@' names and global timers) defines
  - script functions defined twice
  - calls to functions the game does not register (would fail to build)
  - the number of arguments of a call to a game function that does not fit
The functions and their overloads come from the game itself: run each game
once with OPENHPL_SCRIPT_API=build/porting/api/<game>.txt (and, say,
OPENHPL_SMOKE=none so it quits at once). Parameter names, to know which
argument is a callback, come from the sources (static ... __stdcall Name(...)).
"""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", ".."))
STEAM = os.path.expanduser("~/.local/share/Steam/steamapps/common")
GAMES = {
    "overture": (os.path.join(STEAM, "Penumbra Overture"), ["maps"], "global_script.hps"),
    "blackplague": (os.path.join(STEAM, "Penumbra Black Plague"), ["maps"], "global_script.hps"),
    "requiem": (os.path.join(STEAM, "Penumbra Black Plague"), ["expansion01/maps"], "requiem_global_script.hps"),
}

CALLBACK_PARAM = re.compile(r"Func|Callback", re.I)


def source_params():
    """script name -> list of parameter-name lists, from the C++ sources."""
    params, alias = {}, {}
    sig = re.compile(r"static\s+[\w:<>]+\s+__stdcall\s+(\w+)\s*\(([^)]*)\)", re.S)
    for top in ("game", "engine/sources"):
        for dirpath, _, files in os.walk(os.path.join(ROOT, top)):
            for f in files:
                if not f.endswith(".cpp"):
                    continue
                text = open(os.path.join(dirpath, f), encoding="utf-8", errors="replace").read()
                for cpp, script in re.findall(r'SCRIPT_REGISTER_FUNC_AS\(\s*(\w+)\s*,\s*"(\w+)"', text):
                    alias[cpp] = script
                for m in sig.finditer(text):
                    names = []
                    for p in m.group(2).split(","):
                        p = p.strip()
                        if p and p != "void":
                            names.append(re.split(r"[\s*&]+", p)[-1])
                    params.setdefault(m.group(1), []).append(names)
    out = {}
    for cpp, lists in params.items():
        for names in lists:
            out.setdefault(alias.get(cpp, cpp), []).append(names)
    return out


def game_functions(game):
    """name -> list of signatures (lists of (type, name)); overloads kept."""
    path = os.path.join(ROOT, "build", "porting", "api", game + ".txt")
    if not os.path.exists(path):
        sys.exit("%s missing: run the game with OPENHPL_SCRIPT_API=%s first" % (path, path))
    names = source_params()
    funcs = {}
    for line in open(path):
        m = re.match(r"\s*[\w&@]+\s+(\w+)\s*\(([^)]*)\)", line)
        if not m:
            continue
        types = [t.strip() for t in m.group(2).split(",") if t.strip()]
        pnames = next((n for n in names.get(m.group(1), []) if len(n) == len(types)), [""] * len(types))
        funcs.setdefault(m.group(1), []).append(list(zip(types, pnames)))
    return funcs


def strip_comments(text):
    out, i, n = [], 0, len(text)
    while i < n:
        c = text[i]
        if c == '"':
            j = i + 1
            while j < n and text[j] != '"':
                j += 2 if text[j] == "\\" else 1
            out.append(text[i:j + 1])
            i = j + 1
        elif text.startswith("//", i):
            j = text.find("\n", i)
            i = n if j < 0 else j
        elif text.startswith("/*", i):
            j = text.find("*/", i + 2)
            i = n if j < 0 else j + 2
        else:
            out.append(c)
            i += 1
    return "".join(out)


def split_args(text, start):
    """Arguments of the call whose '(' is at text[start]; returns (args, end)."""
    depth, i, cur, args = 0, start, [], []
    while i < len(text):
        c = text[i]
        if c == '"':
            j = i + 1
            while j < len(text) and text[j] != '"':
                j += 2 if text[j] == "\\" else 1
            cur.append(text[i:j + 1])
            i = j + 1
            continue
        if c == "(":
            depth += 1
            if depth == 1:
                i += 1
                continue
        elif c == ")":
            depth -= 1
            if depth == 0:
                if "".join(cur).strip() or args:
                    args.append("".join(cur).strip())
                return args, i
        elif c == "," and depth == 1:
            args.append("".join(cur).strip())
            cur = []
            i += 1
            continue
        cur.append(c)
        i += 1
    return args, i


KEYWORDS = {"if", "while", "for", "return", "switch", "void", "int", "float", "bool", "string", "else"}


def script_functions(text):
    return re.findall(r"^\s*(?:void|int|float|bool|string)\s+(\w+)\s*\(", text, re.M)


def check(game, funcs, report):
    base, dirs, global_name = GAMES[game]
    files = []
    for d in dirs:
        full = os.path.join(base, d)
        files += sorted(os.path.join(full, f) for f in os.listdir(full) if f.endswith(".hps"))
    global_path = os.path.join(base, dirs[0], global_name)
    global_defs = set(script_functions(strip_comments(open(global_path, errors="replace").read()))) \
        if os.path.exists(global_path) else set()

    for path in files:
        name = os.path.relpath(path, base)
        text = strip_comments(open(path, errors="replace").read())
        defs = script_functions(text)
        own = set(defs)
        for d in sorted({d for d in defs if defs.count(d) > 1}):
            report(game, name, "function '%s' is defined %d times" % (d, defs.count(d)))

        for m in re.finditer(r"\b(\w+)\s*\(", text):
            fname = m.group(1)
            if fname in KEYWORDS:
                continue
            # skip definitions
            before = text[max(0, m.start() - 12):m.start()]
            if re.search(r"(void|int|float|bool|string)\s+$", before):
                continue
            if fname in own or fname in global_defs and path == global_path:
                continue
            if fname not in funcs:
                if fname not in own:
                    report(game, name, "calls '%s', which the game does not have" % fname)
                continue
            args, _ = split_args(text, text.index("(", m.start()))
            sigs = funcs[fname]
            if not any(len(s) == len(args) for s in sigs):
                report(game, name, "%s(%s): %d arguments, the game takes %s" % (
                    fname, ", ".join(args), len(args), "/".join(str(len(s)) for s in sigs)))
                continue
            for sig in sigs:
                if len(sig) != len(args):
                    continue
                for (ptype, pname), arg in zip(sig, args):
                    if not CALLBACK_PARAM.search(pname) or not arg.startswith('"'):
                        continue
                    cb = arg.strip('"').strip()
                    # empty or " ": no callback; "Name"+i: built at run time
                    if not cb or not arg.endswith('"') or '"' in arg[1:-1]:
                        continue
                    glob = cb.startswith("@")
                    cbname = cb.lstrip("@").split("(")[0].strip()
                    # a "global" timer only outlives the map; it runs in the level script too
                    is_global_timer = False
                    where = global_defs if (glob or is_global_timer) else (own | (global_defs if path == global_path else set()))
                    # a level timer made by the global script runs in whatever map is loaded
                    if path == global_path and not (glob or is_global_timer):
                        continue
                    if cbname not in where:
                        near = [d for d in where if d.lower() == cbname.lower()]
                        if near:
                            report(game, name, "%s(%s): callback '%s' differs in case from '%s'" % (
                                fname, ", ".join(args), cb, near[0]))
                            continue
                        report(game, name, "%s(%s): callback '%s' is not defined%s" % (
                            fname, ", ".join(args), cb, " in the global script" if glob or is_global_timer else ""))
                break


def main():
    games = sys.argv[1:] or list(GAMES)
    count = [0]

    def report(game, name, msg):
        count[0] += 1
        print("%s %s: %s" % (game, name, msg))

    for g in games:
        check(g, game_functions(g), report)
    print("%d finding(s)" % count[0])


if __name__ == "__main__":
    main()
