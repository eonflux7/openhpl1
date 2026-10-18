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
#include "impl/GLTexture.h"
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

	cGLTexture::cGLTexture(const tString &asName, iPixelFormat *apPxlFmt, iLowLevelGraphics *apLowLevelGraphics,
		eTextureType aType, bool abUseMipMaps, eTextureTarget aTarget,
		bool abCompress)
	: iTexture(asName, "OGL", apPxlFmt, apLowLevelGraphics, aType, abUseMipMaps, aTarget, abCompress)
	{
		mbContainsData = false;
		mbFloat = false;
		mlFloatChannels = 4;
		mlSampler = 0;
		mbSamplerMipMaps = false;
		mbAlphaCoverageKept = false;

		mpGfxSDL = static_cast<cLowLevelGraphicsSDL*>(mpLowLevelGraphics);

		mfAnimationFramePosition =0;

		mfAnimationDirection = 1;

		mlBpp = 0;

	}

	cGLTexture::~cGLTexture()
	{
		mpGfxSDL->ForgetTexture(this);

		glDeleteTextures(1, &mlTextureHandle);
	}


	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PUBLIC METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	bool cGLTexture::CreateFromBitmap(iBitmap2D *apBitmap)
	{
		return UploadBitmap(apBitmap, 0);
	}

	//-----------------------------------------------------------------------

	bool cGLTexture::CreateAnimFromBitmapVec(tBitmap2DVec *avBitmaps)
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
			if(!UploadBitmap((*avBitmaps)[i], i)) return false;

		return true;
	}

	//-----------------------------------------------------------------------

	bool cGLTexture::CreateCubeFromBitmapVec(tBitmap2DVec *avBitmaps)
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
		GetBitmapFormat(first, channels, format);
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
			GetBitmapFormat(pSrc, lChannels, format);

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

		FinishUpload(GLTarget);

		return true;
	}


	//-----------------------------------------------------------------------

	bool cGLTexture::Create(unsigned int alWidth, unsigned int alHeight, cColor aCol)
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

	bool cGLTexture::CreateFromArray(unsigned char *apPixelData, int alChannels, const cVector3l &avSize)
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

		FinishUpload(GLTarget);

		return true;
	}


	//-----------------------------------------------------------------------

	void cGLTexture::SetPixels2D(int alLevel, const cVector2l &avOffset, const cVector2l &avSize,
		eColorDataFormat aDataFormat, void *apPixelData)
	{
		if(mTarget != eTextureTarget_2D) return;
		MarkContentChanged();

		glBindTexture(GL_TEXTURE_2D, mlTextureHandle);
		glTexSubImage2D(TextureTargetToGL(mTarget),alLevel,avOffset.x,avOffset.y, avSize.x,avSize.y,
						ColorFormatToGL(aDataFormat),GL_UNSIGNED_BYTE,apPixelData);
		mbAlphaCoverageKept = false;
		if(HasMipMaps() && alLevel == 0) glGenerateMipmap(GL_TEXTURE_2D);
		mpGfxSDL->RestoreSetTexture();
	}

	//-----------------------------------------------------------------------

	void cGLTexture::Update(float afTimeStep)
	{
		if(mlFrameCount > 1)
		{
			float fMax = (float)mlFrameCount;
			mfAnimationFramePosition += afTimeStep * (1.0f/mfFrameTime) * mfAnimationDirection;

			if(mfAnimationDirection > 0)
			{
				if(mfAnimationFramePosition >= fMax)
				{
					if(mAnimMode == eTextureAnimMode_Loop)
					{
						mfAnimationFramePosition =0;
					}
					else
					{
						mfAnimationFramePosition = fMax - 1.0f;
						mfAnimationDirection = -1.0f;
					}
				}
			}
			else
			{
				if(mfAnimationFramePosition < 0)
				{
					mfAnimationFramePosition =1;
					mfAnimationDirection = 1.0f;
				}
			}
		}
	}

	//-----------------------------------------------------------------------

	bool cGLTexture::HasAnimation()
	{
		return mlFrameCount > 1;
	}

	void cGLTexture::NextFrame()
	{
		mfAnimationFramePosition += mfAnimationDirection;

		if(mfAnimationDirection > 0)
		{
			float fMax = (float)mlFrameCount;
			if(mfAnimationFramePosition >= fMax)
			{
				if(mAnimMode == eTextureAnimMode_Loop)
				{
					mfAnimationFramePosition =0;
				}
				else
				{
					mfAnimationFramePosition = fMax - 1.0f;
					mfAnimationDirection = -1.0f;
				}
			}
		}
		else
		{
			if(mfAnimationFramePosition < 0)
			{
				mfAnimationFramePosition =1;
				mfAnimationDirection = 1.0f;
			}
		}
	}

	void cGLTexture::PrevFrame()
	{
		mfAnimationFramePosition -= mfAnimationDirection;

		if(mfAnimationDirection < 0)
		{
			float fMax = (float)mlFrameCount;
			if(mfAnimationFramePosition >= fMax)
			{
				if(mAnimMode == eTextureAnimMode_Loop)
				{
					mfAnimationFramePosition =0;
				}
				else
				{
					mfAnimationFramePosition = fMax - 1.0f;
					mfAnimationDirection = -1.0f;
				}
			}
		}
		else
		{
			if(mfAnimationFramePosition < 0)
			{
				mfAnimationFramePosition =1;
				mfAnimationDirection = 1.0f;
			}
		}
	}

	float cGLTexture::GetT()
	{
		return cMath::Modulus(mfAnimationFramePosition,1.0f);
	}

	float cGLTexture::GetAnimationFramePosition()
	{
		return mfAnimationFramePosition;
	}
	void cGLTexture::SetAnimationFramePosition(float afFramePosition)
	{
		mfAnimationFramePosition = afFramePosition;
	}
	int cGLTexture::GetCurrentLowlevelHandle()
	{
		return GetTextureHandle();
	}

	//-----------------------------------------------------------------------

	// Thin things lose alpha as they are averaged into smaller mipmaps, and
	// with it the share of their texels that pass the test. For each mipmap
	// the alpha is found that this share of its texels reaches, and all of
	// its alpha scaled so that this one becomes the reference.
	void cGLTexture::KeepAlphaCoverage(float afAlphaRef)
	{
		if(mbAlphaCoverageKept || mbContainsData==false) return;
		if((mTarget != eTextureTarget_2D && mTarget != eTextureTarget_2DArray) || HasMipMaps()==false || mlBpp != 32 || mbFloat) return;
		mbAlphaCoverageKept = true;
		MarkContentChanged();

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
	GLuint cGLTexture::GetSampler()
	{
		bool bMipMaps = HasMipMaps();
		if(mlSampler == 0 || mbSamplerMipMaps != bMipMaps)
		{
			mlSampler = mpGfxSDL->GetSampler(bMipMaps, ToGLWrapMode(mWrapS), ToGLWrapMode(mWrapT), ToGLWrapMode(mWrapR));
			mbSamplerMipMaps = bMipMaps;
		}
		return mlSampler;
	}

	void cGLTexture::SamplerChanged()
	{
		MarkContentChanged();
		mlSampler = 0;
		mpGfxSDL->TextureSamplerChanged(this);
	}

	// 3D textures are made without them.
	bool cGLTexture::HasMipMaps()
	{
		return mbUseMipMaps && mTarget != eTextureTarget_3D;
	}

	//-----------------------------------------------------------------------

	void cGLTexture::SetWrapS(eTextureWrap aMode)
	{
		if(mWrapS == aMode) return;
		mWrapS = aMode;
		SamplerChanged();
	}

	//-----------------------------------------------------------------------

	void cGLTexture::SetWrapT(eTextureWrap aMode)
	{
		if(mWrapT == aMode) return;
		mWrapT = aMode;
		SamplerChanged();
	}

	//-----------------------------------------------------------------------

	void cGLTexture::SetWrapR(eTextureWrap aMode)
	{
		if(mWrapR == aMode) return;
		mWrapR = aMode;
		SamplerChanged();
	}

	//-----------------------------------------------------------------------

	unsigned int cGLTexture::GetTextureHandle()
	{
		return mlTextureHandle;
	}

	int cGLTexture::GetAnimationFrame() const
	{
		// SetAnimationFramePosition is used by callers to synchronize animations. Guard
		// out-of-range input without changing the legacy loop/oscillate timing.
		return cMath::Clamp((int)mfAnimationFramePosition, 0, mlFrameCount-1);
	}

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PRIVATE METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	bool cGLTexture::UploadBitmap(iBitmap2D *apBitmap, int alLayer)
	{
		if(mType == eTextureType_RenderTarget)
		{
			Error("Rendertarget cannot be created from bitmap\n");
			return false;
		}

		GLenum GLTarget = TextureTargetToGL(mTarget);

		cSDLBitmap2D *pBitmapSrc = static_cast<cSDLBitmap2D *>(apBitmap);

		mlWidth = pBitmapSrc->GetWidth();
		mlHeight = pBitmapSrc->GetHeight();

		// Not of the textures made for drawing into, of the screen's size
		if((!cMath::IsPow2(mlHeight) || !cMath::IsPow2(mlWidth)) && !msName.empty())
		{
			Warning("Texture '%s' does not have a pow2 size!\n",msName.c_str());
		}

		int lChannels =0;
		GLenum format =0;
		GetBitmapFormat(pBitmapSrc, lChannels, format);

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
			glTexSubImage3D(GLTarget, 0, 0, 0, alLayer, mlWidth, mlHeight, 1, format, GL_UNSIGNED_BYTE, pPixelSrc);
		else
			glTexSubImage2D(GLTarget, 0, 0, 0, mlWidth, mlHeight, format, GL_UNSIGNED_BYTE, pPixelSrc);

		if(mbUseMipMaps && (mTarget != eTextureTarget_2DArray || alLayer + 1 == mlFrameCount))
			glGenerateMipmap(GLTarget);
		mbAlphaCoverageKept = false;

		FinishUpload(GLTarget);

		if(mlSizeLevel>0 && pNewSrc)
		{
			hplDeleteArray(pNewSrc);
		}

		return true;
	}

	//-----------------------------------------------------------------------

	bool cGLTexture::EnsureStorage(GLenum aTarget, GLenum aFormat, int alWidth, int alHeight, int alDepth)
	{
		if(alWidth <= 0 || alHeight <= 0 || alDepth <= 0) return false;
		int levels = 1;
		if(HasMipMaps())
			for(int size = cMath::Max(alWidth, alHeight); size > 1; size /= 2) ++levels;
		const cVector3l size(alWidth, alHeight, alDepth);
		if(mlStorageTarget != aTarget || mlStorageFormat != aFormat || mvStorageSize != size || mlStorageLevels != levels)
		{
			GLuint replacement = 0;
			glGenTextures(1, &replacement);
			glBindTexture(aTarget, replacement);
			if(aTarget == GL_TEXTURE_1D)
				glTexStorage1D(aTarget, levels, aFormat, alWidth);
			else if(aTarget == GL_TEXTURE_3D || aTarget == GL_TEXTURE_2D_ARRAY)
				glTexStorage3D(aTarget, levels, aFormat, alWidth, alHeight, alDepth);
			else
				glTexStorage2D(aTarget, levels, aFormat, alWidth, alHeight);
			GLint immutable = GL_FALSE;
			glGetTexParameteriv(aTarget, GL_TEXTURE_IMMUTABLE_FORMAT, &immutable);
			if(!immutable) {
				Error("Couldn't allocate immutable storage for texture '%s' (%d x %d x %d)\n", msName.c_str(), alWidth, alHeight, alDepth);
				glDeleteTextures(1, &replacement);
				glBindTexture(aTarget, 0);
				mpGfxSDL->RestoreSetTexture();
				return false;
			}
			const GLuint oldHandle = mlTextureHandle;
			if(mlTextureHandle) glDeleteTextures(1, &mlTextureHandle);
			mlTextureHandle = replacement;
			mlStorageTarget = aTarget;
			mlStorageFormat = aFormat;
			mvStorageSize = size; mlStorageLevels = levels;
			mbAlphaCoverageKept = false;
			mpGfxSDL->TextureStorageChanged(this, oldHandle);
		}
		else
			glBindTexture(aTarget, mlTextureHandle);
		mlWidth = alWidth;
		mlHeight = alHeight;
		mlDepth = alDepth;
		return true;
	}


	void cGLTexture::FinishUpload(GLenum aGLTarget)
	{
		MarkContentChanged();
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

	void cGLTexture::GetBitmapFormat(cSDLBitmap2D *apSrc, int &alChannels, GLenum &aFormat)
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

	GLenum cGLTexture::ToGLWrapMode(eTextureWrap aMode)
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
