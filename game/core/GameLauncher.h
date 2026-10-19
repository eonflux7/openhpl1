/*
 * This file is part of OpenHPL1, licensed under the GNU GPL v3 or
 * later. See COPYING.
 */
#ifndef GAME_GAME_LAUNCHER_H
#define GAME_GAME_LAUNCHER_H

#include "core/StdAfx.h"
#include "core/Init.h"

using namespace hpl;

/**
 * Picks which of the three games runs and goes to its folder, before cInit
 * starts. In order:
 *  - --game=overture|blackplague|requiem on the command line, or OVERTURE_GAME
 *  - the data in the working directory (Black Plague's folder means Black
 *    Plague; Requiem, in the same folder, has to be asked for)
 *  - the game played last, unless the chooser is to open at every start
 *  - otherwise, or with --choose-game, or when the game asked for isn't
 *    found: a small chooser window, where a game's folder can also be picked.
 *
 * games.cfg, next to the games' personal folders, remembers each game's
 * folder ("overture=<folder>"), the last game ("last=<key>") and whether the
 * chooser opens at every start ("ask=1"). Folders are also looked for in
 * OVERTURE_GAME_DIR, the working directory, next to the executable, every
 * Steam library and (on Windows) GOG's installs. Automated runs
 * (OVERTURE_SMOKE etc.) never open the chooser.
 */
class cGameLauncher
{
public:
	/// False when the player quit or no game could be started.
	static bool Run(const tString& asCommandLine);

	/// The names OVERTURE_GAME and --game take, also the keys in games.cfg.
	static const char* GetGameKey(eGameId aId);

	/// The folder holding a game's data, "" if not found.
	static tString FindGameFolder(eGameId aId);
	/// A game started from this folder: it is found there and starts next time.
	static void RememberGame(eGameId aId, const tString& asFolder);

	static bool GetAskAtStart();
	static void SetAskAtStart(bool abX);

#if defined(WIN32)
	/// Starts the executable again as another game (for Change Game).
	static void StartAgain(eGameId aId);
#endif
};

#endif // GAME_GAME_LAUNCHER_H
