// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/StdAfx.h"
#include "core/GameScripts.h"

#include "core/Init.h"

#include <map>

// Script functions of Black Plague and Requiem that are not ported yet.
//
// The names and argument types are those of the games' own binaries, so
// their scripts compile. Each does nothing but count its calls: the log
// says at exit which were called and how often. A function moves out of
// this file, to GameScripts.cpp, when it is written.

static std::map<tString, int> gmNotPortedCalls;

static void NotPorted(const char *apFunc)
{
	int &lCount = gmNotPortedCalls[apFunc];
	if(lCount==0) Log("Script function '%s' is not ported\n", apFunc);
	++lCount;
}

// BUTTONS

static void SetButtonOn(std::string asArg0, bool abArg1, bool abArg2)
{
	NotPorted("SetButtonOn");
}
SCRIPT_DEFINE_FUNC_3(void, SetButtonOn, string, bool, bool)

static void SetupButton(std::string asArg0, std::string asArg1, std::string asArg2)
{
	NotPorted("SetupButton");
}
SCRIPT_DEFINE_FUNC_3(void, SetupButton, string, string, string)

// OTHER

static void StartRecordDump(std::string asArg0)
{
	NotPorted("StartRecordDump");
}
SCRIPT_DEFINE_FUNC_1(void, StartRecordDump, string)

// The password of the game on the terminal of level 18; false: wrong password
static bool StartSchmup(std::string asArg0)
{
	NotPorted("StartSchmup");
	return false;
}
SCRIPT_DEFINE_FUNC_1(bool, StartSchmup, string)

void cGameScripts::InitNotPorted(iLowLevelSystem *apLowLevelSystem)
{
	// Buttons
	apLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetButtonOn));
	apLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetupButton));

	// Other
	apLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(StartRecordDump));
	apLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(StartSchmup));
}

void cGameScripts::LogNotPorted()
{
	if(gmNotPortedCalls.empty()) return;

	Log("Script functions called that are not ported:\n");
	for(auto &call : gmNotPortedCalls)
		Log("  %s: %d\n", call.first.c_str(), call.second);
}
