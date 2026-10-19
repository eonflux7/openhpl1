#!/usr/bin/env python3
# This file is part of OpenHPL1, licensed under the GNU GPL v3 or
# later. See COPYING.
"""Reference images for renderer changes.

  refshots.py capture OUT_DIR [--maps office,worm] [--size 1280x720]
                              [--set Graphics.Shadows=2 ...]
  refshots.py effects OUT_DIR [--size 1280x720] [--build build-asan]
  refshots.py compare REF_DIR NEW_DIR [--diff-dir DIR]

capture runs the game (OPENHPL_REFSHOTS) in a headless gamescope session
with its own settings file, so the user's settings and screen are left
alone, and with OpenAL Soft's null audio backend, so nothing is heard. It
saves one PNG per map, place and direction, one per 2D screen (2d-*.png),
and hpl.log, to OUT_DIR. The graphics settings are fixed (see SETTINGS) so
that two captures differ only by the build.

With --windows, run from WSL, the Windows build in --build is run instead
(--build may be absolute, e.g. a build-windows on a Windows drive), with
its window hidden in place of the gamescope session.

effects captures the player feedback in the effects lab,
checks its live triggers, and verifies white/black endpoints and the neutral
output. It uses the same isolated profile and headless session as capture.

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
# so that refracting materials are covered, and so are shadows (0: all),
# ambient occlusion, SMAA and multisampling; the noise filter is off, it is
# a random overlay on every frame.
SETTINGS = {
    "Screen": {"Width": "1280", "Height": "720", "FullScreen": "false",
               "Vsync": "false", "Display": "-1"},
    "Graphics": {"Shadows": "0", "ShaderQuality": "3", "PostEffects": "true",
                 "Bloom": "true", "MotionBlur": "false", "DepthOfField": "true",
                 "Refractions": "true", "Gamma": "1.0", "FSAA": "4",
                 "FOV": "70.0", "TextureSizeLevel": "0", "TextureFilter": "1",
                 "TextureAnisotropy": "1.0", "SMAA": "true",
                 "AmbientOcclusion": "true",
                 "NoiseFilter": "false", "LimitFPS": "true"},
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


def run_windows(exe, cwd, env, log, timeout):
    """Runs the Windows build from WSL, returns its exit code. The window
    is hidden (OPENHPL_HIDDEN), and the variables of env the game reads
    are passed on to Windows through WSLENV."""
    names = [n for n in env if n.startswith("OPENHPL_") or n == "ALSOFT_DRIVERS"]
    env = dict(env)
    env["WSLENV"] = ":".join(filter(None, [env.get("WSLENV")] + names))
    try:
        return subprocess.run([exe], cwd=cwd, env=env, stdout=log,
                              stderr=subprocess.STDOUT, timeout=timeout).returncode
    except subprocess.TimeoutExpired:
        print("The game did not exit within %d seconds" % timeout)
        return -1


def windows_path(path):
    return subprocess.check_output(["wslpath", "-w", path], text=True).strip()


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

    exe = os.path.join(SOURCE, args.build,
                       "openhpl1.exe" if args.windows else "openhpl1")
    if not os.path.exists(exe):
        sys.exit("No executable at %s" % exe)
    if not os.path.isdir(args.game_dir):
        sys.exit("Game data not found: %s (set --game-dir)" % args.game_dir)
    if not args.windows and shutil.which("gamescope") is None:
        sys.exit("gamescope is needed for the headless session")

    out = os.path.abspath(args.out_dir)
    if os.path.isdir(out) and os.listdir(out):
        sys.exit("%s is not empty" % out)
    os.makedirs(out, exist_ok=True)

    # The Windows build's profile goes next to it, on the Windows drive
    home = tempfile.mkdtemp(prefix="refshots-home-",
                            dir=os.path.dirname(exe) if args.windows else None)
    try:
        if args.windows:
            personal = os.path.join(home, "Penumbra Overture", "OvertureRemade")
        else:
            personal = os.path.join(home, ".frictionalgames", "Penumbra",
                                    "OvertureRemade")
        os.makedirs(personal)
        write_settings(os.path.join(personal, "settings.cfg"), settings)

        env = dict(os.environ)
        env["HOME"] = home
        game_path = windows_path if args.windows else (lambda p: p)
        if args.windows:
            env["OPENHPL_PROFILE_ROOT"] = windows_path(home)
            env["OPENHPL_HIDDEN"] = "1"
        # OpenAL Soft's null backend: sounds play, with the same timing,
        # but nothing is heard.
        env["ALSOFT_DRIVERS"] = "null"
        if args.command == "effects":
            env.pop("OPENHPL_REFSHOTS", None)
            env["OPENHPL_MAP"] = "effectslab:link01"
            env["OPENHPL_EFFECTS_CAPTURE"] = game_path(out)
        else:
            env["OPENHPL_REFSHOTS"] = args.maps
            env["OPENHPL_REFSHOTS_DIR"] = game_path(out)
        with open(os.path.join(out, "run.log"), "w") as log:
            if args.windows:
                ret = run_windows(exe, args.game_dir, env, log, args.timeout)
            else:
                ret = run_headless(exe, width, height, args.game_dir, env, log,
                                   os.path.join(home, "exit_code"), args.timeout)

        hpl_log = os.path.join(personal, "hpl.log")
        if os.path.exists(hpl_log):
            shutil.copy(hpl_log, os.path.join(out, "hpl.log"))
    finally:
        shutil.rmtree(home, ignore_errors=True)

    label = "EFFECTSHOTS:" if args.command == "effects" else "REFSHOTS:"
    lines = []
    if os.path.exists(os.path.join(out, "hpl.log")):
        with open(os.path.join(out, "hpl.log"), errors="replace") as f:
            lines = [l.rstrip() for l in f if label in l]
    for l in lines:
        if "FAIL" in l or "done" in l:
            print(l)
    frames = len([f for f in os.listdir(out) if f.endswith(".png")])
    print("%d frames in %s" % (frames, out))

    done = any(label + " done" in l for l in lines)
    failed = any("FAIL" in l for l in lines)
    if ret != 0 or not done or failed:
        print("Capture failed (exit code %d), see %s/run.log and hpl.log"
              % (ret, out))
        return 1
    if args.command == "effects":
        return check_effects(out)
    return 0


def check_effects(out):
    """Check visible feedback, neutral output, and opaque endpoints."""
    from PIL import Image, ImageChops, ImageStat

    names = ["neutral", "adaptation", "low-health", "double-vision", "blood", "ice", "panic",
             "flash", "save", "death", "underwater", "combined"]
    expected = ["improved-" + name + ".png" for name in names]
    expected += ["improved-flash-full.png", "improved-save-full.png",
                 "improved-death-full.png", "improved-combined-zero.png",
                 "improved-panic-trough.png", "improved-panic-peak.png",
                 "improved-blood-seed-a.png", "improved-blood-seed-b.png",
                 "improved-blood-seed-a-repeat.png", "actual-underwater.png",
                 "improved-double-vision-trough.png", "improved-double-vision-peak.png",
                 "actual-low-health.png", "actual-health-between-beats.png",
                 "actual-adaptation.png", "effects-panel.png", "actual-save.png"]
    missing = [name for name in expected if not os.path.isfile(os.path.join(out, name))]
    if missing:
        print("Effects check failed: missing " + ", ".join(missing))
        return 1

    def frame(name):
        with Image.open(os.path.join(out, name + ".png")) as source:
            return source.convert("RGB")

    failures = []
    neutral = frame("improved-neutral")
    diff = ImageChops.difference(neutral, frame("improved-combined-zero"))
    if max(ImageStat.Stat(diff).mean) > 0.05:
        failures.append("improved-combined-zero changes the neutral picture")
    for name in names[1:]:
        diff = ImageChops.difference(neutral, frame("improved-" + name))
        if max(ImageStat.Stat(diff).mean) < 0.1:
            failures.append(name + " has no visible feedback")
    splashes = ImageChops.difference(frame("improved-blood-seed-a"), frame("improved-blood-seed-b"))
    if max(ImageStat.Stat(splashes).mean) < 0.1:
        failures.append("blood pattern does not change between hits")
    repeat = ImageChops.difference(frame("improved-blood-seed-a"), frame("improved-blood-seed-a-repeat"))
    if max(ImageStat.Stat(repeat).mean) > 0.05:
        failures.append("blood pattern does not stay still for the same hit")
    pulse = ImageChops.difference(frame("improved-panic-trough"), frame("improved-panic-peak"))
    if max(ImageStat.Stat(pulse).mean) < 0.1:
        failures.append("panic pulse does not change the picture")
    for phase in ("trough", "peak"):
        ghost = ImageChops.difference(neutral, frame("improved-double-vision-" + phase))
        if max(ImageStat.Stat(ghost).mean) < 0.1:
            failures.append("double vision disappears at the pulse " + phase)
    vision_pulse = ImageChops.difference(frame("improved-double-vision-trough"), frame("improved-double-vision-peak"))
    if max(ImageStat.Stat(vision_pulse).mean) < 0.1:
        failures.append("double vision does not gently pulse")
    for name in ("improved-flash-full", "improved-save-full"):
        if min(lo for lo, hi in frame(name).getextrema()) < 254:
            failures.append(name + " does not reach white")
    if max(hi for lo, hi in frame("improved-death-full").getextrema()) > 1:
        failures.append("death does not reach black")
    for failure in failures:
        print("Effects check failed: " + failure)
    if not failures:
        print("Effects pixels: neutral output, visible feedback and opaque endpoints passed")
    return 1 if failures else 0


def compare(args):
    from PIL import Image, ImageChops

    ref = sorted(f for f in os.listdir(args.ref_dir) if f.endswith(".png"))
    new = sorted(f for f in os.listdir(args.new_dir) if f.endswith(".png"))
    if not ref or not new:
        print("Comparison failed: both directories must contain PNG frames")
        return 1
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

    common = argparse.ArgumentParser(add_help=False)
    common.add_argument("out_dir")
    common.add_argument("--size", default="1280x720")
    common.add_argument("--set", action="append", default=[], metavar="SECTION.KEY=VALUE",
                        help="override a setting, e.g. Graphics.Shadows=2")
    common.add_argument("--build", default="build", help="build directory, default build")
    common.add_argument("--game-dir", default=os.environ.get("OPENHPL_GAME_DIR", DEFAULT_GAME_DIR))
    common.add_argument("--timeout", type=int, default=1200, help="seconds")
    common.add_argument("--windows", action="store_true",
                        help="run the Windows build (openhpl1.exe in --build) from WSL")

    c = sub.add_parser("capture", parents=[common], help="run the game headless and save frames")
    c.add_argument("--maps", default="1",
                   help="maps whose name contains one of these (comma list), default all")

    sub.add_parser("effects", parents=[common],
                   help="capture and check player feedback in the effects lab")

    d = sub.add_parser("compare", help="compare two captures")
    d.add_argument("ref_dir")
    d.add_argument("new_dir")
    d.add_argument("--diff-dir")
    d.add_argument("--tolerance", type=int, default=2,
                   help="channel difference that is ignored, default 2")
    d.add_argument("--max-pixels", type=int, default=64,
                   help="pixels over the tolerance that are ignored, default 64")

    args = p.parse_args()
    sys.exit(capture(args) if args.command in ("capture", "effects") else compare(args))


if __name__ == "__main__":
    main()
