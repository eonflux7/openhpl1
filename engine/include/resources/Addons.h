// Copyright (C) 2026 - OpenHPL1 contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_ADDONS_H
#define HPL_ADDONS_H

#include "system/SystemTypes.h"

#include <vector>

namespace hpl
{

	class cResources;

	// An addon: a folder, or a .zip file, of files that replace the game's
	// files of the same name or add new ones. See docs/modding.md.
	class cAddon
	{
	public:
		tString msName; // folder or .zip name
		tString msTitle; // addon.cfg Name, else msName
		tString msSource; // the folder or .zip file
		tString msPath; // the folder used (a .zip is unpacked)
		int mlPriority;
		tStringVec mvGames; // empty: all games
	};

	typedef std::vector<cAddon> tAddonVec;

	class cAddons
	{
	public:
		/**
		 * Finds the addons in the given folders: each subfolder and .zip file
		 * is an addon, and so is a given folder with an addon.cfg in it.
		 * .zip files are unpacked into asUnpackDir, once until they change.
		 * Addons for other games than asGame, disabled ones and ones not in
		 * asOnly (a comma separated list of names; "" for all) are left out.
		 * \return the addons, the one that is used first first.
		 */
		static tAddonVec Find(const tStringVec &avRoots, const tString &asGame,
			const tString &asUnpackDir, const tString &asOnly);

		/**
		 * Adds the folders of the addons, with all their subfolders, to the
		 * resources. They must be added before the game's folders.
		 */
		static void AddToResources(cResources *apResources, const tAddonVec &avAddons);

		/**
		 * Logs the game files the addons replace. Call after the game's
		 * folders have been added.
		 */
		static void LogReplacedFiles(cResources *apResources, const tAddonVec &avAddons);
	};

}; // namespace hpl
#endif // HPL_ADDONS_H
