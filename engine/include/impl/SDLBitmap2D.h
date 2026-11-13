// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_SDL_BITMAP2D_H
#define HPL_SDL_BITMAP2D_H

#include "graphics/Bitmap2D.h"
#include "impl/SDLPixelFormat.h"

#include <SDL3/SDL.h>

namespace hpl {

	class cSDLBitmap2D : public iBitmap2D
	{
	public:
		cSDLBitmap2D(iPixelFormat *apPxlFmt);
		cSDLBitmap2D(SDL_Surface* apSurface,iPixelFormat *apPxlFmt,const tString& asType);
		~cSDLBitmap2D() override;

		bool SaveToFile(const tString& asFile) override;


		void DrawToBitmap(iBitmap2D *apBmp, const cVector2l &avDestPos) override;
		bool HasAlpha() override;
		/**
		 * \todo Support 16 bit format aswell.
		 * \param avSize
		 * \param alBpp
		 * \return
		 */
		bool Create(cVector2l avSize, unsigned int alBpp) override;

		void FillRect(const cRect2l &aRect, const cColor &aColor) override;

		void* GetRawData() override;
		int GetNumChannels() override;

		//SDL Specific
		SDL_Surface* GetSurface();
		SDL_PixelFormat GetPixelFmt32(){ return mSDLPixelFmt32;}

		tString msType;
	private:
		SDL_Surface* mpSurface;
		SDL_PixelFormat mSDLPixelFmt32;
	};
};
#endif // HPL_SDL_BITMAP2D_H
