# Fonts

The build copies this directory to `<executable dir>/fonts/`. `fonts.cfg` lists
which font file replaces each of the game's bitmap fonts; see the comment in
it. Without the directory, or with `OVERTURE_BITMAP_FONTS=1`, the game draws
its own bitmap fonts.

- `Arimo-Regular.ttf`, `Arimo-Bold.ttf`: static instances (weight 400 and 700)
  of the Arimo variable font, made with fontTools
  (`fontTools.varLib.instancer`) from `ofl/arimo/Arimo[wght].ttf` in
  https://github.com/google/fonts. SIL Open Font License 1.1, `Arimo-OFL.txt`.
- `Cousine-Regular.ttf`: from `ofl/cousine` in https://github.com/google/fonts.
  SIL Open Font License 1.1, `Cousine-OFL.txt`.
