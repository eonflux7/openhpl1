#!/usr/bin/env python3
# This file is part of Overture Remade, licensed under the GNU GPL v3 or
# later. See COPYING.
"""Reference images for renderer changes.

  refshots.py capture OUT_DIR [--maps office,worm] [--size 1280x720]
                              [--set Graphics.Shadows=2 ...]
  refshots.py compare REF_DIR NEW_DIR [--diff-dir DIR]

capture runs the game (OVERTURE_REFSHOTS) in a headless gamescope session
with its own settings file, so the user's settings and screen are left
alone, and with OpenAL Soft's null audio backend, so nothing is heard. It
saves one PNG per map, place and direction, and hpl.log, to OUT_DIR. The graphics settings are fixed (see SETTINGS) so that two
captures differ only by the build.

compare lists the frames that differ between two captures and exits with
1 if any do. A frame counts as changed when more than --max-pixels pixels
differ by more than --tolerance in some channel: a few pixels change from
run to run of the same build (see README). With --diff-dir, an image of the
difference is written for each changed frame, brightened 8x. Frames of
the maps in UNSTABLE_MAPS are listed but do not count.
"""

import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
SOURCE = os.path.dirname(HERE)
DEFAULT_GAME_DIR = os.path.expanduser(
    "~/.local/share/Steam/steamapps/common/Penumbra Overture")

# Section -> key -> value. Refractions are on (off by default in the game)
# so that refracting materials are covered; the noise filter is off, it is
# a random overlay on every frame.
SETTINGS = {
    "Screen": {"Width": "1280", "Height": "720", "FullScreen": "false",
               "Vsync": "false", "Display": "-1"},
    "Graphics": {"Shadows": "0", "ShaderQuality": "3", "PostEffects": "true",
                 "Bloom": "true", "MotionBlur": "false",
                 "MotionBlurAmount": "0.3", "DepthOfField": "true",
                 "Refractions": "true", "Gamma": "1.0", "FSAA": "0",
                 "FOV": "70.0", "TextureSizeLevel": "0", "TextureFilter": "1",
                 "TextureAnisotropy": "1.0", "NoiseFilter": "false",
                 "LimitFPS": "true"},
    "Game": {"FirstStart": "false", "Subtitles": "false",
             "ShowPreMenu": "false", "ShowMenu": "false",
             "ShowIntro": "false", "Difficulty": "2",
             "LanguageFile": "English.lang"},
    "Physics": {"PhysicsAccuracy": "2", "UpdatesPerSec": "60.0"},
    "Sound": {"Volume": "0.0"},
}

# Maps whose frames differ from run to run of the same build: scripted
# sequences change their lights over time, and when they start depends on
# sounds ending, which happens in real time. Their changes are listed but
# do not make compare fail.
UNSTABLE_MAPS = ["level01_19_door_shaft", "level01_20_base_entrance"]


def write_settings(path, settings):
    with open(path, "w") as f:
        for section, keys in settings.items():
            attrs = " ".join('%s="%s"' % kv for kv in keys.items())
            f.write("<%s %s />\n" % (section, attrs))


def run_headless(exe, width, height, cwd, env, log, status_file, timeout):
    """Runs exe in a headless gamescope session, returns its exit code.

    gamescope does not always quit when the game does, and ignores SIGTERM,
    so the game's exit code goes to a file and gamescope is killed once it
    is there."""
    wrapper = '"$0"; echo $? > "%s"' % status_file
    cmd = ["gamescope", "--backend", "headless",
           "-W", width, "-H", height, "-w", width, "-h", height,
           "--", "sh", "-c", wrapper, exe]
    proc = subprocess.Popen(cmd, cwd=cwd, env=env, stdout=log,
                            stderr=subprocess.STDOUT)
    start = time.monotonic()
    exited_at = None
    while proc.poll() is None:
        now = time.monotonic()
        if exited_at is None and os.path.exists(status_file):
            exited_at = now
        if (exited_at is not None and now - exited_at > 5) or now - start > timeout:
            proc.kill()
            proc.wait()
            break
        time.sleep(0.2)

    if not os.path.exists(status_file):
        print("The game did not exit within %d seconds" % timeout)
        return -1
    with open(status_file) as f:
        return int(f.read().strip() or -1)


def capture(args):
    settings = {s: dict(k) for s, k in SETTINGS.items()}
    width, height = args.size.split("x")
    settings["Screen"]["Width"] = width
    settings["Screen"]["Height"] = height
    for item in args.set:
        m = re.fullmatch(r"(\w+)\.(\w+)=(.*)", item)
        if not m:
            sys.exit("--set wants Section.Key=Value, not '%s'" % item)
        settings.setdefault(m.group(1), {})[m.group(2)] = m.group(3)

    exe = os.path.join(SOURCE, args.build, "overture")
    if not os.path.exists(exe):
        sys.exit("No executable at %s" % exe)
    if not os.path.isdir(args.game_dir):
        sys.exit("Game data not found: %s (set --game-dir)" % args.game_dir)
    if shutil.which("gamescope") is None:
        sys.exit("gamescope is needed for the headless session")

    out = os.path.abspath(args.out_dir)
    if os.path.isdir(out) and os.listdir(out):
        sys.exit("%s is not empty" % out)
    os.makedirs(out, exist_ok=True)

    home = tempfile.mkdtemp(prefix="refshots-home-")
    try:
        personal = os.path.join(home, ".frictionalgames", "Penumbra",
                                "OvertureRemade")
        os.makedirs(personal)
        write_settings(os.path.join(personal, "settings.cfg"), settings)

        env = dict(os.environ)
        env["HOME"] = home
        # OpenAL Soft's null backend: sounds play, with the same timing,
        # but nothing is heard.
        env["ALSOFT_DRIVERS"] = "null"
        env["OVERTURE_REFSHOTS"] = args.maps
        env["OVERTURE_REFSHOTS_DIR"] = out
        with open(os.path.join(out, "run.log"), "w") as log:
            ret = run_headless(exe, width, height, args.game_dir, env, log,
                               os.path.join(home, "exit_code"), args.timeout)

        hpl_log = os.path.join(personal, "hpl.log")
        if os.path.exists(hpl_log):
            shutil.copy(hpl_log, os.path.join(out, "hpl.log"))
    finally:
        shutil.rmtree(home, ignore_errors=True)

    lines = []
    if os.path.exists(os.path.join(out, "hpl.log")):
        with open(os.path.join(out, "hpl.log"), errors="replace") as f:
            lines = [l.rstrip() for l in f if "REFSHOTS:" in l]
    for l in lines:
        if "FAIL" in l or "done" in l:
            print(l)
    frames = len([f for f in os.listdir(out) if f.endswith(".png")])
    print("%d frames in %s" % (frames, out))

    done = any("REFSHOTS: done" in l for l in lines)
    failed = any("FAIL" in l for l in lines)
    if ret != 0 or not done or failed:
        print("Capture failed (exit code %d), see %s/run.log and hpl.log"
              % (ret, out))
        return 1
    return 0


def compare(args):
    from PIL import Image, ImageChops

    ref = sorted(f for f in os.listdir(args.ref_dir) if f.endswith(".png"))
    new = sorted(f for f in os.listdir(args.new_dir) if f.endswith(".png"))
    missing = sorted(set(ref) - set(new))
    added = sorted(set(new) - set(ref))
    if args.diff_dir:
        os.makedirs(args.diff_dir, exist_ok=True)

    changed = []
    noise = 0
    for name in sorted(set(ref) & set(new)):
        a = Image.open(os.path.join(args.ref_dir, name)).convert("RGB")
        b = Image.open(os.path.join(args.new_dir, name)).convert("RGB")
        if a.size != b.size:
            changed.append((name, "size %dx%d -> %dx%d" % (a.size + b.size)))
            continue
        diff = ImageChops.difference(a, b)
        if diff.getbbox() is None:
            continue
        # Largest channel difference per pixel
        r, g, bl = diff.split()
        peak = ImageChops.lighter(ImageChops.lighter(r, g), bl)
        hist = peak.histogram()
        over = sum(hist[args.tolerance + 1:])
        if over <= args.max_pixels:
            noise += 1
            continue
        top = max(i for i, n in enumerate(hist) if n)
        mean = sum(i * n for i, n in enumerate(hist)) / (a.size[0] * a.size[1])
        changed.append((name, "%d pixels, max %d, mean %.3f" % (over, top, mean)))
        if args.diff_dir:
            diff.point(lambda v: min(255, v * 8)).save(
                os.path.join(args.diff_dir, name))

    def unstable(name):
        return any(name.startswith(m + "-") for m in UNSTABLE_MAPS)

    loose = [c for c in changed if unstable(c[0])]
    changed = [c for c in changed if not unstable(c[0])]
    for name, what in loose:
        print("unstable %s: %s" % (name, what))
    for name, what in changed:
        print("changed  %s: %s" % (name, what))
    for name in missing:
        print("missing  %s" % name)
    for name in added:
        print("new      %s" % name)
    same = len(set(ref) & set(new)) - len(changed) - len(loose)
    print("%d same (%d within tolerance), %d changed, %d changed on unstable maps, "
          "%d missing, %d new"
          % (same, noise, len(changed), len(loose), len(missing), len(added)))
    return 1 if changed or missing or added else 0


def main():
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = p.add_subparsers(dest="command", required=True)

    c = sub.add_parser("capture", help="run the game headless and save frames")
    c.add_argument("out_dir")
    c.add_argument("--maps", default="1",
                   help="maps whose name contains one of these (comma list), default all")
    c.add_argument("--size", default="1280x720")
    c.add_argument("--set", action="append", default=[], metavar="SECTION.KEY=VALUE",
                   help="override a setting, e.g. Graphics.Shadows=2")
    c.add_argument("--build", default="build", help="build directory, default build")
    c.add_argument("--game-dir", default=os.environ.get("OVERTURE_GAME_DIR", DEFAULT_GAME_DIR))
    c.add_argument("--timeout", type=int, default=1200, help="seconds")

    d = sub.add_parser("compare", help="compare two captures")
    d.add_argument("ref_dir")
    d.add_argument("new_dir")
    d.add_argument("--diff-dir")
    d.add_argument("--tolerance", type=int, default=2,
                   help="channel difference that is ignored, default 2")
    d.add_argument("--max-pixels", type=int, default=64,
                   help="pixels over the tolerance that are ignored, default 64")

    args = p.parse_args()
    sys.exit(capture(args) if args.command == "capture" else compare(args))


if __name__ == "__main__":
    main()
