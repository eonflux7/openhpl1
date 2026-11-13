// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_IMAGEMANAGER_H
#define HPL_IMAGEMANAGER_H

#include "resources/ResourceManager.h"
#include "math/MathTypes.h"

namespace hpl {

	class cResourceImage;
	class cFrameTexture;
	class cFrameBitmap;
	class iLowLevelGraphics;
	class iBitmap2D;

	typedef std::list<cFrameBitmap*> tFrameBitmapList;
	typedef tFrameBitmapList::iterator tFrameBitmapListIt;

	typedef std::map<int,cFrameTexture*> tFrameTextureMap;
	typedef tFrameTextureMap::iterator tFrameTextureMapIt;

	class cImageManager :public iResourceManager
	{
	public:
		cImageManager(cFileSearcher *apFileSearcher, iLowLevelGraphics *apLowLevelGraphics,
						iLowLevelResources *apLowLevelResources,iLowLevelSystem *apLowLevelSystem);
		~cImageManager() override;

		iResourceBase* Create(const tString& asName) override;

		void Destroy(iResourceBase* apResource) override;

		void Unload(iResourceBase* apResource) override;

		//Image specifc
		iResourceBase* CreateInFrame(const tString& asName, int alFrameHandle);
		cResourceImage* CreateImage(const tString& asName, int alFrameHandle=-1);
		/**
		 * Draws all updated content to textures. THis must be done before a loaded image can be used.
		 * Use this as unoften as possible.
		 * \return Number of bitmaps flushes
		 */
		int FlushAll();
		void DeleteAllBitmapFrames();

		cResourceImage* CreateFromBitmap(const tString &asName,iBitmap2D* apBmp, int alFrameHandle=-1);

		int CreateFrame(cVector2l avSize);
		void SetFrameLocked(int alHandle, bool abLocked);

		/**
		 * Directory with high-resolution replacements. "name@<N>x.png" (N is 2, 3,
		 * 4 or 8) there replaces the game's "name.*" and is drawn at the original's
		 * size, from its own mipmapped texture. It is N times the original's size,
		 * or N times that plus a margin around it (whole original pixels, the same
		 * on opposite sides) that is drawn outside the original's rectangle. An
		 * image there that the game does not have is drawn at 1/N of its size.
		 * An empty string turns replacements off. An image in asOverrideDir, if
		 * given, is used before one of the same name in asDir (for a game whose
		 * image of that name differs from another game's).
		 */
		void SetHighResDirectory(const tString &asDir, const tString &asOverrideDir=""){
			msHighResDir = asDir; msHighResOverrideDir = asOverrideDir; }
	private:
		cResourceImage *LoadHighRes(const tString &asName, const tString &asOriginalPath);

		iLowLevelGraphics *mpLowLevelGraphics;

		tFrameBitmapList mlstBitmapFrames;
		tFrameTextureMap m_mapTextureFrames;

		tStringList mlstFileFormats;
		cVector2l mvFrameSize;
		int mlFrameHandle;
		tString msHighResDir;
		tString msHighResOverrideDir;

		cResourceImage *FindImage(const tString &asName, tString &asFilePath);
		cResourceImage *AddToFrame(iBitmap2D *apBmp,int alFrameHandle);
		cFrameBitmap *CreateBitmapFrame(cVector2l avSize, bool abMipMaps=false);

	};

};
#endif // HPL_RESOURCEMANAGER_H
