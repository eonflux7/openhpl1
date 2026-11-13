// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
//#include <vld.h>

#include "core/Init.h"
#include "core/GameLauncher.h"
#include "core/SmokeTest.h"
#include "core/RefShots.h"
#include "core/RefView.h"

#include <SDL3/SDL.h>

#ifdef WIN32
	#include <windows.h>
#endif

int hplMain(const tString& asCommandLine)
{
	// Which game, and into its folder
	if(cGameLauncher::Run(asCommandLine)==false) return 0;

	cInit *pInit = hplNew( cInit, () );

	bool bRet = pInit->Init(asCommandLine);
	
	if(bRet==false){
		hplDelete( pInit->mpGame );
		CreateMessageBoxW(_W("Error!"),pInit->msErrorMessage.c_str());
		OpenBrowserWindow(_W("http://support.frictionalgames.com"));
		return 1;
	}

	pInit->Run();

	bool bAutoRunFailed = (pInit->mpSmokeTest && pInit->mpSmokeTest->Failed()) ||
		(pInit->mpRefShots && pInit->mpRefShots->Failed()) ||
		(pInit->mpRefView && pInit->mpRefView->EffectsCaptureFailed());

	pInit->Exit();

	hplDelete( pInit );
	
	cMemoryManager::LogResults();

	// Another game asked for in the menu: started in this process's place
	if(bAutoRunFailed==false) cInit::RunRequestedGame();

	return bAutoRunFailed ? 1 : 0;
}
