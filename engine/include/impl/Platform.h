// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef PLATFORM_H_
#define PLATFORM_H_
#include "system/String.h"
#include <stdio.h>
#if defined(WIN32)
#include <io.h>
#endif

namespace hpl {
	class Platform {
	public:
		static long FileLength(const tWString& asFileName);
		static long FileLength(FILE *pFile);
		static void FindFileInDir(tWStringList &alstStrings,tWString asDir, tWString asMask);
	};
}

#endif /*PLATFORM_H_*/
