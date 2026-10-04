#!/usr/bin/env python3
# This file is part of Overture Remade, licensed under the GNU GPL v3 or
# later. See COPYING.
"""A compact room for player effects, using the physics map's mesh writer.

West: a sheltered dark alcove. North: lamps, detailed props and a save
point. East: a sunken water tank deep enough to put the camera underwater.
F2's Player effects panel supplies safe previews and live feedback.
"""
import os
import sys

from generate import Map, write_collada, write_textures


def build():
    m = Map()
    # A 12 x 12 metre room, with an opening in the floor for the tank.
    for lo, hi in [((-6, -0.3, -6), (3, 0, 6)),
                   ((3, -0.3, -6), (6, 0, 1)),
                   ((3, -0.3, 4), (6, 0, 6)),
                   ((3, -2.5, 1), (6, -2.2, 4))]:
        m.box(lo, hi, 'testmap_floor')
    m.box((-6, 4, -6), (6, 4.3, 6), 'testmap_wall')
    for lo, hi in [((-6.3, -2.5, -6), (-6, 4, 6)),
                   ((6, -2.5, -6), (6.3, 4, 6)),
                   ((-6, -2.5, -6.3), (6, 4, -6)),
                   ((-6, -2.5, 6), (6, 4, 6.3)),
                   ((3, -2.5, 0.7), (6, 0, 1)),
                   ((3, -2.5, 4), (6, 0, 4.3))]:
        m.box(lo, hi, 'testmap_wall')

    m.start('link01', (0, 0, 4), 0)
    m.start('dark', (-4.5, 0, -4.5), 180)
    m.start('water', (4.5, -2.2, 2.5), 90)

    # Shadow-casting partitions isolate the nook from the main lamps.
    m.box((-2.5, 0, -6), (-2.2, 3.5, -1), 'testmap_wall')
    m.box((-6, 0, -1.3), (-3.5, 3.5, -1), 'testmap_wall')
    m.box((-5.5, 0, -5.5), (-3.2, 0.015, -2), 'testmap_mark', '_nocollide')
    m.light('lablight', (1, 3.3, 0), 6, shadows=True)
    m.light('labbright', (3, 3, -4), 4, shadows=True)

    # Near and far edges, small text/details, and a bright halo for bloom.
    m.box((-0.8, 0, -3.7), (1.2, 0.9, -2.4), 'testmap_prop')
    m.ref('mining_room_desklamp', 'lamp', (0.75, 0.9, -3.1))
    for i in range(4):
        m.ref('kitchen_soup_can', f'can{i}', (-0.55 + i * 0.3, 0.96, -2.65))
    m.ref('mine_barrel', 'barrel', (2.5, 0.55, -2.5))
    m.ref('modern_mine_woodcrate_small', 'crate', (3.8, 0.4, -4))
    m.box((-1.8, 0, -5.4), (-0.8, 0.9, -4.4), 'testmap_mark')
    m.ref('items_artefact', 'artefact', (-1.3, 1.0, -4.9))
    m.area('save', 'savepoint', (-1.3, 1.05, -4.9), (0.7, 0.6, 0.7))

    # Steps in and out of a tank with the surface at floor height.
    for i in range(4):
        x = 3.0 + i * 0.45
        m.box((x, -2.2, 3.2), (x + 0.45, -0.45 * (i + 1), 4), 'testmap_prop')
    m.box((3, -0.03, 1), (6, -0.03, 4), 'testmap_water', '_nocollide_noshadow')
    m.area('liquid', 'water', (4.5, -1.1, 2.5), (3, 2.2, 3))
    return m


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else '.'
    os.makedirs(out, exist_ok=True)
    write_textures(out)
    write_collada(os.path.join(out, 'effectslab.dae'), build())


if __name__ == '__main__':
    main()
