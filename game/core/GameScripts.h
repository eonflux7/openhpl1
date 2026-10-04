/*
 * Copyright (C) 2006-2010 - Frictional Games
 *
 * This file is part of Penumbra Overture.
 *
 * Penumbra Overture is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Penumbra Overture is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Penumbra Overture.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef GAME_SCRIPTS_H
#define GAME_SCRIPTS_H

#include "core/StdAfx.h"

#include "core/GameTypes.h"

using namespace hpl;

//------------------------------------
class cGameScripts
{
public:
	static void Init();

	// Black Plague and Requiem functions that only count their calls
	// (GameScriptsNotPorted.cpp), and the count written to the log
	static void InitNotPorted(iLowLevelSystem *apLowLevelSystem);
	static void LogNotPorted();
};

#endif // GAME_SCRIPTS_H
