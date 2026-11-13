#!/usr/bin/env python3
# This file is part of OpenHPL1, licensed under the GNU GPL v3 or
# later. See COPYING.
"""The comment cleanup of main (commits 45b9737, 3841fc1, f3cfb4f, a6ac13a),
for bringing older branches in line before they are merged or rebased.

  comment_cleanup.py [PASS ...]     run passes on the working tree
  comment_cleanup.py --check BASE   compare comment-free code with BASE

Passes, in order (all when none are named):
  separators  //---- and ///// lines; banners that restate C++ sections
  license     18-line GPL notice -> copyright line + SPDX tag
  dead        commented-out code in // lines and /* */ blocks, the notes
              left describing it, and blank lines doubled by the removals

Run on 8c40987 it reproduces main's sources exactly. For a branch made
before the cleanup, from the repository root on a clean tree:

  git merge 8c40987                  # main just before the cleanup; resolve
                                     # the branch's real conflicts here
  python3 tools/comment_cleanup.py
  python3 tools/comment_cleanup.py --check HEAD
  git commit -am "Apply main's comment cleanup"
  git merge main
  git diff --name-only --diff-filter=U | xargs git checkout --ours --
  git add -A && git commit --no-edit

Running the passes without the first merge leaves more conflicts than
before, where main's code moved on since the branch began. The conflicts in
the last merge are only feature hunks next to cleaned lines: the branch
already holds every source change main made since 8c40987, so its side is
right (this holds while main's only source changes since then are the
cleanup commits; afterwards resolve by hand). Docs from main merge normally.

--check strips comments (gcc -fpreprocessed) and whitespace from each
changed file and fails if the code differs from BASE. The dead-code pass is
heuristic: on main its result was reviewed by hand; review the branch's own
new code the same way (git diff -U1, look at notes left next to removals).
"""

import difflib
import re
import subprocess
import sys

SOURCE = re.compile(r"\.(cpp|h|hpp|c|glsl)$")


def tracked(pattern=SOURCE):
	out = subprocess.check_output(["git", "ls-files"]).decode().split("\n")
	return [f for f in out if pattern.search(f) and not f.startswith("vendor/")]


def read(path):
	with open(path, encoding="utf-8", errors="surrogateescape", newline="") as fp:
		return fp.read()


def write(path, text):
	with open(path, "w", encoding="utf-8", errors="surrogateescape", newline="") as fp:
		fp.write(text)


# separators

SEP = re.compile(r"^\s*//\s*[-=/*#~_]{4,}\s*$")
INLINE = re.compile(r"^(\s*)//[/\-=]{3,}\s*([A-Za-z][^/\-=]*?)\s*[/\-=]{3,}\s*$")
BOILER = re.compile(
	r"^((static|public|private|priavte|protected|newton) )*"
	r"(methods?|constructors?|helpers?|callbacks?|save object stuff|"
	r"public serialize class methods)?$", re.I)


def strip_separators(text):
	lines = text.split("\n")
	tail = bool(lines) and lines[-1] == ""
	if tail:
		lines.pop()
	out, mark, i = [], set(), 0
	while i < len(lines):
		line = lines[i]
		if SEP.match(line):
			if (i + 2 < len(lines) and SEP.match(lines[i + 2])
					and lines[i + 1].strip().startswith("//") and not SEP.match(lines[i + 1])):
				label = lines[i + 1].strip()[2:].strip().strip("/").strip()
				if label.startswith("!"):
					out.append(lines[i + 1])
				elif BOILER.match(label) is None:
					out.append(re.match(r"^\s*", lines[i + 1]).group(0) + "// " + label)
				mark.add(len(out))
				i += 3
				continue
			mark.add(len(out))
			i += 1
			continue
		m = INLINE.match(line)
		if m:
			label = m.group(2).strip()
			if BOILER.match(label) is None:
				out.append(m.group(1) + "// " + label)
			else:
				mark.add(len(out))
			i += 1
			continue
		out.append(line)
		i += 1
	res, j = [], 0
	while j < len(out):
		if out[j].strip() == "" or j in mark:
			k, touched = j, j in mark
			while k < len(out) and out[k].strip() == "":
				touched |= k in mark
				k += 1
			touched |= k in mark
			run = out[j:k]
			if touched:
				prev = res[-1].rstrip() if res else ""
				nxt = out[k].strip() if k < len(out) else ""
				keep = 1 if run else 0
				if (prev == "" or nxt.startswith("}") or (nxt == "" and k >= len(out))
						or (prev.endswith("{") and not prev.startswith("namespace"))
						or re.match(r"^\s*(public|private|protected):", prev)):
					keep = 0
				res.extend(run[:keep])
				if k in mark and k < len(out) and out[k].strip() != "":
					mark.discard(k)
				j = k
				continue
			res.extend(run)
			j = k
			continue
		res.append(out[j])
		j += 1
	while tail and res and res[-1].strip() == "":
		res.pop()
	return "\n".join(res) + ("\n" if tail else "")


# license

NOTICE = re.compile(r"""\A/\*
 \* Copyright \(C\) (?P<cr>[^\n]+)
 \*
 \* This file is part of (?P<p>[^\n]+)\.
 \*
(?P<extra>(?: \*[^\n]*\n)*?)(?: \*\n)? \* (?P=p) is free software: you can redistribute it and/or modify
 \* it under the terms of the GNU General Public License as published by
 \* the Free Software Foundation, either version 3 of the License, or
 \* \(at your option\) any later version\.
 \*
 \* (?P=p) is distributed in the hope that it will be useful,
 \* but WITHOUT ANY WARRANTY; without even the implied warranty of
 \* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE\.  See the
 \* GNU General Public License for more details\.
 \*
 \* You should have received a copy of the GNU General Public License
 \* along with (?P=p)\.  If not, see <http://www\.gnu\.org/licenses/>\.
 \*/
""")


def shorten_license(text, path):
	if "General Public License" not in text[:3000]:
		return text
	m = NOTICE.match(text) if "\r\n" not in text else None
	if not m:
		print(f"license: notice not recognised, left as is: {path}", file=sys.stderr)
		return text
	head = f"// Copyright (C) {m.group('cr')}\n// SPDX-License-Identifier: GPL-3.0-or-later\n"
	extra = m.group("extra")
	if extra.strip():
		lines = [re.sub(r"^ \*\s?", "", l) for l in extra.rstrip("\n").split("\n")]
		while lines and not lines[-1].strip():
			lines.pop()
		head += "\n" + "".join(("// " + l).rstrip() + "\n" for l in lines)
	return head + text[m.end():]


# dead code

LINE_COMMENT = re.compile(r"^(\s*)//(?![!<])/*(.*)$")


def looks_like_code(c):
	c = c.strip()
	if c in ("{", "}", "};", "else", "break;", "continue;", "return;"):
		return True
	if re.match(r"^(if|for|while|switch|else if)\s*\(.*\)\s*\{?$", c):
		return True
	if re.match(r"^(else|do)\s*\{?$", c):
		return True
	if re.match(r"^(return|case|delete)\b.*[;:]$", c):
		return True
	if re.match(r"^[A-Za-z_][\w:.]*(->\w+)*\(", c) and (c.count("(") > c.count(")") or c.endswith(";")):
		return True
	if re.match(r"^[A-Za-z_][\w:.]*\s*(\|\||&&|[|&,+])$", c):
		return True
	if "option <name>" in c:
		return False
	if re.match(r"^#\s*(if|ifdef|ifndef|else|elif|endif|include)\b", c):
		return True
	if re.match(r'^tString sStr = ".*;$', c):
		return True
	if re.match(r"^(if|for|while)\s*\(", c) and c.count("(") > c.count(")"):
		return True
	if re.match(r"^[A-Za-z_][\w:<>]*[*&]?\s+[*&]?[a-z]\w*;$", c):
		return True
	if c.endswith(";") or c.endswith("{"):
		body = c.rstrip(";{").strip()
		if (re.search(r"[()=\[\]<>]|->|::|\+\+|--", body)
				and not re.search(r"\b(the|a|is|are|this|that|we|not|but|so|when|and)\s+[a-z]", body)):
			return True
	return False


def strip_dead_lines(text):
	lines = text.split("\n")
	out, i = [], 0
	while i < len(lines):
		if not LINE_COMMENT.match(lines[i]):
			out.append(lines[i])
			i += 1
			continue
		j = i
		while j < len(lines) and LINE_COMMENT.match(lines[j]):
			j += 1
		run = [LINE_COMMENT.match(x).group(2) for x in lines[i:j]]
		flags, depth = [], 0
		for c in run:
			flags.append(depth > 0 or looks_like_code(c))
			if flags[-1]:
				depth = max(0, depth + c.count("(") - c.count(")"))
		blank = [c.strip() == "" for c in run]
		if any(flags) and all(f or b for f, b in zip(flags, blank)):
			if out and out[-1].strip() == "" and j < len(lines) and lines[j].strip() == "":
				j += 1
			i = j
			continue
		out.extend(x for x, f in zip(lines[i:j], flags) if not f)
		i = j
	return "\n".join(out)


def strip_dead_blocks(text):
	lines = text.split("\n")
	blocks, i = [], 0
	while i < len(lines):
		s = lines[i].strip()
		if s.startswith("/*") and not s.startswith(("/**", "/*!")) and i > 2:
			j = i
			while j < len(lines) and "*/" not in (lines[j] if j > i else lines[j][lines[j].index("/*") + 2:]):
				j += 1
			if j >= len(lines):
				break
			if not lines[j].rstrip().endswith("*/"):
				i = j + 1
				continue
			body = [re.sub(r"^\s*/\*|\*/\s*$", "", x).strip() for x in lines[i:j + 1]]
			body = [b for b in body if b]
			code = sum(looks_like_code(b) for b in body)
			if body and code >= max(1, 0.5 * len(body)):
				blocks.append((i, j))
			i = j + 1
			continue
		i += 1
	drop = set()
	for a, b in blocks:
		drop.update(range(a, b + 1))
		after = lines[b + 1].strip() if b + 1 < len(lines) else ""
		if (a > 0 and lines[a - 1].strip().startswith("//")
				and not (a > 1 and lines[a - 2].strip().startswith("//"))
				and (after == "" or after.startswith("}"))):
			drop.add(a - 1)
		lo = min(x for x in drop if x >= a - 1)
		if lo > 0 and b + 1 < len(lines) and lines[lo - 1].strip() == "" and lines[b + 1].strip() == "":
			drop.add(b + 1)
	return "\n".join(l for k, l in enumerate(lines) if k not in drop)


# Notes on main that only described code the dead pass removed. A True flag
# drops the note only where a blank line or "}" follows it, so the same text
# in front of live code stays.
ORPHANS = {
	"engine/include/graphics/ParticleEmitter.h": ["// ---"],
	"engine/sources/game/Game.cpp": [
		"//If not making a single rendering is better to use gpu and",
		"//cpu at the same time and make query checks etc after logic update.",
		"//If any delete has occured in the update this might crash. so skip it for now."],
	"engine/sources/impl/MeshLoaderCollada.cpp": [
		"//Old version , trying to skip because bounding box calc for this is far from",
		"//optimal", "//Shouldn't be needed any more.", "//The streams will be the entity copies.",
		"// Test anim data"],
	"engine/sources/impl/MeshLoaderColladaHelpers.cpp": ["//TEMP: use the one in geometry directly."],
	"engine/sources/impl/MeshLoaderColladaLoader.cpp": ["// Should not be needed."],
	"engine/sources/math/Math.cpp": ["//Check Sphere collision.", "//Not needed I think."],
	"engine/sources/physics/CharacterBody.cpp": [
		"//debug:", "//Make sure the speed doesn't change direction.",
		"//Make sure the speed doesn't change direction.Should not be needed.",
		"//Totally unrealistic force add",
		"//Skip this for now and add better with more object specific stuff"],
	"engine/sources/physics/PhysicsWorld.cpp": ["//quick fix for oscillation, might skip"],
	"engine/sources/scene/MeshEntity.cpp": ["//TODO: bad, bad bad...redo animation storing."],
	"engine/sources/scene/PortalContainer.cpp": ["//Quick fix for thin stuff. (not working it seems)"],
	"engine/sources/scene/Scene.cpp": ["//S"],
	"engine/sources/scene/World3D.cpp": ["//Debug:", "//Not so good to do it here.."],
	"engine/sources/sound/SoundHandler.cpp": [
		"//Needed?", "//\t\t\tcVector3f vPos = cMath::MatrixMul(mpLowLevelSound->GetListenerMatrix(),"],
	"game/core/MapHandler.cpp": ["//remove all local timer"],
	"game/enemies/GameEnemy.cpp": ["//Setup debug"],
	"game/enemies/GameEnemy_Worm.cpp": ["//Used for more stiff worms"],
	"game/player/Player.cpp": [
		"//DEBUG: misc", "//DEBUG: Mouse", "//\t\t\t\t\t\t\t\tpEntry->mpSound->GetPriority(),"],
	"game/player/PlayerHelper.cpp": [
		"//cMath::MatrixInverse(pCam->GetViewMatrix().GetRotation()),", "//Get forward"],
	"game/player/PlayerState_InteractHaptX.cpp": [("//Set cross hair image.", True)],
	"game/player/PlayerState_Weapon.cpp": [
		"//Call entity",
		"//Quick fix but works, the OnLeave of the previous states sets an old cross hair state.",
		"//Try not doing this."],
	"game/player/PlayerState_WeaponHaptX.cpp": [
		"//Call entity",
		"//Quick fix but works, the OnLeave of the previous states sets an old cross hair state."],
	"game/systems/AttackHandler.cpp": ["//Update map handler so all stuff that should be broken is."],
	"game/ui/DemoEndText.cpp": ["//Load fonts"],
	"game/ui/Inventory.cpp": ["//Draw backpack and hands"],
	"game/ui/MapLoadText.cpp": ["//Load graphics"],
}


def drop_orphans(text, path):
	entries = ORPHANS.get(path)
	if not entries:
		return text
	always = {e.strip() for e in entries if isinstance(e, str)}
	if_dangling = {e[0].strip() for e in entries if not isinstance(e, str)}
	lines = text.split("\n")
	out = []
	for k, line in enumerate(lines):
		s = line.strip()
		if s in always:
			continue
		if s in if_dangling:
			nxt = lines[k + 1].strip() if k + 1 < len(lines) else ""
			if nxt == "" or nxt.startswith("}"):
				continue
		out.append(line)
	return "\n".join(out)


def collapse_blanks(before, text):
	"""Drop blank lines that the removals since `before` left doubled."""
	old = before.split("\n")
	new = text.split("\n")
	while True:
		sm = difflib.SequenceMatcher(None, old, new, autojunk=False)
		drop = None
		for tag, _, _, j1, _ in sm.get_opcodes():
			if tag != "delete" or not 0 < j1 < len(new):
				continue
			if new[j1 - 1].strip() == "" and new[j1].strip() == "":
				drop = j1
				break
			if (new[j1 - 1].strip() == "" and new[j1].strip().startswith("}")
					and j1 >= 2 and new[j1 - 2].strip().endswith("{")):
				drop = j1 - 1
				break
		if drop is None:
			return "\n".join(new)
		del new[drop]


def strip_dead(text, path):
	before = text
	text = strip_dead_lines(text)
	text = strip_dead_blocks(text)
	text = drop_orphans(text, path)
	text = text.replace("\t\t\tmpLowLevelSound\t= hplNew", "\t\tmpLowLevelSound\t= hplNew")
	text = text.replace("\t\t\tiLowLevelSound*\tmpLowLevelSound;", "\t\tiLowLevelSound*\tmpLowLevelSound;")
	return collapse_blanks(before, text) if text != before else text


PASSES = {
	"separators": lambda text, path: strip_separators(text),
	"license": shorten_license,
	"dead": strip_dead,
}


def run(names):
	changed = set()
	for name in names:
		for path in tracked():
			text = read(path)
			new = PASSES[name](text, path)
			if new != text:
				write(path, new)
				changed.add(path)
	print(f"{len(changed)} files changed")


def normalised(text):
	out = subprocess.run(["gcc", "-x", "c++", "-fpreprocessed", "-E", "-P", "-w", "-"],
		input=text.encode("utf-8", "surrogateescape"), capture_output=True)
	return re.sub(rb"\s+", b"", out.stdout)


def check(base):
	files = subprocess.check_output(["git", "diff", "--name-only", base, "--"]).decode().split()
	files = [f for f in files if SOURCE.search(f)]
	bad = []
	for path in files:
		try:
			old = subprocess.check_output(["git", "show", f"{base}:{path}"], stderr=subprocess.DEVNULL)
		except subprocess.CalledProcessError:
			continue
		try:
			new = read(path)
		except FileNotFoundError:
			continue
		if normalised(old.decode("utf-8", "surrogateescape")) != normalised(new):
			bad.append(path)
	print(f"checked {len(files)} files, {len(bad)} with code changes")
	for path in bad:
		print("  " + path)
	return 1 if bad else 0


def main(argv):
	if argv[:1] == ["--check"] and len(argv) == 2:
		return check(argv[1])
	names = argv or list(PASSES)
	unknown = [n for n in names if n not in PASSES]
	if unknown:
		print(__doc__)
		return 2
	run(names)
	return 0


if __name__ == "__main__":
	sys.exit(main(sys.argv[1:]))
