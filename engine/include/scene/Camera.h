// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_CAMERA_H
#define HPL_CAMERA_H

#include "math/MathTypes.h"

namespace hpl {

	class iLowLevelGraphics;

	enum eCameraType
	{
		eCameraType_3D,
		eCameraType_LastEnum
	};

	class iCamera
	{
	public:
		virtual ~iCamera(){}

		virtual void SetModelViewMatrix(iLowLevelGraphics* apLowLevel)=0;
		virtual void SetProjectionMatrix(iLowLevelGraphics* apLowLevel)=0;
		virtual cVector3f GetEyePosition()=0;

		virtual eCameraType GetType()=0;
	};

};
#endif // HPL_CAMERA_H
