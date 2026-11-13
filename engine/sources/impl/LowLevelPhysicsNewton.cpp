// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later

#include "impl/LowLevelPhysicsNewton.h"

#include "impl/PhysicsWorldNewton.h"
#include "system/LowLevelSystem.h"

namespace hpl {

	cLowLevelPhysicsNewton::cLowLevelPhysicsNewton()
	{
		// Sets up Newton's global memory allocator.
		NewtonInitGlobals();
	}

	cLowLevelPhysicsNewton::~cLowLevelPhysicsNewton()
	{
		NewtonDestroyGlobals();
	}

	iPhysicsWorld* cLowLevelPhysicsNewton::CreateWorld()
	{
		cPhysicsWorldNewton* pWorld = hplNew( cPhysicsWorldNewton, () );
		return pWorld;
	}
}
