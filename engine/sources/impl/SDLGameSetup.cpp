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
#ifdef INCLUDE_HAPTIC
#include "impl/LowLevelHapticHaptX.h"
#endif
#include "haptic/Haptic.h"

#include <SDL3/SDL.h>

namespace hpl {

	cSDLGameSetup::cSDLGameSetup()
	{
		if (!SDL_Init( SDL_INIT_VIDEO )) {
			FatalError("Error Initializing Display: %s",SDL_GetError());
			exit(1);
		}

		mpLowLevelSystem = hplNew( cLowLevelSystemSDL, () );
		mpLowLevelGraphics = hplNew( cLowLevelGraphicsSDL,() );
		mpLowLevelInput = hplNew( cLowLevelInputSDL,(mpLowLevelGraphics) );
		mpLowLevelResources = hplNew( cLowLevelResourcesSDL,((cLowLevelGraphicsSDL *)mpLowLevelGraphics) );
			mpLowLevelSound	= hplNew( cLowLevelSoundOpenAL,() );
		mpLowLevelPhysics = hplNew( cLowLevelPhysicsNewton,() );

#ifdef INCLUDE_HAPTIC
		mpLowLevelHaptic = hplNew( cLowLevelHapticHaptX,() );
#else
		mpLowLevelHaptic = nullptr;
#endif

	}

	cSDLGameSetup::~cSDLGameSetup()
	{
		Log("- Deleting lowlevel stuff.\n");

		Log("  Physics\n");
		hplDelete(mpLowLevelPhysics);
		Log("  Sound\n");
		hplDelete(mpLowLevelSound);
		Log("  Input\n");
		hplDelete(mpLowLevelInput);
		Log("  Resources\n");
		hplDelete(mpLowLevelResources);
		Log("  System\n");
		hplDelete(mpLowLevelSystem);
		Log("  Graphics\n");
		hplDelete(mpLowLevelGraphics);
		Log("  Haptic\n");
		if(mpLowLevelHaptic) hplDelete(mpLowLevelHaptic);

		SDL_Quit();
	}

	cScene* cSDLGameSetup::CreateScene(cGraphics* apGraphics, cResources *apResources, cSound* apSound,
										cPhysics *apPhysics, cSystem *apSystem,cAI *apAI,
										cHaptic *apHaptic)
	{
		cScene *pScene = hplNew( cScene, (apGraphics,apResources, apSound,apPhysics, apSystem,apAI,apHaptic) );
		return pScene;
	}

	/**
	 * \todo Lowlevelresource and resource both use lowlevel graphics. Can this be fixed??
	 * \param apGraphics
	 * \return
	 */
	cResources* cSDLGameSetup::CreateResources(cGraphics* apGraphics)
	{
		cResources *pResources = hplNew( cResources, (mpLowLevelResources,mpLowLevelGraphics) );
		return pResources;
	}

	cInput* cSDLGameSetup::CreateInput(cGraphics* apGraphics)
	{
		cInput *pInput = hplNew( cInput, (mpLowLevelInput) );
		return pInput;
	}

	cSystem* cSDLGameSetup::CreateSystem()
	{
		cSystem *pSystem = hplNew( cSystem, (mpLowLevelSystem) );
		return pSystem;
	}

	cGraphics* cSDLGameSetup::CreateGraphics()
	{
		cGraphics *pGraphics = hplNew( cGraphics, (mpLowLevelGraphics,mpLowLevelResources) );
		return pGraphics;
	}

	cSound* cSDLGameSetup::CreateSound()
	{
		cSound *pSound = hplNew( cSound, (mpLowLevelSound) );
		return pSound;
	}

	cPhysics* cSDLGameSetup::CreatePhysics()
	{
		cPhysics *pPhysics = hplNew( cPhysics, (mpLowLevelPhysics) );
		return pPhysics;
	}

	cAI* cSDLGameSetup::CreateAI()
	{
		cAI *pAI = hplNew( cAI,() );
		return pAI;
	}

	cHaptic* cSDLGameSetup::CreateHaptic()
	{
		cHaptic *pHaptic = hplNew( cHaptic, (mpLowLevelHaptic) );
		return pHaptic;
	}
}
