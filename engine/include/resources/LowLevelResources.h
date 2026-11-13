// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_LOWLEVELRESOURCES_H
#define HPL_LOWLEVELRESOURCES_H

#include "system/SystemTypes.h"

namespace hpl {

	class iBitmap2D;
	class cMeshLoaderHandler;

	class iLowLevelResources
	{
	public:
		virtual ~iLowLevelResources(){}

		/**
		 * Returns a list of files in a dir
		 * \param &alstStrings list where the files are saved
		 * \param asDir Directory
		 * \param asMask Mask to be used, for example "*.*" to search for all kinds of files.
		 */
		virtual void FindFilesInDir(tWStringList &alstStrings, tWString asDir, tWString asMask)=0;

		virtual void GetSupportedImageFormats(tStringList &alstFormats)=0;
		virtual iBitmap2D* LoadBitmap2D(tString asFilePath, tString asType = "")=0;

		virtual void AddMeshLoaders(cMeshLoaderHandler* apHandler)=0;
	};
};
#endif // HPL_LOWLEVELRESOURCES_H
