# Documentation

Start with the repository [README](../README.md) for build/run instructions
and [ROADMAP](../ROADMAP.md) for current work across all three games.
[CHANGES](../CHANGES.md) summarises changes from the originals; the detailed
log to 2026-11-10 is [archived](archive/CHANGES-2026-11.md).

Current technical guides:

- [Black Plague and Requiem](black-plague-port.md): game selection, stand-ins
  for missing code, porting tools, binary inspection and map/script diagnostics.
- [Dog foundations](dog-foundations.md): Original/Improved AI, animation,
  the generated arena and original-asset regression checks.
- [Shadow-map cache](shadow-cache.md): ownership, invalidation, budgets,
  tests and remaining validation limits.
- [Audio lifecycle](audio.md): EFX slot mutex ownership, worker shutdown and
  startup crash regression coverage.
- [Audio review](audio-review.md): audio defects and the modernization plan
  for the `feat/better-audio` branch.
- [Addons and glTF models](modding.md): addon folders and `.zip` files, how
  the engine reads glTF, `openhpl1-model` conversion and checks, editing in
  Blender, and the conversion's validation record.
- [Level viewer and editor](level-editor.md): the editor window
  (`--editor`: outliner, inspector, gizmo, undo, Play around the game's
  view), driving the running game from files (`OPENHPL_CONTROL`,
  `tools/levelview.py`), moving map nodes and
  saving the map as an addon, live frames before and after, and the
  reference images.
- [Fonts](../fonts/README.md): bundled typefaces and licences.
- [HUD images](../hud/README.md): high-resolution cursor, icons and notebook
  pages, how they replace the game's images and how they were made.
- [Agent instructions](../AGENTS.md): repository conventions and validation.

[Archived plans and reviews](archive/README.md) retain historical evidence.
They may describe removed renderer paths, old executable/worktree names or
temporary tools that are no longer available. Use the live roadmap for status.
