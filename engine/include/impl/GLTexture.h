// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_GL_TEXTURE_H
#define HPL_GL_TEXTURE_H

#include "graphics/Texture.h"
#include "impl/LowLevelGraphicsSDL.h"
#include "impl/SDLBitmap2D.h"

#include <glad/gl.h>
#include <SDL3/SDL.h>


namespace hpl {

	class cGLTexture : public iTexture
	{
	public:
		cGLTexture(const tString &asName, iPixelFormat *apPxlFmt, iLowLevelGraphics *apLowLevelGraphics,
			eTextureType aType, bool abUseMipMaps, eTextureTarget aTarget,
			bool abCompress = false);
		~cGLTexture() override;

		bool CreateFromBitmap(iBitmap2D *apBitmap) override;

		bool CreateAnimFromBitmapVec(tBitmap2DVec *avBitmaps) override;

		bool CreateCubeFromBitmapVec(tBitmap2DVec *avBitmaps) override;
		bool Create(unsigned int alWidth, unsigned int alHeight, cColor aCol) override;

		bool CreateFromArray(unsigned char *apPixelData, int alChannels, const cVector3l &avSize) override;

		void SetPixels2D(	int alLevel, const cVector2l& avOffset, const cVector2l& avSize,
							eColorDataFormat aDataFormat, void *apPixelData) override;


		float GetGamma() override{return 0;}
		void SetGamma(float afGamma) override{}
		int GetHandle() override{return (int) mlTextureHandle;}

		void KeepAlphaCoverage(float afAlphaRef) override;

		void SetWrapS(eTextureWrap aMode) override;
		void SetWrapT(eTextureWrap aMode) override;
		void SetWrapR(eTextureWrap aMode) override;

		void Update(float afTimeStep) override;

		bool HasAnimation() override;
		void NextFrame() override;
		void PrevFrame() override;
		float GetT() override;
		float GetAnimationFramePosition() override;
		void SetAnimationFramePosition(float afFramePosition) override;
		int GetCurrentLowlevelHandle() override;

		// OpenGL handles and storage format.

		unsigned int GetTextureHandle();
		int GetAnimationFrame() const override;

		/// The sampler object the texture is read through, see
		/// cLowLevelGraphicsSDL::GetSampler().
		GLuint GetSampler();

		/// Stores 16-bit floating point channels instead of 8-bit ones.
		/// Set before the texture is created.
		void SetFloat(bool abX){ mbFloat = abX;}
		/// How many of them: 1, 2 or 4 (the default).
		void SetFloatChannels(int alX){ mlFloatChannels = alX;}

	private:
		bool UploadBitmap(iBitmap2D *apBitmap, int alLayer);

		void FinishUpload(GLenum aGLTarget);

		GLenum ToGLWrapMode(eTextureWrap aMode);
		bool HasMipMaps();
		void SamplerChanged();

		void GetBitmapFormat(cSDLBitmap2D *apSrc, int &alChannels, GLenum &aFormat);

		bool EnsureStorage(GLenum aTarget, GLenum aFormat, int alWidth, int alHeight, int alDepth=1);
		GLuint mlTextureHandle = 0;
		int mlFrameCount = 1;
		GLenum mlStorageTarget = 0, mlStorageFormat = 0;
		int mlStorageLevels = 0;
		cVector3l mvStorageSize = cVector3l(0);

		bool mbContainsData;
		GLuint mlSampler;
		bool mbSamplerMipMaps;
		bool mbAlphaCoverageKept;
		bool mbFloat;
		int mlFloatChannels;
		cLowLevelGraphicsSDL* mpGfxSDL;

		float mfAnimationFramePosition;
		float mfAnimationDirection;

	};

};
#endif // HPL_GL_TEXTURE_H
