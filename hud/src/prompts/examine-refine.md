You are recreating the "examine" HUD cursor from the 2007 game Penumbra: Overture at high resolution. Use your image generation tool. Do not write code or do anything else.

Attached:
- player_crosshair_examine_ref.png: the original 20x13 icon, enlarged (grey = transparency).
- previous_attempt_ref.png: a previous high-resolution attempt. Good realism and colours, keep that quality.
- comparison_ref.png: left = original pixels, middle = previous attempt shrunk to 20x13, right = previous attempt.

The previous attempt is not faithful enough. Fix these, compared to the original:
1. The original has a heavy, dark upper eyelid edge: a thick band of dark lashes / shadow along the whole top of the eye opening, so the upper third of the eye reads as dark. The previous attempt has a thin clean lid line instead.
2. The iris sits higher: its top is hidden under that dark upper lid, and the large dark pupil touches the upper shadow. The iris is slightly right of centre.
3. Pupil larger relative to the iris (dilated). A small bright specular highlight sits at the upper-right edge of the pupil.
4. Sclera: cool, slightly bluish white, brightest at the lower-right.
5. Upper lid skin above the dark lash band: a thin pale beige crease band. Lower lid: thin pinkish rim, with a small pink caruncle at the left corner.
6. Keep: realistic, softly lit, the almond silhouette and wide 20:13 framing with the eye filling the canvas width, skin feathering softly to transparency. Fully transparent background (real alpha PNG). No eyebrow, no face.

Canvas 1536x1024. Save the result in the current directory as exactly: examine.png (copy the generated file there). Reply with just the filename.
