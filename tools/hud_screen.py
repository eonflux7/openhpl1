#!/usr/bin/env python3
# This file is part of OpenHPL1, licensed under the GNU GPL v3 or
# later. See COPYING.
"""Fit generated widescreen images to the full-screen images they replace.

  hud_screen.py screen ORIGINAL.jpg GENERATED.png OUT.jpg
  hud_screen.py menu LOGO.jpg BACKGROUND.jpg GENERATED.png OUT_BACKGROUND.jpg
                [--logo OUT_LOGO.jpg]
  hud_screen.py frame GENERATED_FRAME.png OUT_MASK.png
  hud_screen.py framed ORIGINAL.jpg GENERATED.png MASK.png OUT.jpg
  hud_screen.py score ORIGINAL.jpg OUT.jpg [--rect X Y W H]

The game lays its full-screen images out in an 800x600 screen: a loading
screen fills it, the main menu draws its logo at 0,0 800x180 and its
background at 0,180 800x420, whatever the size of the JPEG. A replacement
has the height of that rectangle and is wider, at the same pixel aspect:
the original's rectangle is its middle, and the rest reaches out to a 16:9
screen (1066.7x600 of those units). The output is --scale (1.6) output pixels
per screen unit, so a 16:9 screen is 1706x960.

GENERATED is a 16:9 image whose middle 4:3 part was made after the original
as it is shown (the menu's logo above its background). Image generators
shift and scale that part a little, so it is registered to the original
first: a scale and an offset on each axis, found by minimising the mean
difference between the two, blurred, at low resolution, where the original
is not black (the menu's logo and background each on their own). The generated
image is then resampled so that the original's rectangle lands exactly in
the middle, and its colours are matched to the original's (mean and spread
per channel over the middle part), unless that makes the difference larger.

screen: a loading screen (800x600). menu: the menu logo and background, from
one generated image of the whole menu; the logo (--logo) is only written
from the main menu's image, the pause menu's background uses the same logo
over it. frame: Requiem's loading screens share one ragged black frame around the
picture. A generated 16:9 frame (white picture, black frame) is made a clean
mask at the output size, its sides at the edges of a 16:10 screen (--aspect)
and black beyond, so that 16:10 and 16:9 screens show all of it. framed: a loading screen generated without its
frame is fitted (registered where the original is not black, so on the
picture inside the old frame) and the mask is laid over it in black.
score: mean difference (0..1) between the original and the middle
of a replacement, after shrinking it to the original's shown size.
"""

import argparse
import sys

from PIL import Image, ImageChops, ImageFilter, ImageStat

SCREEN_W, SCREEN_H = 800.0, 600.0
WIDE_W = SCREEN_H * 16.0 / 9.0
LOGO_H = 180.0

# Size registration is done at, in pixels per screen unit
REG_SCALE = 0.25


def shown(path, size):
	"""An original image resized to the rectangle the game draws it in."""
	return Image.open(path).convert("RGB").resize(size, Image.LANCZOS)


def menu_original(logo, background, scale):
	w, h = round(SCREEN_W * scale), round(SCREEN_H * scale)
	lh = round(LOGO_H * scale)
	im = Image.new("RGB", (w, h))
	im.paste(shown(logo, (w, lh)), (0, 0))
	im.paste(shown(background, (w, h - lh)), (0, lh))
	return im


def sample(gen, p, x0, y0, w_units, h_units, size):
	"""Resamples the generated image over screen units x0..x0+w_units,
	y0..y0+h_units into an image of the given size. p = (sx, sy, tx, ty): the
	generated image's pixel for screen unit (x, y) is
	(GW/2 + sx*k*(x-400) + tx, GH/2 + sy*k*(y-300) + ty), k = GH/600."""
	sx, sy, tx, ty = p
	gw, gh = gen.size
	k = gh / SCREEN_H
	# output pixel -> screen unit -> generated pixel
	ux, uy = w_units / size[0], h_units / size[1]
	a = sx * k * ux
	c = gw / 2 + sx * k * (x0 - SCREEN_W / 2) + tx
	e = sy * k * uy
	f = gh / 2 + sy * k * (y0 - SCREEN_H / 2) + ty
	# Where that reaches past the generated image, its edge pixels are
	# repeated rather than black
	pad = 64
	padded = pad_edges(gen, pad)
	return padded.transform(size, Image.AFFINE, (a, 0, c + pad, 0, e, f + pad), Image.BICUBIC)


def pad_edges(im, pad):
	w, h = im.size
	out = Image.new(im.mode, (w + 2 * pad, h + 2 * pad))
	out.paste(im, (pad, pad))
	out.paste(im.crop((0, 0, 1, h)).resize((pad, h)), (0, pad))
	out.paste(im.crop((w - 1, 0, w, h)).resize((pad, h)), (w + pad, pad))
	out.paste(out.crop((0, pad, w + 2 * pad, pad + 1)).resize((w + 2 * pad, pad)), (0, 0))
	out.paste(out.crop((0, h + pad - 1, w + 2 * pad, h + pad)).resize((w + 2 * pad, pad)), (0, h + pad))
	return out


def difference(a, b):
	d = ImageStat.Stat(ImageChops.difference(a, b)).mean
	return sum(d) / len(d) / 255.0


def register(gen, orig, y0=0.0, y1=SCREEN_H):
	"""Finds p (see sample) for which the generated image best matches the
	original in its rows y0..y1 (screen units): a coarse search over the
	scale, then a pattern search, at low resolution, on both images blurred
	so that the generator's finer detail does not count. Only where the
	original is not black counts, so that a frame or a dark border the
	generator drew anew does not pull the picture inside it out of place."""
	h = y1 - y0
	size = (round(SCREEN_W * REG_SCALE), round(h * REG_SCALE))
	blur = ImageFilter.GaussianBlur(1.5)
	ok = orig.size[1] / SCREEN_H
	target = orig.crop((0, round(y0 * ok), orig.size[0], round(y1 * ok))).resize(size, Image.LANCZOS).filter(blur)
	mask = target.convert("L").filter(ImageFilter.GaussianBlur(3)).point(lambda v: 255 if v > 10 else 0)
	if ImageStat.Stat(mask).mean[0] < 255 * 0.05: mask = None
	# Sampling the generated image straight to that size would alias; shrink
	# it first to about that density
	k = gen.size[1] / SCREEN_H
	small_k = REG_SCALE * 2
	small = gen.resize((round(gen.size[0] * small_k / k), round(gen.size[1] * small_k / k)),
		Image.LANCZOS)
	f = small.size[1] / gen.size[1]

	def cost(p):
		sx, sy, tx, ty = p
		im = sample(small, (sx, sy, tx * f, ty * f), 0, y0, SCREEN_W, h, size)
		d = ImageStat.Stat(ImageChops.difference(im.filter(blur), target), mask).mean
		return sum(d) / len(d) / 255.0

	best, p = min((cost([s, s, 0.0, 0.0]), [s, s, 0.0, 0.0])
		for s in [0.8 + 0.025 * i for i in range(17)])
	steps = [0.02, 0.02, 24.0, 24.0]
	for _ in range(80):
		improved = False
		for i in range(4):
			for sign in (1, -1):
				q = list(p)
				q[i] += sign * steps[i]
				c = cost(q)
				if c < best:
					best, p, improved = c, q, True
		if not improved:
			steps = [s / 2 for s in steps]
			if steps[2] < 0.25: break
	return p, best


def match_colours(im, ref_part, im_part):
	"""Gain and offset per channel that give im_part ref_part's mean and spread."""
	rs, gs = ImageStat.Stat(ref_part), ImageStat.Stat(im_part)
	bands = []
	for i, band in enumerate(im.split()):
		gain = rs.stddev[i] / gs.stddev[i] if gs.stddev[i] > 1e-3 else 1.0
		gain = min(max(gain, 0.75), 1.33)
		off = rs.mean[i] - gs.mean[i] * gain
		bands.append(band.point(lambda v, g=gain, o=off: max(0, min(255, round(v * g + o)))))
	return Image.merge("RGB", bands)


def fit(gen_path, orig, scale, bands=((0.0, SCREEN_H),), frame=None):
	"""Registers and resamples the generated image; returns the fitted 16:9
	image (WIDE_W x 600 screen units). Each band of rows (screen units) is
	registered on its own: the menu's logo and background are. A frame mask
	(white where the picture shows) is laid over it in black."""
	gen = Image.open(gen_path).convert("RGB")
	size = (round(WIDE_W * scale), round(SCREEN_H * scale))
	x0 = SCREEN_W / 2 - WIDE_W / 2
	out = Image.new("RGB", size)
	for y0, y1 in bands:
		p, reg = register(gen, orig, y0, y1)
		print(f"  rows {y0:.0f}..{y1:.0f}: scale {p[0]:.3f} x {p[1]:.3f},"
			f" offset {p[2]:+.1f} {p[3]:+.1f} px, blurred difference {reg:.4f}")
		r0, r1 = round(y0 * scale), round(y1 * scale)
		out.paste(sample(gen, p, x0, y0, WIDE_W, y1 - y0, (size[0], r1 - r0)), (0, r0))
	if frame:
		alpha = Image.open(frame).convert("L").resize(size, Image.LANCZOS)
		out = Image.composite(out, Image.new("RGB", size), alpha)

	mid = middle_box(size[0], scale)
	orig_s = orig.resize((mid[2] - mid[0], mid[3] - mid[1]), Image.LANCZOS)
	before = difference(out.crop(mid), orig_s)
	matched = match_colours(out, orig_s, out.crop(mid))
	after = difference(matched.crop(mid), orig_s)
	if after < before:
		out = matched
	print(f"  difference {before:.4f}, colours matched {after:.4f}"
		f"{'' if after < before else ' (not used)'}")
	return out


def make_frame(gen_path, out_path, scale, aspect):
	"""A generated black-and-white frame, as a clean mask at the output size:
	thresholded, with an antialiased edge, narrowed so that its sides are at
	the edges of an aspect-wide screen (16:10, so that a 16:10 screen shows
	all of it) with black beyond them."""
	size = (round(WIDE_W * scale), round(SCREEN_H * scale))
	inner = (round(SCREEN_H * aspect * scale), size[1])
	im = Image.open(gen_path).convert("L").resize((inner[0] * 2, inner[1] * 2), Image.LANCZOS)
	im = im.filter(ImageFilter.MedianFilter(5)).point(lambda v: 255 if v > 127 else 0)
	out = Image.new("L", size)
	out.paste(im.resize(inner, Image.LANCZOS), ((size[0] - inner[0]) // 2, 0))
	out.save(out_path)
	print(f"  {out_path}: {size[0]}x{size[1]}")


def middle_box(width, scale):
	x = round((width - SCREEN_W * scale) / 2)
	return (x, 0, x + round(SCREEN_W * scale), round(SCREEN_H * scale))


def save(im, path):
	im.save(path, quality=92, subsampling=0, optimize=True)
	print(f"  {path}: {im.size[0]}x{im.size[1]}")


def score(orig_path, out_path, rect):
	out = Image.open(out_path).convert("RGB")
	x, y, w, h = rect
	k = out.size[1] / h
	mx = round((out.size[0] - w * k) / 2)
	mid = out.crop((mx, 0, mx + round(w * k), out.size[1]))
	shown_size = (round(w), round(h))
	a = shown(orig_path, shown_size)
	b = mid.resize(shown_size, Image.LANCZOS)
	print(f"{out_path}: {difference(a, b):.4f}")


def main():
	ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
	sub = ap.add_subparsers(dest="mode", required=True)
	s = sub.add_parser("screen")
	s.add_argument("original"); s.add_argument("generated"); s.add_argument("out")
	m = sub.add_parser("menu")
	m.add_argument("logo"); m.add_argument("background"); m.add_argument("generated")
	m.add_argument("out"); m.add_argument("--logo", dest="logo_out")
	m.add_argument("--rows", type=float, nargs="+", default=[],
		help="more band boundaries (screen units) inside the logo, registered on their own")
	fr = sub.add_parser("frame")
	fr.add_argument("generated"); fr.add_argument("out")
	fr.add_argument("--aspect", type=float, default=1.6)
	fd = sub.add_parser("framed")
	fd.add_argument("original"); fd.add_argument("generated"); fd.add_argument("frame")
	fd.add_argument("out")
	c = sub.add_parser("score")
	c.add_argument("original"); c.add_argument("out")
	c.add_argument("--rect", type=float, nargs=4, default=[0, 0, SCREEN_W, SCREEN_H])
	for p in (s, m, fr, fd):
		p.add_argument("--scale", type=float, default=1.6)
	a = ap.parse_args()

	if a.mode == "score":
		score(a.original, a.out, a.rect)
		return 0

	if a.mode == "frame":
		make_frame(a.generated, a.out, a.scale, a.aspect)
		return 0

	reg = 4  # the original at 4 pixels per unit for colour matching
	if a.mode == "framed":
		orig = shown(a.original, (round(SCREEN_W * reg), round(SCREEN_H * reg)))
		save(fit(a.generated, orig, a.scale, frame=a.frame), a.out)
	elif a.mode == "screen":
		orig = shown(a.original, (round(SCREEN_W * reg), round(SCREEN_H * reg)))
		out = fit(a.generated, orig, a.scale)
		save(out, a.out)
	else:
		orig = menu_original(a.logo, a.background, reg)
		rows = [0.0] + sorted(a.rows) + [LOGO_H, SCREEN_H]
		out = fit(a.generated, orig, a.scale, tuple(zip(rows[:-1], rows[1:])))
		lh = round(LOGO_H * a.scale)
		if a.logo_out:
			save(out.crop((0, 0, out.size[0], lh)), a.logo_out)
		save(out.crop((0, lh, out.size[0], out.size[1])), a.out)
	return 0


if __name__ == "__main__":
	sys.exit(main())
