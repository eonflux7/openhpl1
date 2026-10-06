# Porting roadmap: Black Plague and Requiem

The same executable plays Penumbra: Overture, Black Plague and Requiem
(`./run.sh blackplague`, `./run.sh requiem`). The engine is the one the two
later games shipped with; what Frictional never released is the game code
Black Plague added to Overture's. It is written again here, after the games'
binaries and data. How that is done, and the tools for it, are in
[docs/black-plague-port.md](docs/black-plague-port.md).

Nothing below is play-tested unless it says so. "Checked" means the smoke
test (`OVERTURE_SMOKE=all`): every map loaded and run for a few frames with
its script compiled.

## ✅ Step 0: the games start

- [x] One executable picks the game: `OVERTURE_GAME`, or the data in the
      working directory; settings, saves and logs apart
      (`BlackPlagueRemade/`, `RequiemRemade/`)
- [x] Requiem's own files: `requiem_resources.cfg`, `requiem_materials.cfg`,
      `expansion01/config/`, `core/requiem_cache/`, maps in `expansion01/maps`
- [x] Every script function of the two games exists with its true name and
      argument types; those not written yet only count their calls
      (`game/core/GameScriptsNotPorted.cpp`, listed in the log at exit)
- [x] Scripts compile with today's AngelScript (bool forms of `SetLocalVar`,
      `SetGlobalVar`, their `Create` forms and `SetBodyProperty`)
- [x] All maps load: Black Plague 26, Requiem 12, no failures; Overture's 25
      as before. Black Plague's main menu comes up
  - [ ] starting a map at `level03_messhall_entrance:link01` (inside the
        vent, as `OVERTURE_MAP` does) drops the standing player through
        the floor; in the game he arrives crouched from level 2's vent.
        Check when playing through
- [x] The games' folders are left alone: rebuilt mesh caches and AI node
      files go to `cache/` in the personal folder

## Step 1: walking through the levels

What gates progress from one room and map to the next.

- [x] Levers and wheels (`GameLever`, `GameWheel`) and their ten script
      functions. Checked; **to play-test**: pulling, turning, getting stuck,
      a wheel driving a joint (Requiem level 6)
  - [x] the tower's spring-loaded levers flickered between "min" and
        "middle" at rest: a joint pushed onto its limit bounced there under
        Newton 2. Fixed in the engine (hinge and slider stops hold within a
        hair of the limit); the lever now rests at "min". The same bounce
        kept Requiem level 2's five security doors moving, whose move sounds
        took every sound channel (392 sounds not played in 20 s, now none),
        kept Overture's generator-room lever awake, and made the corpse in
        Black Plague's machine room twitch. Overture's other maps tried log
        the same as before. **To play-test**: levers, wheels and sliding
        doors in all three games
- [x] Locked links: `SetLinkLocked`, `SetupLinkLockedProp`
- [x] The loading screen's picture: `ChangeMap`, `SetupLink`,
      `SetupLinkLoadText` with one more argument
- [x] A spider hatching from its egg (what an object breaks into gets
      `OnWorldLoad`)
- [x] Locked doors: `SetDoorLockedProperties` (what is said and heard when
      a locked swing door is tried); a door held by a stick area cannot be
      pulled. **To play-test**
- [x] Moving things: `MoveEntityToArea` (`iGameEntity::MoveToPosition`),
      `TeleportEntityToArea`, `TeleportBodyToArea`, `TeleportPlayerToArea`,
      `CreateEntityAtArea`. Tried headless on the sewer's water and a crate
  - [x] a move under way is kept in a save game (and where a mesh without
        bodies was moved to). Tried headless on the tutorial's shelf: saved
        half-way, loaded, it went on and called back on arriving
- [x] Collision checks: `CheckBodyCollision`, `CheckEntityBodyCollision`,
      `CheckGameEntityCollision`
- [x] Stick areas: `AttachBodyToStickArea`, `DetachBodyFromStickArea`
- [x] `SetNoFallDamage`, `TurnOffGlowStick`, `DropFlare`,
      `SetLowPowerCallback`, `SetNotebookCloseCallback` (the three player
      settings are saved)
- [ ] Buttons (`cGameButton`): `SetupButton`, `SetButtonOn`. No entity file
      and no script of the two games uses them; left for last
- [x] Vent areas (`GameVentArea`): a player who jumps into the mouth of a
      shaft is made to crouch and carried onto its floor. Checked only;
      **to play-test** (levels 2, 3 and 20)
- [x] `SetupDamageArea`'s kind of damage and sound; the new "poison" kind
      with its picture. Seen in a headless frame (Requiem level 2's gas)
- [x] Radio: `AddRadioMessageAtPos` (heard from a place, louder when near,
      its text hidden out of range), `StopAllRadioMessages`. Seen in headless
      frames; **to play-test** with sound
  - [x] `SetRadioOnEndCallbackGui` (the function is in the terminal's script);
        Escape over a terminal skips the radio message heard
- [ ] The menu's music and other files named differently than in Overture;
      go through the "not found" lines of the logs

## Step 2: the Infected

Written after the binary, starting from the Dog (most of its functions are
the same size in both). Checked; tried headless on level 7, where an
Infected with the crowbar went from patrol to hunt, found its way over 48 m
to the player and struck twice. **To play-test**: how it looks, moves and
sounds, the throat grab, doors, the flashlight.

- [x] `cGameEnemy_Infected` and its 13 states (`game/enemies/GameEnemy_Infected`).
      What differs from the Dog: three kinds of attack (a swing standing
      still, a swing while moving in, the grab for the throat), the blow
      landing at the animation's special event, a flinch in place of the
      knock-down with health set back to full, death by animation, a hunt
      sound, fast healing
  - [ ] seen only in the log so far: the frames of the test were too dark
        to show the model and its weapon
  - [ ] set on the player from behind its door (level 7's locker ambush, by
        script from the map's start) it walked into the wall beside the
        doorway; not known whether the original does the same there
- [x] Weapons, the flashlight and the throat grab: `SetInfectedWeapon`,
      `AddInfectedFlashlight` (`iGameEntity::AttachMeshEntityToBone`), the
      zoom of the player's forced look (`cPlayerLookAt::SetZoomTarget`)
- [x] What changed in the enemy base class, for the two later games only:
      line of sight from eye height with nine rays, no seeing through walls
      when touching, one sound at a time that follows the enemy, anything
      movable counted as a door to break, deacceleration set, and
      `PlayEnemyAnim`, `SetEnemyAnimOverCallback`, `SetEnemyHuntCallback`,
      `SetEnemyPatrolOverCallback`, `SetEnemyStopAtLastPatrolNode`
  - [x] `iGameEntity::SetActive` in the binary also takes an object out of
        the player's hands when it is switched off
- [x] Security cameras (`GameCamera`): `SetupSecurityCamera`,
      `SetSecurityCameraAlerted`. Checked only (Requiem level 2 takes the
      calls); **to play-test**
- [x] Save games with an Infected (weapon and flashlight are saved) and a
      camera. Tried headless (`OVERTURE_SCRIPT` "save" and "load"): an
      Infected saved while hunting comes back where it was, patrols, hunts
      again when shown the player and strikes as hard with its crowbar; an
      alerted camera saves and loads. **To play-test**: a save loaded and
      played on

## ✅ Step 3: computer terminals

Written after the binary (`game/ui/GameGui`). All 12 of Black Plague's and
the 3 of Requiem's screens open, headless, with their pictures, fonts and
scripts; folders, the text viewer, pop-ups, typing into a text box and the
close button were tried with injected clicks and keys. **To play-test**: each
terminal in its level, with sound.

- [x] `cGameGui`: the loader for `interfaces/*.gui` (images, animated
      images, textures, fonts, the ten kinds of widget) and the screen's
      own script (`OnLoad`, `OnStart`, `OnUpdate`, `OnExit`, callbacks)
- [x] `cGameGuiHandler`: `OpenScreenGui`, `CloseScreenGui` (the scene is
      not drawn behind a full screen; scan lines over it). The mouse moves a
      pointer as in the menus; Escape closes the screen
- [x] The widget script functions: `SetWidgetBool/Int/Float/Color/Vec2/Vec3/
      String/StringTrans`, `GetWidgetBool/Int/String`, `AddWidgetItem(Trans)`,
      `RemoveWidgetItem`, `SetWidgetCallback`, `SetAttentionWidget`
- [x] Pop-ups and the text viewer: `CreateGuiPopUp(Trans)`,
      `CreateGuiTextView` (the text goes into the notebook the first time)
- [x] Subtitles and radio text are drawn over an open screen
- [x] Wide screens: a gui set the size of the 800 x 600 screen is centred
      like the rest of the interface (engine, `cGuiSet::Render`); clipping
      follows
- [x] `StartSchmup` returns false ("wrong password") until the game is
      written; level 18's terminal script needs the bool to compile

## Step 4: the rest of the game

- [x] Sound environments per sector (`game/systems/GameSoundEnvHandler`):
      `LinkSoundEnvToSector`, `LinkSoundEnvToAllSectors`, `SetDefaultSoundEnv`.
      EFX was switched off on Linux in the engine and crashed in the OpenAL
      wrapper (a source's filter was never made); both fixed, and
      `UseEnvironmentalAudio` in settings.cfg is on unless set off. Seen in
      the log on level 4; **to play-test** by ear
- [x] The gas mask (item type, the effect with its picture, breathing and
      the world heard muffled, `GetPlayerHasGasMask`) and the Collectable
      item (counted, saved). **To play-test**: not seen yet
- [x] Swimming (`cPlayerMoveState_Swim`, the liquid area, under-water
      sounds, air running out and drowning, `cPlayer::SetIsUnderWater`).
      Tried headless in level 5's sewer water: sinking, the air, rising with
      jump held. **To play-test** in open water (level 5, Requiem 2, 4, 9)
- [x] Effects: the dream filter, `StartImageFlash`, black subtitle strips
      and black message text, the flash starting from nothing. Seen in
      headless frames
- [x] `SaveEffectOnArea` (Requiem: the save effect at an area, no save;
      called by no script of either game)
- [x] The endings: `StartEnding1`, `StartEnding2` (`game/ui/Ending`): mails
      typed on a black screen; ending 1 goes back to the map, ending 2 ends
      in "message sent" and the credits. Ending 2 seen in headless frames;
      **to play-test** both (level 22, Requiem level 0), and whether the
      credits' music and text are Black Plague's
- [x] Black Plague's menu (`game/ui/MainMenu`): New Game asks once whether
      to play the tutorial (`level00_tutorial.dae`, `Game/AskTutorial` in
      settings.cfg), after that a Tutorial button of its own; EFX on the
      Sound page; Refractions (on by default in the two later games) on the
      Lighting page; the Rotate key. Seen headless with injected clicks:
      the question, the tutorial map starting, the Tutorial button, the
      options. Refractions were never saved (`GetBool` for `SetBool` in
      `cInit::Exit`, in Overture's code too); fixed
- [x] Requiem's menu: New Game only asks "Sure you want to start a new
      game?" (no difficulty, no intro), "Quick Saves" for the auto saves,
      Quicksave and Quickload keys (F4, F5: the save effect and the newest
      auto or spot save, `AllowQuickSave` from its default settings), no
      Difficulty option. Seen headless
- [x] Holding things (`cPlayerState_Grab`, Black Plague's version): the
      Rotate key (C) turns the object with the mouse about the camera's
      axes; objects not held at a point are turned through their inertia;
      running while holding, Holster puts out a burning flare. Checked
      only; **to play-test**
- [x] The credits: Black Plague's music and a statistics page after the
      text (completion time, save artefacts, notes, collectables; the totals
      from game.cfg), then for all artefacts the secret message and code.
      Requiem: its text, music, time and collectables. The statistics are
      counted and saved (`cPlayer::mlStat_*`). Black Plague's statistics
      seen headless; **to play-test** after a play-through
  - [ ] Requiem's secret after all its artefacts (a message typed out with
        sounds, `cCredits::UpdateText`, texts `9_100`, `9_10x`, `10_aaa`,
        code `ikj765`); the credits end after the statistics for now
  - [x] Requiem's map change without load text or picture: a black
        screen without "Loading" (`DrawLoadingScreen` got a switch for the
        text; Requiem's check for a picture called "black" can never be
        true, so this is the only case it adds)
- [x] Ten auto saves kept in the two later games (Overture: five); the
      newest auto or spot save loaded by one function
      (`cSaveHandler::AutoLoadLatestAutoOrSpot`) as in Requiem
- [x] The rest of the 384 functions of Overture's classes that changed size
      in Black Plague (player, saves, doors, radio, inventory): go through
      them class by class. Most menu widgets only shrank with the compiler.
      The list, by class: `build/porting/step6/changed.txt`
  - [x] `iGameEntity`: the move and the mesh's place saved; switched off,
        it is taken out of the player's hands; particle systems already
        gone are not killed again. The rest only changed with the compiler
  - [x] `cSaveHandler`: a map's sectors (ambient colour, which portals are
        open, as `SetSectorProperties` and `SetSectorPortalActive` leave
        them) and its sound environments are kept, both in save games and
        when the player leaves a map and comes back (they were lost on
        coming back before). Tried headless on level 4
  - [x] `cMapHandler`: entities found by name whatever the case; one
        replaced by `ReplaceEntity` is switched off at once and destroyed
        after all entities have been updated. `ReplaceEntity` without a
        body name crashed (Overture's code; Black Plague fixed it): fixed
        for all three games. Tried headless on level 2's thermostat
  - [x] The player: short of air under water a picture pulses on the
        screen and the player gasps at each peak, faster below a quarter
        and a tenth of the air (`player_lowoxygen.bmp`; it was a timer
        before); out of the water at a reset; "Use … on myself" over the
        health in the inventory with an item in hand; hidden in the dark,
        an enemy close by only frightens the player more, it no longer
        drives him out of hiding and shows him to the enemies. Tried
        headless: the pulse and gasps (under water forced), not the rest
  - [x] Enemies: the spider's attack and the worm within 1.5 m of the
        player run the attack callback (`SpiderAttack01`, `VaporizeWorm`
        in Black Plague's scripts; they never ran before). Inventory: an
        item added to a stack asks its type first, so a second collectable
        is counted for the statistics. Checked only
  - [x] Batteries (Black Plague): kept in the inventory, put in the
        flashlight by their action, by dropping them on the flashlight or
        on the meter (50 power, `player_load_battery`); the meter shows
        only once the flashlight is found and reads "Flashlight"; item
        types may combine items themselves (`OnCombine`), then no "cannot
        combine" message. **To play-test**
  - [x] Smaller: drowning sound, off the ladder out of the water, a jump
        from a ladder is a jump, lamps' `OffColor` and their off material
        after the fade, no lit-change callback when a save is loaded, a
        broken object's sound and particles from its middle, the attack
        volume keeps a shape's offset, item text hidden only while radio
        text shows, pushing keeps the player at his place beside the body
        (and lets go at 1.4 times the reach). Checked only
  - [x] Seen to be done already or only compiler changes: links, swing
        doors, the effect handler, the melee weapon (haptics gone),
        `cPlayer::Damage`, the dog's and spider's hunt speeds (haptics)
  - [x] Left from the screening: the load text's background is the
        link's loading picture, or `loadscreen_default.jpg` (Overture keeps
        `other_load_text_back.jpg`; seen headless with level 3's corridor
        picture); an item of an unknown type is an error, not a crash
        (`cInventory::AddItemFromFile`, all games). `cPlayerLookAt::Update`
        was done with the zoom; `cPlayer::OnPostSceneDraw` and the menu
        widgets changed only with the compiler and the haptics
- [x] Save games of the new entities. Tried headless: a lever stuck by
      script stays stuck at its state (level 4), a wheel stuck at its end
      keeps its angle (level 10's valve, 450°). As in the original,
      `SetWheelStuck` turns the angle to the end but leaves the state
      read by `GetWheelState` as it was. **To play-test**: loaded and played on

## Step 5: playing through

- [ ] Black Plague from start to end
- [ ] Requiem from start to end (14 more functions in its binary than Black
      Plague's; `StartRecordDump`)
- [ ] Physics next to the original where levels depend on it (the same
      Newton 1.53 → 2 differences as Overture's Milestone 1)

## Later

- [ ] The shoot-'em-up (`cShooter`, started by `StartSchmup`): the largest
      single piece, and an extra
- [ ] The Lighting menu page and the other new options, checked on the two
      games' maps
