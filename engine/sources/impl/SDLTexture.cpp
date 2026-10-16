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
#include "impl/SDLTexture.h"
#include "impl/SDLBitmap2D.h"
#include "impl/GpuProfiler.h"

#include "system/LowLevelSystem.h"
#include "math/Math.h"
#include "math.h"

namespace hpl {

	//////////////////////////////////////////////////////////////////////////
	// FORMATS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	// Textures of one and two channels are luminance and luminance-alpha:
	// stored as red and red-green, and read back as (r,r,r,1) and (r,r,r,g)
	// through the texture swizzle.
	static GLenum InternalFormat(int alChannels)
	{
		switch(alChannels)
		{
		case 1: return GL_R8;
		case 2: return GL_RG8;
		case 3: return GL_RGB8;
		}
		return GL_RGBA8;
	}

	static GLenum DataFormat(int alChannels)
	{
		switch(alChannels)
		{
		case 1: return GL_RED;
		case 2: return GL_RG;
		case 3: return GL_RGB;
		}
		return GL_RGBA;
	}

	static void SetSwizzle(GLenum aGLTarget, int alChannels)
	{
		if(alChannels > 2) return;
		const GLint vSwizzle[4] = {GL_RED, GL_RED, GL_RED, alChannels == 2 ? GL_GREEN : GL_ONE};
		glTexParameteriv(aGLTarget, GL_TEXTURE_SWIZZLE_RGBA, vSwizzle);
	}


	//////////////////////////////////////////////////////////////////////////
	// CONSTRUCTORS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	cSDLTexture::cSDLTexture(const tString &asName,iPixelFormat *apPxlFmt,iLowLevelGraphics* apLowLevelGraphics,
								eTextureType aType, bool abUseMipMaps, eTextureTarget aTarget,
								bool abCompress)
	: iTexture(asName,"OGL",apPxlFmt,apLowLevelGraphics,aType,abUseMipMaps, aTarget, abCompress)
	{
		mbContainsData = false;
		mbFloat = false;
		mlFloatChannels = 4;
		mlSampler = 0;
		mbSamplerMipMaps = false;
		mbAlphaCoverageKept = false;

		mpGfxSDL = static_cast<cLowLevelGraphicsSDL*>(mpLowLevelGraphics);

		mlTextureIndex = 0;
		mfTimeCount =0;

		mfTimeDir = 1;

		mlBpp = 0;

	}

	cSDLTexture::~cSDLTexture()
	{
		mpGfxSDL->ForgetTexture(this);

		for(size_t i=0; i<mvTextureHandles.size(); ++i)
		{
			glDeleteTextures(1,(GLuint *)&mvTextureHandles[i]);
		}
	}


	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PUBLIC METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	bool cSDLTexture::CreateFromBitmap(iBitmap2D* pBmp)
	{
		//Generate handles
		if(mvTextureHandles.empty())
		{
			mvTextureHandles.resize(1);
			glGenTextures(1,(GLuint *)&mvTextureHandles[0]);
		}
		else
		{
		}

		return CreateFromBitmapToHandle(pBmp,0);
	}

	//-----------------------------------------------------------------------

	bool cSDLTexture::CreateAnimFromBitmapVec(tBitmap2DVec *avBitmaps)
	{
		mvTextureHandles.resize(avBitmaps->size());

		for(size_t i=0; i< mvTextureHandles.size(); ++i)
		{
			glGenTextures(1,(GLuint *)&mvTextureHandles[i]);
			if(CreateFromBitmapToHandle((*avBitmaps)[i],(int)i)==false)
			{
				return false;
			}
		}

		return true;
	}

	//-----------------------------------------------------------------------

	bool cSDLTexture::CreateCubeFromBitmapVec(tBitmap2DVec *avBitmaps)
	{
		if(mType == eTextureType_RenderTarget || mTarget != eTextureTarget_CubeMap)
		{
			return false;
		}

		if(avBitmaps->size()<6){
			Error("Only %d bitmaps supplied for creation of cube map, 6 needed.",avBitmaps->size());
			return false;
		}

		//Generate handles
		if(mvTextureHandles.empty())
		{
			mvTextureHandles.resize(1);
			glGenTextures(1,(GLuint *)&mvTextureHandles[0]);
		}
		else
		{
			glDeleteTextures(1,(GLuint *)&mvTextureHandles[0]);
			glGenTextures(1,(GLuint *)&mvTextureHandles[0]);
		}

		GLenum GLTarget = InitCreation(0);

		//Create the cube map sides
		for(int i=0; i< 6; i++)
		{
			cSDLBitmap2D *pSrc = static_cast<cSDLBitmap2D*>((*avBitmaps)[i]);

			GLenum target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + i;

			int lChannels;
			GLenum format;
			GetSettings(pSrc, lChannels,format);

			glTexImage2D(target, 0, InternalFormat(lChannels), pSrc->GetWidth(), pSrc->GetHeight(),
				0, format, GL_UNSIGNED_BYTE, pSrc->GetSurface()->pixels);

			mlWidth = pSrc->GetWidth();
			mlHeight = pSrc->GetHeight();
			mlBpp = lChannels * 8;

			if(!cMath::IsPow2(mlHeight) || !cMath::IsPow2(mlWidth))
			{
				Warning("Texture '%s' does not have a pow2 size!\n",msName.c_str());
			}
		}

		if(mbUseMipMaps) glGenerateMipmap(GLTarget);

		PostCreation(GLTarget);

		return true;
	}


	//-----------------------------------------------------------------------

	bool cSDLTexture::Create(unsigned int alWidth, unsigned int alHeight, cColor aCol)
	{
		//Generate handles
		mvTextureHandles.resize(1);
		glGenTextures(1,(GLuint *)&mvTextureHandles[0]);


		if(mType == eTextureType_RenderTarget)
		{
			// Render targets used to be pbuffers. For now this is a plain
			// texture; framebuffer objects will replace it.
			mlWidth = alWidth;
			mlHeight = alHeight;

			if((!cMath::IsPow2(mlHeight) || !cMath::IsPow2(mlWidth)) && mTarget != eTextureTarget_Rect)
			{
				Warning("Texture '%s' does not have a pow2 size!\n",msName.c_str());
			}

			GLenum GLTarget = mpGfxSDL->GetGLTextureTargetEnum(mTarget);

			glBindTexture(GLTarget, mvTextureHandles[0]);
			cGpuProfiler::Label(GL_TEXTURE, mvTextureHandles[0], msName);
			glTexImage2D(GLTarget, 0, GL_RGBA8, mlWidth, mlHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
			glTexParameteri(GLTarget, GL_TEXTURE_MIN_FILTER, HasMipMaps() ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
			glTexParameteri(GLTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GLTarget,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
			glTexParameteri(GLTarget,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
			mWrapS = mWrapT = eTextureWrap_ClampToEdge;
			SamplerChanged();


			mbContainsData = true;
		}
		else
		{
			return false;
		}

		return true;
	}

	//-----------------------------------------------------------------------

	bool cSDLTexture::CreateFromArray(unsigned char *apPixelData, int alChannels, const cVector3l &avSize)
	{
		if(mvTextureHandles.empty())
		{
			mvTextureHandles.resize(1);
			glGenTextures(1,(GLuint *)&mvTextureHandles[0]);
		}

		GLenum GLTarget = InitCreation(0);

		int lChannels = alChannels;
		GLenum format = DataFormat(lChannels);

		mlWidth = avSize.x;
		mlHeight = avSize.y;
		mlDepth = avSize.z;
		mlBpp = lChannels * 8;

		if(!cMath::IsPow2(mlHeight) || !cMath::IsPow2(mlWidth) || !cMath::IsPow2(mlDepth))
		{
			Warning("Texture '%s' does not have a pow2 size!\n",msName.c_str());
		}

		if(mTarget == eTextureTarget_1D)
		{
			glTexImage1D(GLTarget, 0, InternalFormat(lChannels), mlWidth,0,format,
							GL_UNSIGNED_BYTE, apPixelData);
		}
		else if(mTarget == eTextureTarget_2D)
		{
			glTexImage2D(GLTarget, 0, InternalFormat(lChannels), mlWidth, mlHeight,
						0, format, GL_UNSIGNED_BYTE, apPixelData);
		}
		else if(mTarget == eTextureTarget_3D)
		{
			glTexImage3D(GLTarget, 0, InternalFormat(lChannels), avSize.x, avSize.y,avSize.z,
				0, format, GL_UNSIGNED_BYTE, apPixelData);
		}

		if(mbUseMipMaps && mTarget != eTextureTarget_Rect && mTarget != eTextureTarget_3D)
		{
			glGenerateMipmap(GLTarget);
		}

		PostCreation(GLTarget);

		return true;
	}


	//-----------------------------------------------------------------------

	void cSDLTexture::SetPixels2D(	int alLevel, const cVector2l& avOffset, const cVector2l& avSize,
									eColorDataFormat aDataFormat, void *apPixelData)
	{
		if(mTarget != eTextureTarget_2D && mTarget != eTextureTarget_Rect) return;

		glTexSubImage2D(TextureTargetToGL(mTarget),alLevel,avOffset.x,avOffset.y, avSize.x,avSize.y,
						ColorFormatToGL(aDataFormat),GL_UNSIGNED_BYTE,apPixelData);
	}

	//-----------------------------------------------------------------------

	void cSDLTexture::Update(float afTimeStep)
	{
		if(mvTextureHandles.size() > 1)
		{
			float fMax = (float)(mvTextureHandles.size());
			mfTimeCount += afTimeStep * (1.0f/mfFrameTime) * mfTimeDir;

			if(mfTimeDir > 0)
			{
				if(mfTimeCount >= fMax)
				{
					if(mAnimMode == eTextureAnimMode_Loop)
					{
						mfTimeCount =0;
					}
					else
					{
						mfTimeCount = fMax - 1.0f;
						mfTimeDir = -1.0f;
					}
				}
			}
			else
			{
				if(mfTimeCount < 0)
				{
					mfTimeCount =1;
					mfTimeDir = 1.0f;
				}
			}
		}
	}

	//-----------------------------------------------------------------------

	bool cSDLTexture::HasAnimation()
	{
		return mvTextureHandles.size() > 1;
	}

	void cSDLTexture::NextFrame()
	{
		mfTimeCount += mfTimeDir;

		if(mfTimeDir > 0)
		{
			float fMax = (float)(mvTextureHandles.size());
			if(mfTimeCount >= fMax)
			{
				if(mAnimMode == eTextureAnimMode_Loop)
				{
					mfTimeCount =0;
				}
				else
				{
					mfTimeCount = fMax - 1.0f;
					mfTimeDir = -1.0f;
				}
			}
		}
		else
		{
			if(mfTimeCount < 0)
			{
				mfTimeCount =1;
				mfTimeDir = 1.0f;
			}
		}
	}

	void cSDLTexture::PrevFrame()
	{
		mfTimeCount -= mfTimeDir;

		if(mfTimeDir < 0)
		{
			float fMax = (float)(mvTextureHandles.size());
			if(mfTimeCount >= fMax)
			{
				if(mAnimMode == eTextureAnimMode_Loop)
				{
					mfTimeCount =0;
				}
				else
				{
					mfTimeCount = fMax - 1.0f;
					mfTimeDir = -1.0f;
				}
			}
		}
		else
		{
			if(mfTimeCount < 0)
			{
				mfTimeCount =1;
				mfTimeDir = 1.0f;
			}
		}
	}

	float cSDLTexture::GetT()
	{
		return cMath::Modulus(mfTimeCount,1.0f);
	}

	float cSDLTexture::GetTimeCount()
	{
		return mfTimeCount;
	}
	void cSDLTexture::SetTimeCount(float afX)
	{
		mfTimeCount = afX;
	}
	int cSDLTexture::GetCurrentLowlevelHandle()
	{
		return GetTextureHandle();
	}

	//-----------------------------------------------------------------------

	// Thin things lose alpha as they are averaged into smaller mipmaps, and
	// with it the share of their texels that pass the test. For each mipmap
	// the alpha is found that this share of its texels reaches, and all of
	// its alpha scaled so that this one becomes the reference.
	void cSDLTexture::KeepAlphaCoverage(float afAlphaRef)
	{
		if(mbAlphaCoverageKept || mbContainsData==false) return;
		if(mTarget != eTextureTarget_2D || HasMipMaps()==false || mlBpp != 32 || mbFloat) return;
		mbAlphaCoverageKept = true;

		const int lRef = cMath::Clamp((int)ceilf(afAlphaRef * 255.0f), 1, 255);
		std::vector<unsigned char> vPixels;

		for(unsigned int lHandle : mvTextureHandles)
		{
			glBindTexture(GL_TEXTURE_2D, lHandle);

			float fShare = 0;
			for(int lLevel=0; ; ++lLevel)
			{
				GLint lW = 0, lH = 0;
				glGetTexLevelParameteriv(GL_TEXTURE_2D, lLevel, GL_TEXTURE_WIDTH, &lW);
				glGetTexLevelParameteriv(GL_TEXTURE_2D, lLevel, GL_TEXTURE_HEIGHT, &lH);
				if(lW <= 0 || lH <= 0) break;

				const size_t lTexels = (size_t)lW * lH;
				vPixels.resize(lTexels * 4);
				glGetTexImage(GL_TEXTURE_2D, lLevel, GL_RGBA, GL_UNSIGNED_BYTE, vPixels.data());

				size_t vAlphaCount[256] = {};
				for(size_t i=0; i<lTexels; ++i) ++vAlphaCount[vPixels[i*4 + 3]];

				if(lLevel == 0)
				{
					size_t lPass = 0;
					for(int i=lRef; i<256; ++i) lPass += vAlphaCount[i];
					// All of it passes, or none: nothing to keep
					if(lPass == 0 || lPass == lTexels) break;
					fShare = (float)lPass / (float)lTexels;
					continue;
				}

				size_t lWanted = (size_t)(fShare * (float)lTexels + 0.5f);
				if(lWanted > 0)
				{
					int lAlpha = 255;
					for(size_t lCount = 0; lAlpha > 1; --lAlpha)
					{
						lCount += vAlphaCount[lAlpha];
						if(lCount >= lWanted) break;
					}

					float fScale = cMath::Min((float)lRef / (float)lAlpha, 16.0f);
					if(fabsf(fScale - 1.0f) > 0.01f)
					{
						for(size_t i=0; i<lTexels; ++i)
							vPixels[i*4 + 3] = (unsigned char)cMath::Min((float)vPixels[i*4 + 3] * fScale + 0.5f, 255.0f);
						glTexSubImage2D(GL_TEXTURE_2D, lLevel, 0, 0, lW, lH, GL_RGBA, GL_UNSIGNED_BYTE, vPixels.data());
					}
				}

				if(lW == 1 && lH == 1) break;
			}
		}

		//The texture set on this unit has been replaced.
		mpGfxSDL->RestoreSetTexture();
	}

	//-----------------------------------------------------------------------

	// How the texture is read (filter, wrap) is not kept in the texture
	// but in a sampler object, shared with every texture that is read the
	// same way.
	GLuint cSDLTexture::GetSampler()
	{
		bool bMipMaps = HasMipMaps();
		if(mlSampler == 0 || mbSamplerMipMaps != bMipMaps)
		{
			mlSampler = mpGfxSDL->GetSampler(bMipMaps, GetGLWrap(mWrapS), GetGLWrap(mWrapT), GetGLWrap(mWrapR));
			mbSamplerMipMaps = bMipMaps;
		}
		return mlSampler;
	}

	void cSDLTexture::SamplerChanged()
	{
		mlSampler = 0;
		mpGfxSDL->TextureSamplerChanged(this);
	}

	// Rectangle and 3D textures are made without them.
	bool cSDLTexture::HasMipMaps()
	{
		return mbUseMipMaps && mTarget != eTextureTarget_Rect && mTarget != eTextureTarget_3D;
	}

	//-----------------------------------------------------------------------

	void cSDLTexture::SetWrapS(eTextureWrap aMode)
	{
		if(mWrapS == aMode) return;
		mWrapS = aMode;
		SamplerChanged();
	}

	//-----------------------------------------------------------------------

	void cSDLTexture::SetWrapT(eTextureWrap aMode)
	{
		if(mWrapT == aMode) return;
		mWrapT = aMode;
		SamplerChanged();
	}

	//-----------------------------------------------------------------------

	void cSDLTexture::SetWrapR(eTextureWrap aMode)
	{
		if(mWrapR == aMode) return;
		mWrapR = aMode;
		SamplerChanged();
	}

	//-----------------------------------------------------------------------

	unsigned int cSDLTexture::GetTextureHandle()
	{
		if(mvTextureHandles.size() > 1)
		{
			int lFrame = (int) mfTimeCount;
			return mvTextureHandles[lFrame];
		}
		else
		{
			return mvTextureHandles[0];
		}
	}

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PRIVATE METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	bool cSDLTexture::CreateFromBitmapToHandle(iBitmap2D* pBmp, int alHandleIdx)
	{
		if(mType == eTextureType_RenderTarget)
		{
			Error("Rendertarget cannot be created from bitmap\n");
			return false;
		}

		//For some reason checking for ARB texture is not working on radeon cards.

		GLenum GLTarget =InitCreation(alHandleIdx);

		cSDLBitmap2D *pBitmapSrc = static_cast<cSDLBitmap2D*>(pBmp);

		mlWidth = pBitmapSrc->GetWidth();
		mlHeight = pBitmapSrc->GetHeight();

		if((!cMath::IsPow2(mlHeight) || !cMath::IsPow2(mlWidth)) && mTarget != eTextureTarget_Rect)
		{
			Warning("Texture '%s' does not have a pow2 size!\n",msName.c_str());
		}

		int lChannels =0;
		GLenum format =0;
		GetSettings(pBitmapSrc,lChannels,format);

		mlBpp = lChannels * 8;

		unsigned char *pPixelSrc = (unsigned char*)pBitmapSrc->GetSurface()->pixels;

		unsigned char *pNewSrc = nullptr;
		if(mlSizeLevel>0 && (int)mlWidth > mvMinLevelSize.x*2)
		{

			int lOldW = mlWidth;

			int lSizeDiv = (int)pow((float)2,(int)mlSizeLevel);

			mlWidth /= lSizeDiv;
			mlHeight /= lSizeDiv;

			while(mlWidth < (unsigned int)mvMinLevelSize.x)
			{
				mlWidth*=2;
				mlHeight*=2;
				lSizeDiv/=2;
			}


			pNewSrc = hplNewArray( unsigned char, lChannels * mlWidth * mlHeight);

			// Each new texel is the average of the old ones it covers
			const int lBlock = lSizeDiv * lSizeDiv;
			unsigned char *pNewPixel = pNewSrc;
			for(unsigned int y=0; y<mlHeight; ++y)
			for(unsigned int x=0; x<mlWidth; ++x)
			for(int c=0; c<lChannels; ++c)
			{
				const unsigned char *pOldPixel = pPixelSrc + ((size_t)y*lSizeDiv*lOldW + (size_t)x*lSizeDiv)*lChannels + c;
				int lSum = 0;
				for(int by=0; by<lSizeDiv; ++by)
				for(int bx=0; bx<lSizeDiv; ++bx)
					lSum += pOldPixel[((size_t)by*lOldW + bx)*lChannels];
				*pNewPixel++ = (unsigned char)((lSum + lBlock/2) / lBlock);
			}

			pPixelSrc = pNewSrc;
		}


		//Clear error flags
		while(glGetError()!=GL_NO_ERROR);

		if(mTarget == eTextureTarget_1D)
			glTexImage1D(GLTarget, 0, InternalFormat(lChannels), mlWidth,0,format,
			GL_UNSIGNED_BYTE, pPixelSrc);
		else
		{
			GLenum lFloatFormat = mlFloatChannels == 1 ? GL_R16F : (mlFloatChannels == 2 ? GL_RG16F : GL_RGBA16F);
			glTexImage2D(GLTarget, 0, mbFloat ? lFloatFormat : InternalFormat(lChannels), mlWidth, mlHeight,
			0, format, GL_UNSIGNED_BYTE, pPixelSrc);
		}

		if(glGetError()!=GL_NO_ERROR) return false;

		if(mbUseMipMaps && mTarget != eTextureTarget_Rect)
		{
			glGenerateMipmap(GLTarget);
		}

		PostCreation(GLTarget);

		if(mlSizeLevel>0 && pNewSrc)
		{
			hplDeleteArray(pNewSrc);
		}

		return true;
	}

	//-----------------------------------------------------------------------

	GLenum cSDLTexture::InitCreation(int alHandleIdx)
	{
		GLenum GLTarget = mpGfxSDL->GetGLTextureTargetEnum(mTarget);

		glBindTexture(GLTarget, mvTextureHandles[alHandleIdx]);

		return GLTarget;
	}

	//-----------------------------------------------------------------------

	void cSDLTexture::PostCreation(GLenum aGLTarget)
	{
		// What a unit without a sampler object reads it with; drawing goes
		// through the texture's sampler (GetSampler()).
		glTexParameteri(aGLTarget, GL_TEXTURE_MIN_FILTER, HasMipMaps() ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
		glTexParameteri(aGLTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		// Rectangle textures cannot repeat
		GLint lWrap = mTarget == eTextureTarget_Rect ? GL_CLAMP_TO_EDGE : GL_REPEAT;
		glTexParameteri(aGLTarget,GL_TEXTURE_WRAP_S,lWrap);
		glTexParameteri(aGLTarget,GL_TEXTURE_WRAP_T,lWrap);
		glTexParameteri(aGLTarget,GL_TEXTURE_WRAP_R,lWrap);
		SetSwizzle(aGLTarget, mlBpp / 8);


		mbContainsData = true;

		// Animation frames are created one at a time; label those that exist.
		for(unsigned int lHandle : mvTextureHandles)
			if(glIsTexture(lHandle)) cGpuProfiler::Label(GL_TEXTURE, lHandle, msName);

		//Creating the texture has replaced the one set on this unit.
		mpGfxSDL->RestoreSetTexture();
	}

	//-----------------------------------------------------------------------

	void cSDLTexture::GetSettings(cSDLBitmap2D* apSrc, int &alChannels, GLenum &aFormat)
	{
		SDL_Surface *pSurface =  apSrc->GetSurface();
		alChannels = SDL_BYTESPERPIXEL(pSurface->format);

		// Images are normalised to byte-ordered RGB(A) at load time.
		if(alChannels==4)		aFormat = GL_RGBA;
		else if(alChannels==3)	aFormat = GL_RGB;
		else if(alChannels==1)	aFormat = GL_RED;
		else					aFormat = GL_RGBA;
	}

	//-----------------------------------------------------------------------

	GLenum cSDLTexture::GetGLWrap(eTextureWrap aMode)
	{
		switch(aMode)
		{
		// GL_CLAMP is gone from the core profile; drivers treated it as this
		case eTextureWrap_Clamp: return GL_CLAMP_TO_EDGE;
		case eTextureWrap_Repeat: return mTarget == eTextureTarget_Rect ? GL_CLAMP_TO_EDGE : GL_REPEAT;
		case eTextureWrap_ClampToEdge: return GL_CLAMP_TO_EDGE;
		case eTextureWrap_ClampToBorder: return GL_CLAMP_TO_BORDER;
		}

		return GL_REPEAT;
	}


	//-----------------------------------------------------------------------

}
