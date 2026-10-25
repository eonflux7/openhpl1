/*
 * Copyright (C) 2026 - OpenHPL1 contributors
 *
 * This file is part of HPL1 Engine.
 *
 * HPL1 Engine is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * HPL1 Engine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with HPL1 Engine.  If not, see <http://www.gnu.org/licenses/>.
 */
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
