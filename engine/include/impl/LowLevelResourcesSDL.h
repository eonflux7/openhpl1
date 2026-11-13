// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_LOWLEVELRESOURCES_SDL_H
#define HPL_LOWLEVELRESOURCES_SDL_H

#include "resources/LowLevelResources.h"
#include "impl/LowLevelGraphicsSDL.h"
#include "system/SystemTypes.h"

namespace hpl {

	class cLowLevelResourcesSDL : public iLowLevelResources
	{
	public:
		cLowLevelResourcesSDL(cLowLevelGraphicsSDL *apLowLevelGraphics);
		~cLowLevelResourcesSDL() override;

		void FindFilesInDir(tWStringList &alstStrings, tWString asDir, tWString asMask) override;

		void GetSupportedImageFormats(tStringList &alstFormats) override;
		iBitmap2D* LoadBitmap2D(tString asFilePath, tString asType = "") override;

		void AddMeshLoaders(cMeshLoaderHandler* apHandler) override;

	private:
		tString mvImageFormats[30];
		cLowLevelGraphicsSDL *mpLowLevelGraphics;
	};
};
#endif // HPL_LOWLEVELRESOURCES_SDL_H
