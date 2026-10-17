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
#include "core/StdAfx.h"
#include "core/GameScripts.h"

#include "core/Init.h"

#include <map>

//////////////////////////////////////////////////////////////////////////
// Script functions of Black Plague and Requiem that are not ported yet.
//
// The names and argument types are those of the games' own binaries, so
// their scripts compile. Each does nothing but count its calls: the log
// says at exit which were called and how often. A function moves out of
// this file, to GameScripts.cpp, when it is written.
//////////////////////////////////////////////////////////////////////////

static std::map<tString, int> gmNotPortedCalls;

static void NotPorted(const char *apFunc)
{
	int &lCount = gmNotPortedCalls[apFunc];
	if(lCount==0) Log("Script function '%s' is not ported\n", apFunc);
	++lCount;
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// BUTTONS

//-----------------------------------------------------------------------

static void __stdcall SetButtonOn(std::string asArg0, bool abArg1, bool abArg2)
{
	NotPorted("SetButtonOn");
}
SCRIPT_DEFINE_FUNC_3(void, SetButtonOn, string, bool, bool)

//-----------------------------------------------------------------------

static void __stdcall SetupButton(std::string asArg0, std::string asArg1, std::string asArg2)
{
	NotPorted("SetupButton");
}
SCRIPT_DEFINE_FUNC_3(void, SetupButton, string, string, string)

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// OTHER

//-----------------------------------------------------------------------

static void __stdcall StartRecordDump(std::string asArg0)
{
	NotPorted("StartRecordDump");
}
SCRIPT_DEFINE_FUNC_1(void, StartRecordDump, string)

//-----------------------------------------------------------------------

// The password of the game on the terminal of level 18; false: wrong password
static bool __stdcall StartSchmup(std::string asArg0)
{
	NotPorted("StartSchmup");
	return false;
}
SCRIPT_DEFINE_FUNC_1(bool, StartSchmup, string)

//-----------------------------------------------------------------------

void cGameScripts::InitNotPorted(iLowLevelSystem *apLowLevelSystem)
{
	// Buttons
	apLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetButtonOn));
	apLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetupButton));

	// Other
	apLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(StartRecordDump));
	apLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(StartSchmup));
}

//-----------------------------------------------------------------------

void cGameScripts::LogNotPorted()
{
	if(gmNotPortedCalls.empty()) return;

	Log("Script functions called that are not ported:\n");
	for(auto &call : gmNotPortedCalls)
		Log("  %s: %d\n", call.first.c_str(), call.second);
}

//-----------------------------------------------------------------------
