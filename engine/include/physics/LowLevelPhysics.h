// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_LOWLEVELPHYSICS_H
#define HPL_LOWLEVELPHYSICS_H

#include "system/SystemTypes.h"
#include "math/MathTypes.h"

namespace hpl {

	class iPhysicsWorld;


	class iLowLevelPhysics
	{
	public:
		virtual ~iLowLevelPhysics(){}

		virtual iPhysicsWorld* CreateWorld()=0;
	};
};
#endif // HPL_LOWLEVELPHYSICS_H
