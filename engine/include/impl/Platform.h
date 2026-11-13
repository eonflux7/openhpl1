// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef PLATFORM_H_
#define PLATFORM_H_
#include "system/String.h"

namespace hpl {
	class Platform {
	public:
		static void FindFileInDir(tWStringList &alstStrings,tWString asDir, tWString asMask);
	};
}

#endif /*PLATFORM_H_*/
