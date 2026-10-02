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
#ifndef HPL_SDL_TEXTURE_H
#define HPL_SDL_TEXTURE_H

#include "graphics/Texture.h"
#include "impl/LowLevelGraphicsSDL.h"
#include "impl/SDLBitmap2D.h"

#include <glad/gl.h>
#include <SDL3/SDL.h>


namespace hpl {

	class cSDLTexture : public iTexture
	{
	public:
		cSDLTexture(const tString &asName,iPixelFormat *apPxlFmt,iLowLevelGraphics* apLowLevelGraphics,
					eTextureType aType, bool abUseMipMaps, eTextureTarget aTarget,
					bool abCompress=false);
		~cSDLTexture() override;

		bool CreateFromBitmap(iBitmap2D* pBmp) override;

		bool CreateAnimFromBitmapVec(tBitmap2DVec *avBitmaps) override;

		bool CreateCubeFromBitmapVec(tBitmap2DVec *avBitmaps) override;
		bool Create(unsigned int alWidth, unsigned int alHeight, cColor aCol) override;

		bool CreateFromArray(unsigned char *apPixelData, int alChannels, const cVector3l &avSize) override;

		void SetPixels2D(	int alLevel, const cVector2l& avOffset, const cVector2l& avSize,
							eColorDataFormat aDataFormat, void *apPixelData) override;


		float GetGamma() override{return 0;}
		void SetGamma(float afGamma) override{}
		int GetHandle() override{return (int) mvTextureHandles[0];}

		void KeepAlphaCoverage(float afAlphaRef) override;

		void SetWrapS(eTextureWrap aMode) override;
		void SetWrapT(eTextureWrap aMode) override;
		void SetWrapR(eTextureWrap aMode) override;

		void Update(float afTimeStep) override;

		bool HasAnimation() override;
		void NextFrame() override;
		void PrevFrame() override;
		float GetT() override;
		float GetTimeCount() override;
		void SetTimeCount(float afX) override;
		int GetCurrentLowlevelHandle() override;

		/// SDL / OGL Specific ///////////

		unsigned int GetTextureHandle();

		/// The sampler object the texture is read through, see
		/// cLowLevelGraphicsSDL::GetSampler().
		GLuint GetSampler();

		/// Stores 16-bit floating point channels instead of 8-bit ones.
		/// Set before the texture is created.
		void SetFloat(bool abX){ mbFloat = abX;}
		/// How many of them: 1, 2 or 4 (the default).
		void SetFloatChannels(int alX){ mlFloatChannels = alX;}

	private:
		bool CreateFromBitmapToHandle(iBitmap2D* pBmp, int alHandleIdx);

		GLenum InitCreation(int alHandleIdx);
		void PostCreation(GLenum aGLTarget);

		GLenum GetGLWrap(eTextureWrap aMode);
		bool HasMipMaps();
		void SamplerChanged();

		void GetSettings(cSDLBitmap2D* apSrc, int &alChannels, GLenum &aFormat);

		tUIntVec mvTextureHandles;
		bool mbContainsData;
		GLuint mlSampler;
		bool mbSamplerMipMaps;
		bool mbAlphaCoverageKept;
		bool mbFloat;
		int mlFloatChannels;
		cLowLevelGraphicsSDL* mpGfxSDL;

		float mfTimeCount;
		int mlTextureIndex;
		float mfTimeDir;

	};

};
#endif // HPL_SDL_TEXTURE_H
