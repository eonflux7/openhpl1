#!/usr/bin/env python3
# This file is part of OpenHPL1, licensed under the GNU GPL v3 or
# later. See COPYING.
"""A hall with every Overture particle system and the entities that emit them.

  particlelab.py <output dir>

Writes particlelab.dae, its level script particlelab.hps and the grid
textures. Overture's data; started with OPENHPL_MAP=particlelab[:start].

North (start 'ambient'): the 44 looping systems, placed in the map as levels
do (_ps_ nodes) and always on. The large weather and fog systems stand in the
back row, those that fall from a ceiling hang 5 m up.
Emitter row (start 'emitters'): lamps and the tripod (a button lights and
puts out each), the furnace, a carried lamp and a smoking flask (always on),
a thrown flare (a button lights it; it burns out).
Middle (start 'oneshots'): a button for each of the 53 one-shot systems; it
plays the system on the button.
South (start 'breakables'): every breakable entity with a BreakPS, a button
in front of each breaks it. F6 reloads the map to rebuild them (OPENHPL_MAP
turns developer mode on).
East annex (start 'blast'): lit dynamite and a burning gas canister (a
button lights each; they explode a few seconds later) and the rubble pile (a
button breaks it), with buttons at the hall's east wall, outside their blast
radii (5.5, 6.5 and 20 m).

Using any button or pedestal shows the system's name and what emits it in the
game as a subtitle.
"""
import os
import sys

from generate import Map, node, write_collada, write_textures

# -------------------------------------------------------------------------
# Overture's particle systems and what emits them. Gathered from the game's
# maps (_ps_ nodes and CreateParticleSystem in scripts), entities (BreakPS,
# HitPS, DisappearPS, _ps_ nodes in their models) and materials.cfg.

# Looping systems (an emitter respawns). Large ones go in the back row.
LOOP_LARGE = [
    ('particle_snow', 'map 01_02, script 01_01'),
    ('particle_snowdust', 'map 01_01'),
    ('particle_snowdust02', 'map 01_01'),
    ('particle_ice_cave_smoke', 'map 01_15'),
    ('particle_cloud_slow', 'map 01_02'),
    ('particle_water_drop', 'maps 01_13_A, 01_16'),
    ('particle_water_fog_invert', 'script 01_20'),
    ('particle_water_cave_tunnels_hole', 'map 01_12'),
    ('particle_water_cave_tunnels_hole02', 'map 01_12'),
    ('particle_sea_splash', 'unused'),
]
# (system, source, hangs from the ceiling)
LOOP = [
    ('particle_acid_mist', 'map 01_12', False),
    ('particle_black_steam', 'maps 01_19, 01_20', False),
    ('particle_coal_falling', 'map 01_19', True),
    ('particle_dark_fog', 'map 01_03', False),
    ('particle_dust_saw_hand', 'script 01_15', False),
    ('particle_dust_wood', 'script 01_07_B', True),
    ('particle_dynamite_smoke', 'item_dynamite_thrown model', False),
    ('particle_dynamite_sparks', 'item_dynamite_thrown model', False),
    ('particle_fire01', 'unused', False),
    ('particle_fire_startup01', 'script 01_12', False),
    ('particle_flame01', 'unused', False),
    ('particle_flame02', 'door_shaft_furnace_on model', False),
    ('particle_flare_fire', 'items_flare_thrown, hud_object_flare models', False),
    ('particle_flare_smoke', 'items_flare_thrown, hud_object_flare models', False),
    ('particle_gas_burn', 'modern_mine_gas_canister_burning model', False),
    ('particle_glass_bottle_smoke', 'item_glass_container_smoking model', False),
    ('particle_large_gas_fire', 'unused', False),
    ('particle_mist', 'map 01_20, script 01_02', False),
    ('particle_mist_thick', 'script 01_02', False),
    ('particle_promo_mist', 'unused', False),
    ('particle_smoke', 'unused', False),
    ('particle_smoke_falling', 'map 01_14', True),
    ('particle_smoke_falling_small', 'map 01_14', True),
    ('particle_smoke_lantern', 'mine_paraffin_lamp models', False),
    ('particle_snow_falling', 'maps 01_02, 01_15', True),
    ('particle_snow_falling_small', 'map 01_05', True),
    ('particle_spark', 'scripts 01_03, 01_19', False),
    ('particle_steam', 'script 01_04', False),
    ('particle_steam_large', 'scripts 01_13_B, 01_13_C', False),
    ('particle_steam_storage', 'map 01_11', False),
    ('particle_tripod_flame', 'door_shaft_tripod model', False),
    ('particle_water_fog', 'maps 01_08, 01_17', False),
    ('particle_water_mist', 'maps 01_08, 01_17', False),
    ('particle_water_splat', 'map 01_12', False),
]
# One-shot systems (no emitter respawns): played by a button.
ONESHOT = [
    ('particle_blood_splat01', 'character_dog HitPS; Flesh hits'),
    ('particle_broom_impact', 'hud_object_broom HitPS'),
    ('particle_chemical_mix_fail', 'script 01_19'),
    ('particle_chemical_mix_succed', 'script 01_19'),
    ('particle_dust_falling', 'scripts 01_07_A, 01_13_A, 01_17'),
    ('particle_dust_large', 'script 01_04'),
    ('particle_dust_small', 'scripts 01_04, 01_07_A'),
    ('particle_dynamite_explosion', 'item_dynamite_thrown, modern_mine_gas_canister_burning BreakPS'),
    ('particle_expl_cavein', 'script 01_16'),
    ('particle_expl_dust', 'mine_barrel, mine_keg_gunpowder, water_cave_pillar BreakPS; script 01_19'),
    ('particle_expl_dust_small', 'breakable doors, crates and boards BreakPS'),
    ('particle_expl_tnt', 'door_shaft_rubble BreakPS; scripts 01_03, 01_12'),
    ('particle_fan_break', 'script 01_14'),
    ('particle_fuse_blow', 'door_shaft_cords BreakPS; script 01_09'),
    ('particle_glass_crash', 'kitchen_whiskeybottle, mining_room_jar01 BreakPS; Glass hits'),
    ('particle_horror_dust', 'scripts 01_07_B, 01_13_B, 01_13_C'),
    ('particle_ice_break', 'ice_cave_ice01-09 BreakPS'),
    ('particle_ice_crash', 'outside_ice BreakPS'),
    ('particle_imp_dirt', 'Dirt impacts'),
    ('particle_imp_dirt_hard', 'Dirt hits'),
    ('particle_imp_generic_hard', 'Generic_Hard, Dyn_Book hits'),
    ('particle_imp_generic_soft', 'Generic_Soft, Dyn_Paper, Dyn_Meat hits'),
    ('particle_imp_hammer', 'unused'),
    ('particle_imp_ice', 'Ice hits'),
    ('particle_imp_metal', 'Dyn_Metal_Can hits; door_wiredoorlock BreakPS'),
    ('particle_imp_metal_hard', 'Metal hits; script 01_18'),
    ('particle_imp_metal_special', 'Special_Metal hits'),
    ('particle_imp_metal_weak', 'Metal hits (slow)'),
    ('particle_imp_pickaxe', 'unused'),
    ('particle_imp_snow', 'NormalWater hits'),
    ('particle_imp_stone_hard', 'Rock hits'),
    ('particle_imp_stone_special', 'Special_Rock hits'),
    ('particle_imp_stone_weak', 'Rock hits (slow)'),
    ('particle_imp_water_low', 'objects falling into water (slow)'),
    ('particle_imp_water_med', 'objects falling into water'),
    ('particle_imp_wood', 'unused'),
    ('particle_imp_wood_hard', 'Wood hits'),
    ('particle_imp_wood_special', 'Special_Wood, breakable barrel hits'),
    ('particle_imp_wood_weak', 'Wood hits (slow)'),
    ('particle_mine_bottle', 'unused'),
    ('particle_mine_bottle_broken', 'old_mine_bottle BreakPS'),
    ('particle_refinery_steam', 'script 01_14'),
    ('particle_snow_impact', 'Snow impacts'),
    ('particle_spark_burst', 'mining_room_microscope BreakPS; scripts 01_07'),
    ('particle_spark_burst_small', 'script 01_06'),
    ('particle_spider_blood_splat01', 'spider, rockworm HitPS; Spider_Flesh hits'),
    ('particle_spider_disappear', 'spider DisappearPS'),
    ('particle_spider_egg_splat', 'spider eggs, mining_room_snail, base_entrance_man BreakPS'),
    ('particle_stick_rock_dust', 'scripts 01_03, 01_12, 01_16'),
    ('particle_stonedust', 'mining_room_wall, modern_mine_wall, worm_nest_junk BreakPS'),
    ('particle_stonedust_small', 'water_cave_wall BreakPS'),
    ('particle_stonedust_trans', 'script 01_19'),
    ('particle_water_death', 'script 01_15'),
]
# Breakable entities: (entity, BreakPS, height of the origin, width along x,
# on a table). The nine ice_cave_ice sheets (12-25 m) share one system, so
# only ice_cave_ice02 is placed, by itself; worm_nest_junk is a 60 m piece
# of cave, worm_nest_junk_falling stands in for it.
BREAKABLE = [
    ('kitchen_whiskeybottle', 'particle_glass_crash', 0.2, 1, True),
    ('mining_room_jar01', 'particle_glass_crash', 0.2, 1, True),
    ('old_mine_bottle', 'particle_mine_bottle_broken', 0.2, 1, True),
    ('mining_room_microscope', 'particle_spark_burst', 0.2, 1, True),
    ('mining_room_snail', 'particle_spider_egg_splat', 0.2, 1, True),
    ('door_wiredoorlock', 'particle_imp_metal', 0.2, 1, True),
    ('door_shaft_cords', 'particle_fuse_blow', 0.5, 1.2, False),
    ('iron_mine_toolbox', 'particle_expl_dust_small', 0.5, 1, False),
    ('old_mine_toolbox', 'particle_expl_dust_small', 0.5, 1, False),
    ('mine_barrel', 'particle_expl_dust', 0.55, 1, False),
    ('mine_keg_gunpowder', 'particle_expl_dust', 0.6, 1.2, False),
    ('modern_mine_woodcrate', 'particle_expl_dust_small', 0.6, 1.3, False),
    ('modern_mine_woodcrate02', 'particle_expl_dust_small', 0.6, 1.3, False),
    ('modern_mine_woodcrate_battery', 'particle_expl_dust_small', 0.6, 1.3, False),
    ('new_storage_ventilation', 'particle_expl_dust_small', 0.5, 1.3, False),
    ('furniture_coffin_breakable', 'particle_expl_dust_small', 0, 1.5, False),
    ('base_entrance_man', 'particle_spider_egg_splat', 0, 1.5, False),
    ('water_cave_spider_egg', 'particle_spider_egg_splat', 0.5, 1, False),
    ('water_cave_spider_egg_special', 'particle_spider_egg_splat', 0.5, 1, False),
    ('water_cave_pillar', 'particle_expl_dust', 0.5, 1, False),
    ('outside_ice', 'particle_ice_crash', 0.5, 2, False),
    ('mining_room_wall', 'particle_stonedust', 0.5, 2, False),
    ('water_cave_wall', 'particle_stonedust_small', 0.5, 2.2, False),
    ('water_cave_wall_nodebris', 'particle_stonedust_small', 0.5, 2.2, False),
    ('water_cave_wall_special', 'particle_stonedust_small', 0.5, 2.2, False),
    ('water_cave_wall_special_weak', 'particle_stonedust_small', 0.5, 2.2, False),
    ('modern_mine_beam', 'particle_expl_dust_small', 0.5, 2, False),
    ('modern_mine_woodfence', 'particle_expl_dust_small', 0.5, 3, False),
    ('ice_cave_wood', 'particle_expl_dust_small', 0.08, 3, False),
    ('ice_cave_wood_small', 'particle_expl_dust_small', 1.5, 2, False),
    ('door_mine_shaft', 'particle_expl_dust_small', 0.94, 1.2, False),
    ('door_explosiveroom', 'particle_expl_dust_small', 1.06, 2, False),
    ('door_modern_mine', 'particle_expl_dust_small', 1.06, 2, False),
    ('door_modern_mine_invert', 'particle_expl_dust_small', 1.06, 2, False),
    ('door_double_rusty', 'particle_expl_dust_small', 0, 2.2, False),
    ('door_wiredoor_inverse_breakable', 'particle_expl_dust_small', 0, 2, False),
    ('modern_mine_container', 'particle_expl_dust_small', 1, 3, False),
    ('modern_mine_container_invert', 'particle_expl_dust_small', 1, 3, False),
    ('water_cave_boulder_board', 'particle_expl_dust_small', 0.5, 4.6, False),
    ('worm_nest_barricade', 'particle_expl_dust_small', 0.5, 4, False),
    ('worm_nest_barricade_broken01', 'particle_expl_dust_small', 0.5, 4, False),
    ('worm_nest_hole', 'particle_expl_dust_small', 0.5, 3.5, False),
    ('worm_nest_junk_falling', 'particle_stonedust', 0.53, 5, False),
    ('mine_planks', 'particle_expl_dust_small', 0.5, 7, False),
    ('modern_mine_wall', 'particle_stonedust', 0.5, 9.5, False),
]

HALF_X, HALF_Z, HEIGHT, WALL = 46, 40, 12, 0.5
ANNEX_X, ANNEX_Z, ANNEX_H = 82, 7, 8


class Lab:
    """The map and the script lines for its buttons and pedestals."""

    def __init__(self):
        self.m = Map()
        self.start_lines = []   # OnStart()
        self.funcs = []         # callbacks
        self.count = 0

    def name(self, prefix):
        self.count += 1
        return f'{prefix}{self.count}'

    def ps(self, system, pos):
        self.m.nodes.append(node(f'_ps_{system}_{self.m.unique(self.name("ps"))}', pos,
                                 scale=(1, 1, 1)))

    def pedestal(self, x, z, height=1.0, size=0.5):
        h = size / 2
        self.m.box((x - h, 0, z - h), (x + h, height, z + h), 'testmap_prop')

    def button(self, x, y, z, label, body=''):
        """A script area on top of a pedestal: interacting shows the label
        and runs body (script lines, the area's name is asEntity)."""
        area = self.name('b')
        self.m.box((x - 0.15, y, z - 0.15), (x + 0.15, y + 0.06, z + 0.15), 'testmap_mark', '_nocollide')
        self.m.area('script', area, (x, y + 0.2, z), (0.5, 0.4, 0.5))
        func = f'Use_{area}'
        self.start_lines.append(f'AddEntityCallback("PlayerInteract", "{area}", "{func}");')
        text = label.replace('"', "'")
        self.funcs.append(f'void {func}(string asEntity)\n{{\n'
                          f'\tAddSubTitle("{text}", 5.0f);\n{body}}}\n')
        return area

    def hps(self):
        lines = ['// Particle lab, generated by particlelab.py: every Overture particle',
                 '// system and the entities that emit them.',
                 '',
                 'int glFlare = 0;',
                 '',
                 'void OnStart()',
                 '{',
                 '\tSetAmbientColor(0.16f, 0.16f, 0.17f);',
                 '\tGiveItem("flashlight", "item_flashlight.ent", -1);']
        lines += ['\t' + l for l in self.start_lines]
        lines += ['}', '',
                  'void OnLoad()', '{', '\tSetAmbientColor(0.16f, 0.16f, 0.17f);', '}', '',
                  'void OnUpdate() {}', 'void OnExit() {}', '']
        return '\n'.join(lines) + '\n' + '\n'.join(self.funcs)


def build():
    lab = Lab()
    m = lab.m

    # Hall, and an annex east of it for the explosives
    m.box((-HALF_X, -0.5, -HALF_Z), (HALF_X, 0, HALF_Z), 'testmap_floor')
    m.box((-HALF_X, HEIGHT, -HALF_Z), (HALF_X, HEIGHT + 0.5, HALF_Z), 'testmap_wall', '_noshadow')
    for lo, hi in [((-HALF_X - WALL, 0, -HALF_Z), (-HALF_X, HEIGHT, HALF_Z)),
                   ((HALF_X, 0, -HALF_Z), (HALF_X + WALL, HEIGHT, -ANNEX_Z)),
                   ((HALF_X, 0, ANNEX_Z), (HALF_X + WALL, HEIGHT, HALF_Z)),
                   ((HALF_X, ANNEX_H, -ANNEX_Z), (HALF_X + WALL, HEIGHT, ANNEX_Z)),
                   ((-HALF_X, 0, -HALF_Z - WALL), (HALF_X, HEIGHT, -HALF_Z)),
                   ((-HALF_X, 0, HALF_Z), (HALF_X, HEIGHT, HALF_Z + WALL))]:
        m.box(lo, hi, 'testmap_wall', '_noshadow')
    m.box((HALF_X, -0.5, -ANNEX_Z), (ANNEX_X, 0, ANNEX_Z), 'testmap_floor')
    m.box((HALF_X, ANNEX_H, -ANNEX_Z), (ANNEX_X, ANNEX_H + 0.5, ANNEX_Z), 'testmap_wall', '_noshadow')
    for lo, hi in [((HALF_X, 0, -ANNEX_Z - WALL), (ANNEX_X, ANNEX_H, -ANNEX_Z)),
                   ((HALF_X, 0, ANNEX_Z), (ANNEX_X, ANNEX_H, ANNEX_Z + WALL)),
                   ((ANNEX_X, 0, -ANNEX_Z), (ANNEX_X + WALL, ANNEX_H, ANNEX_Z))]:
        m.box(lo, hi, 'testmap_wall', '_noshadow')

    for i, x in enumerate([-33, -11, 11, 33]):
        for j, z in enumerate([-24, 0, 24]):
            m.light(f'light{i}{j}', (x, HEIGHT - 1, z), 26)
    m.light('lightannex', (64, ANNEX_H - 1, 0), 22)

    # Starts: OPENHPL_MAP=particlelab:<name>. rot_y 0 faces north, -90 east.
    m.start('link01', (0, 0, 4), 0)
    m.start('ambient', (0, 0, -10), 0)
    m.start('emitters', (-20, 0, -6), 0)
    m.start('oneshots', (0, 0, 4), 0)
    m.start('breakables', (0, 0, 8), 180)
    m.start('blast', (42, 0, 0), -90)

    # Looping systems, always on. Large ones along the north wall.
    for i, (ps, src) in enumerate(LOOP_LARGE):
        x = -40 + i * 80 / (len(LOOP_LARGE) - 1)
        lab.pedestal(x, -31, 0.6, 1.0)
        lab.button(x, 0.6, -31, f'{ps} ({src})')
        lab.ps(ps, (x, 3, -31))
    per_row = (len(LOOP) + 1) // 2
    for i, (ps, src, ceiling) in enumerate(LOOP):
        x = -40 + (i % per_row) * 80 / (per_row - 1)
        z = -23 if i < per_row else -16
        lab.pedestal(x, z)
        lab.button(x, 1.0, z, f'{ps} ({src})')
        lab.ps(ps, (x, 5.5 if ceiling else 1.5, z))

    # Emitting entities: (entity, x, y, kind, label)
    z = -9
    lab.start_lines.append('SetGameEntityActive("flare", false);')
    for ent, name, x, y, kind, label in [
            ('mine_paraffin_lamp', 'lamp', -40, 1.2, 'lamp', 'particle_smoke_lantern'),
            ('mine_paraffin_lamp_smaller', 'lampsmall', -36, 1.2, 'lamp', 'particle_smoke_lantern'),
            ('mine_paraffin_lamp_dynamic', 'lampcarried', -32, 1.3, None, 'particle_smoke_lantern (always lit)'),
            ('door_shaft_tripod', 'tripod', -28, 0.5, 'lamp', 'particle_tripod_flame'),
            ('item_glass_container_smoking', 'flask', -24, 1.15, None, 'particle_glass_bottle_smoke'),
            ('items_flare_thrown', 'flare', -20, 1.15, 'flare', 'particle_flare_fire, particle_flare_smoke'),
            ('door_shaft_furnace_on', 'furnace', -12, 1.0, None, 'particle_flame02')]:
        if y > 0.5 and ent != 'door_shaft_furnace_on':
            lab.pedestal(x, z, 1.0, 0.6)
        m.ref(ent, name, (x, y, z))
        text = f'{ent}: {label}'
        bx = x - 1.5 if ent == 'door_shaft_furnace_on' else x
        if kind == 'lamp':
            var = f'gl{name}'
            lab.funcs.append(f'int {var} = 1;\n')
            body = (f'\t{var} = 1 - {var};\n'
                    f'\tSetLampLit("{name}", {var} == 1, true);\n')
            lab.pedestal(bx, z + 2.5)
            lab.button(bx, 1.0, z + 2.5, text + ' - toggles', body)
        elif kind == 'flare':
            body = ('\tif(glFlare == 1) return;\n'
                    '\tglFlare = 1;\n'
                    '\tSetGameEntityActive("flare", true);\n')
            lab.pedestal(bx, z + 2.5)
            lab.button(bx, 1.0, z + 2.5, text + ' - lit once, burns out (F6 reloads)', body)
        else:
            lab.pedestal(bx + 1.2 if ent != 'door_shaft_furnace_on' else -7, z + 2.5)
            lab.button(bx + 1.2 if ent != 'door_shaft_furnace_on' else -7, 1.0, z + 2.5, text)

    # One-shot systems: the button plays the system on itself.
    per_row = (len(ONESHOT) + 1) // 2
    for i, (ps, src) in enumerate(ONESHOT):
        x = -40 + (i % per_row) * 78 / (per_row - 1)
        z = -2 if i < per_row else 2
        lab.pedestal(x, z)
        body = f'\tCreateParticleSystem("fx" + asEntity, "{ps}", asEntity, 1, 1, 1);\n'
        lab.button(x, 1.0, z, f'{ps} ({src})', body)

    # Breakables, packed into rows along x, buttons south of each.
    x, z, gap = -43, 12, 2.0
    for ent, ps, y, width, table in BREAKABLE:
        if x + width > 43:
            x, z = -43, z + 8
        cx = x + width / 2
        name = lab.name('brk')
        if table:
            m.box((cx - 0.4, 0, z - 0.4), (cx + 0.4, 0.8, z + 0.4), 'testmap_prop')
            y += 0.8
        m.ref(ent, name, (cx, y, z))
        body = f'\tDamageEntity("{name}", 10000, 100);\n'
        lab.pedestal(cx, z + 3)
        lab.button(cx, 1.0, z + 3, f'{ent}: {ps} on breaking', body)
        x += width + gap

    # One ice sheet (about 12 x 20 m, flat on the floor), turned along x.
    m.ref('ice_cave_ice02', 'ice', (6, 0.07, 32), 90)
    lab.pedestal(-6, 32)
    lab.button(-6, 1.0, 32, 'ice_cave_ice02: particle_ice_break on breaking',
               '\tDamageEntity("ice", 10000, 100);\n')

    # Explosives in the annex; their buttons stay in the hall.
    # Both have a fuse (Disappear) that runs from the map's start, so they
    # wait inactive until their button.
    lab.start_lines.append('SetGameEntityActive("dynamite", false);')
    lab.start_lines.append('SetGameEntityActive("canister", false);')
    m.ref('item_dynamite_thrown', 'dynamite', (56, 0.2, 0))
    m.ref('modern_mine_gas_canister_burning', 'canister', (64, 1.0, 0))
    m.ref('door_shaft_rubble', 'rubble', (76, 0.45, 0))
    for zb, ent, label, body in [
            (-3, 'item_dynamite_thrown',
             'particle_dynamite_sparks and _smoke, then particle_dynamite_explosion (5.5 m blast)',
             '\tSetGameEntityActive("dynamite", true);\n'),
            (0, 'modern_mine_gas_canister_burning',
             'particle_gas_burn for 4 s, then particle_dynamite_explosion (6.5 m blast)',
             '\tSetGameEntityActive("canister", true);\n'),
            (3, 'door_shaft_rubble', 'particle_expl_tnt on breaking (20 m blast)',
             '\tDamageEntity("rubble", 10000, 100);\n')]:
        lab.pedestal(43, zb)
        lab.button(43, 1.0, zb, f'{ent}: {label}', body)
    return lab


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else '.'
    os.makedirs(out, exist_ok=True)
    lab = build()
    write_textures(out)
    write_collada(os.path.join(out, 'particlelab.dae'), lab.m)
    with open(os.path.join(out, 'particlelab.hps'), 'w') as f:
        f.write(lab.hps())


if __name__ == '__main__':
    main()
