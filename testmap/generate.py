#!/usr/bin/env python3
# This file is part of OpenHPL1, licensed under the GNU GPL v3 or
# later. See COPYING.
#
# Generates the physics test map: a grey hall with the game's doors, drawers,
# levers, valves, grabbable and pushable objects, stacks, steps, ramps, a
# ladder and liquid, force and stick areas. The level script is testmap.hps.
#
#   generate.py <output dir>
#
# Writes testmap.dae (Collada, as exported by ColladaMaya for HPL1) and the
# grid textures and materials it uses. Entities come from the game data.
#
# Map conventions (see cMeshLoaderCollada::AddSceneObjects):
#   _room<name>                  sector; children are static geometry
#   <name>[_nocollide][_noshadow] static mesh
#   _ref_<entity file>_<name>    entity from <entity file>.ent
#   _area_<type>_<name>          area sized by its geometry times its scale
#   _start_<name>                player start position
#   point light: scale x = radius, y = specular, z < 0.1 = no shadows

import math
import os
import struct
import sys

# -------------------------------------------------------------------------
# Textures and materials

def write_tga(path, size, pixel):
    """Uncompressed 32-bit TGA, pixel(x, y) -> (r, g, b, a), origin top left."""
    with open(path, 'wb') as f:
        f.write(struct.pack('<BBBHHBHHHHBB', 0, 0, 2, 0, 0, 0, 0, 0,
                            size, size, 32, 8 | 0x20))
        rows = bytearray()
        for y in range(size):
            for x in range(size):
                r, g, b, a = pixel(x, y)
                rows += bytes((b, g, r, a))
        f.write(rows)


def grid(base, line, size=128):
    """One tile per metre: a line at the tile edge and a faint one at half."""
    def pixel(x, y):
        edge = x < 2 or y < 2
        half = x == size // 2 or y == size // 2
        c = line if edge else tuple((b + l) // 2 for b, l in zip(base, line)) if half else base
        return c + (255,)
    return pixel


TEXTURES = {
    # name: (pixel function, material type, physics material)
    'testmap_floor': (grid((125, 125, 125), (60, 60, 60)), 'Diffuse', 'Rock'),
    'testmap_wall': (grid((105, 105, 110), (55, 55, 60)), 'Diffuse', 'Rock'),
    'testmap_prop': (grid((175, 170, 160), (120, 115, 105)), 'Diffuse', 'Wood'),
    'testmap_mark': (grid((200, 170, 40), (120, 100, 20)), 'Diffuse', 'Metal'),
    'testmap_water': (lambda x, y: (40, 90, 160, 90), 'Alpha', 'NormalWater'),
}

MATERIAL = '''<Material>
  <Main Type="{type}" PhysicsMaterial="{phys}" UseAlpha="{alpha}" DepthTest="True" Value="1" />
  <TextureUnits>
    <Diffuse File="{name}.tga" Compress="false" Type="2D" Mipmaps="true" Wrap="Repeat" AnimMode="None" AnimFrameTime="1" />
  </TextureUnits>
</Material>
'''


def write_textures(out):
    for name, (pixel, mtype, phys) in TEXTURES.items():
        write_tga(os.path.join(out, name + '.tga'), 128, pixel)
        with open(os.path.join(out, name + '.mat'), 'w') as f:
            f.write(MATERIAL.format(type=mtype, phys=phys, name=name,
                                    alpha='True' if mtype == 'Alpha' else 'False'))

# -------------------------------------------------------------------------
# Static geometry: boxes baked in world space, merged per material

def rot_x(deg):
    c, s = math.cos(math.radians(deg)), math.sin(math.radians(deg))
    return lambda v: (v[0], v[1] * c - v[2] * s, v[1] * s + v[2] * c)


def rot_z(deg):
    c, s = math.cos(math.radians(deg)), math.sin(math.radians(deg))
    return lambda v: (v[0] * c - v[1] * s, v[0] * s + v[1] * c, v[2])


class Mesh:
    def __init__(self):
        self.pos, self.norm, self.uv, self.tris = [], [], [], []

    def quad(self, corners, normal, uvs):
        base = len(self.pos)
        self.pos += corners
        self.norm += [normal] * 4
        self.uv += uvs
        self.tris += [(base, base + 1, base + 2), (base, base + 2, base + 3)]

    def box(self, lo, hi, rotate=None, pivot=None):
        """Axis-aligned box from lo to hi, optionally rotated about pivot.
        UVs are world-planar so the 1 m grid lines up across boxes."""
        (x0, y0, z0), (x1, y1, z1) = lo, hi
        faces = [  # corners counter-clockwise seen from outside, normal
            ([(x1, y0, z0), (x1, y1, z0), (x1, y1, z1), (x1, y0, z1)], (1, 0, 0)),
            ([(x0, y0, z1), (x0, y1, z1), (x0, y1, z0), (x0, y0, z0)], (-1, 0, 0)),
            ([(x0, y1, z0), (x0, y1, z1), (x1, y1, z1), (x1, y1, z0)], (0, 1, 0)),
            ([(x0, y0, z1), (x0, y0, z0), (x1, y0, z0), (x1, y0, z1)], (0, -1, 0)),
            ([(x1, y0, z1), (x1, y1, z1), (x0, y1, z1), (x0, y0, z1)], (0, 0, 1)),
            ([(x0, y0, z0), (x0, y1, z0), (x1, y1, z0), (x1, y0, z0)], (0, 0, -1)),
        ]
        for corners, n in faces:
            axis = [i for i in range(3) if n[i] == 0]
            uvs = [(c[axis[0]], c[axis[1]]) for c in corners]
            if rotate:
                p = pivot or (0, 0, 0)
                corners = [tuple(a + b for a, b in zip(rotate(tuple(c[i] - p[i] for i in range(3))), p))
                           for c in corners]
                n = rotate(n)
            self.quad(corners, n, uvs)


class Map:
    def __init__(self):
        self.meshes = {}      # (material, flags) -> Mesh
        self.nodes = []       # extra visual scene nodes (xml strings)
        self.names = set()

    def mesh(self, material, flags=''):
        return self.meshes.setdefault((material, flags), Mesh())

    def box(self, lo, hi, material='testmap_prop', flags='', **kw):
        self.mesh(material, flags).box(lo, hi, **kw)

    def unique(self, name):
        assert name not in self.names, name
        assert '_' not in name, f'entity names may not contain "_": {name}'
        self.names.add(name)
        return name

    def ref(self, entity, name, pos, rot_y=0, rot_x=0, rot_z=0):
        self.nodes.append(node(f'_ref_{entity}_{self.unique(name)}', pos, (rot_x, rot_y, rot_z)))

    def area(self, kind, name, pos, size, rot=(0, 0, 0)):
        self.nodes.append(node(f'_area_{kind}_{self.unique(name)}', pos, rot, size,
                               geometry='unitcube'))

    def start(self, name, pos, rot_y):
        self.nodes.append(node(f'_start_{name}', pos, (0, rot_y, 0)))

    def light(self, name, pos, radius, shadows=False):
        self.nodes.append(node(name, pos, (0, 0, 0), (radius, 1, 1 if shadows else 0),
                               light='pointlight'))

# -------------------------------------------------------------------------
# Collada output

def fmt(values):
    return ' '.join(f'{v:.6g}' for v in values)


def node(name, pos, rot=(0, 0, 0), scale=None, geometry=None, light=None):
    rx, ry, rz = rot
    xml = [f'    <node id="{name}" name="{name}">',
           f'      <translate sid="translate">{fmt(pos)}</translate>',
           f'      <rotate sid="rotateZ">0 0 1 {rz:g}</rotate>',
           f'      <rotate sid="rotateY">0 1 0 {ry:g}</rotate>',
           f'      <rotate sid="rotateX">1 0 0 {rx:g}</rotate>']
    if scale:
        xml.append(f'      <scale sid="scale">{fmt(scale)}</scale>')
    if geometry:
        xml.append(instance_geometry(geometry, 'testmap_mark'))
    if light:
        xml.append(f'      <instance_light url="#{light}"/>')
    xml.append('    </node>')
    return '\n'.join(xml)


def instance_geometry(geom, material):
    return (f'      <instance_geometry url="#{geom}">\n'
            f'        <bind_material><technique_common>\n'
            f'          <instance_material symbol="{material}" target="#{material}"/>\n'
            f'        </technique_common></bind_material>\n'
            f'      </instance_geometry>')


def geometry_xml(gid, mesh, material):
    pos = [c for p in mesh.pos for c in p]
    norm = [c for n in mesh.norm for c in n]
    uv = [c for t in mesh.uv for c in t]
    idx = ' '.join(f'{i} {i} {i}' for t in mesh.tris for i in t)
    return f'''    <geometry id="{gid}" name="{gid}">
      <mesh>
        <source id="{gid}-pos">
          <float_array id="{gid}-pos-array" count="{len(pos)}">{fmt(pos)}</float_array>
          <technique_common><accessor source="#{gid}-pos-array" count="{len(mesh.pos)}" stride="3">
            <param name="X" type="float"/><param name="Y" type="float"/><param name="Z" type="float"/>
          </accessor></technique_common>
        </source>
        <source id="{gid}-norm">
          <float_array id="{gid}-norm-array" count="{len(norm)}">{fmt(norm)}</float_array>
          <technique_common><accessor source="#{gid}-norm-array" count="{len(mesh.norm)}" stride="3">
            <param name="X" type="float"/><param name="Y" type="float"/><param name="Z" type="float"/>
          </accessor></technique_common>
        </source>
        <source id="{gid}-uv">
          <float_array id="{gid}-uv-array" count="{len(uv)}">{fmt(uv)}</float_array>
          <technique_common><accessor source="#{gid}-uv-array" count="{len(mesh.uv)}" stride="2">
            <param name="S" type="float"/><param name="T" type="float"/>
          </accessor></technique_common>
        </source>
        <vertices id="{gid}-vtx"><input semantic="POSITION" source="#{gid}-pos"/></vertices>
        <triangles material="{material}" count="{len(mesh.tris)}">
          <input semantic="VERTEX" source="#{gid}-vtx" offset="0"/>
          <input semantic="NORMAL" source="#{gid}-norm" offset="1"/>
          <input semantic="TEXCOORD" source="#{gid}-uv" offset="2" set="0"/>
          <p>{idx}</p>
        </triangles>
      </mesh>
    </geometry>'''


def write_collada(path, m):
    unit = Mesh()
    unit.box((-0.5, -0.5, -0.5), (0.5, 0.5, 0.5))

    images, materials, effects = [], [], []
    for name in TEXTURES:
        images.append(f'    <image id="{name}-image" name="{name}-image"><init_from>{name}.tga</init_from></image>')
        materials.append(f'    <material id="{name}" name="{name}"><instance_effect url="#{name}-fx"/></material>')
        effects.append(f'''    <effect id="{name}-fx" name="{name}"><profile_COMMON><technique sid="standard"><lambert>
      <diffuse><texture texture="{name}-image" texcoord="UVSET0"/></diffuse>
    </lambert></technique></profile_COMMON></effect>''')

    geometries = [geometry_xml('unitcube', unit, 'testmap_mark')]
    room = []
    for i, ((material, flags), mesh) in enumerate(sorted(m.meshes.items())):
        gid = f'geom{i}{flags}'
        geometries.append(geometry_xml(gid + '-geom', mesh, material))
        room.append(f'      <node id="{gid}" name="{gid}">\n'
                    + instance_geometry(gid + '-geom', material) + '\n      </node>')

    with open(path, 'w') as f:
        f.write(f'''<?xml version="1.0" encoding="utf-8"?>
<COLLADA xmlns="http://www.collada.org/2005/11/COLLADASchema" version="1.4.0">
  <asset>
    <contributor><authoring_tool>OpenHPL1 testmap/generate.py</authoring_tool></contributor>
    <unit name="meter" meter="1.000000"/>
    <up_axis>Y_UP</up_axis>
  </asset>
  <library_images>
{chr(10).join(images)}
  </library_images>
  <library_materials>
{chr(10).join(materials)}
  </library_materials>
  <library_effects>
{chr(10).join(effects)}
  </library_effects>
  <library_geometries>
{chr(10).join(geometries)}
  </library_geometries>
  <library_lights>
    <light id="pointlight" name="pointlight"><technique_common><point>
      <color sid="color">0.52 0.51 0.49</color>
      <constant_attenuation>1</constant_attenuation><linear_attenuation>0</linear_attenuation>
      <quadratic_attenuation>0</quadratic_attenuation>
    </point></technique_common></light>
  </library_lights>
  <library_visual_scenes>
    <visual_scene id="testmap" name="testmap">
    <node id="_roomhall" name="_roomhall">
{chr(10).join(room)}
    </node>
{chr(10).join(m.nodes)}
    </visual_scene>
  </library_visual_scenes>
  <scene><instance_visual_scene url="#testmap"/></scene>
</COLLADA>
''')

# -------------------------------------------------------------------------
# The map. North is -z, the player starts in the middle facing north.
#
#   north wall:  hinged and sliding furniture
#   z = -9:      free-standing swing doors; floor hatches west of them
#   east wall:   valves, levers and a stick area
#   centre:      grab table, stacks, updraft and conveyor, push objects
#   west:        throwing target, liquid pool
#   south:       steps, ramps, crouch tunnel, platform with ladder and stairs

HALF_X, HALF_Z, HEIGHT, WALL = 24, 16, 7, 0.5


def build():
    m = Map()

    # Hall
    m.box((-HALF_X, -0.5, -HALF_Z), (HALF_X, 0, HALF_Z), 'testmap_floor')
    m.box((-HALF_X, HEIGHT, -HALF_Z), (HALF_X, HEIGHT + 0.5, HALF_Z), 'testmap_wall', '_noshadow')
    for lo, hi in [((-HALF_X - WALL, 0, -HALF_Z), (-HALF_X, HEIGHT, HALF_Z)),
                   ((HALF_X, 0, -HALF_Z), (HALF_X + WALL, HEIGHT, HALF_Z)),
                   ((-HALF_X, 0, -HALF_Z - WALL), (HALF_X, HEIGHT, -HALF_Z)),
                   ((-HALF_X, 0, HALF_Z), (HALF_X, HEIGHT, HALF_Z + WALL))]:
        m.box(lo, hi, 'testmap_wall', '_noshadow')

    # Start positions: OVERTURE_MAP=testmap:<name>. rot_y 0 faces north, 90 west.
    m.start('link01', (0, 0, 6), 0)
    for name, pos, rot in [('doors', (-9, 0, -4), 0), ('furniture', (-9, 0, -10.5), 0),
                           ('levers', (18, 0, -8), -90), ('grab', (-13, 0, 4.5), 0),
                           ('stacks', (-3.5, 0, 4), 0), ('push', (9, 0, 7), 0),
                           ('pool', (-13, 0, 3.5), 90), ('steps', (-10, 0, 5), 180),
                           ('ladder', (9, 0, 12.5), -90)]:
        m.start(name, pos, rot)

    for i, (x, z) in enumerate([(-14, -8), (0, -8), (14, -8), (-14, 8), (0, 8), (14, 8)]):
        m.light(f'light{i}', (x, 6.5, z), 20)
    m.light('lightshadow', (-2, 5, 0), 12, shadows=True)

    # Swing doors, free-standing. Width along x unless rotated.
    doors = [('door_oldwood', 1.06, 0), ('door_mining_room', 1.06, 0),
             ('door_modern_mine', 1.06, 0), ('door_wooden_door', 1.06, 0),
             ('door_wiredoor', 0, 90), ('door_mine_shaft', 0.94, 90),
             ('door_double_rusty', 0, 90)]
    for i, (ent, y, rot) in enumerate(doors):
        m.ref(ent, f'door{i}', (-20 + i * 3.5, y, -9), rot)

    # Hinged and sliding furniture along the north wall
    furniture = [('furniture_woodenlocker', 0), ('door_shaft_wardrobe', 0),
                 ('furniture_coffin', 0), ('office_filecabinet', 0),
                 ('furniture_desk01', 0.54), ('mining_room_desk', 0),
                 ('boat_locker', 0), ('furniture_locker', 0),
                 ('mining_room_cabinet', 0), ('new_storage_medic_locker', 0),
                 ('furniture_small_locker', 0)]
    for i, (ent, y) in enumerate(furniture):
        m.ref(ent, f'furniture{i}', (-20 + i * 3.5, y, -14))

    # Floor hatches
    m.ref('outside_hatch', 'hatch0', (-20, 0, -5))
    m.ref('mine_smallhatch', 'hatch1', (-17, 0, -5))

    # East wall: valves at chest height, floor lever, a panel of sticks
    m.ref('mine_valve', 'valve0', (23.97, 1.3, -12), 180)
    m.ref('modern_mine_valve', 'valve1', (23.85, 1.3, -9.5))
    m.ref('worm_nest_valve', 'valve2', (23.85, 1.3, -7))
    m.ref('modern_mine_lever', 'lever0', (21, 0, -12))
    m.box((20.5, 0, -5), (22, 0.9, -3.8))
    m.ref('new_storage_stick', 'stick0', (21, 0.92, -4.4))
    m.ref('modern_mine_control_stick', 'stick1', (21.6, 0.9, -4.4))

    # Stick area on the east wall with a marker plate, a can on a shelf below
    m.box((23.9, 0.95, -1.4), (24, 1.45, -0.6), 'testmap_mark', '_nocollide')
    m.area('stick', 'stickarea', (23.7, 1.2, -1), (0.4, 0.4, 0.6))
    m.box((23.4, 0.7, -1.5), (24, 0.75, -0.5))
    m.ref('kitchen_soup_can', 'stickcan', (23.7, 0.82, -1))

    # Grab table and loose objects west of the centre
    m.box((-15.5, 0, -0.6), (-12.5, 0.8, 0.6))
    for i in range(4):
        m.ref('kitchen_soup_can', f'can{i}', (-15.2 + i * 0.2, 0.86, -0.3))
    m.ref('kitchen_whiskeybottle', 'bottle0', (-14.2, 0.96, -0.3))
    m.ref('kitchen_whiskeybottle', 'bottle1', (-13.9, 0.96, -0.3))
    m.ref('mining_room_book', 'book0', (-13.5, 0.81, -0.3))
    m.ref('office_typewriter', 'typewriter', (-14.6, 0.95, 0.25))
    m.ref('mine_rock02', 'rock0', (-13.8, 0.81, 0.25))
    m.ref('iron_mine_toolbox', 'toolbox0', (-13.1, 0.81, 0.25))
    m.ref('furniture_woodchair', 'chair0', (-15, 0, 1.5), 180)
    m.ref('furniture_woodchair', 'chair1', (-13, 0, 1.5), 180)
    m.ref('mine_wood_plank', 'plank0', (-11, 0.12, -1), 0, 0, 90)
    m.ref('mine_steel_rod', 'rod0', (-11, 0.22, 0.5))
    m.ref('mine_canister', 'canister0', (-10, 0.5, 2))
    m.ref('mine_ladder', 'ladder0', (-12, 0.05, 3), 0, 90)
    m.ref('boat_life_ring', 'ring0', (-10, 0.4, 3.5))
    m.ref('mine_ammobox', 'ammobox0', (-14, 0, 3.5))

    # Stacks: a crate pyramid and a tower of small wooden crates
    k = 0
    for row in range(4):
        for i in range(4 - row):
            x = -6 + (i + row * 0.5) * 0.72
            m.ref('mine_crate', f'crate{k}', (x, 0.17 + row * 0.37, 0))
            k += 1
    for i in range(4):
        m.ref('modern_mine_woodcrate_small', f'smallcrate{i}', (-1, -0.3 + i * 0.65, 0))

    # Force areas: an updraft over a marker pad, a conveyor pushing north
    m.box((-1, 0, -5), (1, 0.01, -3), 'testmap_mark', '_nocollide')
    m.area('force', 'updraft', (0, 3.5, -4), (2, 7, 2))
    m.ref('mine_crate', 'updraftcrate', (0, 0.2, -4))
    m.box((3, 0, -6), (5, 0.01, -2), 'testmap_mark', '_nocollide')
    m.area('force', 'conveyor', (4, 0.5, -4), (2, 4, 1), (-90, 0, 0))
    m.ref('modern_mine_woodcrate_small', 'conveyorcrate', (4, -0.3, -2.5))

    # Push objects east of the centre, and a ramp to push them up
    push = [('mine_barrel', 0.55), ('modern_mine_crate', 0.61), ('iron_mine_crate', 0),
            ('mine_rock01', 0.22), ('modern_mine_cart', 0.18), ('modern_mine_woodcrate', 0.6),
            ('water_cave_boulder', 0.78)]
    for i, (ent, y) in enumerate(push):
        m.ref(ent, f'push{i}', (5 + (i % 4) * 2.5, y, 0 + (i // 4) * 3))
    m.box((14, -1, -2), (16, 0, 2), rotate=rot_z(15), pivot=(14, 0, 0))

    # Throwing target: cans on a pedestal by the west wall
    m.box((-23.5, 0, -4.5), (-22.5, 1, -3.5))
    k = 0
    for row in range(3):
        for i in range(3 - row):
            m.ref('kitchen_soup_can', f'target{k}', (-23, 1.06 + row * 0.11, -4.1 + (i + row * 0.5) * 0.1))
            k += 1

    # Liquid pool: raised walls, the area fills it to 0.9 m
    x0, x1, z0, z1, h = -23, -17, 1, 6, 1.0
    for lo, hi in [((x0, 0, z0), (x1, h, z0 + 0.3)), ((x0, 0, z1 - 0.3), (x1, h, z1)),
                   ((x0, 0, z0), (x0 + 0.3, h, z1)), ((x1 - 0.3, 0, z0), (x1, h, z1))]:
        m.box(lo, hi)
    m.box((x1, 0, 3), (x1 + 0.6, 0.5, 4))
    m.box((x0 + 0.3, 0.9, z0 + 0.3), (x1 - 0.3, 0.9, z1 - 0.3), 'testmap_water', '_nocollide_noshadow')
    m.area('liquid', 'pool', ((x0 + x1) / 2, 0.45, (z0 + z1) / 2), (x1 - x0 - 0.6, 0.9, z1 - z0 - 0.6))
    m.ref('modern_mine_woodcrate_small', 'floatcrate', (-21, 1.2, 3.5))
    m.ref('mine_wood_plank', 'floatplank', (-19, 1.4, 3.5), 0, 0, 90)
    m.ref('mine_barrel', 'floatbarrel', (-20, 1.8, 4.5))

    # Steps of increasing height
    for i, h in enumerate([0.1, 0.2, 0.3, 0.4, 0.5, 0.6]):
        x = -20 + i * 2
        m.box((x, 0, 9.5), (x + 1.5, h, 11), 'testmap_mark' if i % 2 else 'testmap_prop')

    # Ramps of increasing slope, rising to the south
    for i, a in enumerate([10, 20, 30, 40, 50]):
        x = -7 + i * 2
        m.box((x, -0.3, 9), (x + 1.5, 0, 13), rotate=rot_x(-a), pivot=(x, 0, 9))

    # Crouch tunnel, roof 1.1 m above the floor
    m.box((4, 0, 9.5), (8, 1.1, 9.7))
    m.box((4, 0, 11.3), (8, 1.1, 11.5))
    m.box((4, 1.1, 9.5), (8, 1.3, 11.5))

    # Platform 3 m up, with a ladder on its west face and stairs to the north
    m.box((14, 0, 9), (24, 3, 16), 'testmap_wall')
    for z in (12, 13):
        m.box((13.85, 0, z - 0.04), (13.93, 3.6, z + 0.04), flags='_nocollide')
    for i in range(11):
        m.box((13.85, 0.3 + i * 0.3, 12), (13.93, 0.34 + i * 0.3, 13), flags='_nocollide')
    # The player hangs on the area's local +z side, so turn that to face west.
    m.area('ladder', 'ladder', (13.75, 1.6, 12.5), (1.0, 3.2, 0.3), (0, -90, 0))
    for i in range(15):
        m.box((15, 0, 9 - (15 - i) * 0.3), (17, 0.2 * (i + 1), 9 - (14 - i) * 0.3))

    return m


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else '.'
    os.makedirs(out, exist_ok=True)
    write_textures(out)
    write_collada(os.path.join(out, 'testmap.dae'), build())


if __name__ == '__main__':
    main()
