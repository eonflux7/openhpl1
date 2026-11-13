// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_SDL_GAMESETUP_H
#define HPL_SDL_GAMESETUP_H

#include "game/LowLevelGameSetup.h"
#include "graphics/LowLevelGraphics.h"
#include "input/LowLevelInput.h"
#include "physics/LowLevelPhysics.h"
#include "resources/LowLevelResources.h"
#include "sound/LowLevelSound.h"
#include "system/LowLevelSystem.h"

namespace hpl {

	class cSDLGameSetup : public iLowLevelGameSetup
	{
	public:
		cSDLGameSetup();
		~cSDLGameSetup() override;

		cInput* CreateInput(cGraphics* apGraphics) override;
		cSystem* CreateSystem() override;
		cGraphics* CreateGraphics() override;
		cResources* CreateResources(cGraphics* apGraphics) override;
		cScene* CreateScene(cGraphics* apGraphics, cResources* apResources, cSound* apSound,
							cPhysics *apPhysics, cSystem *apSystem,cAI *apAI) override;
		cSound* CreateSound() override;
		cPhysics* CreatePhysics() override;
		cAI* CreateAI() override;

	private:
		iLowLevelSystem *mpLowLevelSystem;
		iLowLevelGraphics *mpLowLevelGraphics;
		iLowLevelInput *mpLowLevelInput;
		iLowLevelResources *mpLowLevelResources;
		iLowLevelSound*	mpLowLevelSound;
		iLowLevelPhysics* mpLowLevelPhysics;
	};
};
#endif // HPL_LOWLEVELGAMESETUP_SDL_H
