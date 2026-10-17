#!/usr/bin/env python3
"""Run original-asset Dog regressions and save/reload in an isolated profile."""
import argparse
import os
from pathlib import Path
import shutil
import refshots

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', type=Path, default=ROOT/'build')
    parser.add_argument('--out', type=Path)
    parser.add_argument('--maps', default='testmap')
    parser.add_argument('--ai', choices=('original', 'improved'), default='improved')
    parser.add_argument('--game-dir', type=Path, default=refshots.DEFAULT_GAME_DIR)
    parser.add_argument('--timeout', type=int, default=180)
    args = parser.parse_args()
    exe = args.build.resolve()/'openhpl1'
    if not exe.is_file() or not shutil.which('gamescope'):
        parser.error('Build the game with BUILD_TESTING=ON and install gamescope first.')
    out = (args.out or args.build/'dog-foundations-check').resolve()
    out.mkdir(parents=True, exist_ok=True)
    # Legacy AI graph caches are written beside map files. Keep writable
    # map copies here; mesh/animation caches are redirected by the profile.
    game_data = out/'game-data'
    game_data.mkdir(exist_ok=True)
    for source in args.game_dir.resolve().iterdir():
        target = game_data/source.name
        if source.name == 'maps':
            shutil.copytree(source, target, dirs_exist_ok=True)
        elif not target.exists():
            target.symlink_to(source, target_is_directory=source.is_dir())
    profile = out/'profile'
    personal = profile/'.frictionalgames/Penumbra/OvertureRemade'
    personal.mkdir(parents=True, exist_ok=True)
    settings = {section: dict(keys) for section, keys in refshots.SETTINGS.items()}
    settings['Game']['LastInitOK'] = 'true'
    settings['AI'] = {'ImprovedDogAI': 'true' if args.ai == 'improved' else 'false'}
    refshots.write_settings(personal/'settings.cfg', settings)
    env = dict(os.environ, OVERTURE_PROFILE_ROOT=str(profile), ALSOFT_DRIVERS='null',
               OVERTURE_SMOKE=args.maps, OVERTURE_SMOKE_FRAMES='180',
               OVERTURE_DOG_REGRESSIONS='1')
    env.setdefault('ASAN_OPTIONS', 'detect_leaks=0')
    # AngelScript's string class uses memcpy(nullptr, nullptr, 0) while
    # compiling scripts. Suppress only that existing vendored-code warning;
    # every other UBSan finding still aborts the test.
    suppression = out/'ubsan.supp'
    suppression.write_text('nonnull-attribute:as_string.cpp\n')
    env.setdefault('UBSAN_OPTIONS', f'halt_on_error=1:print_stacktrace=1:suppressions={suppression}')
    env.pop('OVERTURE_REFSHOTS', None)
    env.pop('OVERTURE_MAP', None)
    status = out/'exit_code'
    status.unlink(missing_ok=True)
    with (out/'run.log').open('w') as log:
        result = refshots.run_headless(str(exe), '1280', '720', str(game_data),
                                      env, log, str(status), args.timeout)
    game_log = personal/'hpl.log'
    lines = game_log.read_text(errors='replace').splitlines() if game_log.is_file() else []
    for line in lines:
        if 'DOGTEST:' in line or 'DOGARENA:' in line or 'SMOKE:' in line:
            print(line)
    success = result == 0 and any('SMOKE: done' in line for line in lines)
    success = success and not any('FAIL' in line and ('DOGTEST:' in line or 'DOGARENA:' in line or 'SMOKE:' in line)
                                 for line in lines)
    if 'testmap' in args.maps or args.maps in {'1', 'all'}:
        success = success and any('DOGTEST: done' in line for line in lines)
    if 'dogtest' in args.maps or args.maps in {'1', 'all'}:
        success = success and any('DOGARENA: done' in line for line in lines)
    print(f'{"PASS" if success else "FAIL"}, exit {result}; output: {out}')
    return 0 if success else 1


if __name__ == '__main__':
    raise SystemExit(main())
