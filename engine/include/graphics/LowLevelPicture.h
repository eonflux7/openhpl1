// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_LOWLEVEL_PICTURE_H
#define HPL_LOWLEVEL_PICTURE_H

#include "system/SystemTypes.h"
#include "system/String.h"

namespace hpl {

	class iLowLevelPicture
	{
	public:
		iLowLevelPicture(tString asType): msType(asType) {}
		virtual ~iLowLevelPicture(){}

		tString GetType(){return msType;}

		unsigned int GetHeight(){return  mlHeight;}
		unsigned int GetWidth(){return  mlWidth;}
		unsigned int GetDepth(){return  mlDepth;}

		unsigned int GetBpp(){return  mlBpp;}

		virtual bool HasAlpha()=0;

		void SetPath(tString asPath){msPath = asPath;}
		tString GetPath(){return msPath;}
		tString GetFileName(){return cString::GetFileName(msPath);}

	protected:
		unsigned int mlHeight;
		unsigned int mlWidth;
		unsigned int mlDepth;
		unsigned int mlBpp;

	private:
		tString msType;
		tString msPath;
	};

};
#endif // HPL_LOWLEVEL_PICTURE_H
