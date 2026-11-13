// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_COLOR_H
#define HPL_COLOR_H

#include <list>
#include <vector>

#include "system/SystemTypes.h"

namespace hpl {

	class cColor
	{
	public:
		float r,g,b,a;

		cColor(float afR, float afG, float afB, float afA);
		cColor(float afR, float afG, float afB);
		cColor();
		cColor(float afVal);
		cColor(float afVal, float afA);

		cColor operator*(float afVal) const;
		cColor operator/(float afVal) const;

		cColor operator+(const cColor &aCol) const;
		cColor operator-(const cColor &aCol) const;
		cColor operator*(const cColor &aCol) const;
		cColor operator/(const cColor &aCol) const;

		bool operator==(cColor aCol) const;

		tString ToString() const;

		tString ToFileString() const;

		void FromVec(float *apV);
	};

	typedef std::list<cColor> tColorList;
	typedef tColorList::iterator tColorListIt;

	typedef std::vector<cColor> tColorVec;
	typedef tColorVec::iterator tColorVecIt;

};
#endif // HPL_COLOR_H
