// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "impl/PhysicsControllerNewton.h"

#include "physics/PhysicsWorld.h"
#include "physics/PhysicsBody.h"

#include "system/LowLevelSystem.h"

namespace hpl {

	cPhysicsControllerNewton::cPhysicsControllerNewton(const tString &asName, iPhysicsWorld *apWorld)
		: iPhysicsController(asName,apWorld)
	{
	}

	cPhysicsControllerNewton::~cPhysicsControllerNewton()
	{

	}
}
