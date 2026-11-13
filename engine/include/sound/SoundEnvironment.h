// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_SOUNDENVIRONMENT_H
#define HPL_SOUNDENVIRONMENT_H

#include "system/String.h"

namespace hpl
{
	class cSoundManager;

	class iSoundEnvironment
	{
	public:
		iSoundEnvironment ( ){}
		virtual ~iSoundEnvironment () {}

		virtual bool CreateFromFile(const tString &asFile){ return false;}

		tString& GetName() { return mstrName; }
		tString& GetFileName() { return msFileName; }
		void SetFileName(const tString & asFileName){ msFileName = asFileName; }

	protected:
		tString mstrName;
		tString msFileName;
	};

}


#endif	// HPL_SOUNDENVIRONMENT_H
