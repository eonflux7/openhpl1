# Addons and glTF models

OpenHPL1 can load **addons**, which are folders or `.zip` files whose files
replace the game's files or add new ones. It also reads models and maps as
**glTF 2.0** (`.glb`, `.gltf`) as well as the games' Collada (`.dae`).
`openhpl1-model` converts the games' Collada files to glTF and checks that
the engine reads the same data from both. The installed game is never changed.

## Addons

### Where they go

Every subfolder and `.zip` file in these folders is an addon:

| Folder | Use |
|--------|-----|
| `<personal folder>/addons/` | Normal place; per game, created on start |
| `<executable folder>/addons/` | Portable installs; use `Games=` to limit them to a game |
| `OPENHPL_ADDON_DIRS` | Development: a list of folders (`;` on Windows, `:` elsewhere). An entry with an `addon.cfg` in it is one addon, otherwise a folder of addons |

The personal folders are `~/.frictionalgames/Penumbra/OvertureRemade/` on
Linux (`BlackPlagueRemade/`, `RequiemRemade/`) and
`Documents\Penumbra Overture\OvertureRemade\` and
`Documents\Penumbra\BlackPlagueRemade\` (and `RequiemRemade\`) on Windows, or
the matching folders under `OPENHPL_PROFILE_ROOT`.

`OPENHPL_ADDONS=0` turns addons off. `OPENHPL_ADDONS=a,b` uses only the
addons with those folder or `.zip` names.

### How files are found

The engine finds a file by its **name only**, case-insensitively, in the
folders listed in the game's `resources.cfg`. Addon folders, with all their
subfolders, are searched **before** the game's folders. So
`addons/mymod/anything/office_cabinet.jpg` replaces the game's
`office_cabinet.jpg`, and new files are found the same way. Keeping the
game's folder layout is tidy but not needed.

- A request for a `.dae` file is answered by a `.glb` or `.gltf` of the same
  name if that is in an earlier folder (or the same one). Maps, `.ent` files
  and scripts keep asking for `.dae` names, and the converted or new models
  are used.
- Within one addon, the file in the first subfolder in path order wins.
  Avoid two files with the same name in one addon. For example, Black Plague's
  `models/` and Requiem's `expansion01/models/` have files with the same
  names. Requiem's own `requiem_resources.cfg` lists Black Plague's folders
  first, so Black Plague's files win there.
- Hidden files and folders (names starting with `.`) are left out.
- These files are read by path, not by name, and an addon cannot replace them:
  `resources.cfg`, `materials.cfg`, `config/*.cfg` (game, startup and default
  settings), the shipped `core/cache/*.collcach` mesh caches, and the
  executable's own `shaders/`, `fonts/` and `hud/`.

### addon.cfg

Optional, at the top of the addon:

```xml
<Addon Name="Remastered office" Games="overture" Priority="0" Enabled="true" />
```

| Attribute | Default | Meaning |
|-----------|---------|---------|
| `Name` | folder name | Shown in the log |
| `Games` | all | `overture`, `blackplague`, `requiem`, separated by spaces or commas |
| `Priority` | 0 | Higher is searched first; equal priorities go by folder/zip name |
| `Enabled` | true | `false` leaves the addon out |

A `.zip` file is unpacked once into `<personal folder>/cache/addons/`, and
again when the file changes. If it holds a single folder, that folder is the
addon. Entries whose paths would leave the unpack folder (`..`, absolute
paths) are skipped.

`hpl.log` lists the addons in the order they are used and every game file
they replace, for example
`'office_cabinet.glb' replaces 'models/office/office_cabinet.dae'`.

### Particle flipbooks

A particle material can be a flipbook: a sheet of frames that each particle
plays over its life, from the first frame at birth to the last at death,
with each frame blended into the next. `Flipbook` on the material's `Main`
gives the columns and rows; the frames run row by row from the top left:

```xml
<Main Type="Alpha" PhysicsMaterial="Default" UseAlpha="False" Flipbook="8 8" FlipbookBlend="True" />
```

Every emitter that uses the material then plays it, whatever `SubDiv` and
`SubDivType` its `.ps` file gives, so an addon only replaces the `.mat` and
its texture. `FlipbookBlend="False"` steps from frame to frame. In a `.ps`
file, `SubDivBlend="True"` blends the frames of an emitter's own
`SubDivType="Animation"` sub division.

The panel's Particles window (F2) switches a system an addon replaces
between the addon's look and the game's, to compare them.

### Caches and saves

- Collada mesh caches of addon files are kept in
  `<personal folder>/cache/external/<folder hash>/`, apart from the game's.
  Turning an addon off never leaves its cache in use for the game's file.
- Meshes, animations and maps loaded from a `.glb` that stands in for a `.dae`
  keep the `.dae` name, so saves name the same files with or without the
  addon. A save still remembers entity and map names. An addon that removes or
  renames entities in a map can make older saves of that map fail to load.

### Reloading the map while editing

With **Developer Mode** on (Options → Game; `OPENHPL_MAP` and `OPENHPL_DEBUG`
turn it on for a run), the map is loaded again from its files:

- when files in an addon folder are saved, added or removed (looked at twice
  a second; the reload waits until a file has stopped changing), so
  exporting from Blender into the addon is enough;
- or with **F6**.

The map is loaded as on a first visit: its saved state is dropped and its
script's `OnStart()` runs again, so intro sequences replay and items you
picked up are back in the map (and in your inventory). The player stays
where they stood, looking the same way. Physics bodies, joints and lights
are made anew from the files, so edited colliders and entities work as in a
fresh game. Meshes, materials and textures that only the map used are read
again; ones still in use elsewhere, such as the hand models, are not.
Files added since the game started, also in new folders, are found. If the
map's file can't be read (a broken export), `hpl.log` says so and the
loaded map stays; the next save of the file tries again. A broken entity
model is logged and that entity is left out, as on a normal load. `.zip`
addons are not watched.

The level editor ([level-editor.md](level-editor.md)) saves moved nodes
into an addon of its own and reloads the same way; `tools/levelview.py`
drives all of this from outside the game.

## glTF models

### What the engine reads

A glTF file is read into the same data as a Collada file, so maps, `.ent`
files and the naming conventions work the same.

| glTF | Engine (Collada meaning) |
|------|-------------------------|
| node name | node name: `_collider_box_*`, `_joint_hinge_*`, `_area_<type>_<name>`, `_ref_<entity>_<name>`, `_sound_*`, `_ps_*`, `_bb_*`, `_beam_*`, `_start_*`, `_node_*`, `_room*`, `_portal*`, `static_*` and `noshadow`/`nocollide` parameters as in the original level editor's output |
| node translation, rotation | node transform |
| node scale | node scale. Lights, colliders (`_collider`, `_charcollider`) and areas (`_area`) keep it out of their transform: it is their size, or a light's radius (x), specular (y) and shadows (z ≥ 0.1) |
| mesh name | geometry name: sub mesh names, `.ent` `PHYSICS SubName`, joint bodies |
| mesh primitive | geometry; a second material becomes a node `<name>_1` next to the first (avoid it on special nodes) |
| `POSITION`, `NORMAL`, `TEXCOORD_0` | vertices (glTF and the engine both have v = 0 at the top of the image); missing normals are made smooth |
| `TANGENT` | tangents (the engine's bitangent is `cross(normal, tangent) * w`, as in glTF); without it they are made like for Collada |
| material | the material file: the base colour texture's file name, else the material's `hpl_material` extra, else the material's name. The extension becomes `.mat`, so `wall.png`, `wall.jpg` and a material called `wall` all use `wall.mat`. glTF's PBR values are not used |
| skin | skin with its joints and inverse bind matrices |
| `KHR_lights_punctual` | light: colour × intensity is the engine's colour (brightness). Spot cone = 2 × outer cone angle. An intensity over 100 is taken to be in physical units (Blender's watts) and left out |
| first animation (or the one called `Default`) | the model's animation. Translation and rotation are used. Scale, morph weights and other animations are not used (the log says so) |
| empty node with a `_ref`, `_bb`, `_ps`, `_sound` or `_beam` name | gets the empty geometry the engine expects for these |
| root node without its own meaning whose children are joints (Blender armature) | left out; its transform is applied to its children |

Extras written by the converter (Blender keeps them as custom properties):

| Extra | On | Meaning |
|-------|----|---------|
| `hpl_id` | node | The Collada id when it is not the name: bone names, animation targets |
| `hpl_joint` | node | A joint that no skin uses |
| `hpl_source_file` | node | An external file instance in a map (entity or `static_` mesh) |
| `hpl_geometry` | node | The geometry name of a node without a mesh. `false` means the node had no geometry, so none is made |
| `hpl_scale` | node | The Collada scale of a node written as a matrix (sheared) |
| `hpl_matrix` | node | The exact Collada transform |
| `hpl_material` | material | The material file |
| `hpl_type`, `hpl_color`, `hpl_angle` | light | Collada light type, exact colour and angle |
| `hpl_length` | animation | The animation's length in seconds |
| `hpl_engine_translation`, `hpl_engine_rotation`, `hpl_hash` | rotation sampler | The engine's exact animation keys |

The exact values (`hpl_matrix`, `hpl_color`, `hpl_angle`, the engine keys) are
used only while the glTF values still agree with them (to 1e-4, or the same
hash for animation keys). Moving, rotating or re-animating something in an
editor makes the engine use the edited glTF values. Unedited converted files
load bit for bit like the Collada originals, which keeps physics the same:
resting bodies settle differently on the smallest difference.

### Converting the games' models

`openhpl1-model` is built next to the game (`build/openhpl1-model`,
`build-windows\openhpl1-model.exe`):

```sh
# One file, and checking that the engine reads the same from both:
openhpl1-model convert office_cabinet.dae office_cabinet.glb
openhpl1-model verify office_cabinet.dae
# A whole game into a folder (with the textures next to each model for editors):
openhpl1-model convert-dir "<game>/redist" converted --verify --textures
# After editing: what changed compared to the original
openhpl1-model compare office_cabinet.dae edited/office_cabinet.glb
openhpl1-model info level01_05_office.glb
```

`verify` and `convert-dir --verify` load both files with the engine's loader
and compare nodes, transforms, geometry (as the engine uses it, with skin bind
shapes applied), materials, skins, lights, bones and animation keys, and the
order of sub meshes, lights and skins. `--exact` compares bit for bit instead
of within tolerances. `--log <file>` keeps the loader's log.

The converted files contain the game's assets: keep them for yourself and do
not distribute them. Distribute only what you have made.

The converter writes what the engine uses, as the loader makes it:

- Triangles of further materials of a Collada geometry are not used by the
  engine (`seem to have multitexturing` in the log) and are not written.
- A skin's bind shape matrix is applied to its vertices.
- Animations are written as the keys the engine makes from the Collada
  channels, as local joint transforms.

### Editing in Blender

Blender imports and exports glTF itself. Blender 5.1 can no longer import
Collada, so convert first. These settings were tested with Blender 5.1.2.

Import: File → Import → glTF 2.0, with the default settings.

Export: File → Export → glTF 2.0:

- Format **glTF Binary (.glb)**.
- Include → **Custom Properties** on (they carry the `hpl_*` extras: Collada
  ids, materials, joints without skins) and **Punctual Lights** on (off by
  default; without it lights are left out).
- Transform → **+Y Up** on (default).
- Data → Mesh: UVs and Normals on. **Tangents** on only if you baked normal
  maps against Blender's tangents; off lets the engine make them.
- Data → Lighting → **Lighting Mode: the same as when importing** (the default,
  Standard, both times). Otherwise light colours change. The engine's colour
  is the brightness: a light with an intensity above 100 (Blender's default
  1000 W light) is taken to be in physical units and its colour alone is used.
- Data → Armature/Skinning: **Include All Bone Influences** on (Blender keeps
  only 4 per vertex by default). Blender also makes each vertex's weights add
  up to 1, which the engine does as well.
- Animation: **Always Sample Animations** off keeps the original keys; on, a
  key is written for every frame, which moved the dog's bones by up to 2°
  between the original keys. One action is the model's animation (call it
  `Default` if there are several).

Keep names: object names are node names (`_collider_box_1` etc.), mesh data
names are sub mesh names that `.ent` files use, and bone names are what game
code and animations use (`hpl_id` keeps a Collada id when Blender has to
rename a duplicate). Blender adds `.001` to duplicate names, so rename
special nodes as needed.

Check an edited model against the original with
`openhpl1-model compare original.dae edited.glb`. Errors are differences that
the engine sees. Notes are differences it does not use: a joint's own
transform when every bone has a track, a skinned mesh's place in the tree, a
longer joint list, resampled animation within 0.5° and 0.001 units. After a
Blender import and export with the settings above:

| File | What still differed |
|------|---------------------|
| `office_filecabinet` | a few normals (Blender recomputes them), one duplicate name renamed `.001` |
| `character_dog_base`, `character_dog_run` | a few normals; one duplicate joint name renamed (its bone keeps its id); with sampling off, one key of one track by a little more than the tolerance |
| `level01_05_office` (a map) | a few normals; two meshes with a few more vertices in another order; mesh order |

## Validation record (2026-10-24)

- All 1,084 Collada files of Overture and Black Plague/Requiem convert, and
  `convert-dir --verify --exact` reads them back bit for bit (Linux GCC,
  ASan/UBSan and Windows MSVC builds; no sanitizer reports).
- Windows, Overture reference captures (101 frames): the new build without
  addons matches the previous commit. Two ice-cave frames change between runs
  of either build. With every Overture model and map replaced by its glTF
  conversion through an addon, 91 frames match. Two are the ice-cave frames.
  The other seven (the six 2D screens over the reloaded boat cabin, and the
  destroyed shaft's first view, with a chair) show models that the Collada
  build had loaded earlier in the run: it reads them back from its own cache,
  which stores values to six significant digits. glTF files are always read
  at full precision, and loose physics bodies settle slightly differently.
  A capture that loads each model only once (boat cabin and destroyed shaft)
  matches in every map view.
- Windows, Black Plague captures of six maps (182 frames): two runs without
  addons already differ in six frames (2D screens, two infirmary lights, the
  examination room). With the glTF addon, three more level 15 frames differ
  (a loose plank, a fire light). Captured alone, level 15 matches in all 50
  frames, so this is the same cache effect as above.
- Windows smoke tests (all maps, save, reload) pass for Overture, Black Plague
  and Requiem with the glTF addons, with the same log errors as without them.
