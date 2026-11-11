#!/usr/bin/env python3
# This file is part of OpenHPL1, licensed under the GNU GPL v3 or
# later. See COPYING.
"""Frame-time benchmark: GPU sections and main-thread CPU per viewpoint.

  framebench.py list
  framebench.py run OUT_DIR [--features ao,lights|all] [--survey] [--build DIR]
                    [--frames N] [--size WxH] [--set SECTION.KEY=VALUE]
  framebench.py show OUT_DIR [--top N]
  framebench.py compare BEFORE_DIR AFTER_DIR

Each feature has one viewpoint (tools/refviews.txt) where it costs the most,
found with an all-maps survey (docs/fix-opti-run.md). run loads only the maps
those viewpoints need, holds the camera there, lets 120 frames settle and
measures --frames more. --survey measures every viewpoint of every map
instead (about 20 minutes).

The game runs through tools/levelview.py (hidden window, isolated profile,
null audio) with the reference-capture graphics settings, vsync off and one
logic step per frame, so each frame shows the same thing from run to run.
Each viewpoint is measured twice:

- GPU: frames drawn as fast as they can be (option pace 0), so the GPU keeps
  its clocks up (at 60 Hz it is lightly loaded and its clocks, and timings,
  wander). Section times (OPENHPL_GPU_LOG) are medians over the frames;
  "frame" is the median uncapped frame time.
- CPU: at 60 Hz (option pace 1), where the main thread does not wait for the
  GPU. tools/framebench_sampler.c, preloaded into the game, samples its stack
  at 1 kHz and times each swap; busy time leaves out the pacing sleep.

Numbers are for the machine they were taken on; compare runs of the same
machine, size and settings. Linux only (needs gcc and addr2line).
"""

import argparse
import bisect
import collections
import json
import os
import re
import shutil
import signal
import struct
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
SOURCE = os.path.dirname(HERE)
sys.path.insert(0, HERE)
import levelview  # noqa: E402

# feature -> (map, viewpoint, what is measured, description). A measure is a
# GPU section path ("gpu:/World/Lights") or a function whose stack time on
# the main thread is counted ("cpu:hpl::cRenderer3D::RenderWorld").
FEATURES = {
    "ao": ("level01_07_lead_mine_shafts_a", "view02", ["gpu:/World/Ambient occlusion"],
           "ambient occlusion"),
    "lights": ("level00_01_boat_cabin", "view01", ["gpu:/World/Lights", "-gpu:/World/Lights/Shadow maps"],
               "light passes without their shadow maps"),
    "shadows": ("level01_07_lead_mine_shafts_c", "view02",
                ["gpu:/World/Lights/Shadow maps"], "shadow-map rendering (GPU)"),
    "shadowcpu": ("level01_07_lead_mine_shafts_c", "view02",
                  ["cpu:hpl::cRenderer3D::RenderShadowMap"], "shadow casters and maps (CPU)"),
    "transparent": ("level01_01_outside", "view03", ["gpu:/World/Transparent"],
                    "transparent objects, halos, particles"),
    "z": ("level01_04_old_storage", "view01", ["gpu:/World/Z"], "depth and ambient pass"),
    "bloom": ("level01_09_generator_room", "view02", ["gpu:/Post effects/Bloom"], "bloom"),
    "smaa": ("level01_14_refinery", "view03", ["gpu:/Scene output/SMAA"], "SMAA"),
    "submit": ("level01_15_icecave", "view03", ["cpu:hpl::cRenderer3D::RenderWorld"],
               "draw submission (CPU)"),
    "visibility": ("level01_14_refinery", "view01", ["cpu:hpl::cRenderer3D::UpdateRenderList"],
                   "render list and visibility (CPU)"),
    "physics": ("level01_13_b_iron_storage_tunnels", "view02", ["cpu:hpl::iPhysicsWorld::Update"],
                "physics (CPU)"),
}

# Main-thread time by kind, the first that a sample's stack contains
CPU_KINDS = [
    ("physics", "hpl::iPhysicsWorld::Update"),
    ("game update", "hpl::cUpdater::Update"),
    ("visibility", "hpl::cRenderer3D::UpdateRenderList"),
    ("shadow maps", "hpl::cRenderer3D::RenderShadowMap"),
    ("render world", "hpl::cRenderer3D::RenderWorld"),
    ("post effects", "hpl::cRendererPostEffects::Render"),
    ("scene, other", "hpl::cScene::Render"),
    ("swap", "hpl::cLowLevelGraphicsSDL::SwapBuffers"),
]
IDLE = ("hpl::cLowLevelSystemSDL::SleepPrecise", "hpl::cLogicTimer::WantUpdate")
SETTLE_FRAMES = 120


def build_sampler(build):
    src = os.path.join(HERE, "framebench_sampler.c")
    lib = os.path.join(build, "framebench_sampler.so")
    if not os.path.exists(lib) or os.path.getmtime(lib) < os.path.getmtime(src):
        subprocess.check_call(["gcc", "-O2", "-shared", "-fPIC", "-o", lib, src, "-ldl"])
    return lib


def measure(args):
    out = os.path.abspath(args.out_dir)
    if os.path.exists(out):
        shutil.rmtree(out)
    os.makedirs(out)
    build = args.build if os.path.isabs(args.build) else os.path.join(SOURCE, args.build)
    lib = build_sampler(build)

    if args.survey:
        plan = None
    else:
        names = list(FEATURES) if args.features == "all" else args.features.split(",")
        for n in names:
            if n not in FEATURES:
                sys.exit("Unknown feature '%s' (framebench.py list)" % n)
        plan = collections.OrderedDict()
        for n in names:
            m, v = FEATURES[n][0], FEATURES[n][1]
            plan.setdefault(m, [])
            if v not in plan[m]:
                plan[m].append(v)

    os.environ["LD_PRELOAD"] = lib
    os.environ["FRAMEBENCH_OUT"] = out
    os.environ["OPENHPL_GPU_LOG"] = os.path.join(out, "gpu.log")
    session = os.path.join(out, "session")
    start = ["start", "--hidden", "--fresh", "--session", session, "--build", build,
             "--size", args.size, "--set", "Screen.Vsync=false"]
    for s in args.set:
        start += ["--set", s]
    try:
        if levelview.main(start) != 0:
            sys.exit("The game did not start")
    finally:
        for name in ("LD_PRELOAD", "FRAMEBENCH_OUT", "OPENHPL_GPU_LOG"):
            del os.environ[name]

    game = levelview.Session(session)
    pid = game.info["pid"]

    def send(command):
        ok, lines = game.send(command, timeout=600)
        if not ok:
            print("  %s: %s" % (command, " ".join(lines)))
        return ok, lines

    if plan is None:
        ok, lines = send("maps")
        plan = collections.OrderedDict((l.split()[0], None) for l in lines if l.strip())

    windows = []
    try:
        for m, views in plan.items():
            if not send("map " + m)[0]:
                continue
            if views is None:
                ok, lines = send("views")
                names = [l.split()[0] for l in lines if l.strip() and not l.startswith(" ")]
                views = [v for v in names if v.startswith("view")] or names[:3]
            for v in views:
                if not send("view " + v)[0]:
                    continue
                # GPU: frames as fast as they can be drawn, one step each,
                # so the GPU keeps its clocks up
                send("option pace 0")
                send("wait %d" % SETTLE_FRAMES)
                g0 = time.monotonic()
                send("wait %d" % args.frames)
                g1 = time.monotonic()
                # CPU: at 60 Hz, where the main thread does not wait for
                # the GPU in the driver
                send("option pace 1")
                send("wait %d" % SETTLE_FRAMES)
                os.kill(pid, signal.SIGUSR1)  # the sampler takes stacks
                c0 = time.monotonic()
                send("wait %d" % args.frames)
                c1 = time.monotonic()
                os.kill(pid, signal.SIGUSR2)
                windows.append({"map": m, "view": v, "gpu": [g0, g1], "cpu": [c0, c1]})
                print("  %s %s" % (m, v), flush=True)
    finally:
        levelview.main(["stop", "--session", session])
    with open(os.path.join(out, "windows.json"), "w") as f:
        json.dump({"size": args.size, "set": args.set, "frames": args.frames,
                   "windows": windows}, f, indent=1)
    results = analyse(out)
    print_results(results, 8)
    return 0


def analyse(out):
    with open(os.path.join(out, "windows.json")) as f:
        info = json.load(f)
    swaps = [l.split() for l in open(os.path.join(out, "swaps.txt"))]
    swap_number = [int(s[0]) for s in swaps]
    swap_end = [float(s[2]) for s in swaps]

    gpu = {}
    if os.path.exists(os.path.join(out, "gpu.log")):
        for line in open(os.path.join(out, "gpu.log")):
            p = line.rstrip("\n").split("\t")
            sections = {}
            for kv in p[5:]:
                k, v = kv.rsplit("=", 1)
                sections[k] = float(v)
            gpu[int(p[0])] = (float(p[1]), int(p[2]), int(p[3]), sections)

    modules = []
    for line in open(os.path.join(out, "maps.txt")):
        p = line.split()
        if len(p) >= 6 and "x" in p[1]:
            lo, hi = (int(x, 16) for x in p[0].split("-"))
            modules.append((lo, hi, int(p[2], 16), p[5]))
    modules.sort()

    samples = []
    raw = open(os.path.join(out, "samples.bin"), "rb").read()
    o = 0
    while o + 8 <= len(raw):
        frame, n = struct.unpack_from("II", raw, o)
        o += 8
        samples.append((frame, struct.unpack_from("%dQ" % n, raw, o)))
        o += 8 * n

    names = symbolise({a for _, stack in samples for a in stack}, modules)
    by_frame = collections.defaultdict(list)
    for frame, stack in samples:
        # The first two entries are the signal handler and its return
        stack = [names[a] for a in stack[2:]]
        if stack and not any(f in IDLE for f in stack[:10]):
            by_frame[frame].append(stack)

    results = []
    def window(span):
        lo = bisect.bisect_left(swap_end, span[0])
        hi = bisect.bisect_left(swap_end, span[1])
        return range(lo + 1, hi)

    for w in info["windows"]:
        gpu_range, cpu_range = window(w["gpu"]), window(w["cpu"])
        if len(gpu_range) < 10 or len(cpu_range) < 10:
            continue
        times = [(swap_end[k] - swap_end[k - 1]) * 1000 for k in gpu_range]
        frames = [swap_number[k] for k in gpu_range]
        # The profiler numbers frames from 1, the sampler from 0
        frame_gpu = [gpu[f + 1] for f in frames if f + 1 in gpu]
        # GPU times are medians over the frames: a frame that waited on
        # something else (an upload, the compositor) does not move them
        per_section = collections.defaultdict(list)
        for g in frame_gpu:
            for k, v in g[3].items():
                per_section[k].append(v)
        sections = {k: pct(v + [0.0] * (len(frame_gpu) - len(v)), 0.5)
                    for k, v in per_section.items()}
        busy_gpu = [g[0] - g[3].get("/Present", 0) for g in frame_gpu]

        kinds = collections.Counter()
        functions = collections.Counter()
        busy = 0
        frames = [swap_number[k] for k in cpu_range]
        for f in frames:
            for stack in by_frame.get(f, []):
                busy += 1
                unique = set(stack)
                functions.update(unique)
                kinds[next((k for k, fn in CPU_KINDS if fn in unique), "other")] += 1
        n = len(frames)
        results.append({
            "map": w["map"], "view": w["view"], "frames": n,
            "frame_ms": pct(times, 0.5), "frame_p99": pct(times, 0.99),
            "cpu_ms": busy / n,
            "gpu_ms": pct(busy_gpu, 0.5), "gpu_p99": pct(busy_gpu, 0.99),
            "gpu_frames": len(frame_gpu),
            "draw_calls": sum(g[1] for g in frame_gpu) / max(1, len(frame_gpu)),
            "triangles": sum(g[2] for g in frame_gpu) / max(1, len(frame_gpu)),
            "gpu_sections": dict(sections),
            "cpu_kinds": {k: v / n for k, v in kinds.items()},
            "cpu_functions": {k: v / n for k, v in functions.most_common(60)},
        })
    with open(os.path.join(out, "results.json"), "w") as f:
        json.dump(results, f, indent=1)
    return results


def symbolise(addresses, modules):
    def locate(a):
        i = bisect.bisect_right(modules, (a, 1 << 64)) - 1
        if i >= 0 and modules[i][0] <= a < modules[i][1]:
            lo, hi, offset, path = modules[i]
            return path, a - lo + offset
        return "?", a

    names = {}
    per_file = collections.defaultdict(list)
    for a in addresses:
        path, offset = locate(a)
        per_file[path].append((a, offset))
    for path, items in per_file.items():
        if path.endswith("/openhpl1"):
            # Return addresses point after the call
            proc = subprocess.run(["addr2line", "-f", "-C", "-e", path] +
                                  [hex(off - 1) for _, off in items], capture_output=True, text=True)
            lines = proc.stdout.splitlines()
            for k, (a, _) in enumerate(items):
                name = lines[2 * k] if 2 * k < len(lines) else "?"
                names[a] = re.sub(r"\(.*", "", name)
        else:
            for a, _ in items:
                names[a] = "[%s]" % os.path.basename(path)
    return names


def pct(values, q):
    values = sorted(values)
    return values[min(len(values) - 1, int(q * len(values)))] if values else 0


def feature_value(result, measures):
    total = 0.0
    for m in measures:
        sign = -1 if m.startswith("-") else 1
        kind, name = m.lstrip("-").split(":", 1)
        table = result["gpu_sections"] if kind == "gpu" else result["cpu_functions"]
        total += sign * table.get(name, 0.0)
    return total


def features_of(result):
    return [(n, f) for n, f in FEATURES.items() if f[0] == result["map"] and f[1] == result["view"]]


def load(out):
    path = os.path.join(out, "results.json")
    if not os.path.exists(path):
        return analyse(out)
    with open(path) as f:
        return json.load(f)


def print_results(results, top):
    print("\n%-34s %-7s %6s %6s %6s %6s %5s" % ("viewpoint", "", "frame", "CPU", "GPU", "GPUp99", "draws"))
    for r in results:
        print("%-34s %-7s %6.2f %6.2f %6.2f %6.2f %5.0f" % (r["map"][:34], r["view"], r["frame_ms"],
              r["cpu_ms"], r["gpu_ms"], r["gpu_p99"], r["draw_calls"]))
        for name, f in features_of(r):
            print("    feature %-12s %6.3f ms  (%s)" % (name, feature_value(r, f[2]), f[3]))
    if top <= 0:
        return
    for r in results:
        print("\n%s %s" % (r["map"], r["view"]))
        print("  GPU ms: " + ", ".join("%s %.2f" % (k, v) for k, v in
              sorted(r["gpu_sections"].items(), key=lambda kv: -kv[1])
              if v >= 0.05 and k != "/Present"))
        print("  CPU ms: " + ", ".join("%s %.2f" % kv for kv in
              sorted(r["cpu_kinds"].items(), key=lambda kv: -kv[1])))
        hpl = [(k, v) for k, v in r["cpu_functions"].items()
               if not k.startswith(("[", "main", "_start", "hplMain", "hpl::cGame::Run"))]
        print("  CPU functions (inclusive): " + ", ".join("%s %.2f" % (k.replace("hpl::", ""), v)
                                                         for k, v in hpl[:top]))


def show(args):
    print_results(load(args.out_dir), args.top)
    return 0


def compare(args):
    before = {(r["map"], r["view"]): r for r in load(args.before)}
    after = {(r["map"], r["view"]): r for r in load(args.after)}
    print("%-34s %-7s %-12s %8s %8s %8s" % ("viewpoint", "", "", "before", "after", "change"))

    def row(key, label, a, b):
        change = (b - a) / a * 100 if a > 0.001 else 0.0
        print("%-34s %-7s %-12s %8.3f %8.3f %+7.1f%%" % (key[0][:34], key[1], label, a, b, change))

    for key in before:
        if key not in after:
            continue
        a, b = before[key], after[key]
        for name, f in features_of(a):
            row(key, name, feature_value(a, f[2]), feature_value(b, f[2]))
        row(key, "CPU", a["cpu_ms"], b["cpu_ms"])
        row(key, "GPU", a["gpu_ms"], b["gpu_ms"])
        row(key, "GPU p99", a["gpu_p99"], b["gpu_p99"])
        if abs(a["draw_calls"] - b["draw_calls"]) > 0.5:
            row(key, "draw calls", a["draw_calls"], b["draw_calls"])
    return 0


def list_features(args):
    for name, (m, v, measures, text) in FEATURES.items():
        print("%-12s %-34s %-7s %s" % (name, m, v, text))
    return 0


def main(argv=None):
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = p.add_subparsers(dest="command", required=True)
    sub.add_parser("list", help="the features and their viewpoints")
    r = sub.add_parser("run", help="run the game and measure")
    r.add_argument("out_dir")
    r.add_argument("--features", default="all", help="comma list, default all")
    r.add_argument("--survey", action="store_true", help="every viewpoint of every map")
    r.add_argument("--build", default="build")
    r.add_argument("--frames", type=int, default=300, help="frames measured per viewpoint")
    r.add_argument("--size", default="1920x1080")
    r.add_argument("--set", action="append", default=[], metavar="SECTION.KEY=VALUE")
    s = sub.add_parser("show", help="print a run's results")
    s.add_argument("out_dir")
    s.add_argument("--top", type=int, default=8, help="functions listed per viewpoint, 0 for none")
    c = sub.add_parser("compare", help="feature, CPU and GPU times of two runs")
    c.add_argument("before")
    c.add_argument("after")
    args = p.parse_args(argv)
    if args.command == "run":
        return measure(args)
    if args.command == "show":
        return show(args)
    if args.command == "compare":
        return compare(args)
    return list_features(args)


if __name__ == "__main__":
    sys.exit(main())
