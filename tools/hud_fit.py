#!/usr/bin/env python3
# This file is part of OpenHPL1, licensed under the GNU GPL v3 or
# later. See COPYING.
"""Fit generated high-resolution images to the game images they replace.

  hud_fit.py icon ORIGINAL.bmp GENERATED.png OUT.png [--scale 4] [--margin M]
             [--box X0 Y0 X1 Y1] [--edge LO HI]
  hud_fit.py page ORIGINAL.bmp GENERATED.png OUT.png [--scale 3]
  hud_fit.py sheet ORIGINAL.bmp GENERATED.png OUT.png [--scale 3] [--margin M]
  hud_fit.py curl ORIGINAL_PAGE.bmp PAGE.png ORIGINAL_CORNER.bmp OUT.png
             --draw X Y W H [--side right|left] [--fold A B] [--scale 8] [--margin MX MY]
  hud_fit.py tint ORIGINAL.bmp ORIGINAL_TINTED.bmp FITTED.png OUT.png
  hud_fit.py soften IN.png OUT.png --sigma S
  hud_fit.py score ORIGINAL.bmp OUT.png

The game's HUD images are 32-bit BMPs whose alpha most readers drop; they
are read here directly. OUT is --scale times the original's size plus
--margin original pixels on every side, which the engine draws outside the
original's rectangle, so edges and shadows fade out instead of being cut at
the sprite's border.

icon: the generated image's opaque part (alpha > 24) is scaled into the box
the original's opaque part fills (or --box, in original pixels, which may
reach into the margin), with the generated alpha and colours matched to the
original's (mean and spread per channel). --edge remaps alpha LO..HI to
0..255, for a generated rim that is too soft.
page: the generated image fills the box of every non-transparent original
pixel and takes the enlarged original alpha (for the cover, which has no
hard edge of its own).
sheet: a generated sheet of paper on a transparent background; its sheet
(alpha > 128) is fitted to the original's rectangle and it gets a soft
shadow in the margin.
curl: renders the page-turn corner from the finished sheet, so it matches the
page exactly: the page's corner beyond the fold line is folded back over
the page (the flap shows the sheet's back, shaded like a curl), the page
underneath shows where it was, and the flap casts a shadow. --draw is where
the game draws the corner image, in the page's original pixels; the corner
image is drawn stretched there, so it is rendered at that shape and then
resized to the corner's size. Only the part that differs from the page is
kept (the rest is transparent), so nothing is drawn twice over the page.
--fold A B are the fold line's distances from the corner along the bottom
and the side, in page pixels.
tint: recolours FITTED as ORIGINAL_TINTED recolours ORIGINAL (same shape,
different colours, e.g. the red "invalid" hand from the "active" one).
soften: blurs the premultiplied image (for flat art such as the pointer).

score prints the mean premultiplied RGBA difference (0..1) between the
original and OUT (without its margin) shrunk back to the original's size.
Needs Pillow and NumPy.
"""

import argparse
import struct

import numpy as np
from PIL import Image


def load(path):
	if str(path).lower().endswith(".bmp"):
		data = open(path, "rb").read()
		offset = struct.unpack("<I", data[10:14])[0]
		width, height = struct.unpack("<ii", data[18:26])
		bpp = struct.unpack("<H", data[28:30])[0]
		if bpp == 32:
			image = Image.frombuffer("RGBA", (width, abs(height)),
				data[offset:offset + width * abs(height) * 4], "raw", "BGRA", 0, -1 if height > 0 else 1)
			return image.copy()
	return Image.open(path).convert("RGBA")


def opaque_box(alpha, threshold):
	ys, xs = np.where(alpha > threshold)
	return xs.min(), ys.min(), xs.max() + 1, ys.max() + 1


def match_colours(rgb, reference, mask, reference_mask):
	out = rgb.copy()
	for c in range(3):
		src, ref = rgb[..., c][mask], reference[..., c][reference_mask]
		out[..., c] = (rgb[..., c] - src.mean()) / (src.std() + 1e-6) * ref.std() + ref.mean()
	return np.clip(out, 0, 255)


def place(generated, threshold, box, size):
	"""Scales GENERATED so that its part with alpha > threshold fills BOX
	(output pixels) on a transparent canvas of SIZE; what lies outside that
	part (soft rims, fades) comes along at the same scale."""
	gx0, gy0, gx1, gy1 = opaque_box(np.array(generated)[..., 3], threshold)
	sx, sy = (box[2] - box[0]) / (gx1 - gx0), (box[3] - box[1]) / (gy1 - gy0)
	full = generated.resize((round(generated.width * sx), round(generated.height * sy)), Image.LANCZOS)
	canvas = Image.new("RGBA", size)
	canvas.paste(full, (round(box[0] - gx0 * sx), round(box[1] - gy0 * sy)))
	return canvas


def fit_icon(original, generated, scale, margin, box, edge):
	if box is None:
		box = opaque_box(np.array(original)[..., 3], 24)
	size = ((original.width + 2 * margin) * scale, (original.height + 2 * margin) * scale)
	canvas = place(generated, 24, [(v + margin) * scale for v in box], size)
	out, ref = np.array(canvas).astype(float), np.array(original).astype(float)
	out[..., :3] = match_colours(out[..., :3], ref[..., :3], out[..., 3] > 200, ref[..., 3] > 200)
	if edge:
		out[..., 3] = np.clip((out[..., 3] - edge[0]) * 255 / (edge[1] - edge[0]), 0, 255)
	return Image.fromarray(np.uint8(out + 0.5), "RGBA")


def fit_page(original, generated, scale):
	x0, y0, x1, y1 = opaque_box(np.array(original)[..., 3], 0)
	size = (original.width * scale, original.height * scale)
	canvas = Image.new("RGB", size)
	canvas.paste(generated.convert("RGB").resize(((x1 - x0) * scale, (y1 - y0) * scale), Image.LANCZOS),
		(x0 * scale, y0 * scale))
	gen, ref = np.array(canvas).astype(float), np.array(original).astype(float)
	alpha = np.array(original.getchannel("A").resize(size, Image.BILINEAR)).astype(float)
	rgb = match_colours(gen, ref[..., :3], alpha > 250, ref[..., 3] > 250)
	return Image.fromarray(np.dstack([rgb, alpha]).astype(np.uint8), "RGBA")


def over(top, bottom):
	"""Premultiplied 'over' of float RGBA (0..1) arrays."""
	return top + bottom * (1 - top[..., 3:4])


def premultiply(image):
	a = np.array(image).astype(float) / 255
	return np.concatenate([a[..., :3] * a[..., 3:4], a[..., 3:4]], -1)


def unpremultiply(p):
	alpha = p[..., 3:4]
	rgb = p[..., :3] / np.maximum(alpha, 1e-6)
	return Image.fromarray(np.uint8(np.clip(np.concatenate([rgb, alpha], -1) * 255 + 0.5, 0, 255)), "RGBA")


def shadow_of(alpha, sigma, offset, opacity):
	"""A black premultiplied shadow of ALPHA (0..1), blurred and offset (pixels)."""
	shifted = np.zeros_like(alpha)
	dx, dy = offset
	h, w = alpha.shape
	shifted[max(dy, 0):h + min(dy, 0), max(dx, 0):w + min(dx, 0)] = \
		alpha[max(-dy, 0):h + min(-dy, 0), max(-dx, 0):w + min(-dx, 0)]
	a = gaussian(shifted, sigma) * opacity
	return np.dstack([np.zeros(alpha.shape + (3,)), a])


def fit_sheet(original, generated, scale, margin):
	size = ((original.width + 2 * margin) * scale, (original.height + 2 * margin) * scale)
	box = [margin * scale, margin * scale, (original.width + margin) * scale, (original.height + margin) * scale]
	sheet = premultiply(place(generated, 128, box, size))
	# Lying on the table under soft light from above: a close contact shadow
	# and a wide faint one, both only outside the sheet.
	shadow = over(shadow_of(sheet[..., 3], scale * 0.6, (0, round(scale * 0.4)), 0.35),
		shadow_of(sheet[..., 3], scale * 3.0, (0, round(scale * 1.5)), 0.30))
	return unpremultiply(over(sheet, shadow))


def sample(image, x, y):
	"""Bilinear sample of float array IMAGE at float pixel coordinates."""
	h, w = image.shape[:2]
	x, y = np.clip(x - 0.5, 0, w - 1.001), np.clip(y - 0.5, 0, h - 1.001)
	x0, y0 = np.floor(x).astype(int), np.floor(y).astype(int)
	fx, fy = (x - x0)[..., None], (y - y0)[..., None]
	return (image[y0, x0] * (1 - fx) * (1 - fy) + image[y0, x0 + 1] * fx * (1 - fy)
		+ image[y0 + 1, x0] * (1 - fx) * fy + image[y0 + 1, x0 + 1] * fx * fy)


def fit_curl(page_original, page, corner, draw, side, fold, scale, margin):
	"""See the module notes. Coordinates are in page pixels (the original page's
	size, 0,0 at its top-left); the page PNG holds them at page_scale with its
	margin around them."""
	page_scale = page_margin = None
	for s in (2, 3, 4, 8):
		mx, my = page.width / s - page_original.width, page.height / s - page_original.height
		if mx >= 0 and mx % 2 == 0 and my == mx and page.width % s == 0:
			page_scale, page_margin = s, mx / 2
			break
	if page_scale is None:
		raise SystemExit(f"{page.size} is not 2, 3, 4 or 8 times {page_original.size} plus a margin")
	src = premultiply(page)

	out_w, out_h = (corner.width + 2 * margin[0]) * scale, (corner.height + 2 * margin[1]) * scale
	dx, dy, dw, dh = draw
	# Page pixels of the corner image, margin included
	mx, my = margin[0] * dw / corner.width, margin[1] * dh / corner.height
	region = (dx - mx, dy - my, dx + dw + mx, dy + dh + my)
	# Render at a uniform density first (the corner is drawn stretched)
	density = 6
	rw, rh = round((region[2] - region[0]) * density), round((region[3] - region[1]) * density)
	px = region[0] + (np.arange(rw) + 0.5) / density
	py = region[1] + (np.arange(rh) + 0.5) / density
	X, Y = np.meshgrid(px, py)

	def page_at(x, y):
		return sample(src, (x + page_margin) * page_scale, (y + page_margin) * page_scale)

	# Fold line from P (on the bottom edge, A from the corner) to Q (on the
	# side, B up from the corner); it is mirrored for the left corner.
	w, h = page_original.width, page_original.height
	cx = w if side == "right" else 0
	sgn = -1 if side == "right" else 1
	P = np.array([cx + sgn * fold[0], h], float)
	Q = np.array([cx, h - fold[1]], float)
	d = (Q - P) / np.linalg.norm(Q - P)
	n = np.array([-d[1], d[0]])
	corner_pt = np.array([cx, h], float)
	if np.dot(corner_pt - P, n) < 0:
		n = -n
	# Signed distance from the fold line, positive on the corner's side
	dist = (X - P[0]) * n[0] + (Y - P[1]) * n[1]
	# Mirror image across the fold line (where the flap's paper comes from)
	RX, RY = X - 2 * dist * n[0], Y - 2 * dist * n[1]

	base = page_at(X, Y)
	removed = np.clip(dist * density + 0.5, 0, 1)[..., None]
	# The page underneath, where the corner was lifted off: same sheet, a
	# little darker, and in the flap's shadow close to the fold.
	under = base.copy()
	under[..., :3] *= (0.84 + 0.12 * np.clip(dist / 30, 0, 1) ** 0.7)[..., None]
	top = base * (1 - removed) + under * removed

	# The flap: the corner mirrored over the fold, so it lies on the page side.
	flap = page_at(RX, RY)
	flap *= np.clip(-dist * density + 0.5, 0, 1)[..., None]
	# Its back is a little paler and plainer than the front, lit like a curl:
	# brightest just past the fold, falling off toward the tip.
	t = np.clip(-dist / max(fold), 0, 1)
	light = 1.02 - 0.20 * t ** 1.5 + 0.06 * np.exp(-((-dist - 3) / 3) ** 2)
	paper = flap[..., :3] / np.maximum(flap[..., 3:4], 1e-6)
	paper = paper * 0.65 + paper.mean(axis=(0, 1), where=flap[..., 3:4] > 0.5) * 0.35
	flap[..., :3] = np.clip(paper * light[..., None], 0, 1) * flap[..., 3:4]
	# A thin darker line at the fold itself
	crease = np.exp(-((dist * density) / (0.8 * density)) ** 2) * 0.25
	flap[..., :3] *= (1 - crease)[..., None]

	shadow = shadow_of(flap[..., 3], 2.0 * density, (round(-sgn * 1.0 * density), round(1.5 * density)), 0.35)
	shadow *= top[..., 3:4]
	result = over(flap, over(shadow, top))

	# Keep only what differs from the page underneath; the rest is drawn by
	# the page itself, so the corner adds nothing there.
	diff = np.abs(result - base).max(-1)
	keep = np.clip(gaussian((diff > 0.004).astype(float), 1.5 * density) * 3, 0, 1)[..., None]
	result *= keep

	rendered = unpremultiply(result)
	return rendered.resize((out_w, out_h), Image.LANCZOS)


def tint(original, tinted, fitted):
	a, b = np.array(original).astype(float), np.array(tinted).astype(float)
	mask = a[..., 3] > 128
	lum = a[..., :3].mean(-1)[mask]
	out = np.array(fitted).astype(float)
	out_lum = out[..., :3].mean(-1)
	for c in range(3):
		out[..., c] = np.clip(np.polyval(np.polyfit(lum, b[..., c][mask], 2), out_lum), 0, 255)
	return Image.fromarray(out.astype(np.uint8), "RGBA")


def gaussian(x, sigma, mode="constant"):
	r = int(np.ceil(sigma * 3))
	k = np.exp(-0.5 * (np.arange(-r, r + 1) / sigma) ** 2)
	k /= k.sum()
	for axis in (0, 1):
		pad = [(0, 0)] * x.ndim
		pad[axis] = (r, r)
		padded = np.pad(x, pad, mode=mode)
		x = sum(k[i] * np.take(padded, range(i, i + x.shape[axis]), axis=axis) for i in range(2 * r + 1))
	return x


def soften(image, sigma):
	return unpremultiply(gaussian(premultiply(image), sigma))


def without_margin(original, result):
	"""RESULT's part that covers ORIGINAL's rectangle (its margin removed)."""
	for s in (8, 4, 3, 2):
		if result.width % s or result.height % s:
			continue
		mx, my = result.width // s - original.width, result.height // s - original.height
		if mx >= 0 and my >= 0 and mx % 2 == 0 and my % 2 == 0:
			return result.crop((mx // 2 * s, my // 2 * s, result.width - mx // 2 * s, result.height - my // 2 * s))
	raise SystemExit(f"{result.size} is not 2, 3, 4 or 8 times {original.size} plus an even margin")


def score(original, result):
	result = without_margin(original, result)
	return np.abs(premultiply(original) - premultiply(result.resize(original.size, Image.BOX))).mean()


def main():
	ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
	sub = ap.add_subparsers(dest="command", required=True)
	icon = sub.add_parser("icon")
	icon.add_argument("original"); icon.add_argument("generated"); icon.add_argument("out")
	icon.add_argument("--scale", type=int, default=4)
	icon.add_argument("--margin", type=int, default=0)
	icon.add_argument("--box", type=float, nargs=4, metavar=("X0", "Y0", "X1", "Y1"))
	icon.add_argument("--edge", type=float, nargs=2, metavar=("LO", "HI"))
	page = sub.add_parser("page")
	page.add_argument("original"); page.add_argument("generated"); page.add_argument("out")
	page.add_argument("--scale", type=int, default=3)
	sheet = sub.add_parser("sheet")
	sheet.add_argument("original"); sheet.add_argument("generated"); sheet.add_argument("out")
	sheet.add_argument("--scale", type=int, default=3)
	sheet.add_argument("--margin", type=int, default=8)
	curl = sub.add_parser("curl")
	curl.add_argument("original"); curl.add_argument("page"); curl.add_argument("corner"); curl.add_argument("out")
	curl.add_argument("--draw", type=float, nargs=4, required=True, metavar=("X", "Y", "W", "H"))
	curl.add_argument("--side", choices=("right", "left"), default="right")
	curl.add_argument("--fold", type=float, nargs=2, default=(80, 90), metavar=("A", "B"))
	curl.add_argument("--scale", type=int, default=8)
	curl.add_argument("--margin", type=int, nargs=2, default=(6, 5), metavar=("MX", "MY"))
	tinted = sub.add_parser("tint")
	tinted.add_argument("original"); tinted.add_argument("tinted"); tinted.add_argument("fitted"); tinted.add_argument("out")
	soft = sub.add_parser("soften")
	soft.add_argument("original", metavar="in"); soft.add_argument("out")
	soft.add_argument("--sigma", type=float, required=True, help="in pixels of IN")
	scored = sub.add_parser("score")
	scored.add_argument("original"); scored.add_argument("out")
	args = ap.parse_args()

	original = load(args.original)
	if args.command == "soften":
		soften(original, args.sigma).save(args.out)
		print(f"{args.out}: softened by {args.sigma}")
		return
	if args.command == "score":
		print(f"{score(original, load(args.out)):.4f}")
		return
	if args.command == "curl":
		corner = load(args.corner)
		result = fit_curl(original, load(args.page), corner, args.draw, args.side, args.fold,
			args.scale, args.margin)
		result.save(args.out)
		print(f"{args.out}: {result.width}x{result.height}")
		return
	if args.command == "icon":
		result = fit_icon(original, load(args.generated), args.scale, args.margin, args.box, args.edge)
	elif args.command == "page":
		result = fit_page(original, load(args.generated), args.scale)
	elif args.command == "sheet":
		result = fit_sheet(original, load(args.generated), args.scale, args.margin)
	else:
		result = tint(original, load(args.tinted), load(args.fitted))
	result.save(args.out)
	print(f"{args.out}: {result.width}x{result.height}, difference {score(original, result):.4f}")


if __name__ == "__main__":
	main()
