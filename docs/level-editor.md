# Level viewer and editor

Maps are looked at, changed and checked with the game's own renderer,
physics and scripts. Four parts:

- **The editor window** (`game/core/Editor.cpp`): `openhpl1 --editor`
  (or `OPENHPL_EDITOR=1`) opens a map in an editor instead of the menus:
  Dear ImGui panels around the game's 3D view, a free camera, picking, a
  gizmo, undo, save and Play. See [The editor](#the-editor).
- **Live control** (`game/core/LiveControl.cpp`): with `OPENHPL_CONTROL=<folder>`
  the game takes commands from files in that folder. This is what tools and
  agents use.
- **Level editor** (`game/core/LevelEditor.cpp`): the editing underneath:
  moves, turns and scales the nodes of the loaded map's file, moves what
  the world made of them at once, keeps the undo steps, and saves the map
  as an addon. The editor window, the F2 panel's Level editor window and
  the live commands all use it.
- **`tools/levelview.py`**: starts the game for live control with its own
  profile, sends commands, takes and compares frames. It also does the
  reference images that `tools/refshots.py` did (`refshots.py` still works
  and runs the same code).

Everything here needs developer mode, which `--editor`, `OPENHPL_CONTROL`
and `OPENHPL_MAP` turn on for the run.

## The editor

```sh
./run.sh --editor                      # Linux; the game's first map
OPENHPL_MAP=level01_05_office ./run.sh --editor
platform\windows\run.cmd --editor      # Windows
tools/levelview.py start --editor --map level01_05_office   # with live control too
```

The window can be resized (any game window can now: the frame's buffers
follow), and the side panels shrink in a narrow one; the left panel is
widened by dragging its edge. It has a menu bar with the tools, the Outliner (the map file's
nodes as a tree; a filter or a kind gives a list) and Maps (double-click
opens one) on the left, the Inspector (the selected node: transform,
file, parent, a light's colour and radius; the map's file, unsaved
changes, Save, Reset all, Revert) on the right, and hpl.log at the bottom,
with its errors counted and coloured. The 3D view between them is the
game's renderer, with all its lights, shadows and effects.

The map is held still while it is edited: no physics, no scripts' timers,
no entities moving (`cScene::SetUpdateMap`, `cMapHandler::SetPaused`).
**Simulate** lets it run. **Play** (F8) puts the player where the camera
is, with the HUD, and gives the game the mouse and keys; F8 again comes
back and reloads the map to how it was (Play > Reload when play stops),
keeping unsaved changes.

| Input | Does |
|-------|------|
| Right button held | look around; WASD move, Q/E (or Ctrl/Space) down and up, Shift faster, the wheel sets the speed |
| Wheel | forward and back |
| Middle button | sideways and up |
| Alt + left button | around the selection |
| Click | selects what is under the pointer: the dots of lights, areas, sounds, start positions and particles first, then what the physics ray hits (the entity, or the smallest map mesh there) |
| Double-click, F | the camera to the selection, from a side with room (walls are looked past) |
| Q, W, E, R | select only, move, turn, scale |
| X | the gizmo in the world's axes or the node's own (scaling is always the node's) |
| G | snap: steps set in the Edit menu (0.25 m, 15 degrees, 0.1) |
| Ctrl+Z, Ctrl+Y (Ctrl+Shift+Z) | undo, redo; a gizmo drag or a field's drag is one step |
| Ctrl+S, Ctrl+R | save, reload |
| Esc | deselect |
| F2, F3 | the debug panels (also under View → Debug windows), frame times |

The gizmo: arrows move along an axis and the ring in the middle moves in
the view's plane; rings turn around an axis; boxes scale along an axis
and the middle scales all three. The selection's box is drawn, and a
selected light's radius. Saving writes the map to the edit folder and
reloads it (see [Level editor](#level-editor)).

An agent drives the same window with `tools/levelview.py start --editor`:
every live command works, and `select`, `undo`, `redo`, `play`,
`simulate` and `mouse` (the pointer, in window pixels: move, press,
release, click with any button) are for the editor. `shot` leaves the
panels out unless `--panel` is given. Typing and the right-button camera
are not available that way (`camera`, `look` and `select NODE focus`
move the camera).

## Quick start (live control)

```sh
# Linux build, a window of its own (or --headless with gamescope, --hidden)
tools/levelview.py start --map level01_05_office --addons ~/mymod
# From WSL: the Windows build (OPENHPL_WINDOWS_BUILD, else build-windows/)
# is the default once it is built; game data from OPENHPL_GAME_DIR
tools/levelview.py start --map level01_05_office --hidden

tools/levelview.py views                       # viewpoints of tools/refviews.txt, start positions
tools/levelview.py view view01                 # hold the camera there
tools/levelview.py shot /tmp/office.png        # a frame of the 3D view
tools/levelview.py nodes chair                 # nodes whose name, kind or file has "chair"
tools/levelview.py look woodchair1             # camera on it, short of walls
tools/levelview.py set woodchair1 move 0 0 0.5 turn 0 30 0
tools/levelview.py save                        # writes the map, reloads it, answers when done
tools/levelview.py shot /tmp/office-after.png
tools/levelview.py revert                      # back to the map as it was
tools/levelview.py log --grep error            # the game's hpl.log
tools/levelview.py stop
```

A command prints the game's answer and exits with 1 if the game answers
with an error, so scripts and agents can check each step. `--help` on
any command lists its options.

### Checking an asset before and after

```sh
tools/levelview.py start --map level00_01_boat_cabin --addons <overlay> --hidden
tools/levelview.py shots /tmp/before           # reloads, then every viewpoint
# replace or export the asset into the addon folder (or set/save nodes)
tools/levelview.py shots /tmp/after
tools/levelview.py compare /tmp/before /tmp/after --live --diff-dir /tmp/diff
```

`shots` reloads the map first (with the camera held, so the player's body
is out of the way) and sends the whole set as one batch: with the fixed
steps a session starts with, every set is taken at the same frames after
the load. The first set of a session is taken twice and the first one
thrown away. Two sets of an unchanged map still differ slightly (up to
about 16 in a colour channel, in a few thousand pixels): the game runs a
different number of frames between them, and some of what that changes
outlasts a reload. `compare --live` allows for that (a tolerance of 24); a
moved model or a new texture differs by far more. For the full, strict
check of a renderer change use `capture` (below), which starts the game
for each run.

## Live control

`OPENHPL_CONTROL=<folder>`: the folder gets a file `ready` (process id,
game, edit folder) while the game runs. A command is a file `<id>.cmd`,
written under another name and renamed; the game reads the files in the
order of their names, one at a time, deletes them, and writes `<id>.out`:
`ok` or `error: <why>` on the first line, then the answer. Commands that
take time (loading a map, saving a frame) are answered when they are done.
A file with several lines runs them one after the other without frames in
between, stops at the first error, and gives one answer
(`levelview.py batch`). Old `.cmd` and `.out` files are removed when the
game starts.

| Command | Does |
|---------|------|
| `status` | game, state (`Default` is playing), map, its file, camera (x y z yaw pitch), whether it is held, unsaved changes, the edit folder and the map's saved edits |
| `maps [filter]` | maps that can be loaded |
| `map <name> [start]` | loads a map (from the main menu too) and answers when it has settled (30 frames) |
| `reload` | loads the map again from its files, as F6 does; an error if the file can't be read |
| `views` | the map's viewpoints from `tools/refviews.txt` (with direction and flashlight), its start positions, or some lights |
| `view <name>` | holds the camera at a viewpoint (with its flashlight setting), or puts the player on a start position |
| `camera [x y z [yaw pitch]]` | where the camera is; with numbers, holds it there (radians) |
| `look <node> [dist [yaw pitch]]` | holds the camera looking at what a node shows, from above at the side the camera looks from (or turned until nothing is in the way), short of walls |
| `release` | lets go of the camera: the player stands where it was and walks on |
| `option <name> <0\|1>` | `flashlight`, `flicker` (lamps flicker), `hud`, `godmode`, `dof` (depth of field), `fixedstep` (one logic step per frame, and the same random numbers after every load) |
| `shot <file> [frames] [2d] [panel]` | saves a PNG after some frames (10): the 3D view only, unless `2d`; `panel` opens the F2 panel and shows it |
| `nodes [filter]` / `node <name>` | nodes of the map's file: name, kind, world position, rotation (degrees), scale, entity name, file, parent, `edited` |
| `set <node> ...` | `pos x y z`, `rot x y z` (degrees, X then Y then Z), `scale x y z`, `move x y z`, `turn x y z` (adds to the angles), several in one line |
| `reset [node]` | undoes unsaved changes, of one node or all |
| `save` / `revert` | see the level editor; both reload the map and answer when it is back |
| `pick` | the node under the middle of the screen |
| `select [node [focus]]` | the editor's selection (`--editor`), with `focus` the camera on it |
| `undo` / `redo` | the editor's steps; answers with the step's name |
| `play <0\|1>` / `simulate <0\|1>` | the editor's Play and Simulate |
| `mouse <x> <y> [move\|press\|release\|click] [left\|right\|middle]` | the editor's pointer, in window pixels |
| `window <w> <h>` | resizes the window, as dragging its edge does |
| `script <line>` | runs a line in the map's script, e.g. `script SetLampLit("lamp01", false, false);` (a line that does not compile is only logged; `levelview.py script` looks for that) |
| `wait <frames>` | |
| `quit` | |

Holding the camera works as the F2 panel's Fly: the player's body stays
where it was, switched off, and does not adapt to the dark or narrow the
view. The panel shows that the live control holds it, with a button to let
go. Nodes are named as in the map file, or by the entity, area or sound
they make: `woodchair1` finds `_ref_furniture_woodchair_woodchair1`.

`levelview.py start` writes a settings file with the reference images'
fixed graphics settings, in a window of `--size` (1280x720), with depth of
field off (scripts blur the view with it now and then), 16x anisotropic
texture filtering (the game's default; the reference images keep 1),
item flashing off,
the game's sounds off (`--sound` for them) and fixed steps (`--real-time`
for normal timing). `--set Section.Key=Value` changes a setting. A session
keeps its files (control folder, profile with settings, saves, logs and
caches, frames) in `~/.cache/openhpl1/levelview`, or for the Windows build
in `<build>/levelview` on the Windows drive, or `--session`; the other
commands use the last session started (or `--session`,
`OPENHPL_LEVELVIEW_SESSION`). `--fresh` empties the profile first.

## Level editor

The editor window, the F2 panel's **Level editor** window (it opens when
a node is picked), or the live commands above. The loaded map's file (its `.dae`, or the `.glb` of an addon that stands in for
it) is read when the editor is first used on a map. A node is picked from
the list (filter by name, kind or file) or with **Pick** (what the middle
of the screen shows), and its world position, rotation and scale are
changed with the fields. Children move along. Changes are made to the
file's data, and what the world made of the node follows at once: map
meshes (made dynamic: they leave the static shadow casters), their
bodies, game entities' bodies (or mesh), lights (a light's radius is its
scale's x), areas, start positions, sounds, particles, billboards and
beams. A scale shows on meshes but not in bodies until the map is saved
and reloaded; with the world held still, what rests on a moved object
stays where it was until it runs. **Save and reload** writes the whole
map and reloads it, with the player and camera where they were, so the
change is seen with its physics, lights and scripts as on a first visit.
A reload without a save (F6, `reload`, the end of Play) makes the unsaved
changes again in the new world. **Reset node** and **Reset all** undo
unsaved changes; **Revert** removes the saved map. Undo steps are kept
over saves and reloads of the same map.

Saved maps go to the **edit folder**, an addon of its own with priority
1000 (above other addons): `OPENHPL_EDIT_DIR`, else
`<personal folder>/addons/editor/`. It is made, with its `addon.cfg`, in
developer mode, and kept when `OPENHPL_ADDONS` names only some addons; with
`OPENHPL_ADDONS=0` there is no edit folder and nothing can be saved. A
saved `<map>.glb` replaces the game's map and an addon's copy of it until
it is reverted or removed. The saved file differs from the one read only in
the changed nodes (`openhpl1-model compare` shows them); it holds the whole
map, so treat it as the game's data.

Next to it, `<map>.edits.txt` lists every node changed since the map was
first saved there, with its world transform now and before the first
change, in the map's glTF space (+Y up, metres, quaternions x y z w):

```
# node<TAB>x y z<TAB>rotation x y z w<TAB>scale x y z<TAB>was<TAB>x y z<TAB>...
_ref_furniture_smalltable01_smalltable1	1.58351 -0.031037 0.256093	0 0.8191521 0 0.5735765	1 1 1	was	1.48351 -0.031037 -0.043907	0 0.7071068 0 0.7071068	1 1 1
```

This is how changes go back to a map's source: a Blender scene imported
from the same file can apply the difference of each node (penumbra-
definitive's `hpl_tools.py` has an "Apply engine edits" operator that
does). Areas, lights and colliders keep their size in the scale and the
rest of the transform without it, as the engine reads them
([modding.md](modding.md)). Angles shown may come back as another set of
angles for the same rotation (0 115 0 as 180 65 180).

## Reference images

`levelview.py capture`, `effects` and `compare` are the former
`tools/refshots.py` (which still runs them): see the README's "Reference
images". They start the game for each run, headless, with fixed steps and a
fixed seed from the start, and are the strict check for renderer changes.

## Validation record

The editor window, checked with the Windows build from WSL (hidden
window, 1600x900) and Overture's `level00_01_boat_cabin` with
penumbra-definitive's overlay, driven with live commands and `mouse`:

- The window: Outliner tree, Inspector, log, helper dots, selection box,
  gizmo (shots with `--panel`).
- A drag of the move gizmo's X arrow moved `smalltable1` along x only
  (0.29 m), one undo step; a drag of the rotate gizmo's Y ring turned it
  about Y; clicks selected a pillow, an area's dot and a map mesh; the
  toolbar's Rotate was clicked.
- Live moves without saving: the entity, the map's own table mesh
  (`bordet`) and the spot light move at once; undo and redo give the
  frames back (redo equal to the edited frame within 1).
- Play: the player at the camera with the HUD; stopping reloaded the map
  and made the unsaved change again (same frame as before the reload).
- Reset, undo after reset, save (one node in `edits.txt`), revert.
- Focus frames the node from inside the cabin in a cramped corner.
- Linux and ASan CTest pass; the MSVC build has no warnings; the Windows
  smoke test passes (28 maps).
- Resizing (`window`): 1280x720 to 1700x800, 900x900 and 1920x1000; the
  frames have the new size, the view its aspect, the scene and screen
  buffers are made again (hpl.log), the panels fit. Anisotropy 16 against
  1 in the same view: the floor's far planks are sharp.
- Not run: flying with the right button, keyboard shortcuts and typing
  (no keyboard or relative mouse through the live control), a visible
  window by hand, the Linux build in a window.

Earlier checks of the live control and the level editor, same setup:

- `start`, `status`, `maps`, `map` (cabin to office), `views`, `view`,
  `camera`, `look`, `pick`, `release`, `option`, `shot` (also `--2d
  --panel`), `nodes`, `node`, `set`, `reset`, `save`, `revert`, `script`
  (also a line that does not compile), `batch`, `stop`, and errors for bad
  arguments and unknown nodes (exit code 1).
- In `level00_01_boat_cabin` with penumbra-definitive's overlay: a table
  moved and saved is seen moved after the reload, and the flashlight lying
  on it falls to the floor; `revert` puts it back. Three nodes (an entity,
  the spot light, an area) changed over two saves: `openhpl1-model compare`
  finds exactly those nodes changed in the saved map; Blender 5.1.2,
  applying `edits.txt` to the imported overlay map and exporting it, gives
  the same world transforms to 0.00004.
- `shots` sets of the unchanged cabin taken back to back with random pauses
  between them differ by at most 16 in a channel; a moved table differs by
  up to 240 (mean 13). Two `capture` runs of the cabin (22 frames) are the
  same, as before the move into `levelview.py`.
- The Windows smoke test passes (28 maps, each reloaded after its save),
  and the panel's Level editor section shows the picked, edited node
  (`shot --panel` at 1920x1080).
- Not run: the Linux build in a live session (no gamescope on the machine
  used; WSLg is not used for game runs), Black Plague and Requiem, editing
  by hand with mouse and keyboard.
