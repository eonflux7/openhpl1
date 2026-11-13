// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_HAPTIC_SURFACE_H
#define HPL_HAPTIC_SURFACE_H

#include "system/SystemTypes.h"
#include "math/MathTypes.h"
#include "haptic/HapticTypes.h"

namespace hpl {

	class iHapticSurface
	{
	public:
		iHapticSurface(const tString& asName, eHapticSurfaceType aType) : mType(aType){}
		virtual ~iHapticSurface(){}

		eHapticSurfaceType GetType(){ return mType;}
		tString& GetName(){ return msName; }

	private:
		eHapticSurfaceType mType;
		tString msName;
	};

};
#endif // HPL_HAPTIC_SURFACE_H