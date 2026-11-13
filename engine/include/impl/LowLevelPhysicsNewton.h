// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_LOWLEVELPHYSICS_NEWTON_H
#define HPL_LOWLEVELPHYSICS_NEWTON_H

#include "physics/LowLevelPhysics.h"
#include <Newton.h>

namespace hpl {

	class cLowLevelPhysicsNewton : public iLowLevelPhysics
	{
	public:
		cLowLevelPhysicsNewton();
		~cLowLevelPhysicsNewton() override;

		iPhysicsWorld* CreateWorld() override;
	};
};
#endif // HPL_LOWLEVELPHYSICS_NEWTON_H
