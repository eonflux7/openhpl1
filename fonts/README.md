# Fonts

The build copies this directory to `<executable dir>/fonts/`. `fonts.cfg` lists
which font file replaces each of the game's bitmap fonts; see the comment in
it. Without the directory, or with `OPENHPL_BITMAP_FONTS=1`, the game draws
its own bitmap fonts.

- `Arimo-Regular.ttf`, `Arimo-Bold.ttf`: static instances (weight 400 and 700)
  of the Arimo variable font, made with fontTools
  (`fontTools.varLib.instancer`) from `ofl/arimo/Arimo[wght].ttf` in
  https://github.com/google/fonts. SIL Open Font License 1.1, `Arimo-OFL.txt`.
- `Cousine-Regular.ttf`: from `ofl/cousine` in https://github.com/google/fonts.
  SIL Open Font License 1.1, `Cousine-OFL.txt`. The computer text (the
  original is Courier New, which Cousine matches in metrics) and the debug
  panels.
- `Recursive-MonoCasualItalic.ttf`: the text on paper (notebook notes, tasks
  and, with the high-resolution HUD, the notes list). A static instance (`MONO=1, CASL=1, wght=380, slnt=-8, CRSV=1`) of
  the Recursive variable font, made with fontTools
  (`fontTools.varLib.instancer`) from
  `ofl/recursive/Recursive[CASL,CRSV,MONO,slnt,wght].ttf` in
  https://github.com/google/fonts. SIL Open Font License 1.1,
  `Recursive-OFL.txt`.
- `Recursive-MonoCasualItalicBold.ttf`: the same at `wght=700`, for the
  notebook's cover menu and headings (`notebook_bold.fnt` in `fonts.cfg`, a
  font of its own with `cour.fnt`'s metrics).
