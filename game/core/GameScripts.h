// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GAME_SCRIPTS_H
#define GAME_SCRIPTS_H

#include "core/StdAfx.h"

#include "core/GameTypes.h"

using namespace hpl;

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
