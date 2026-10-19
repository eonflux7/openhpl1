#!/usr/bin/env python3
# This file is part of OpenHPL1, licensed under the GNU GPL v3 or
# later. See COPYING.
"""Rasterise the vector HUD sources.

  hud_raster.py [--scale 4] [NAME ...]

Each hud/src/NAME.svg is drawn to hud/NAME@<scale>x.png. The SVG's viewBox
is the size of the game image it replaces, so the PNG is exactly <scale>
times that size, which the engine needs to work out the on-screen size.
Uses rsvg-convert if it is installed, otherwise the cairosvg module.
"""

import argparse
import pathlib
import re
import shutil
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
SRC = ROOT / "hud" / "src"
OUT = ROOT / "hud"


def view_size(svg):
	m = re.search(r'viewBox="\s*0\s+0\s+([\d.]+)\s+([\d.]+)\s*"', svg.read_text())
	if not m:
		sys.exit(f"{svg}: no viewBox starting at 0 0")
	w, h = float(m.group(1)), float(m.group(2))
	if not (w.is_integer() and h.is_integer()):
		sys.exit(f"{svg}: viewBox size must be whole pixels")
	return int(w), int(h)


def rasterise(svg, png, w, h):
	if shutil.which("rsvg-convert"):
		subprocess.run(["rsvg-convert", "-w", str(w), "-h", str(h), "-o", str(png), str(svg)], check=True)
		return
	try:
		import cairosvg
	except ImportError:
		sys.exit("Needs rsvg-convert (librsvg2-bin) or the cairosvg Python module")
	cairosvg.svg2png(url=str(svg), write_to=str(png), output_width=w, output_height=h)


def main():
	ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
	ap.add_argument("--scale", type=int, default=4, choices=(2, 3, 4, 8))
	ap.add_argument("names", nargs="*")
	args = ap.parse_args()

	svgs = [SRC / f"{n}.svg" for n in args.names] if args.names else sorted(SRC.glob("*.svg"))
	for svg in svgs:
		w, h = view_size(svg)
		png = OUT / f"{svg.stem}@{args.scale}x.png"
		rasterise(svg, png, w * args.scale, h * args.scale)
		print(f"{png.relative_to(ROOT)}: {w * args.scale}x{h * args.scale}")


if __name__ == "__main__":
	main()
