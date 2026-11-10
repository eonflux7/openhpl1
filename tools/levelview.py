#!/usr/bin/env python3
# This file is part of OpenHPL1, licensed under the GNU GPL v3 or
# later. See COPYING.
"""Look at and edit maps in the running game, and take reference images.

The live viewer and level editor (docs/level-editor.md):

  levelview.py start [--map level01_05_office[:link01]] [--game overture]
                     [--addons DIR ...] [--editor] [--hidden] [--windows] [--build DIR]
  levelview.py status | maps [filter] | map NAME [START] | reload
  levelview.py views | view NAME | camera [X Y Z [YAW PITCH]] | look NODE [DIST [YAW PITCH]]
  levelview.py release | option NAME 0|1 | wait FRAMES | pick
  levelview.py shot FILE.png [--frames N] [--2d] [--panel]
  levelview.py shots OUT_DIR             every viewpoint, after a reload
  levelview.py batch FILE|-              lines run back to back, one answer
  levelview.py nodes [FILTER] | node NAME
  levelview.py set NODE [pos X Y Z] [rot X Y Z] [scale X Y Z] [move ...] [turn ...]
  levelview.py reset [NODE] | save | revert | script 'Command();'
  levelview.py select [NODE [focus]] | undo | redo | play 0|1 | simulate 0|1   (--editor)
  levelview.py mouse X Y [move|press|release|click] [left|right|middle]       (--editor)
  levelview.py log [-n 40] [--grep TEXT] | cmd 'raw command' | stop

start runs the game with OPENHPL_CONTROL, its own profile (settings, saves,
logs, caches) and settings, and returns once it takes commands. With
--editor the game opens in the level editor's window (also what
`openhpl1 --editor` does), and the commands drive what is seen there. The other
commands are sent to that game, one at a time, and print its answer; they
exit with 1 if the game answers with an error. A session's files are in
~/.cache/openhpl1/levelview (for the Windows build: next to it, in
levelview/), or --session.

Reference images for renderer changes (formerly refshots.py):

  levelview.py capture OUT_DIR [--maps office,worm] [--size 1280x720]
                               [--set Graphics.Shadows=2 ...]
  levelview.py effects OUT_DIR [--size 1280x720] [--build build-asan]
  levelview.py compare REF_DIR NEW_DIR [--diff-dir DIR] [--live]

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

compare lists the frames that differ between two captures (or two sets of
live shots) and exits with 1 if any do. A frame counts as changed when more
than --max-pixels pixels differ by more than --tolerance in some channel: a
few pixels change from run to run of the same build (see README). With
--diff-dir, an image of the difference is written for each changed frame,
brightened 8x. Frames of the maps in UNSTABLE_MAPS are listed but do not
count.
"""

import argparse
import json
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


# ---------------------------------------------------------------------------
# The live viewer: a running game that takes commands (game/core/LiveControl.h)

STATE_DIR = os.path.join(os.environ.get("XDG_CACHE_HOME", os.path.expanduser("~/.cache")),
                         "openhpl1")
SESSION_POINTER = os.path.join(STATE_DIR, "levelview-session")

# The game's settings for a live session: those of the reference images, in
# a window (which can be resized), with the game's sounds off unless --sound
LIVE_SETTINGS = {s: dict(k) for s, k in SETTINGS.items()}
LIVE_SETTINGS["Graphics"]["LimitFPS"] = "true"
# Fixed-step logic runs once per frame: without vsync a fast GPU makes the game run too fast
LIVE_SETTINGS["Screen"]["Vsync"] = "true"
# Scripts blur the view with it now and then (waking up in the boat cabin)
LIVE_SETTINGS["Graphics"]["DepthOfField"] = "false"
# Items glow now and then to be noticed, at times that differ from set to set
LIVE_SETTINGS["Game"]["FlashItems"] = "false"
# Textures as the game filters them (its default): the reference images
# keep 1, so that their frames stay comparable with older ones
LIVE_SETTINGS["Graphics"]["TextureAnisotropy"] = "16.0"

# Folders of the game's profile under OPENHPL_PROFILE_ROOT (Init.cpp)
PERSONAL_DIRS = {
    ("linux", "overture"): ".frictionalgames/Penumbra/OvertureRemade",
    ("linux", "blackplague"): ".frictionalgames/Penumbra/BlackPlagueRemade",
    ("linux", "requiem"): ".frictionalgames/Penumbra/RequiemRemade",
    ("windows", "overture"): "Penumbra Overture/OvertureRemade",
    ("windows", "blackplague"): "Penumbra/BlackPlagueRemade",
    ("windows", "requiem"): "Penumbra/RequiemRemade",
}


# Two sets of live shots of an unchanged map differ by up to about 16 in a
# channel, in a few thousand pixels: the game ran for a different number of
# frames between them, and some of what that changes outlasts a reload. A
# moved or changed model differs by far more.
LIVE_TOLERANCE = 24


class Session:
    """The files of a live session: session.json in its folder says where
    the control folder, profile and log are, and how the game was run."""

    def __init__(self, folder):
        self.folder = os.path.abspath(folder)
        self.info = {}
        path = os.path.join(self.folder, "session.json")
        if os.path.exists(path):
            with open(path) as f:
                self.info = json.load(f)

    @property
    def control(self):
        return os.path.join(self.folder, "control")

    def save(self):
        with open(os.path.join(self.folder, "session.json"), "w") as f:
            json.dump(self.info, f, indent=1)

    def running(self, thorough=False):
        """The game removes "ready" when it quits, not when it crashes:
        the process is looked for too (a Windows one only if thorough, it
        takes a while)."""
        if not os.path.exists(os.path.join(self.control, "ready")):
            return False
        if self.info.get("windows"):
            pid = self.info.get("game_pid")
            if thorough and pid:
                found = subprocess.run(["tasklist.exe", "/FI", "PID eq %d" % pid, "/NH"],
                                       capture_output=True, text=True).stdout
                return str(pid) in found
            return True
        pid = self.info.get("pid")
        if pid:
            try:
                os.kill(pid, 0)
            except OSError:
                return False
        return True

    def game_path(self, path):
        return windows_path(path) if self.info.get("windows") else path

    def hpl_log(self):
        path = os.path.join(self.folder, "profile", self.info.get("personal", ""), "hpl.log")
        return path if os.path.exists(path) else None

    def send(self, command, timeout=300):
        """Sends a command, returns (ok, answer lines)."""
        if not self.running():
            sys.exit("No game is running in %s (levelview.py start)" % self.folder)
        os.makedirs(self.control, exist_ok=True)
        # Names sort in the order they were sent
        name = "%020d-%d" % (time.time_ns(), os.getpid())
        temp = os.path.join(self.control, name + ".part")
        with open(temp, "w", newline="\n") as f:
            f.write(command + "\n")
        os.replace(temp, os.path.join(self.control, name + ".cmd"))

        out = os.path.join(self.control, name + ".out")
        start = last_look = time.monotonic()
        while not os.path.exists(out):
            now = time.monotonic()
            if now - start > timeout:
                sys.exit("No answer to '%s' within %d seconds" % (command, timeout))
            thorough = now - last_look > 5
            if thorough:
                last_look = now
            if not self.running(thorough) and not os.path.exists(out):
                sys.exit("The game quit before it answered '%s'; see levelview.py log" % command)
            time.sleep(0.05)
        with open(out, errors="replace") as f:
            text = f.read()
        os.remove(out)
        lines = text.splitlines()
        if lines and lines[0] == "ok":
            return True, lines[1:]
        if lines and lines[0].startswith("error: "):
            return False, [lines[0][7:]] + lines[1:]
        return False, ["unreadable answer: %r" % text]


def find_session(args):
    folder = getattr(args, "session", None) or os.environ.get("OPENHPL_LEVELVIEW_SESSION")
    if not folder and os.path.exists(SESSION_POINTER):
        with open(SESSION_POINTER) as f:
            folder = f.read().strip()
    if not folder:
        sys.exit("No session: start one with levelview.py start")
    return Session(folder)


def in_wsl():
    try:
        with open("/proc/version") as f:
            return "microsoft" in f.read().lower()
    except OSError:
        return False


def default_build(windows):
    if windows:
        return os.environ.get("OPENHPL_WINDOWS_BUILD", os.path.join(SOURCE, "build-windows"))
    return os.path.join(SOURCE, "build")


def start(args):
    windows = args.windows or (in_wsl() and not args.linux and
                               os.path.exists(os.path.join(default_build(True), "openhpl1.exe"))
                               and args.build is None)
    build = args.build or default_build(windows)
    build = build if os.path.isabs(build) else os.path.join(SOURCE, build)
    exe = os.path.join(build, "openhpl1.exe" if windows else "openhpl1")
    if not os.path.exists(exe):
        sys.exit("No executable at %s (--build)" % exe)
    game_dir = args.game_dir or os.environ.get("OPENHPL_GAME_DIR") or DEFAULT_GAME_DIR
    if windows and re.match(r"^[A-Za-z]:\\", game_dir):
        game_dir = subprocess.check_output(["wslpath", "-u", game_dir], text=True).strip()
    if not os.path.isdir(game_dir):
        sys.exit("Game data not found: %s (--game-dir or OPENHPL_GAME_DIR)" % game_dir)
    if args.headless and shutil.which("gamescope") is None:
        sys.exit("gamescope is needed for --headless")

    folder = args.session or (os.path.join(build, "levelview") if windows
                              else os.path.join(STATE_DIR, "levelview"))
    session = Session(folder)
    if session.running() or (session.info and process_alive(session)):
        sys.exit("A game is already running in %s (levelview.py stop)" % folder)
    shutil.rmtree(session.control, ignore_errors=True)
    os.makedirs(session.control)
    profile = os.path.join(session.folder, "profile")
    personal = PERSONAL_DIRS[("windows" if windows else "linux", args.game)]
    if args.fresh:
        shutil.rmtree(profile, ignore_errors=True)
    os.makedirs(os.path.join(profile, personal), exist_ok=True)

    settings = {s: dict(k) for s, k in LIVE_SETTINGS.items()}
    width, height = args.size.split("x")
    settings["Screen"]["Width"], settings["Screen"]["Height"] = width, height
    if args.sound:
        settings["Sound"]["Volume"] = "1.0"
    for item in args.set:
        m = re.fullmatch(r"(\w+)\.(\w+)=(.*)", item)
        if not m:
            sys.exit("--set wants Section.Key=Value, not '%s'" % item)
        settings.setdefault(m.group(1), {})[m.group(2)] = m.group(3)
    write_settings(os.path.join(profile, personal, "settings.cfg"), settings)

    path = windows_path if windows else (lambda p: p)
    env = dict(os.environ)
    for name in [n for n in env if n.startswith("OPENHPL_")]:
        if name not in ("OPENHPL_GL_DEBUG", "OPENHPL_SHADER_DIR", "OPENHPL_PHYSLOG", "OPENHPL_PHYSTRACE"):
            del env[name]
    env["OPENHPL_GAME"] = args.game
    env["OPENHPL_PROFILE_ROOT"] = path(profile)
    env["OPENHPL_CONTROL"] = path(session.control)
    if args.map:
        env["OPENHPL_MAP"] = args.map
    if args.addons:
        sep = ";" if windows else ":"
        env["OPENHPL_ADDON_DIRS"] = sep.join(path(os.path.abspath(a)) for a in args.addons)
    if args.no_addons:
        env["OPENHPL_ADDONS"] = "0"
    if args.edit_dir:
        os.makedirs(args.edit_dir, exist_ok=True)
        env["OPENHPL_EDIT_DIR"] = path(os.path.abspath(args.edit_dir))
    if args.editor:
        env["OPENHPL_EDITOR"] = "1"
    if args.hidden:
        env["OPENHPL_HIDDEN"] = "1"
    if not args.sound:
        env["ALSOFT_DRIVERS"] = "null"
    if windows:
        names = [n for n in env if n.startswith("OPENHPL_") or n == "ALSOFT_DRIVERS"]
        env["WSLENV"] = ":".join(filter(None, [os.environ.get("WSLENV")] + names))

    if args.headless:
        cmd = ["gamescope", "--backend", "headless", "-W", width, "-H", height,
               "-w", width, "-h", height, "--", exe]
    else:
        cmd = [exe]
    log = open(os.path.join(session.folder, "run.log"), "w")
    proc = subprocess.Popen(cmd, cwd=game_dir, env=env, stdout=log, stderr=subprocess.STDOUT,
                            stdin=subprocess.DEVNULL, start_new_session=True)
    session.info = {"windows": windows, "pid": proc.pid, "exe": exe, "game": args.game,
                    "game_dir": game_dir, "personal": personal, "map": args.map or "",
                    "started": time.time()}
    session.save()
    os.makedirs(STATE_DIR, exist_ok=True)
    with open(SESSION_POINTER, "w") as f:
        f.write(session.folder + "\n")

    ready = os.path.join(session.control, "ready")
    start_time = time.monotonic()
    while not os.path.exists(ready):
        if proc.poll() is not None:
            sys.exit("The game quit at start (exit code %d); see %s/run.log and levelview.py log"
                     % (proc.returncode, session.folder))
        if time.monotonic() - start_time > args.timeout:
            sys.exit("The game did not start within %d seconds" % args.timeout)
        time.sleep(0.2)
    # A Windows game's own process id, for stop
    with open(ready) as f:
        for line in f:
            if line.startswith("pid "):
                session.info["game_pid"] = int(line.split()[1])
    session.save()

    ok, lines = session.send(("" if args.real_time else "option fixedstep 1\n") + "status",
                             timeout=args.timeout)
    print("Started %s in %s" % (args.game, session.folder))
    print("\n".join(lines))
    return 0 if ok else 1


def process_alive(session):
    """Whether the game's process is still there (it writes its settings and
    log after it has stopped taking commands)."""
    if session.info.get("windows"):
        pid = session.info.get("game_pid")
        if not pid:
            return False
        found = subprocess.run(["tasklist.exe", "/FI", "PID eq %d" % pid, "/NH"],
                               capture_output=True, text=True).stdout
        return str(pid) in found
    try:
        pid, _ = os.waitpid(session.info.get("pid", 0), os.WNOHANG)
    except ChildProcessError:
        pass
    try:
        os.kill(session.info.get("pid", 0), 0)
        return True
    except (OSError, TypeError):
        return False


def stop(args):
    session = find_session(args)
    if session.running():
        try:
            session.send("quit", timeout=30)
        except SystemExit:
            pass
        start_time = time.monotonic()
        while (session.running() or process_alive(session)) and time.monotonic() - start_time < 30:
            time.sleep(0.5)
    if os.path.exists(os.path.join(session.control, "ready")) or process_alive(session):
        # It did not quit (or crashed and left the file)
        pid = session.info.get("game_pid")
        if session.info.get("windows") and pid:
            subprocess.run(["taskkill.exe", "/PID", str(pid), "/F"], stdout=subprocess.DEVNULL,
                           stderr=subprocess.DEVNULL)
        elif session.info.get("pid"):
            try:
                os.killpg(session.info["pid"], 9)
            except OSError:
                pass
        if os.path.exists(os.path.join(session.control, "ready")):
            os.remove(os.path.join(session.control, "ready"))
        print("Killed the game")
    else:
        print("Stopped")
    return 0


def show_log(args):
    session = find_session(args)
    path = session.hpl_log()
    if path is None:
        sys.exit("No hpl.log in %s" % session.folder)
    with open(path, errors="replace") as f:
        lines = [l.rstrip("\n") for l in f]
    if args.grep:
        lines = [l for l in lines if args.grep.lower() in l.lower()]
    print("\n".join(lines[-args.n:] if args.n > 0 else lines))
    return 0


def send_and_print(session, command, timeout=300):
    ok, lines = session.send(command, timeout)
    for line in lines:
        print(line if ok else "error: " + line)
    return 0 if ok else 1


def quote(arg):
    return '"%s"' % arg if (" " in arg or not arg) else arg


def live_command(args):
    session = find_session(args)
    if args.command == "cmd":
        return send_and_print(session, " ".join(args.words), args.timeout)
    if args.command == "script":
        # A line that does not compile is only logged by the game
        line = " ".join(args.words)
        if send_and_print(session, "script " + line, args.timeout):
            return 1
        path = session.hpl_log()
        if path:
            with open(path, errors="replace") as f:
                tail = f.readlines()[-20:]
            failed = [l for l in tail if "couldn't run '%s'" % line in l]
            if failed:
                print("error: " + failed[-1].strip())
                errors = [l.strip() for l in tail if l.startswith("ExecuteString")]
                if errors:
                    print(errors[-1])
                return 1
        return 0
    if args.command == "shot":
        return take_shot(session, args.file, args.frames, args.with_2d, args.timeout, args.panel)
    return send_and_print(session, " ".join([args.command] + [quote(w) for w in args.words]),
                          args.timeout)


def take_shot(session, file, frames, with_2d, timeout=300, panel=False):
    """The game saves the frame into the session, it is moved from there:
    the game may be a Windows program that cannot write where asked."""
    shots = os.path.join(session.folder, "shots")
    os.makedirs(shots, exist_ok=True)
    temp = os.path.join(shots, "%d.png" % time.time_ns())
    command = "shot %s %d%s%s" % (quote(session.game_path(temp)), frames, " 2d" if with_2d else "",
                                  " panel" if panel else "")
    ok, lines = session.send(command, timeout)
    if not ok:
        print("error: " + "\n".join(lines))
        return 1
    os.makedirs(os.path.dirname(os.path.abspath(file)), exist_ok=True)
    shutil.move(temp, file)
    print(file)
    return 0


def take_shots(args):
    """A frame from every viewpoint of the loaded map, named as capture names
    them, so that two sets can be compared. The map is reloaded first and
    the lines are sent as one batch: with fixed steps (the default of start)
    each set is taken at the same frames after the load."""
    session = find_session(args)
    ok, status = session.send("status")
    mapname = dict(l.split(" ", 1) for l in status if " " in l).get("map", "")
    if not ok or not mapname:
        sys.exit("No map is loaded")
    ok, views = session.send("views")
    places = [v.split("\t") for v in views if v]
    manual = [p for p in places if p[1] == "viewpoint"]
    places = manual or [p for p in places if p[1] == "start"]
    if args.views:
        wanted = args.views.split(",")
        places = [p for p in places if p[0] in wanted]
    if not places:
        sys.exit("%s has no viewpoints or start positions to shoot from" % mapname)

    shots = os.path.join(session.folder, "shots")
    os.makedirs(shots, exist_ok=True)
    os.makedirs(args.out_dir, exist_ok=True)
    # Held before the reload, the player's body is out of the way (as in
    # every later set) and the map starts the same
    lines = [] if args.no_reload else ["view " + places[0][0], "reload"]
    files = []
    for place in places:
        name = "%s-%s%s.png" % (mapname, place[0], "" if place[1] == "viewpoint" else "-000")
        temp = os.path.join(shots, "%d-%s" % (time.time_ns(), name))
        files.append((temp, os.path.join(args.out_dir, name)))
        lines += ["view " + place[0], "shot %s %d" % (quote(session.game_path(temp)), args.frames)]
    # The camera stays held: let go, the player would stand where it was
    # (above a table, say) and knock things over before the next set
    # The first set of a session differs from later ones by a few pixels
    # (what the game did before it was first held): one is taken and thrown
    # away first
    for attempt in range(1 if session.info.get("warm") or args.no_reload else 2):
        ok, answer = session.send("\n".join(lines), args.timeout)
        if not ok:
            print("error: " + "\n".join(answer))
            return 1
    session.info["warm"] = True
    session.save()
    for temp, final in files:
        shutil.move(temp, final)
        print(final)
    return 0


def send_batch(args):
    session = find_session(args)
    f = sys.stdin if args.file == "-" else open(args.file)
    lines = [l.strip() for l in f if l.strip() and not l.lstrip().startswith("#")]
    return send_and_print(session, "\n".join(lines), args.timeout)


LIVE_COMMANDS = {
    "status": "map, camera and edits",
    "maps": "the maps that can be loaded [FILTER]",
    "map": "load a map: NAME [START], answers once it has settled",
    "reload": "load the map again from its files",
    "views": "the map's viewpoints (tools/refviews.txt) and start positions",
    "view": "camera to a viewpoint or start position: NAME",
    "camera": "where the camera is, or hold it: [X Y Z [YAW PITCH]] (radians)",
    "look": "hold the camera looking at a node, short of walls: NODE [DIST [YAW PITCH]]",
    "release": "let go of the camera, the player walks on from there",
    "option": "flashlight, flicker, hud, godmode, dof or fixedstep: NAME 0|1",
    "nodes": "the nodes of the map's file [FILTER: name, kind or file]",
    "node": "one node: NAME",
    "set": "change a node: NODE [pos|rot|scale|move|turn X Y Z]... (degrees)",
    "reset": "undo unsaved changes [NODE]",
    "save": "save the changed map into the edit addon and reload it",
    "revert": "remove the map's saved edits and reload it",
    "pick": "the node in the middle of the screen",
    "particles": "the particle systems (name, .ps, distance, new/old/-, position), or show "
                 "those an addon replaces as the addon has them or as the game does: "
                 "[new|old [NAME] | pick NAME|none | cycle]",
    "wait": "let FRAMES frames pass",
    "select": "the editor's selection (start --editor): [NODE [focus]]",
    "undo": "undo the editor's last step",
    "redo": "redo the editor's next step",
    "play": "the editor's Play, the player starting at the camera: 0|1",
    "simulate": "let the world run while editing: 0|1",
    "window": "resize the window: WIDTH HEIGHT",
    "mouse": "the editor's pointer: X Y [move|press|release|click] [left|right|middle]",
}


def main(argv=None):
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = p.add_subparsers(dest="command", required=True)

    session_opt = argparse.ArgumentParser(add_help=False)
    session_opt.add_argument("--session", help="session folder (default: the last started)")
    session_opt.add_argument("--timeout", type=int, default=300, help="seconds to wait for an answer")

    s = sub.add_parser("start", help="run the game and let it take commands")
    s.add_argument("--game", default="overture", choices=["overture", "blackplague", "requiem"])
    s.add_argument("--map", help="map[:start] to load at once (else the main menu)")
    s.add_argument("--addons", action="append", default=[], metavar="DIR",
                   help="addon folder, or a folder of addons (OPENHPL_ADDON_DIRS)")
    s.add_argument("--no-addons", action="store_true", help="OPENHPL_ADDONS=0")
    s.add_argument("--edit-dir", help="where the level editor saves (OPENHPL_EDIT_DIR); "
                   "default: the profile's addons/editor")
    s.add_argument("--editor", action="store_true",
                   help="open the level editor's window (OPENHPL_EDITOR): panels around the 3D view")
    s.add_argument("--hidden", action="store_true", help="no window (OPENHPL_HIDDEN)")
    s.add_argument("--headless", action="store_true", help="in a headless gamescope session (Linux)")
    s.add_argument("--windows", action="store_true",
                   help="run the Windows build from WSL (the default in WSL when it is built)")
    s.add_argument("--linux", action="store_true", help="in WSL, run the Linux build")
    s.add_argument("--build", help="build folder (default build, or build-windows / "
                   "OPENHPL_WINDOWS_BUILD for the Windows build)")
    s.add_argument("--game-dir", help="game data (default OPENHPL_GAME_DIR)")
    s.add_argument("--size", default="1280x720")
    s.add_argument("--set", action="append", default=[], metavar="SECTION.KEY=VALUE",
                   help="override a setting, e.g. Graphics.Shadows=2")
    s.add_argument("--sound", action="store_true", help="play the game's sounds")
    s.add_argument("--fresh", action="store_true", help="start with an empty profile")
    s.add_argument("--real-time", action="store_true",
                   help="steps of the frame's length (default: fixed steps, for repeatable shots)")
    s.add_argument("--session", help="session folder")
    s.add_argument("--timeout", type=int, default=180, help="seconds to wait for the game")

    sub.add_parser("stop", parents=[session_opt], help="quit the game")

    lg = sub.add_parser("log", parents=[session_opt], help="the game's hpl.log")
    lg.add_argument("-n", type=int, default=40, help="last lines (0: all)")
    lg.add_argument("--grep", help="only lines with this text (any case)")

    for name, text in LIVE_COMMANDS.items():
        c = sub.add_parser(name, parents=[session_opt], help=text)
        c.add_argument("words", nargs="*")
    c = sub.add_parser("script", parents=[session_opt], help="run a line in the map's script")
    c.add_argument("words", nargs="+")
    c = sub.add_parser("cmd", parents=[session_opt], help="send a command as it is")
    c.add_argument("words", nargs="+")

    c = sub.add_parser("shot", parents=[session_opt], help="save a frame of the 3D view")
    c.add_argument("file")
    c.add_argument("--frames", type=int, default=10, help="frames to wait first, default 10")
    c.add_argument("--2d", dest="with_2d", action="store_true", help="with the HUD and 2D")
    c.add_argument("--panel", action="store_true", help="with the panel: the F2 windows, opened, or the editor's window")

    c = sub.add_parser("shots", parents=[session_opt],
                       help="a frame from every viewpoint of the loaded map, after reloading it")
    c.add_argument("out_dir")
    c.add_argument("--views", help="only these (comma list)")
    c.add_argument("--frames", type=int, default=10)
    c.add_argument("--no-reload", action="store_true", help="from the map as it is")

    c = sub.add_parser("batch", parents=[session_opt],
                       help="send the lines of a file (- for stdin) as one command: they run "
                       "back to back")
    c.add_argument("file")

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

    c = sub.add_parser("capture", parents=[common],
                       help="reference images: run the game headless and save frames")
    c.add_argument("--maps", default="1",
                   help="maps whose name contains one of these (comma list), default all")

    sub.add_parser("effects", parents=[common],
                   help="capture and check player feedback in the effects lab")

    d = sub.add_parser("compare", help="compare two captures or sets of shots")
    d.add_argument("ref_dir")
    d.add_argument("new_dir")
    d.add_argument("--diff-dir")
    d.add_argument("--tolerance", type=int, default=2,
                   help="channel difference that is ignored, default 2")
    d.add_argument("--max-pixels", type=int, default=64,
                   help="pixels over the tolerance that are ignored, default 64")
    d.add_argument("--live", action="store_true",
                   help="for two sets of live shots: a tolerance of 24 (see docs/level-editor.md)")

    args = p.parse_args(argv)
    if args.command in ("capture", "effects"):
        return capture(args)
    if args.command == "compare":
        if args.live:
            args.tolerance = max(args.tolerance, LIVE_TOLERANCE)
        return compare(args)
    if args.command == "start":
        return start(args)
    if args.command == "stop":
        return stop(args)
    if args.command == "log":
        return show_log(args)
    if args.command == "shots":
        return take_shots(args)
    if args.command == "batch":
        return send_batch(args)
    return live_command(args)


if __name__ == "__main__":
    sys.exit(main())
