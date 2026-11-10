#!/usr/bin/env python3
# This file is part of OpenHPL1, licensed under the GNU GPL v3 or
# later. See COPYING.
"""Builds the "OpenHPL FX" addon: particle flipbooks from Unity Labs' CC0
image sequences in place of the games' small particle pictures.

  fxpack.py build [--out DIR] [--cache DIR] [--only NAME,...] [--list]

Unity Labs released the sequences under CC0 1.0 (public domain), see
https://unity.com/blog/engine-platform/free-vfx-image-sequences-flipbooks.
The sequences are downloaded into the cache once. Each replaced material
becomes a .mat of the same name with Flipbook="columns rows", which makes
the emitters that use it play the frames over each particle's life with
each frame blended into the next (docs/modding.md, Particle flipbooks).
The games' .ps files are not changed.

Use the addon with OPENHPL_ADDON_DIRS=<out> or copy it into the personal
folder's addons/. Needs numpy.
"""

import argparse
import io
import os
import sys
import urllib.request
import zipfile

import numpy as np

URL = 'https://unity3d.com/files/labs/downloads/vfx/assets01/{0}/{0}-sequence-tga.zip'

# material: the game's material name the addon replaces
# sequence: Unity Labs sequence; type: the material's blend type
# grid: columns, rows; frame: width, height of a frame in the sheet
# premultiply: colour times alpha, for added (Additive) materials
# depth_test: the original material's DepthTest
# frames: the part of the sequence that is used (first, last), spread over
# the sheet; None for all of it
EFFECTS = [
	dict(material='particle_vent_smoke01', sequence='WispySmoke01', type='Alpha',
		 grid=(8, 8), frame=(256, 256), premultiply=False, depth_test=True, frames=None),
	# The first frames light the fire; it burns from about frame 60
	dict(material='particle_flame02', sequence='Flame02', type='Additive',
		 grid=(16, 4), frame=(128, 256), premultiply=True, depth_test=True, frames=(60, 187)),
]


#############################################################
# TGA

def read_tga(data):
	"""RGBA float32 array (rows top first) from TGA bytes: true colour,
	24 or 32 bits, plain or run-length."""
	id_len, cmap_type, img_type = data[0], data[1], data[2]
	w = int.from_bytes(data[12:14], 'little')
	h = int.from_bytes(data[14:16], 'little')
	bpp, desc = data[16], data[17]
	if cmap_type != 0 or img_type not in (2, 10) or bpp not in (24, 32):
		raise ValueError('unsupported TGA (type %d, %d bits)' % (img_type, bpp))
	ch = bpp // 8
	pos = 18 + id_len + int.from_bytes(data[5:7], 'little') * ((data[7] + 7) // 8)
	count = w * h
	if img_type == 2:
		px = np.frombuffer(data, np.uint8, count * ch, pos).reshape(count, ch)
	else:
		px = np.empty((count, ch), np.uint8)
		i = 0
		while i < count:
			head = data[pos]; pos += 1
			n = (head & 0x7f) + 1
			if head & 0x80:
				px[i:i + n] = np.frombuffer(data, np.uint8, ch, pos); pos += ch
			else:
				px[i:i + n] = np.frombuffer(data, np.uint8, n * ch, pos).reshape(n, ch); pos += n * ch
			i += n
	img = px.reshape(h, w, ch)[..., [2, 1, 0] + ([3] if ch == 4 else [])]
	if ch == 3:
		img = np.concatenate([img, np.full((h, w, 1), 255, np.uint8)], axis=2)
	if not desc & 0x20:
		img = img[::-1]
	return img.astype(np.float32) / 255.0


def write_tga(path, img):
	"""Plain 32-bit TGA, bottom row first, from RGBA float rows top first."""
	h, w, _ = img.shape
	px = np.clip(np.rint(img * 255.0), 0, 255).astype(np.uint8)[::-1][..., [2, 1, 0, 3]]
	head = bytes([0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0]) + w.to_bytes(2, 'little') + \
		h.to_bytes(2, 'little') + bytes([32, 8])
	with open(path, 'wb') as f:
		f.write(head)
		f.write(px.tobytes())


#############################################################
# Resampling

def filter_matrix(n_in, n_out):
	"""Weights (n_out x n_in) of a triangle filter as wide as one output
	pixel, so that shrinking averages instead of skipping pixels."""
	scale = n_in / n_out
	width = max(scale, 1.0)
	centres = (np.arange(n_out) + 0.5) * scale
	x = np.arange(n_in) + 0.5
	wts = np.clip(1.0 - np.abs(x[None, :] - centres[:, None]) / width, 0.0, None)
	return wts / wts.sum(axis=1, keepdims=True)


def resample(img, w, h):
	"""Resizes premultiplied RGBA so that clear pixels do not darken edges."""
	pre = img.copy()
	pre[..., :3] *= pre[..., 3:4]
	my = filter_matrix(img.shape[0], h)
	mx = filter_matrix(img.shape[1], w)
	out = np.einsum('yi,ijc,xj->yxc', my, pre, mx, optimize=True)
	a = out[..., 3:4]
	out[..., :3] = np.where(a > 1e-5, out[..., :3] / np.maximum(a, 1e-5), 0.0)
	return out


def fit(img, w, h):
	"""Scales to fit w x h keeping the aspect, centred on a clear frame."""
	scale = min(w / img.shape[1], h / img.shape[0])
	fw = max(1, round(img.shape[1] * scale))
	fh = max(1, round(img.shape[0] * scale))
	small = resample(img, fw, fh)
	out = np.zeros((h, w, 4), np.float32)
	x0, y0 = (w - fw) // 2, (h - fh) // 2
	out[y0:y0 + fh, x0:x0 + fw] = small
	return out


#############################################################
# Building

def fetch(sequence, cache):
	path = os.path.join(cache, sequence + '-sequence-tga.zip')
	if not os.path.exists(path):
		os.makedirs(cache, exist_ok=True)
		url = URL.format(sequence)
		print('  downloading', url)
		req = urllib.request.Request(url, headers={'User-Agent': 'openhpl1-fxpack'})
		with urllib.request.urlopen(req, timeout=120) as r:
			data = r.read()
		with open(path + '.part', 'wb') as f:
			f.write(data)
		os.replace(path + '.part', path)
	return path


def frames(zip_path):
	with zipfile.ZipFile(zip_path) as z:
		names = sorted(n for n in z.namelist() if n.lower().endswith('.tga'))
		for name in names:
			yield read_tga(z.read(name))


def crop_box(images, aspect, keep=0.99, margin=0.06):
	"""The box (x0, y0, x1, y1) that holds the keep part of all the images'
	opacity together, with a margin, widened to the frame's aspect (w/h).
	Faint haze reaches the edges of most frames, so a box round every
	visible pixel would keep nearly all of them."""
	mass = np.zeros(images[0].shape[:2], np.float64)
	for img in images:
		mass += img[..., 3]
	h, w = mass.shape
	total = mass.sum()
	if total <= 0:
		return 0, 0, w, h
	cut = (1.0 - keep) / 2.0
	def bounds(profile):
		c = np.cumsum(profile) / total
		return int(np.searchsorted(c, cut)), int(np.searchsorted(c, 1.0 - cut)) + 1
	x0, x1 = bounds(mass.sum(axis=0))
	y0, y1 = bounds(mass.sum(axis=1))
	bw, bh = (x1 - x0) * (1 + 2 * margin), (y1 - y0) * (1 + 2 * margin)
	if bw / bh < aspect:
		bw = bh * aspect
	else:
		bh = bw / aspect
	cx, cy = (x0 + x1) / 2, (y0 + y1) / 2
	return (int(round(cx - bw / 2)), int(round(cy - bh / 2)),
			int(round(cx + bw / 2)), int(round(cy + bh / 2)))


def crop(img, box):
	"""img inside box; outside the picture is clear."""
	x0, y0, x1, y1 = box
	out = np.zeros((y1 - y0, x1 - x0, 4), np.float32)
	h, w = img.shape[:2]
	sx0, sy0, sx1, sy1 = max(x0, 0), max(y0, 0), min(x1, w), min(y1, h)
	out[sy0 - y0:sy1 - y0, sx0 - x0:sx1 - x0] = img[sy0:sy1, sx0:sx1]
	return out


def build_sheet(effect, cache):
	cols, rows = effect['grid']
	fw, fh = effect['frame']
	src = list(frames(fetch(effect['sequence'], cache)))
	first, last = effect['frames'] or (0, len(src) - 1)
	last = min(last, len(src) - 1)
	count = cols * rows
	if last - first + 1 < count:
		raise ValueError('%s: frames %d-%d, %d wanted' % (effect['sequence'], first, last, count))
	# Evenly over the part used, its first and last included
	pick = np.rint(np.linspace(first, last, count)).astype(int)
	# The sequences leave much empty room round the smoke: keep the part
	# that any of the frames uses, so that the sheet's pixels go to it
	box = crop_box([src[i] for i in pick], fw / fh)
	sheet = np.zeros((rows * fh, cols * fw, 4), np.float32)
	for i, idx in enumerate(pick):
		cell = fit(crop(src[idx], box), fw, fh)
		if effect['premultiply']:
			cell[..., :3] *= cell[..., 3:4]
		y, x = divmod(i, cols)
		sheet[y * fh:(y + 1) * fh, x * fw:(x + 1) * fw] = cell
	return sheet, len(src)


def material_xml(effect, texture):
	cols, rows = effect['grid']
	return ('<Material>\n'
			'  <Main Type="%s" PhysicsMaterial="Default" UseAlpha="False" DepthTest="%s" '
			'Flipbook="%d %d" FlipbookBlend="True" />\n'
			'  <TextureUnits>\n'
			'    <Diffuse File="%s" Compress="false" Type="2D" Mipmaps="true" Wrap="ClampToEdge" />\n'
			'  </TextureUnits>\n'
			'</Material>\n') % (effect['type'], 'True' if effect['depth_test'] else 'False',
								cols, rows, texture)


def build(args):
	chosen = EFFECTS
	if args.only:
		names = set(n.strip().lower() for n in args.only.split(','))
		chosen = [e for e in EFFECTS if e['material'] in names or e['sequence'].lower() in names]
		if not chosen:
			sys.exit('nothing matches --only %s' % args.only)

	out = os.path.abspath(args.out)
	os.makedirs(os.path.join(out, 'particles'), exist_ok=True)
	with open(os.path.join(out, 'addon.cfg'), 'w') as f:
		f.write('<Addon Name="OpenHPL FX" Priority="0" Enabled="true" />\n')
	with open(os.path.join(out, 'README.txt'), 'w') as f:
		f.write('Built by tools/fxpack.py from Unity Labs image sequences, released\n'
				'under CC0 1.0 (https://creativecommons.org/publicdomain/zero/1.0/):\n'
				'https://unity.com/blog/engine-platform/free-vfx-image-sequences-flipbooks\n\n')
		for e in chosen:
			f.write('%s.mat: %s\n' % (e['material'], e['sequence']))

	for e in chosen:
		print('%s <- %s' % (e['material'], e['sequence']))
		sheet, n = build_sheet(e, args.cache)
		texture = 'openhplfx_%s.tga' % e['sequence'].lower()
		write_tga(os.path.join(out, 'particles', texture), sheet)
		with open(os.path.join(out, 'particles', e['material'] + '.mat'), 'w') as f:
			f.write(material_xml(e, texture))
		print('  %d of %d frames, %dx%d sheet' % (e['grid'][0] * e['grid'][1], n,
												 sheet.shape[1], sheet.shape[0]))
	print('addon:', out)
	print('use it with: OPENHPL_ADDON_DIRS=%s ./run.sh' % out)


def main():
	root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
	p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
	sub = p.add_subparsers(dest='cmd', required=True)
	b = sub.add_parser('build', help='download the sequences and write the addon')
	b.add_argument('--out', default=os.path.join(root, 'build', 'addons', 'openhpl-fx'))
	b.add_argument('--cache', default=os.path.join(root, 'build', 'fx-cache'))
	b.add_argument('--only', help='materials or sequences, comma separated')
	b.add_argument('--list', action='store_true', help='list the replacements and stop')
	args = p.parse_args()
	if args.list:
		for e in EFFECTS:
			print('%-28s %-14s %s %dx%d' % (e['material'], e['sequence'], e['type'], *e['grid']))
		return
	build(args)


if __name__ == '__main__':
	main()
