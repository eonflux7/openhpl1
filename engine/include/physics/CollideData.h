// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_COLLIDE_DATA_H
#define HPL_COLLIDE_DATA_H

#include "math/MathTypes.h"
#include "system/SystemTypes.h"

namespace hpl {

	class cCollidePoint
	{
	public:
		cVector3f mvPoint;
		cVector3f mvNormal;
		float mfDepth;
	};

	typedef std::vector<cCollidePoint> tCollidePointVec;

	class cCollideData
	{
	public:
		tCollidePointVec mvContactPoints;
		int mlNumOfPoints;

		void SetMaxSize(int alSize){
			mvContactPoints.resize(alSize);
		}
	};

};
#endif // HPL_COLLIDE_DATA_H
