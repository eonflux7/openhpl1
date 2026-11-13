// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_PIXELFORMAT_H
#define HPL_PIXELFORMAT_H

#include "system/SystemTypes.h"

namespace hpl {

	class iPixelFormat
	{
	public:
		iPixelFormat(tString asType) : msType(asType){}
		~iPixelFormat(){}
	private:
		tString msType;
	};

};
#endif // HPL_PIXELFORMAT_H
