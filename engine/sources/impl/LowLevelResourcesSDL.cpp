// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "impl/LowLevelResourcesSDL.h"
#include "impl/SDLBitmap2D.h"
#include "impl/MeshLoaderCollada.h"
#ifdef INCLUDE_THEORA
#endif
#include "impl/Platform.h"
#include "system/String.h"

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include "resources/MeshLoaderHandler.h"

namespace hpl {

	cLowLevelResourcesSDL::cLowLevelResourcesSDL(cLowLevelGraphicsSDL *apLowLevelGraphics)
	{
		mvImageFormats[0] = "BMP";mvImageFormats[1] = "LBM";mvImageFormats[2] = "PCX";
		mvImageFormats[3] = "GIF";mvImageFormats[4] = "JPEG";mvImageFormats[5] = "PNG";
		mvImageFormats[6] = "JPG";
		mvImageFormats[7] = "TGA";mvImageFormats[8] = "TIFF";mvImageFormats[9] = "TIF";
		mvImageFormats[10] =  "";

		mpLowLevelGraphics = apLowLevelGraphics;
	}

	cLowLevelResourcesSDL::~cLowLevelResourcesSDL()
	{

	}

	iBitmap2D* cLowLevelResourcesSDL::LoadBitmap2D(tString asFilePath, tString asType)
	{
		tString tType;
		if(asType != "")
			asFilePath = cString::SetFileExt(asFilePath,asType);

		tType = cString::GetFileExt(asFilePath);
		SDL_Surface* pSurface = IMG_Load(asFilePath.c_str());
		if(pSurface==nullptr){
			//Error handling stuff?
			return nullptr;
		}

		// Textures are uploaded based on channel count only, so bring every
		// image into byte-ordered RGB/RGBA. 8 bit palettised images stay as
		// they are and are uploaded as single channel (alpha) textures.
		if(pSurface->format != SDL_PIXELFORMAT_INDEX8)
		{
			SDL_PixelFormat targetFormat = SDL_ISPIXELFORMAT_ALPHA(pSurface->format) ?
												SDL_PIXELFORMAT_RGBA32 : SDL_PIXELFORMAT_RGB24;
			if(pSurface->format != targetFormat)
			{
				SDL_Surface* pConverted = SDL_ConvertSurface(pSurface, targetFormat);
				SDL_DestroySurface(pSurface);
				pSurface = pConverted;
				if(pSurface==nullptr) return nullptr;
			}
		}

		iBitmap2D* pBmp = mpLowLevelGraphics->CreateBitmap2DFromSurface(pSurface,
													cString::GetFileExt(asFilePath));
		pBmp->SetPath(asFilePath);

		return pBmp;
	}

	void cLowLevelResourcesSDL::GetSupportedImageFormats(tStringList &alstFormats)
	{
		int lPos = 0;

		while(mvImageFormats[lPos]!="")
		{
			alstFormats.push_back(mvImageFormats[lPos]);
			lPos++;
		}
	}

	void cLowLevelResourcesSDL::AddMeshLoaders(cMeshLoaderHandler* apHandler)
	{
		apHandler->AddLoader(new cMeshLoaderCollada(mpLowLevelGraphics));
	}

	//This is a windows implementation of this...I think.
	void cLowLevelResourcesSDL::FindFilesInDir(tWStringList &alstStrings,tWString asDir, tWString asMask)
	{
		Platform::FindFileInDir(alstStrings, asDir,asMask);
	}
}
