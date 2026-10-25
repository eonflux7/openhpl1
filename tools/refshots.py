#!/usr/bin/env python3
# This file is part of OpenHPL1, licensed under the GNU GPL v3 or
# later. See COPYING.
"""Reference images for renderer changes: now part of levelview.py.

  refshots.py capture OUT_DIR [--maps office,worm] [--size 1280x720] ...
  refshots.py effects OUT_DIR ...
  refshots.py compare REF_DIR NEW_DIR [--diff-dir DIR]

are the same as levelview.py capture, effects and compare.
"""

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import levelview  # noqa: E402

if __name__ == "__main__":
    if len(sys.argv) < 2 or sys.argv[1] not in ("capture", "effects", "compare"):
        print(__doc__.strip())
        sys.exit(2)
    sys.exit(levelview.main(sys.argv[1:]))
