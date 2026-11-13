// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_SPRING_H
#define HPL_SPRING_H

#include "math/MathTypes.h"

namespace hpl {

	template <class T>
	class cSpring
	{
	public:
		float k;
		float b;

		cSpring(){}

		cSpring(float afK, float afB)
		{
			k = afK; b = afB;
		}

		// Public

		T Output(T aError, T aV)
		{
			return aError * k - aV * b;
		}
	};

	typedef cSpring<float> cSpringf;
	typedef cSpring<cVector3f> cSpringVec3;
};
#endif // HPL_SPRING_H
