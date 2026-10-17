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
		const GLint vSwizzle[4] = {GL_RED, alChannels <= 2 ? GL_RED : GL_GREEN,
			alChannels <= 2 ? GL_RED : GL_BLUE, alChannels == 2 ? GL_GREEN : (alChannels == 1 ? GL_ONE : GL_ALPHA)};
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

		mfTimeCount =0;

		mfTimeDir = 1;

		mlBpp = 0;

	}

	cSDLTexture::~cSDLTexture()
	{
		mpGfxSDL->ForgetTexture(this);

		glDeleteTextures(1, &mlTextureHandle);
	}


	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PUBLIC METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	bool cSDLTexture::CreateFromBitmap(iBitmap2D* pBmp)
	{

		return CreateFromBitmapToHandle(pBmp,0);
	}

	//-----------------------------------------------------------------------

	bool cSDLTexture::CreateAnimFromBitmapVec(tBitmap2DVec *avBitmaps)
	{
		if(avBitmaps->empty() || mTarget != eTextureTarget_2D || mType == eTextureType_RenderTarget) return false;
		GLint maxLayers = 0;
		glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &maxLayers);
		if(avBitmaps->size() > (size_t)maxLayers) return false;
		auto* first = static_cast<cSDLBitmap2D*>((*avBitmaps)[0]);
		for(auto* bitmap : *avBitmaps) {
			auto* frame = static_cast<cSDLBitmap2D*>(bitmap);
			if(frame->GetWidth() != first->GetWidth() || frame->GetHeight() != first->GetHeight() ||
				frame->GetSurface()->format != first->GetSurface()->format) {
				Error("Animated texture '%s' has incompatible frame dimensions or format\n", msName.c_str());
				return false;
			}
		}
		mlFrameCount = (int)avBitmaps->size();
		mTarget = eTextureTarget_2DArray;
		for(int i=0; i<mlFrameCount; ++i)
			if(!CreateFromBitmapToHandle((*avBitmaps)[i], i)) return false;

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

		cSDLBitmap2D* first = static_cast<cSDLBitmap2D*>((*avBitmaps)[0]);
		int channels; GLenum format;
		GetSettings(first, channels, format);
		for(int i=0; i<6; ++i) {
			auto* face = static_cast<cSDLBitmap2D*>((*avBitmaps)[i]);
			if(face->GetWidth() != first->GetWidth() || face->GetHeight() != first->GetHeight() ||
				face->GetWidth() != face->GetHeight() || face->GetSurface()->format != first->GetSurface()->format)
				return false;
		}
		GLenum GLTarget = GL_TEXTURE_CUBE_MAP;
		if(!EnsureStorage(GLTarget, InternalFormat(channels), first->GetWidth(), first->GetHeight())) return false;

		//Create the cube map sides
		for(int i=0; i< 6; i++)
		{
			cSDLBitmap2D *pSrc = static_cast<cSDLBitmap2D*>((*avBitmaps)[i]);

			GLenum target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + i;

			int lChannels;
			GLenum format;
			GetSettings(pSrc, lChannels,format);

			glTexSubImage2D(target, 0, 0, 0, pSrc->GetWidth(), pSrc->GetHeight(),
				format, GL_UNSIGNED_BYTE, pSrc->GetSurface()->pixels);

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

		if(mType == eTextureType_RenderTarget)
		{
			// Resizing replaces storage; repeated creation at the same size reuses it.
			mlWidth = alWidth;
			mlHeight = alHeight;

			if((!cMath::IsPow2(mlHeight) || !cMath::IsPow2(mlWidth)) && !msName.empty())
			{
				Warning("Texture '%s' does not have a pow2 size!\n",msName.c_str());
			}

			GLenum GLTarget = mpGfxSDL->GetGLTextureTargetEnum(mTarget);

			if(!EnsureStorage(GLTarget, GL_RGBA8, mlWidth, mlHeight)) return false;
			cGpuProfiler::Label(GL_TEXTURE, mlTextureHandle, msName);
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
		GLenum GLTarget = TextureTargetToGL(mTarget);

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

		if(!EnsureStorage(GLTarget, InternalFormat(lChannels), mlWidth, mlHeight, mlDepth)) return false;
		if(mTarget == eTextureTarget_1D)
			glTexSubImage1D(GLTarget, 0, 0, mlWidth, format, GL_UNSIGNED_BYTE, apPixelData);
		else if(mTarget == eTextureTarget_2D)
			glTexSubImage2D(GLTarget, 0, 0, 0, mlWidth, mlHeight, format, GL_UNSIGNED_BYTE, apPixelData);
		else if(mTarget == eTextureTarget_3D)
			glTexSubImage3D(GLTarget, 0, 0, 0, 0, mlWidth, mlHeight, mlDepth, format, GL_UNSIGNED_BYTE, apPixelData);

		if(mbUseMipMaps && mTarget != eTextureTarget_3D)
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
		if(mTarget != eTextureTarget_2D) return;

		glBindTexture(GL_TEXTURE_2D, mlTextureHandle);
		glTexSubImage2D(TextureTargetToGL(mTarget),alLevel,avOffset.x,avOffset.y, avSize.x,avSize.y,
						ColorFormatToGL(aDataFormat),GL_UNSIGNED_BYTE,apPixelData);
		mbAlphaCoverageKept = false;
		if(HasMipMaps() && alLevel == 0) glGenerateMipmap(GL_TEXTURE_2D);
		mpGfxSDL->RestoreSetTexture();
	}

	//-----------------------------------------------------------------------

	void cSDLTexture::Update(float afTimeStep)
	{
		if(mlFrameCount > 1)
		{
			float fMax = (float)mlFrameCount;
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
		return mlFrameCount > 1;
	}

	void cSDLTexture::NextFrame()
	{
		mfTimeCount += mfTimeDir;

		if(mfTimeDir > 0)
		{
			float fMax = (float)mlFrameCount;
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
			float fMax = (float)mlFrameCount;
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
		if((mTarget != eTextureTarget_2D && mTarget != eTextureTarget_2DArray) || HasMipMaps()==false || mlBpp != 32 || mbFloat) return;
		mbAlphaCoverageKept = true;

		const int lRef = cMath::Clamp((int)ceilf(afAlphaRef * 255.0f), 1, 255);
		std::vector<unsigned char> vPixels;

		const GLenum target = TextureTargetToGL(mTarget);
		glBindTexture(target, mlTextureHandle);
		// Read each mip once for the whole array. Each layer keeps its own
		// base coverage; reading the full array once per layer would make
		// animation loading quadratic in the number of frames.
		std::vector<float> shares(mlFrameCount, -1.0f);
		for(int level=0; level<mlStorageLevels; ++level)
		{
			GLint width=0, height=0;
			glGetTexLevelParameteriv(target, level, GL_TEXTURE_WIDTH, &width);
			glGetTexLevelParameteriv(target, level, GL_TEXTURE_HEIGHT, &height);
			if(width <= 0 || height <= 0) break;
			const size_t texels = (size_t)width * height;
			vPixels.resize(texels * 4 * mlFrameCount);
			glGetTexImage(target, level, GL_RGBA, GL_UNSIGNED_BYTE, vPixels.data());
			for(int layer=0; layer<mlFrameCount; ++layer)
			{
				if(level > 0 && shares[layer] < 0) continue;
				unsigned char* pixels = vPixels.data() + layer*texels*4;
				size_t counts[256] = {};
				for(size_t i=0; i<texels; ++i) ++counts[pixels[i*4+3]];
				if(level == 0)
				{
					size_t pass=0;
					for(int alpha=lRef; alpha<256; ++alpha) pass += counts[alpha];
					if(pass > 0 && pass < texels) shares[layer] = (float)pass / (float)texels;
					continue;
				}
				const size_t wanted = (size_t)(shares[layer] * (float)texels + 0.5f);
				if(wanted == 0) continue;
				int alpha=255;
				for(size_t count=0; alpha>1; --alpha) {
					count += counts[alpha];
					if(count >= wanted) break;
				}
				const float scale = cMath::Min((float)lRef / (float)alpha, 16.0f);
				if(fabsf(scale - 1.0f) <= 0.01f) continue;
				for(size_t i=0; i<texels; ++i)
					pixels[i*4+3] = (unsigned char)cMath::Min((float)pixels[i*4+3] * scale + 0.5f, 255.0f);
				if(mTarget == eTextureTarget_2DArray)
					glTexSubImage3D(target, level, 0, 0, layer, width, height, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
				else glTexSubImage2D(target, level, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
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

	// 3D textures are made without them.
	bool cSDLTexture::HasMipMaps()
	{
		return mbUseMipMaps && mTarget != eTextureTarget_3D;
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
		return mlTextureHandle;
	}

	int cSDLTexture::GetAnimationFrame() const
	{
		// SetTimeCount is used by callers to synchronize animations. Guard
		// out-of-range input without changing the legacy loop/oscillate timing.
		return cMath::Clamp((int)mfTimeCount, 0, mlFrameCount-1);
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

		GLenum GLTarget = TextureTargetToGL(mTarget);

		cSDLBitmap2D *pBitmapSrc = static_cast<cSDLBitmap2D*>(pBmp);

		mlWidth = pBitmapSrc->GetWidth();
		mlHeight = pBitmapSrc->GetHeight();

		// Not of the textures made for drawing into, of the screen's size
		if((!cMath::IsPow2(mlHeight) || !cMath::IsPow2(mlWidth)) && !msName.empty())
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


		GLenum floatFormat = mlFloatChannels == 1 ? GL_R16F : (mlFloatChannels == 2 ? GL_RG16F : GL_RGBA16F);
		if(!EnsureStorage(GLTarget, mbFloat ? floatFormat : InternalFormat(lChannels),
			mlWidth, mlHeight, mTarget == eTextureTarget_2DArray ? mlFrameCount : 1)) {
			if(pNewSrc) hplDeleteArray(pNewSrc);
			return false;
		}
		if(mTarget == eTextureTarget_1D)
			glTexSubImage1D(GLTarget, 0, 0, mlWidth, format, GL_UNSIGNED_BYTE, pPixelSrc);
		else if(mTarget == eTextureTarget_2DArray)
			glTexSubImage3D(GLTarget, 0, 0, 0, alHandleIdx, mlWidth, mlHeight, 1, format, GL_UNSIGNED_BYTE, pPixelSrc);
		else
			glTexSubImage2D(GLTarget, 0, 0, 0, mlWidth, mlHeight, format, GL_UNSIGNED_BYTE, pPixelSrc);

		if(mbUseMipMaps && (mTarget != eTextureTarget_2DArray || alHandleIdx+1 == mlFrameCount))
			glGenerateMipmap(GLTarget);
		mbAlphaCoverageKept = false;

		PostCreation(GLTarget);

		if(mlSizeLevel>0 && pNewSrc)
		{
			hplDeleteArray(pNewSrc);
		}

		return true;
	}

	//-----------------------------------------------------------------------

	bool cSDLTexture::EnsureStorage(GLenum target, GLenum format, int width, int height, int depth)
	{
		if(width <= 0 || height <= 0 || depth <= 0) return false;
		int levels = 1;
		if(HasMipMaps()) for(int size = cMath::Max(width, height); size > 1; size /= 2) ++levels;
		const cVector3l size(width, height, depth);
		if(mlStorageTarget != target || mlStorageFormat != format || mvStorageSize != size || mlStorageLevels != levels)
		{
			GLuint replacement = 0;
			glGenTextures(1, &replacement);
			glBindTexture(target, replacement);
			if(target == GL_TEXTURE_1D) glTexStorage1D(target, levels, format, width);
			else if(target == GL_TEXTURE_3D || target == GL_TEXTURE_2D_ARRAY)
				glTexStorage3D(target, levels, format, width, height, depth);
			else glTexStorage2D(target, levels, format, width, height);
			GLint immutable = GL_FALSE;
			glGetTexParameteriv(target, GL_TEXTURE_IMMUTABLE_FORMAT, &immutable);
			if(!immutable) {
				Error("Couldn't allocate immutable storage for texture '%s' (%d x %d x %d)\n", msName.c_str(), width, height, depth);
				glDeleteTextures(1, &replacement);
				glBindTexture(target, 0);
				mpGfxSDL->RestoreSetTexture();
				return false;
			}
			const GLuint oldHandle = mlTextureHandle;
			if(mlTextureHandle) glDeleteTextures(1, &mlTextureHandle);
			mlTextureHandle = replacement;
			mlStorageTarget = target; mlStorageFormat = format;
			mvStorageSize = size; mlStorageLevels = levels;
			mbAlphaCoverageKept = false;
			mpGfxSDL->TextureStorageChanged(this, oldHandle);
		}
		else glBindTexture(target, mlTextureHandle);
		mlWidth = width; mlHeight = height; mlDepth = depth;
		return true;
	}


	void cSDLTexture::PostCreation(GLenum aGLTarget)
	{
		// What a unit without a sampler object reads it with; drawing goes
		// through the texture's sampler (GetSampler()).
		glTexParameteri(aGLTarget, GL_TEXTURE_MIN_FILTER, HasMipMaps() ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
		glTexParameteri(aGLTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(aGLTarget,GL_TEXTURE_WRAP_S,GL_REPEAT);
		glTexParameteri(aGLTarget,GL_TEXTURE_WRAP_T,GL_REPEAT);
		glTexParameteri(aGLTarget,GL_TEXTURE_WRAP_R,GL_REPEAT);
		SetSwizzle(aGLTarget, mlBpp / 8);


		mbContainsData = true;

		cGpuProfiler::Label(GL_TEXTURE, mlTextureHandle, msName);

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
		else if(alChannels==2) aFormat = GL_RG;
		else					aFormat = GL_RGBA;
	}

	//-----------------------------------------------------------------------

	GLenum cSDLTexture::GetGLWrap(eTextureWrap aMode)
	{
		switch(aMode)
		{
		// GL_CLAMP is gone from the core profile; drivers treated it as this
		case eTextureWrap_Clamp: return GL_CLAMP_TO_EDGE;
		case eTextureWrap_Repeat: return GL_REPEAT;
		case eTextureWrap_ClampToEdge: return GL_CLAMP_TO_EDGE;
		case eTextureWrap_ClampToBorder: return GL_CLAMP_TO_BORDER;
		}

		return GL_REPEAT;
	}


	//-----------------------------------------------------------------------

}
