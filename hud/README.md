# HUD images

The build copies this directory to `<executable dir>/hud/`. Each
`name@<N>x.png` here replaces the game's `name.bmp` (or other image) of the
same name: it is laid out at the original's size, from its own mipmapped
texture, so at high resolutions it shows N times the detail. N is 2, 3, 4 or
8. The PNG is N times the original's size, or N times the original plus a
margin of M original pixels on each side (M may differ between width and
height, but is whole and the same on opposite sides). The margin is drawn
around the original's rectangle, scaled with it, so edges, wrists and
shadows fade out there instead of being cut at the sprite's border. Any
other size is not used: the original is drawn, with a warning in `hpl.log`.
Images not listed here are unchanged. `OPENHPL_ORIGINAL_HUD=1` draws all of
the game's own images.

These images are not covered by OpenHPL1's GPL licence. The generated ones
(the hands, eye, door, ladder, notebook, paper, item icons and the loading
and menu screens, logos included) were made after Frictional Games' own
images, which remain theirs; they are included as fan work, to be changed or
removed if Frictional Games asks. The SVGs in `src/` and the images drawn by
`tools/hud_pencil.py` are this project's own.

Overture and Black Plague (and so Requiem) ship byte-identical copies of
every image replaced here, so one set serves all three games.

| File | Replaces | Margin | Made from |
| --- | --- | --- | --- |
| `player_crosshair_pointer@4x.png` | Menu/inventory/notebook mouse arrow (32x34) | | `src/player_crosshair_pointer.svg` |
| `player_crosshair_active@4x.png` | Open hand: interact (27x26) | 3 | Generated, `src/prompts/hands.md` |
| `player_crosshair_grab@4x.png` | Fist: grab (27x26) | 3 | Generated, `src/prompts/hands.md` |
| `player_crosshair_inactive@4x.png` | Pointing hand: nothing to use (27x26) | 3 | Generated, `src/prompts/hands.md` |
| `player_crosshair_pickup@4x.png` | Pinching hand: pick up (27x26) | 3 | Generated, `src/prompts/hands.md` |
| `player_crosshair_invalid@4x.png` | Red hand: can't use (27x26) | 3 | `active`, recoloured |
| `player_crosshair_examine@4x.png` | Eye: examine (20x13) | 2 | Generated, `src/prompts/examine.md` |
| `player_crosshair_doorlink@4x.png` | Open door: go to another map (27x31) | 3 | Generated, `src/prompts/door-ladder.md`; a faint blue glow around the frame (`hud_fit.py glow`) |
| `player_crosshair_ladder@4x.png` | Ladder: climb (27x31) | 3 | Generated, `src/prompts/door-ladder.md` |
| `player_crosshair_cross@4x.png` | Overture's crosshair (17x17), a broken cross in the original: the same dot as Black Plague's, centred | | `src/player_crosshair_cross.svg` |
| `blackplague/player_crosshair_cross@4x.png` | Black Plague's and Requiem's dot crosshair (9x9), smaller than the original's | | `src/blackplague/player_crosshair_cross.svg` |
| `notebook_open@3x.png` | Paper page for notes and tasks (350x460) | 8 | Generated, `src/prompts/paper.md` |
| `notebook_front@3x.png` | Notebook cover (350x460) | | Generated, `src/prompts/notebook.md` |
| `notebook_nextpage@8x.png` | Turned-down corner: next page (79x70) | 6, 5 | Rendered from the paper page |
| `notebook_prevpage@8x.png` | Turned-down corner: previous page (79x70) | 6, 5 | Rendered from the paper page |

New images, which the game does not have, are named `name@<N>x.png` too and
are laid out at 1/N of their size. They are used by the notebook's
remastered look (only with these images, not with `OPENHPL_ORIGINAL_HUD=1`):

| File | Use | Made from |
| --- | --- | --- |
| `notebook_underline_1..3@4x.png` | Pencil line drawn in under a hovered entry (200x8) | `tools/hud_pencil.py` |
| `notebook_rule@4x.png` | Ruled line under each slot of the notes list (340x2) | `tools/hud_pencil.py` |
| `notebook_margin@4x.png` | Red margin line of the notes list (3x460) | `tools/hud_pencil.py` |
| `notebook_unread@4x.png` | Ink dot after a note not read yet (6x6) | `tools/hud_pencil.py` |

`python3 tools/hud_pencil.py` draws them from fixed random seeds, so it
gives the same files every time.

`hud_keycap@4x.png` (16x16, from `src/hud_keycap.svg`) is the key that
tips and the inventory draw a key's or mouse button's name on ("L Ctrl",
"Esc", "L Mouse"; `cButtonHandler::GetKeyLabel`). The game tints it with
the text's colour and keeps its 6-unit ends while stretching the middle to
the name, which is drawn in the text's own font (`iFontData`, text between
`kwcKeyCapStart` and `kwcKeyCapEnd`). Without it (`OPENHPL_ORIGINAL_HUD=1`)
the short name is plain text.

Not replaced: `notebook_background` (a flat 22% black over the screen, so
there is nothing to sharpen). `notebook_textback`, `notebook_listtextback`
(the dark smudge behind a hovered entry) and `notebook_tab` (the strip beside
each entry of the notes list) are not drawn in the remastered look; the
pencil underline and the ruled page take their place.

## Inventory

With the images below, the inventory (`game/ui/Inventory.cpp`) is a drawer
that slides in at the right edge of the screen (past the 4:3 area on a wide
screen) instead of the original's grid in the middle; with
`OPENHPL_ORIGINAL_HUD=1`, or without the drawer's images, it is the
original.

- 4x6 cells for the 24 slots, the hovered item's name and description under
  them, and health and battery at the bottom as two labelled, segmented
  bars with their percentage (health green, amber then red, battery pale
  then amber then red; both pulse when low). The original's health figure
  only hinted at three states; the bar shows how much is left. The nine hotkey slots are a row
  at the bottom of the screen beside the drawer, numbered.
- Items of the type `Normal` (keys, parts and the like, which only scripts
  use: what the story needs) have a small glowing amber light in their cell's corner.
- An item let go of over the scene is dropped as before; let go of over the
  drawer (not on a cell) it goes back to its cell.
- The slot indices, and so scripts and saves, are unchanged.
- Two looks: metal (the default, after the original: a darkened strip with
  a rusty rail at its edge and rusty iron cell frames) and a sheet of paper
  written on in the notebook's fonts (`OPENHPL_INVENTORY_LOOK=paper`).

| File | Use | Made from |
| --- | --- | --- |
| `inventory_*@4x.png` (items) | Item icons, replacing the game's (about 60x60) | Generated, `src/prompts/inventory-icons.md` |
| `blackplague/inventory_saw@4x.png` | Black Plague's saw, which differs from Overture's | Generated, as above |
| `inventory_drawer_metal@3x.png` | The metal drawer (300 x screen height) | `tools/hud_inventory.py` |
| `src/textures/rust.png` | Rusted iron photograph mapped onto the metal look's frames and rail (not loaded by the game) | Generated, `src/prompts/rust.md`, scaled to 640x640 |
| `inventory_drawer_paper@3x.png` | The paper drawer | Generated, `src/prompts/inventory-drawer.md`, then `tools/hud_inventory.py` |
| `inventory_cell_<look>[_hover]@4x.png` | A cell of the grid (62x62) | `tools/hud_inventory.py` |
| `inventory_hotkey_<look>[_hover]@4x.png` | A hotkey slot (48x48) | `tools/hud_inventory.py` |
| `inventory_quest@4x.png` | The quest item mark (12x12), in both looks | `src/inventory_quest.svg`, `tools/hud_raster.py` |
| `inventory_meter@4x.png`, `inventory_meter_ticks@4x.png` | Health and battery bar: trough (124x9), segment ticks over the fill | `src/*.svg`, `tools/hud_raster.py` |
| `inventory_meter_fill@4x.png` | The bar's fill, stretched and tinted by the game | As above |
| `inventory_icon_health@4x.png`, `inventory_icon_battery@4x.png` | The bars' icons (14x14), tinted with the bar's colour | As above |

`blackplague/` holds images that Black Plague and Requiem use in place of
Overture's image of the same name. The item icons are drawn fitted into
their cell whatever their size, so they keep the original's size (as
replacements must) but not its framing.

### Item icons

Every item image of Overture and Black Plague (Requiem has none of its own)
was generated again as one set: the same camera (a three-quarter view from
about 35 degrees above), the same light from the upper left, long and flat
things lying diagonally with their working end to the upper right, things
that stand standing. Images that are byte-identical in both games were made
once. Codex's image generation (October 2026) made them in batches of six
from `src/prompts/inventory-icons.md`, with each original enlarged as its
reference, the object descriptions in `src/inventory-icons.tsv`, and three
icons of a first trial batch (acetone jar, crowbar, key) as style
references for the rest. `tools/hud_fit.py item` fits each: the object,
scaled so that its longer side is 90% of the original's shorter side,
centred, with a soft shadow down and to the right. From the repository
root, with the originals of a game in `ORIG` and a generated image `GEN`:

```sh
python3 tools/hud_fit.py item $ORIG/inventory_crowbar.bmp $GEN/crowbar.png hud/inventory_crowbar@4x.png
```

The output keeps the original's file name (case included, e.g.
`inventory_chemical_A`). Black Plague's `tutorial_chemical_A..C` are
Overture's `chemical_A..C` and `item_hand` is `hand`, so those are copies.

`python3 tools/hud_inventory.py PAPER.png CANVAS.png` makes the drawer
images from the two generated backgrounds and draws the cells, hotkey slots
and marks from fixed random seeds.

## Notebook

With these images the notebook (`game/ui/Notebook.cpp`) is drawn as follows;
with `OPENHPL_ORIGINAL_HUD=1` it is drawn as in the original game.

- Hovering over an entry of the cover or the notes list draws a pencil line
  under it from left to right in 0.2 s (`cGraphicsDrawer::DrawGfxObjectPart`
  draws the part done so far) and darkens a note title to full black; moving
  away fades it out. The notes list uses three different lines in turn.
- The notes list is a ruled index page: the heading "Notes" (the game's own
  text), a red margin line, a pencil rule under each of the 11 slots,
  numbers in the margin, titles cut short with "..." where they would run off
  the page, and an ink dot after notes not read yet. A note counts as read
  once it has been opened; the game saved that flag but never set it, so in
  saves from before this every note shows as unread until opened.
- The tasks, notes list and note pages show "page / pages".
- The notes list is written in the paper's font, like the notes; its
  heading and the cover menu use the same font in bold (see
  `fonts/README.md`), the cover menu at a size that gives it the original's
  capital height.

## Loading and menu screens

`screens/` holds widescreen versions of the full-screen JPEGs: loading
screens (1707x960), and the main and pause menu's logo (1707x288) and
background (1707x672). `cGraphicsHelper::LoadScreenImage` looks for the
image's name, as `.jpg`, in `screens/requiem/` (Requiem only), then
`screens/blackplague/` (Black Plague and Requiem), then `screens/`; without
one, or with `OPENHPL_ORIGINAL_HUD=1`, the game's own image is drawn. A
widescreen image keeps the height of the original's rectangle in the 800x600
screen and is centred, so the original's 4:3 part is its middle and the rest
fills a 16:9 screen.

They were generated as 16:9 images whose middle was made after the original
as the game shows it, then fitted by `tools/hud_screen.py` (see its
`--help`): registered to the original, resampled so the original's
rectangle lands in the middle, and colour-matched to it. The output's pixels
come from the generated image only. Requiem's loading screens share one
ragged black frame: `src/screens/requiem_frame.png` is its mask, made from a
generated frame by `hud_screen.py frame` and laid over each screen by
`hud_screen.py framed`.

## Vector images

`src/*.svg` have a viewBox the size of the original image, in its pixels, and
use plain geometry (no filters), so any renderer draws them alike.
`python3 tools/hud_raster.py` draws each to `name@4x.png` here (needs
`rsvg-convert` or the `cairosvg` module).

The pointer was fitted to the original: the arrow polygon, outline width and
halo were adjusted until the SVG, drawn at 4x and shrunk to 32x34, differs
from the original by 1.7% (mean premultiplied RGBA). The game draws the
sprite centred on the mouse, so the arrow's tip sits at the centre (16,17).
The PNG is then blurred slightly (`hud_fit.py soften --sigma 1.2`), so its
edges are no crisper than the enlarged original's.

## Generated images

The hands, eye, door, ladder, paper and cover were made with Codex's image generation
(October 2026), from the prompts in `src/prompts/`, with the original image
enlarged as the reference; the hands, eye and paper on a transparent
background. `tools/hud_fit.py` fits each result to the game image (the
32-bit BMP, read with its alpha). From the repository root, with the
originals from `redist/graphics/` in `ORIG` and the generated images in
`GEN`:

```sh
for h in active grab inactive pickup; do
  python3 tools/hud_fit.py icon $ORIG/player_crosshair_$h.bmp $GEN/$h.png \
    hud/player_crosshair_$h@4x.png --margin 3
done
for h in doorlink ladder; do
  python3 tools/hud_fit.py icon $ORIG/player_crosshair_$h.bmp $GEN/$h.png \
    hud/player_crosshair_$h@4x.png --margin 3
done
python3 tools/hud_fit.py glow hud/player_crosshair_doorlink@4x.png \
  hud/player_crosshair_doorlink@4x.png --rect 28 16 102 115
python3 tools/hud_fit.py tint $ORIG/player_crosshair_active.bmp \
  $ORIG/player_crosshair_invalid.bmp hud/player_crosshair_active@4x.png \
  hud/player_crosshair_invalid@4x.png
python3 tools/hud_fit.py icon $ORIG/player_crosshair_examine.bmp $GEN/examine.png \
  hud/player_crosshair_examine@4x.png --margin 2 --box -1 0 21 13 --edge 80 220
python3 tools/hud_fit.py sheet $ORIG/notebook_open.bmp $GEN/paper.png \
  hud/notebook_open@3x.png --margin 8
python3 tools/hud_fit.py curl $ORIG/notebook_open.bmp hud/notebook_open@3x.png \
  $ORIG/notebook_nextpage.bmp hud/notebook_nextpage@8x.png --draw 231 340 119 120 --fold 52 58
python3 tools/hud_fit.py curl $ORIG/notebook_open.bmp hud/notebook_open@3x.png \
  $ORIG/notebook_prevpage.bmp hud/notebook_prevpage@8x.png --draw 0 340 119 120 --side left --fold 52 58
python3 tools/hud_fit.py page $ORIG/notebook_front.bmp $GEN/notebook.png \
  hud/notebook_front@3x.png
```

- `icon` scales the generated image's opaque part into the box the
  original's fills and matches its colours to the original's (mean and
  spread per channel), keeping the generated outline: crisp, with no blur
  that would read as a glow. The hands' wrists fade out into the margin
  instead of being cut at the bottom as in the original. The original eye is
  cut off at its corners; the new one is the whole eye, a little wider than
  the original's box (`--box`), with its soft skin rim tightened
  (`--edge`).
- `tint` recolours the fitted `active` hand the way the game's `invalid`
  hand recolours its `active` hand.
- `sheet` fits the generated sheet of paper to the page's rectangle and lays
  a soft shadow under it in the margin. The paper departs from the
  original on purpose: a clean, real-looking sheet with a fibrous, slightly
  irregular edge and a few foxing spots, instead of the original's
  blotchy texture cut off at a rectangle with ragged corners.
- `curl` renders the page-turn corners from the finished page, so they are
  the same paper: the corner beyond a fold line is turned back over the page
  (its back paler and shaded like a curl), the page underneath shows where
  it was, and the flap casts a shadow. The game draws these images stretched
  to 119x120 page pixels at the page's bottom corners; they are rendered at
  that shape and then resized. Everything that would look the same as the
  page is transparent, so the corner shows no seam and nothing is drawn
  twice. Run it again after changing the page.
- `page` (the cover) fills the original's box with the generated picture and
  takes the enlarged original alpha; the cover has no hard edge of its own.

Mean difference from the original after shrinking back to its size (margin
removed), 0..1: pointer 0.017, active 0.052, grab 0.039, inactive 0.049,
pickup 0.040, invalid 0.054, doorlink 0.052, ladder 0.059, examine 0.089 (the whole eye), cover 0.031,
paper 0.027 (a new design). A second, more specific eye prompt
(`src/prompts/examine-refine.md`) matched worse and was not used;
`notebook.md` also asked for a paper page, which the paper prompt replaced.

These images recreate Frictional Games' artwork, which is not covered by
this repository's GPL; the same applies to the pointer's design.
