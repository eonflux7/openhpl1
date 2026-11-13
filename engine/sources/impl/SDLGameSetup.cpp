// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "impl/SDLGameSetup.h"
#include "impl/KeyboardSDL.h"
#include "impl/MouseSDL.h"
#include "impl/LowLevelGraphicsSDL.h"
#include "impl/LowLevelResourcesSDL.h"
#include "impl/LowLevelSystemSDL.h"
#include "impl/LowLevelInputSDL.h"
#include "impl/LowLevelSoundOpenAL.h"
#include "impl/LowLevelPhysicsNewton.h"

#include <SDL3/SDL.h>

namespace hpl {

	cSDLGameSetup::cSDLGameSetup()
	{
		if (!SDL_Init( SDL_INIT_VIDEO )) {
			FatalError("Error Initializing Display: %s",SDL_GetError());
			exit(1);
		}

		mpLowLevelSystem = new cLowLevelSystemSDL();
		mpLowLevelGraphics = new cLowLevelGraphicsSDL();
		mpLowLevelInput = new cLowLevelInputSDL(mpLowLevelGraphics);
		mpLowLevelResources = new cLowLevelResourcesSDL((cLowLevelGraphicsSDL *)mpLowLevelGraphics);
		mpLowLevelSound	= new cLowLevelSoundOpenAL();
		mpLowLevelPhysics = new cLowLevelPhysicsNewton();
	}

	cSDLGameSetup::~cSDLGameSetup()
	{
		Log("- Deleting lowlevel stuff.\n");

		Log("  Physics\n");
		delete mpLowLevelPhysics;
		Log("  Sound\n");
		delete mpLowLevelSound;
		Log("  Input\n");
		delete mpLowLevelInput;
		Log("  Resources\n");
		delete mpLowLevelResources;
		Log("  System\n");
		delete mpLowLevelSystem;
		Log("  Graphics\n");
		delete mpLowLevelGraphics;

		SDL_Quit();
	}

	cScene* cSDLGameSetup::CreateScene(cGraphics* apGraphics, cResources *apResources, cSound* apSound,
										cPhysics *apPhysics, cSystem *apSystem,cAI *apAI)
	{
		cScene *pScene = new cScene(apGraphics,apResources, apSound,apPhysics, apSystem,apAI);
		return pScene;
	}

	/**
	 * \todo Lowlevelresource and resource both use lowlevel graphics. Can this be fixed??
	 * \param apGraphics
	 * \return
	 */
	cResources* cSDLGameSetup::CreateResources(cGraphics* apGraphics)
	{
		cResources *pResources = new cResources(mpLowLevelResources,mpLowLevelGraphics);
		return pResources;
	}

	cInput* cSDLGameSetup::CreateInput(cGraphics* apGraphics)
	{
		cInput *pInput = new cInput(mpLowLevelInput);
		return pInput;
	}

	cSystem* cSDLGameSetup::CreateSystem()
	{
		cSystem *pSystem = new cSystem(mpLowLevelSystem);
		return pSystem;
	}

	cGraphics* cSDLGameSetup::CreateGraphics()
	{
		cGraphics *pGraphics = new cGraphics(mpLowLevelGraphics,mpLowLevelResources);
		return pGraphics;
	}

	cSound* cSDLGameSetup::CreateSound()
	{
		cSound *pSound = new cSound(mpLowLevelSound);
		return pSound;
	}

	cPhysics* cSDLGameSetup::CreatePhysics()
	{
		cPhysics *pPhysics = new cPhysics(mpLowLevelPhysics);
		return pPhysics;
	}

	cAI* cSDLGameSetup::CreateAI()
	{
		cAI *pAI = new cAI();
		return pAI;
	}
}
