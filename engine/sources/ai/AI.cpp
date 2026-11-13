// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "ai/AI.h"

#include "ai/AINodeGenerator.h"
#include "system/LowLevelSystem.h"

namespace hpl {

	cAI::cAI() : iUpdateable("HPL_AI")
	{
		mpAINodeGenerator = new cAINodeGenerator();
	}

	cAI::~cAI()
	{
		delete mpAINodeGenerator;
	}

	void cAI::Reset()
	{

	}

	void cAI::Update(float afTimeStep)
	{

	}

	void cAI::Init()
	{

	}
}
