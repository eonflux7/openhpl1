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
