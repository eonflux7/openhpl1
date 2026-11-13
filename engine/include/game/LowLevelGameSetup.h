// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_LOWLEVELGAMESETUP_H
#define HPL_LOWLEVELGAMESETUP_H

#include "system/System.h"
#include "input/Input.h"
#include "graphics/Graphics.h"
#include "resources/Resources.h"
#include "scene/Scene.h"
#include "sound/Sound.h"
#include "physics/Physics.h"
#include "ai/AI.h"

namespace hpl {

	class iLowLevelGameSetup
	{
	public:
		virtual ~iLowLevelGameSetup(){}

		virtual cInput* CreateInput(cGraphics* apGraphics)=0;
		virtual cSystem* CreateSystem()=0;
		virtual cGraphics* CreateGraphics()=0;
		virtual cResources* CreateResources(cGraphics* apGraphics)=0;
		virtual cScene* CreateScene(cGraphics* apGraphics, cResources* apResources, cSound* apSound,
									cPhysics *apPhysics, cSystem *apSystem,cAI *apAI)=0;
		virtual cSound* CreateSound()=0;
		virtual cPhysics* CreatePhysics()=0;
		virtual cAI* CreateAI()=0;
	};
};
#endif // HPL_LOWLEVELGAMESETUP_H
