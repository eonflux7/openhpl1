// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_PHYSICS_CONTROLLER_NEWTON_H
#define HPL_PHYSICS_CONTROLLER_NEWTON_H

#include "physics/PhysicsController.h"

namespace hpl {

	class iPhysicsWorld;

	class cPhysicsControllerNewton : public iPhysicsController
	{
	public:
		cPhysicsControllerNewton(const tString &asName, iPhysicsWorld *apWorld);
		~cPhysicsControllerNewton() override;
	};
};
#endif // HPL_PHYSICS_CONTROLLER_NEWTON_H
