// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_SDL_PIXELFORMAT_H
#define HPL_SDL_PIXELFORMAT_H

#include "graphics/PixelFormat.h"
#include <SDL3/SDL.h>

namespace hpl {

	class cSDLPixelFormat : public iPixelFormat
	{
	public:
		cSDLPixelFormat(): iPixelFormat("SDL")
		{
		}

		// 32 bit RGBA, R in the first byte on every platform.
		SDL_PixelFormat GetSDLPixelFormat32()
		{
			return SDL_PIXELFORMAT_RGBA32;
		}

		const SDL_PixelFormatDetails* GetSDLPixelFormatDetails32()
		{
			return SDL_GetPixelFormatDetails(SDL_PIXELFORMAT_RGBA32);
		}
	};

};
#endif // HPL_SDL_PIXELFORMAT_H
