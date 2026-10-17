/*
 * Copyright (C) 2006-2010 - Frictional Games
 *
 * This file is part of HPL1 Engine.
 *
 * HPL1 Engine is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * HPL1 Engine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with HPL1 Engine.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "impl/LowLevelResourcesSDL.h"
#include "impl/SDLBitmap2D.h"
#include "impl/MeshLoaderMSH.h"
#include "impl/MeshLoaderCollada.h"
#ifdef INCLUDE_THEORA
#endif
#include "impl/Platform.h"
#include "system/String.h"

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include "resources/MeshLoaderHandler.h"
#include "resources/VideoManager.h"

namespace hpl {

	//////////////////////////////////////////////////////////////////////////
	// CONSTRUCTORS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	cLowLevelResourcesSDL::cLowLevelResourcesSDL(cLowLevelGraphicsSDL *apLowLevelGraphics)
	{
		mvImageFormats[0] = "BMP";mvImageFormats[1] = "LBM";mvImageFormats[2] = "PCX";
		mvImageFormats[3] = "GIF";mvImageFormats[4] = "JPEG";mvImageFormats[5] = "PNG";
		mvImageFormats[6] = "JPG";
		mvImageFormats[7] = "TGA";mvImageFormats[8] = "TIFF";mvImageFormats[9] = "TIF";
		mvImageFormats[10] =  "";

		mpLowLevelGraphics = apLowLevelGraphics;
	}

	//-----------------------------------------------------------------------

	cLowLevelResourcesSDL::~cLowLevelResourcesSDL()
	{

	}

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PUBLIC METHOD
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

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

	//-----------------------------------------------------------------------

	void cLowLevelResourcesSDL::GetSupportedImageFormats(tStringList &alstFormats)
	{
		int lPos = 0;

		while(mvImageFormats[lPos]!="")
		{
			alstFormats.push_back(mvImageFormats[lPos]);
			lPos++;
		}
	}
	//-----------------------------------------------------------------------

	void cLowLevelResourcesSDL::AddMeshLoaders(cMeshLoaderHandler* apHandler)
	{
		//apHandler->AddLoader(hplNew( cMeshLoaderFBX,(mpLowLevelGraphics)));
		apHandler->AddLoader(hplNew( cMeshLoaderMSH,(mpLowLevelGraphics)));
		apHandler->AddLoader(hplNew( cMeshLoaderCollada,(mpLowLevelGraphics)));
	}

	//-----------------------------------------------------------------------

	void cLowLevelResourcesSDL::AddVideoLoaders(cVideoManager* apManager)
	{
		//No video formats: Overture has no videos, and the Theora loader
		//was compiled out upstream too.
	}

	//-----------------------------------------------------------------------

	//This is a windows implementation of this...I think.
	void cLowLevelResourcesSDL::FindFilesInDir(tWStringList &alstStrings,tWString asDir, tWString asMask)
	{
		Platform::FindFileInDir(alstStrings, asDir,asMask);
	}

	//-----------------------------------------------------------------------

}
