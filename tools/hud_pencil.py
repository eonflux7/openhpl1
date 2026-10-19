#!/usr/bin/env python3
# This file is part of OpenHPL1, licensed under the GNU GPL v3 or
# later. See COPYING.
"""Draw the notebook's pencil and ink marks into hud/.

  hud_pencil.py

These are new images (the game has none of them), so each is laid out at a
quarter of its pixel size (`@4x`):

  notebook_underline_1..3   pencil underlines under a hovered note title,
                            stretched to the title's width (200x8)
  notebook_rule             a faint hand-ruled pencil line under each entry
                            of the notes list (340x2)
  notebook_margin           the faded red margin line of the list (3x460)
  notebook_unread           an ink dot after a note not read yet (6x6)

Each is drawn from a fixed random seed, so running this again gives the same
files. Needs NumPy and Pillow.
"""

import pathlib

import numpy as np
from PIL import Image

OUT = pathlib.Path(__file__).resolve().parent.parent / "hud"
SCALE = 4


def gaussian(x, sigma):
	r = int(np.ceil(sigma * 3))
	k = np.exp(-0.5 * (np.arange(-r, r + 1) / sigma) ** 2)
	k /= k.sum()
	for axis in (0, 1):
		pad = [(0, 0)] * x.ndim
		pad[axis] = (r, r)
		padded = np.pad(x, pad, mode="wrap")
		x = sum(k[i] * np.take(padded, range(i, i + x.shape[axis]), axis=axis) for i in range(2 * r + 1))
	return x


def smooth_noise(rng, n, scale):
	"""1D noise, -1..1, varying over about SCALE samples."""
	points = rng.uniform(-1, 1, n // max(int(scale), 1) + 3)
	return np.interp(np.arange(n) / scale, np.arange(len(points)), points)


def grain(rng, shape, coarse):
	"""Paper tooth: where graphite catches, 0..1."""
	g = gaussian(rng.normal(0, 1, shape), 1.2) + 0.7 * gaussian(rng.normal(0, 1, shape), coarse)
	return np.clip(0.84 + 0.2 * g / g.std(), 0, 1)


def stroke(rng, w, h, centre, half_width, pressure, colour, coarse=2.0):
	"""A pencil line along x: CENTRE(x) and HALF_WIDTH(x) in pixels,
	PRESSURE(x) 0..1."""
	y = np.arange(h)[:, None] + 0.5
	cover = np.clip(half_width[None, :] - np.abs(y - centre[None, :]) + 0.5, 0, 1)
	alpha = cover * pressure[None, :] * grain(rng, (h, w), coarse)
	rgba = np.zeros((h, w, 4))
	rgba[..., :3] = colour
	rgba[..., 3] = np.clip(alpha, 0, 1)
	return rgba


def save(rgba, name):
	path = OUT / f"{name}@{SCALE}x.png"
	Image.fromarray(np.uint8(np.clip(rgba * 255 + 0.5, 0, 255)), "RGBA").save(path)
	print(f"{path.relative_to(OUT.parent)}: {rgba.shape[1]}x{rgba.shape[0]}")


def underline(seed):
	rng = np.random.default_rng(seed)
	w, h = 200 * SCALE, 8 * SCALE
	t = np.linspace(0, 1, w)
	# Drawn quickly from left to right: a slight rise and sag, a little wobble
	centre = (h / 2 + rng.uniform(-0.6, 0.6) * SCALE * (t - 0.5)
		+ rng.uniform(0.6, 1.2) * SCALE * (4 * t * (1 - t) - 0.5)
		+ 0.25 * SCALE * smooth_noise(rng, w, w / 5))
	# Presses in at the start, lifts off in a longer flick at the end
	pressure = np.clip(np.minimum(t / 0.04, (1 - t) / 0.18), 0, 1) ** 0.8
	pressure *= 0.82 + 0.12 * smooth_noise(rng, w, w / 7)
	half_width = (0.5 + 0.5 * pressure) * SCALE
	return stroke(rng, w, h, centre, half_width, np.clip(pressure * 1.1, 0, 1), (0.13, 0.12, 0.12))


def rule():
	rng = np.random.default_rng(11)
	w, h = 340 * SCALE, 2 * SCALE
	t = np.linspace(0, 1, w)
	centre = h / 2 + 0.1 * SCALE * smooth_noise(rng, w, w / 4)
	pressure = np.clip(np.minimum(t / 0.01, (1 - t) / 0.01), 0, 1) * (0.75 + 0.2 * smooth_noise(rng, w, w / 9))
	return stroke(rng, w, h, centre, np.full(w, 0.22 * SCALE), pressure, (0.36, 0.35, 0.34), coarse=3)


def margin():
	rng = np.random.default_rng(23)
	w, h = 3 * SCALE, 460 * SCALE
	t = np.linspace(0, 1, h)
	centre = w / 2 + 0.08 * SCALE * smooth_noise(rng, h, h / 6)
	pressure = 0.8 + 0.15 * smooth_noise(rng, h, h / 10)
	# Printed: even, a little faded
	line = stroke(rng, h, w, centre, np.full(h, 0.45 * SCALE), pressure, (0.72, 0.28, 0.22), coarse=6)
	return line.transpose(1, 0, 2)


def unread():
	rng = np.random.default_rng(31)
	n = 6 * SCALE
	y, x = np.mgrid[0:n, 0:n] + 0.5
	angle = np.arctan2(y - n / 2, x - n / 2)
	# A blot of ink, a little uneven at the edge
	radius = 1.5 * SCALE * (1 + 0.08 * np.sin(3 * angle + rng.uniform(0, 6)) + 0.05 * np.sin(5 * angle))
	cover = np.clip(radius - np.hypot(x - n / 2, y - n / 2) + 0.5, 0, 1)
	rgba = np.zeros((n, n, 4))
	rgba[..., :3] = (0.16, 0.11, 0.08)
	rgba[..., 3] = cover * 0.92
	return rgba


def main():
	for i in (1, 2, 3):
		save(underline(i), f"notebook_underline_{i}")
	save(rule(), "notebook_rule")
	save(margin(), "notebook_margin")
	save(unread(), "notebook_unread")


if __name__ == "__main__":
	main()
