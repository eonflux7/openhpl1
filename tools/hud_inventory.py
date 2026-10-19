#!/usr/bin/env python3
# This file is part of OpenHPL1, licensed under the GNU GPL v3 or
# later. See COPYING.
"""Draw the remastered inventory drawer's images into hud/.

  hud_inventory.py PAPER.png

PAPER is the generated paper drawer (a long sheet of paper on a transparent
background; see hud/src/prompts/inventory-drawer.md). The metal look, the
default, is drawn here from a generated rust photograph
(hud/src/textures/rust.png, prompt in hud/src/prompts/rust.md): the
original's darkened background and rusty frames. Each look gets:

  inventory_drawer_<look>       the drawer, stretched to 300 x screen height
  inventory_cell_<look>[_hover] an item cell of the grid (62x62)
  inventory_hotkey_<look>[_hover] a hotkey slot (48x48)

New images, laid out at 1/N of their size (`@<N>x`). The cells and marks are
drawn from fixed random seeds, so running this again gives the same files.
Needs NumPy and Pillow.
"""

import argparse
import pathlib

import numpy as np
from PIL import Image

from hud_pencil import gaussian, grain, smooth_noise

OUT = pathlib.Path(__file__).resolve().parent.parent / "hud"
SCALE = 4
CELL = 62
HOTKEY = 48
GRAPHITE = (0.2, 0.19, 0.18)


def save(rgba, name, scale=SCALE):
	path = OUT / f"{name}@{scale}x.png"
	Image.fromarray(np.uint8(np.clip(rgba * 255 + 0.5, 0, 255)), "RGBA").save(path)
	print(f"{path.relative_to(OUT.parent)}: {rgba.shape[1]}x{rgba.shape[0]}")


def load(path):
	return np.asarray(Image.open(path).convert("RGBA"), dtype=np.float64) / 255


def over(top, bottom):
	a = top[..., 3:] + bottom[..., 3:] * (1 - top[..., 3:])
	rgb = (top[..., :3] * top[..., 3:] + bottom[..., :3] * bottom[..., 3:] * (1 - top[..., 3:])) / np.maximum(a, 1e-6)
	return np.concatenate([rgb, a], axis=-1)


def drawer(path, size=(300, 600), scale=3):
	image = Image.open(path).convert("RGBA")
	return np.asarray(image.resize((size[0] * scale, size[1] * scale), Image.LANCZOS), dtype=np.float64) / 255


def rounded_box(n, half, radius):
	"""Signed distance (pixels) to a rounded square of half-size HALF
	centred in an NxN image, negative inside, and its unit gradient."""
	y, x = np.mgrid[0:n, 0:n] + 0.5
	px, py = x - n / 2, y - n / 2
	qx = np.abs(px) - half + radius
	qy = np.abs(py) - half + radius
	ox, oy = np.maximum(qx, 0), np.maximum(qy, 0)
	outside = np.hypot(ox, oy)
	dist = outside + np.minimum(np.maximum(qx, qy), 0) - radius
	# Gradient: away from the nearest point of the inner square
	gx = np.where(outside > 0, ox / np.maximum(outside, 1e-6), (qx >= qy) * 1.0) * np.sign(px)
	gy = np.where(outside > 0, oy / np.maximum(outside, 1e-6), (qy > qx) * 1.0) * np.sign(py)
	return dist, gx, gy


RUST = OUT / "src" / "textures" / "rust.png"


def rust_texture(rng, shape):
	"""SHAPE of the rust photograph (src/textures/rust.png, generated),
	tiled from a random offset and toned down to the game's dark palette."""
	tex = np.asarray(Image.open(RUST).convert("RGB"), dtype=np.float64) / 255
	h, w = shape
	oy, ox = rng.integers(0, tex.shape[0]), rng.integers(0, tex.shape[1])
	ys = (np.arange(h) + oy) % tex.shape[0]
	xs = (np.arange(w) + ox) % tex.shape[1]
	t = tex[ys][:, xs]
	grey = t.mean(axis=-1, keepdims=True)
	return np.clip((grey + 0.75 * (t - grey)) * 0.78, 0, 1)


def rust(rng, shape, dist, gx, gy, w, scale=SCALE):
	"""A flat band of rusted iron, half-width W along DIST = 0, its edges
	bevelled and lit from the upper left, with a dark outline. GX, GY: unit
	direction across. The colour and relief come from the rust photograph;
	its darkest pits eat into the band's edges."""
	h, n = shape
	tex = rust_texture(rng, (h, n))
	lum = tex.mean(axis=-1)
	relief = gaussian(lum, 0.6 * scale)
	pits = np.clip((0.22 - relief) / 0.12, 0, 1)
	ring = np.abs(dist) - w + 0.5 * scale * pits
	cover = np.clip(0.5 - ring, 0, 1)
	outline = np.clip(0.5 - (np.abs(dist) - w - 0.45 * scale), 0, 1)
	# Bevel: the outer 0.8 unit of each side slopes away
	bevel = 0.8 * scale
	slope = np.clip((np.abs(dist) - (w - bevel)) / bevel, 0, 1) * np.sign(dist)
	ny_x, ny_y = slope * gx * 0.8, slope * gy * 0.8
	# Relief of the rust itself
	ry, rx = np.gradient(relief)
	ny_x = ny_x - rx * 6 * scale
	ny_y = ny_y - ry * 6 * scale
	nz = np.ones_like(dist)
	norm = np.sqrt(ny_x ** 2 + ny_y ** 2 + nz ** 2)
	light = np.array([-0.5, -0.6, 0.62])
	light /= np.linalg.norm(light)
	diffuse = np.clip((ny_x * light[0] + ny_y * light[1] + nz * light[2]) / norm, 0, 1)
	flat = light[2]
	shade = 0.35 + 0.65 * diffuse / flat
	rgba = np.zeros((h, n, 4))
	bar = np.clip(tex * shade[..., None], 0, 1)
	rgba[..., :3] = bar * cover[..., None] + np.array([0.03, 0.022, 0.016]) * (1 - cover[..., None])
	rgba[..., 3] = np.maximum(cover, 0.85 * outline)
	return rgba


def rust_frame(seed, size, inset, width, radius, bright=1.0, fill=0.38):
	"""A rounded frame of rusty iron like the original's slots, with the
	space inside it darkened."""
	rng = np.random.default_rng(seed)
	n = size * SCALE
	half = (size / 2 - inset - width / 2) * SCALE
	dist, gx, gy = rounded_box(n, half, radius * SCALE)
	w = width * SCALE / 2
	frame = rust(rng, (n, n), dist, gx, gy, w)
	frame[..., :3] = np.clip(frame[..., :3] * bright, 0, 1)
	# Darkened inside, deeper in the frame's shadow at the upper left
	inside = np.clip(-(dist + w) / 1.2, 0, 1)
	shadow = np.clip(1 + (dist + w) / (2.5 * SCALE), 0, 1) * np.clip(0.5 - (gx + gy) * 0.35, 0, 1)
	back = np.zeros((n, n, 4))
	back[..., 3] = inside * np.clip(fill + 0.35 * shadow, 0, 1)
	# A soft drop shadow outside, so the frame stands off any background
	drop = gaussian(np.roll(frame[..., 3], (int(0.6 * SCALE), int(0.5 * SCALE)), (0, 1)), 0.4 * SCALE)
	under = np.zeros((n, n, 4))
	under[..., 3] = 0.45 * drop * (dist > 0)
	return over(frame, over(back, under))


def glow(size, inset, strength):
	"""A faint warm light inside a cell (under the mouse)."""
	n = size * SCALE
	dist, _, _ = rounded_box(n, (size / 2 - inset) * SCALE, 4 * SCALE)
	y, x = np.mgrid[0:n, 0:n] + 0.5
	inside = np.clip(-dist / (3 * SCALE), 0, 1)
	centre = 1 - 0.7 * np.hypot(x - n / 2, y - n / 2) / (n / 2)
	rgba = np.zeros((n, n, 4))
	rgba[..., :3] = (0.95, 0.8, 0.55)
	rgba[..., 3] = inside * np.clip(centre, 0, 1) * strength
	return rgba


def dark_drawer(seed, size=(300, 600), scale=3):
	"""The original inventory darkened the whole screen; the drawer darkens
	its strip, fading in from a rusty rail down its left edge."""
	rng = np.random.default_rng(seed)
	w, h = size[0] * scale, size[1] * scale
	x = np.arange(w)[None, :] + 0.5
	fade = np.clip((x - 3 * scale) / (40 * scale), 0, 1)
	small = gaussian(rng.normal(0, 1, (h // (10 * scale), h // (10 * scale))), 3)
	mottle = np.asarray(Image.fromarray(np.float32(small / small.std())).resize((h, h), Image.BICUBIC))[:, :w]
	back = np.zeros((h, w, 4))
	back[..., :3] = (0.035, 0.03, 0.028)
	back[..., 3] = np.clip((0.8 + 0.03 * mottle) * (0.7 + 0.3 * fade), 0, 1)
	rail_x = 2.5 * scale
	dist = np.broadcast_to(x - rail_x, (h, w))
	rail = rust(rng, (h, w), dist, np.ones((h, w)), np.zeros((h, w)), 1.8 * scale, scale)
	return over(rail, back)


def pencil_line(rng, n, w, h, p0, p1, width, pressure):
	"""Alpha of a hand-drawn pencil line from P0 to P1 (pixels) on a WxH
	image, a little past both ends, with a slight bow and wobble."""
	y, x = np.mgrid[0:h, 0:w] + 0.5
	d = np.subtract(p1, p0)
	length = np.hypot(*d)
	u = d / length
	nrm = np.array([-u[1], u[0]])
	t = ((x - p0[0]) * u[0] + (y - p0[1]) * u[1]) / length
	bow = rng.uniform(-0.6, 0.6) * SCALE * 4 * np.clip(t, 0, 1) * (1 - np.clip(t, 0, 1))
	wobble = np.interp(np.clip(t, 0, 1) * (n - 1), np.arange(n), 0.25 * SCALE * smooth_noise(rng, n, n / 4))
	dist = np.abs((x - p0[0]) * nrm[0] + (y - p0[1]) * nrm[1] - bow - wobble)
	start, end = rng.uniform(-0.04, 0.0), rng.uniform(1.0, 1.05)
	along = np.clip(np.minimum((t - start) / 0.05, (end - t) / 0.12), 0, 1) ** 0.8
	return np.clip(width - dist + 0.5, 0, 1) * along * pressure


def pencil_box(seed, size, inset, pressure=0.55):
	rng = np.random.default_rng(seed)
	n = size * SCALE
	a, b = inset * SCALE, (size - inset) * SCALE
	j = lambda: rng.uniform(-0.8, 0.8) * SCALE
	corners = [(a + j(), a + j()), (b + j(), a + j()), (b + j(), b + j()), (a + j(), b + j())]
	alpha = np.zeros((n, n))
	for i in range(4):
		p0, p1 = corners[i], corners[(i + 1) % 4]
		alpha = np.maximum(alpha, pencil_line(rng, n, n, n, p0, p1, 0.32 * SCALE, pressure))
	alpha *= grain(rng, (n, n), 2.0)
	rgba = np.zeros((n, n, 4))
	rgba[..., :3] = GRAPHITE
	rgba[..., 3] = np.clip(alpha, 0, 1)
	return rgba


def shading(seed, size, inset, strength):
	"""A light graphite wash inside the box, rubbed in unevenly."""
	rng = np.random.default_rng(seed)
	n = size * SCALE
	y, x = np.mgrid[0:n, 0:n] + 0.5
	a, b = (inset + 1.2) * SCALE, (size - inset - 1.2) * SCALE
	inside = np.clip(np.minimum.reduce([x - a, b - x, y - a, b - y]) / (2 * SCALE), 0, 1)
	wash = inside * (0.75 + 0.25 * gaussian(rng.normal(0, 1, (n, n)), 6 * SCALE) * 6) * grain(rng, (n, n), 3.0)
	rgba = np.zeros((n, n, 4))
	rgba[..., :3] = GRAPHITE
	rgba[..., 3] = np.clip(wash * strength, 0, 1)
	return rgba


def patch(texture, seed, size, rough, shadow):
	"""A square cut from TEXTURE (the drawer's) with a slightly uneven edge
	and a soft shadow under it, for the hotkey slots."""
	rng = np.random.default_rng(seed)
	n = size * SCALE
	h, w = texture.shape[:2]
	y0, x0 = int(h * rng.uniform(0.3, 0.6)), int(w * rng.uniform(0.25, 0.45))
	piece = texture[y0:y0 + n, x0:x0 + n].copy()
	y, x = np.mgrid[0:n, 0:n] + 0.5
	m = 2.2 * SCALE
	edge = np.minimum.reduce([x, n - x, y, n - y]) - m
	edge += rough * SCALE * gaussian(rng.normal(0, 1, (n, n)), 0.8 * SCALE) * 3
	piece[..., 3] = np.clip(edge / (0.7 * SCALE), 0, 1)
	under = np.zeros((n, n, 4))
	soft = gaussian(np.roll(piece[..., 3], (int(0.8 * SCALE), int(0.6 * SCALE)), (0, 1)), 1.2 * SCALE)
	under[..., 3] = soft * shadow
	return over(piece, under)


def main():
	ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
	ap.add_argument("paper")
	args = ap.parse_args()

	# Metal: the original's rusty slot frames on a darkened strip
	save(dark_drawer(81), "inventory_drawer_metal", 3)
	save(rust_frame(71, CELL, 2, 3.2, 7), "inventory_cell_metal")
	save(over(glow(CELL, 3, 0.16), rust_frame(71, CELL, 2, 3.2, 7, bright=1.5, fill=0.22)), "inventory_cell_metal_hover")
	save(rust_frame(72, HOTKEY, 2, 2.8, 6, fill=0.55), "inventory_hotkey_metal")
	save(over(glow(HOTKEY, 3, 0.16), rust_frame(72, HOTKEY, 2, 2.8, 6, bright=1.5, fill=0.4)), "inventory_hotkey_metal_hover")

	paper = drawer(args.paper)
	save(paper, "inventory_drawer_paper", 3)

	# Paper: pencil boxes, shaded a little when the mouse is over one
	save(pencil_box(41, CELL, 2), "inventory_cell_paper")
	save(over(pencil_box(41, CELL, 2), shading(42, CELL, 2, 0.16)), "inventory_cell_paper_hover")
	tag = patch(load(args.paper), 51, HOTKEY, 0.5, 0.45)
	save(over(pencil_box(52, HOTKEY, 6, 0.4), tag), "inventory_hotkey_paper")
	save(over(shading(53, HOTKEY, 6, 0.16), over(pencil_box(52, HOTKEY, 6, 0.4), tag)), "inventory_hotkey_paper_hover")


if __name__ == "__main__":
	main()
