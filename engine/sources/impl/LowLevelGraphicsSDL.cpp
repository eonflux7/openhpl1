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

#include "impl/LowLevelGraphicsSDL.h"
#include "math/Math.h"
#include "impl/SDLBitmap2D.h"
#include "impl/SDLFontData.h"
#include "impl/SDLTexture.h"

#include "impl/GLSLProgram.h"
#include "system/LowLevelSystem.h"
#include "impl/VertexBufferVBO.h"



namespace hpl
{


	//-------------------------------------------------

	// The matrix glOrtho multiplies by.
	static cMatrixf OrthoMatrix(float afLeft, float afRight, float afBottom, float afTop,
		float afNear, float afFar)
	{
		return cMatrixf(2.0f / (afRight - afLeft), 0, 0, -(afRight + afLeft) / (afRight - afLeft),
			0, 2.0f / (afTop - afBottom), 0, -(afTop + afBottom) / (afTop - afBottom),
			0, 0, -2.0f / (afFar - afNear), -(afFar + afNear) / (afFar - afNear),
			0, 0, 0, 1);
	}

	//-------------------------------------------------




	//////////////////////////////////////////////////////////////////////////
	// CONSTRUCTORS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	cLowLevelGraphicsSDL::cLowLevelGraphicsSDL()
	: mTargets(mState), mStream(mState, mGpuProfiler),
	  mFinalPasses(mState, mTargets, mGpuProfiler), mShadows(mState, mTargets)
	{
		mpPixelFormat = hplNew(cSDLPixelFormat, ());
	}

	cLowLevelGraphicsSDL::~cLowLevelGraphicsSDL()
	{
		// Release every GL owner while the SDL context (or caller's EGL
		// context) is current. Reset is idempotent; member teardown is inert.
		mDebugOverlay.Exit();
		mGpuProfiler.Exit();
		mFinalPasses.Reset();
		mShadows.Reset();
		mStream.Reset();
		mTargets.DeleteFramebuffers();
		mState.Reset();
		if(glUseProgram) cGLSLProgram::ExitPrograms();
		hplDelete(mpPixelFormat);
	}

	bool cLowLevelGraphicsSDL::Init(int alWidth, int alHeight, int alBpp, int abFullscreen,
		int alMultisampling, const tString &asWindowCaption, int alDisplay)
	{
		if(!mWindow.Init(alWidth, alHeight, alBpp, abFullscreen, asWindowCaption, alDisplay)) return false;
		if(!cGLSLProgram::InitPrograms(this))
		{
			FatalError("Could not initialize linked shader programs.\n");
			return false;
		}
		mGpuProfiler.Init();
		if(!mState.Init()) return false;
		mStream.Init();
		ShowCursor(false);
		if(!mTargets.Init(mWindow.GetSize(), alMultisampling))
		{
			FatalError("Couldn't create the framebuffers!\n");
			return false;
		}
		mShadows.Init();
		Log(" Setting up OpenGL\n");
		SetupGL();
		mWindow.SwapBuffers();
		return true;
	}

	int cLowLevelGraphicsSDL::GetDisplayNum()
	{
		return mWindow.GetDisplayNum();
	}

	tString cLowLevelGraphicsSDL::GetDisplayName(int alDisplay)
	{
		return mWindow.GetDisplayName(alDisplay);
	}

	cVector2l cLowLevelGraphicsSDL::GetDisplaySize(int alDisplay)
	{
		return mWindow.GetDisplaySize(alDisplay);
	}

	void cLowLevelGraphicsSDL::GetDisplayModes(int alDisplay, std::vector<cVector2l> &avModes)
	{
		mWindow.GetDisplayModes(alDisplay, avModes);
	}

	int cLowLevelGraphicsSDL::GetDisplay()
	{
		return mWindow.GetDisplay();
	}

	void cLowLevelGraphicsSDL::SetupGL()
	{
		mState.SetupGL();
	}

	int cLowLevelGraphicsSDL::GetCaps(eGraphicCaps aType)
	{
		return mState.GetCaps(aType);
	}

	void cLowLevelGraphicsSDL::ShowCursor(bool abX)
	{
		mWindow.ShowCursor(abX);
	}

	void cLowLevelGraphicsSDL::SetVsyncActive(bool abX)
	{
		mWindow.SetVsyncActive(abX);
	}

	void cLowLevelGraphicsSDL::SetMultisampling(int alSamples)
	{
		mTargets.SetMultisampling(alSamples);
	}

	GLuint cLowLevelGraphicsSDL::GetSampler(bool abMipMaps, GLenum aWrapS, GLenum aWrapT, GLenum aWrapR)
	{
		return mState.GetSampler(abMipMaps, aWrapS, aWrapT, aWrapR);
	}

	void cLowLevelGraphicsSDL::SetTextureFilter(eTextureFilter aFilter, float afAnisotropy)
	{
		mState.SetTextureFilter(aFilter, afAnisotropy);
	}

	void cLowLevelGraphicsSDL::BindSampler(unsigned int alUnit, GLuint alSampler)
	{
		mState.BindSampler(alUnit, alSampler);
	}

	void cLowLevelGraphicsSDL::TextureSamplerChanged(iTexture *apTex)
	{
		mState.TextureSamplerChanged(apTex);
	}

	void cLowLevelGraphicsSDL::SetGammaCorrection(float afX)
	{
		mFinalPasses.SetGammaCorrection(afX);
	}

	float cLowLevelGraphicsSDL::GetGammaCorrection()
	{
		return mFinalPasses.GetGammaCorrection();
	}

	void cLowLevelGraphicsSDL::SaveScreenToBMP(const tString &asFile)
	{
		mWindow.SaveScreenToBMP(asFile, mpPixelFormat, mTargets.GetOutputFramebuffer());
	}

	iBitmap2D *cLowLevelGraphicsSDL::CreateBitmap2D(const cVector2l &avSize, unsigned int alBpp)
	{
		cSDLBitmap2D *pBmp = hplNew(cSDLBitmap2D, (mpPixelFormat));
		pBmp->Create(avSize, alBpp);

		return pBmp;
	}

	iFontData *cLowLevelGraphicsSDL::CreateFontData(const tString &asName)
	{
		return hplNew(cSDLFontData, (asName, this));
	}

	iBitmap2D *cLowLevelGraphicsSDL::CreateBitmap2DFromSurface(SDL_Surface *apSurface, const tString &asType)
	{
		cSDLBitmap2D *pBmp = hplNew(cSDLBitmap2D, (apSurface, mpPixelFormat, asType));

		pBmp->msType = asType;

		return pBmp;
	}

	iGpuProgram *cLowLevelGraphicsSDL::CreateGpuProgram(const tString &asName)
	{
		return hplNew(cGLSLProgram, (asName));
	}

	iTexture *cLowLevelGraphicsSDL::CreateTexture(bool abUseMipMaps, eTextureType aType, eTextureTarget aTarget)
	{
		return hplNew(cSDLTexture, ("", mpPixelFormat, this, aType, abUseMipMaps, aTarget));
	}

	iTexture *cLowLevelGraphicsSDL::CreateTexture(const tString &asName, bool abUseMipMaps, eTextureType aType, eTextureTarget aTarget)
	{
		return hplNew(cSDLTexture, (asName, mpPixelFormat, this, aType, abUseMipMaps, aTarget));
	}

	iTexture *cLowLevelGraphicsSDL::CreateTexture(iBitmap2D *apBmp, bool abUseMipMaps, eTextureType aType,
		eTextureTarget aTarget)
	{
		cSDLTexture *pTex = hplNew(cSDLTexture, ("", mpPixelFormat, this, aType, abUseMipMaps, aTarget));
		pTex->CreateFromBitmap(apBmp);

		return pTex;
	}

	iTexture *cLowLevelGraphicsSDL::CreateTexture(const cVector2l &avSize, int alBpp, cColor aFillCol,
		bool abUseMipMaps, eTextureType aType, eTextureTarget aTarget)
	{
		cSDLTexture *pTex = nullptr;

		if(aType == eTextureType_RenderTarget)
		{
			pTex = hplNew(cSDLTexture, ("", mpPixelFormat, this, aType, abUseMipMaps, aTarget));
			pTex->Create(avSize.x, avSize.y, aFillCol);
		}
		else
		{
			// 64 bits: 16-bit floating point channels
			bool bFloat = alBpp == 64;
			iBitmap2D *pBmp = CreateBitmap2D(avSize, bFloat ? 32 : alBpp);
			pBmp->FillRect(cRect2l(0, 0, 0, 0), aFillCol);

			pTex = hplNew(cSDLTexture, ("", mpPixelFormat, this, aType, abUseMipMaps, aTarget));
			pTex->SetFloat(bFloat);
			bool bRet = pTex->CreateFromBitmap(pBmp);

			hplDelete(pBmp);

			if(bRet == false)
			{
				hplDelete(pTex);
				return nullptr;
			}
		}
		return pTex;
	}

	iTexture *cLowLevelGraphicsSDL::CreateFloatTexture(const cVector2l &avSize, int alChannels, eTextureTarget aTarget)
	{
		iBitmap2D *pBmp = CreateBitmap2D(avSize, 32);
		pBmp->FillRect(cRect2l(0, 0, 0, 0), cColor(0, 0));

		cSDLTexture *pTex = hplNew(cSDLTexture, ("", mpPixelFormat, this, eTextureType_Normal, false, aTarget));
		pTex->SetFloat(true);
		pTex->SetFloatChannels(alChannels);
		bool bRet = pTex->CreateFromBitmap(pBmp);
		hplDelete(pBmp);

		if(bRet == false)
		{
			hplDelete(pTex);
			return nullptr;
		}
		return pTex;
	}

	void cLowLevelGraphicsSDL::SetDrawMatrices(const cDrawMatrices &aMatrices)
	{
		mState.SetDrawMatrices(aMatrices);
	}

	cMatrixf cLowLevelGraphicsSDL::CreateOrthoProjection(const cVector2f &avSize, float afMin, float afMax)
	{
		return OrthoMatrix(0, avSize.x, avSize.y, 0, afMin, afMax);
	}

	cMatrixf cLowLevelGraphicsSDL::CreateVirtualOrthoProjection(float afMin, float afMax)
	{
		cVector2f vOffset = GetVirtualOffset();

		return OrthoMatrix(-vOffset.x, mvVirtualSize.x + vOffset.x,
			mvVirtualSize.y + vOffset.y, -vOffset.y, afMin, afMax);
	}

	void cLowLevelGraphicsSDL::SetTexture(unsigned int alUnit, iTexture *apTex)
	{
		mState.SetTexture(alUnit, apTex);
	}

	void cLowLevelGraphicsSDL::TextureStorageChanged(iTexture *apTex)
	{
		mState.TextureStorageChanged(apTex);
	}

	void cLowLevelGraphicsSDL::ForgetTexture(iTexture *apTex)
	{
		mState.ForgetTexture(apTex);
	}

	void cLowLevelGraphicsSDL::RestoreSetTexture()
	{
		mState.RestoreSetTexture();
	}

	iVertexBuffer *cLowLevelGraphicsSDL::CreateVertexBuffer(tVertexFlag aFlags,
		eVertexBufferDrawType aDrawType,
		eVertexBufferUsageType aUsageType,
		int alReserveVtxSize, int alReserveIdxSize)
	{
		return hplNew(cVertexBufferVBO, (this, aFlags, aDrawType, aUsageType, alReserveVtxSize, alReserveIdxSize));
	}

	void cLowLevelGraphicsSDL::FlushRendering()
	{
		glFlush();
	}

	void cLowLevelGraphicsSDL::SwapBuffers()
	{
		glFlush();

		cGLSLProgram::ReloadChangedFiles();

		BeginGpuSection("Present");
		mFinalPasses.PresentFrame(mWindow.GetWindow(), mDebugOverlay);
		EndGpuSection();

		mWindow.CaptureFrame(mGpuProfiler);

		mGpuProfiler.EndFrame();
		mWindow.SwapBuffers();
		mGpuProfiler.BeginFrame();

		mTargets.BindFramebuffer(mTargets.GetOutputFramebuffer(), mWindow.GetSize());
	}

	void cLowLevelGraphicsSDL::SetRenderScale(float afScale)
	{
		mTargets.SetRenderScale(afScale);
	}

	float cLowLevelGraphicsSDL::GetRenderScale()
	{
		return mTargets.GetRenderScale();
	}

	void cLowLevelGraphicsSDL::SetLinearLightingTuning(const cLinearLightingTuning &aTuning)
	{
		mState.SetLinearLightingTuning(aTuning);
	}

	void cLowLevelGraphicsSDL::BeginScene()
	{
		mTargets.BeginScene();
	}

	void cLowLevelGraphicsSDL::ResolveSceneDepth()
	{
		mTargets.ResolveSceneDepth();
	}

	bool cLowLevelGraphicsSDL::BeginSceneColorRead()
	{
		return mTargets.BeginSceneColorRead();
	}

	void cLowLevelGraphicsSDL::ResolveScene()
	{
		mTargets.ResolveScene();
	}

	void cLowLevelGraphicsSDL::EndScene()
	{
		mFinalPasses.EndScene();
	}

	void cLowLevelGraphicsSDL::DrawTriangles(const tVertexVec &avVertices)
	{
		mStream.DrawTriangles(avVertices, mTargets.GetDrawState(mShadows.IsActive()));
	}

	void cLowLevelGraphicsSDL::ClearScreen()
	{
		mState.ClearScreen();
	}

	void cLowLevelGraphicsSDL::SetClearColor(const cColor &aCol)
	{
		mState.SetClearColor(aCol);
	}

	void cLowLevelGraphicsSDL::SetClearDepth(float afDepth)
	{
		mState.SetClearDepth(afDepth);
	}

	void cLowLevelGraphicsSDL::SetClearColorActive(bool abX)
	{
		mState.SetClearColorActive(abX);
	}

	void cLowLevelGraphicsSDL::SetClearDepthActive(bool abX)
	{
		mState.SetClearDepthActive(abX);
	}

	void cLowLevelGraphicsSDL::SetColorWriteActive(bool abR, bool abG, bool abB, bool abA)
	{
		mState.SetColorWriteActive(abR, abG, abB, abA);
	}

	void cLowLevelGraphicsSDL::SetDepthWriteActive(bool abX)
	{
		mState.SetDepthWriteActive(abX);
	}

	void cLowLevelGraphicsSDL::SetDepthTestActive(bool abX)
	{
		mState.SetDepthTestActive(abX);
	}

	void cLowLevelGraphicsSDL::SetDepthTestFunc(eDepthTestFunc aFunc)
	{
		mState.SetDepthTestFunc(aFunc);
	}

	void cLowLevelGraphicsSDL::SetAlphaCutoff(float afMin)
	{
		mState.SetAlphaCutoff(afMin);
	}

	void cLowLevelGraphicsSDL::SetCullActive(bool abX)
	{
		mState.SetCullActive(abX);
	}

	void cLowLevelGraphicsSDL::SetCullMode(eCullMode aMode)
	{
		mState.SetCullMode(aMode);
	}

	void cLowLevelGraphicsSDL::SetScissorActive(bool abX)
	{
		mState.SetScissorActive(abX);
	}

	void cLowLevelGraphicsSDL::SetScissorRect(const cRect2l &aRect)
	{
		glScissor(aRect.x, (mTargets.GetTargetSize().y - aRect.y - 1) - aRect.h, aRect.w, aRect.h);
	}

	void cLowLevelGraphicsSDL::SetBlendActive(bool abX)
	{
		mState.SetBlendActive(abX);
	}

	void cLowLevelGraphicsSDL::SetBlendFunc(eBlendFunc aSrcFactor, eBlendFunc aDestFactor)
	{
		mState.SetBlendFunc(aSrcFactor, aDestFactor);
	}

	void cLowLevelGraphicsSDL::DrawLine(const cVector3f &avBegin, const cVector3f &avEnd, cColor aCol)
	{
		mStream.DrawLine(avBegin, avEnd, aCol, mTargets.GetDrawState(mShadows.IsActive()));
	}

	void cLowLevelGraphicsSDL::DrawBoxMaxMin(const cVector3f &avMax, const cVector3f &avMin, cColor aCol)
	{
		mStream.DrawBoxMaxMin(avMax, avMin, aCol, mTargets.GetDrawState(mShadows.IsActive()));
	}

	void cLowLevelGraphicsSDL::DrawSphere(const cVector3f &avPos, float afRadius, cColor aCol)
	{
		mStream.DrawSphere(avPos, afRadius, aCol, mTargets.GetDrawState(mShadows.IsActive()));
	}

	void cLowLevelGraphicsSDL::DrawLine2D(const cVector2f &avBegin, const cVector2f &avEnd, float afZ, cColor aCol)
	{
		mStream.DrawLine2D(avBegin, avEnd, afZ, aCol, mTargets.GetDrawState(mShadows.IsActive()));
	}

	bool cLowLevelGraphicsSDL::DrawSceneCopy(iTexture *apTarget, const cVector2l &avPos, const cVector2l &avSize)
	{
		return mFinalPasses.DrawSceneCopy(apTarget, avPos, avSize);
	}

	void cLowLevelGraphicsSDL::SetRenderTarget(iTexture *pTex)
	{
		mTargets.SetRenderTarget(pTex);
	}

	void cLowLevelGraphicsSDL::SetRenderTargets(iTexture **apTextures, int alNum)
	{
		mTargets.SetRenderTargets(apTextures, alNum);
	}

	void cLowLevelGraphicsSDL::SetSceneColorOnly(bool abX)
	{
		mTargets.SetSceneColorOnly(abX);
	}

	void cLowLevelGraphicsSDL::BeginShadowMap(bool abCube, int alFace, int alSize)
	{
		mShadows.BeginShadowMap(abCube, alFace, alSize);
	}

	void cLowLevelGraphicsSDL::EndShadowMap()
	{
		mShadows.EndShadowMap();
	}

	void cLowLevelGraphicsSDL::SetLightLook(const cLightLook &aLook)
	{
		mState.SetLightLook(aLook);
	}

	void cLowLevelGraphicsSDL::SetShadowMapParams(const cShadowMapParams &aParams)
	{
		mState.SetShadowMapParams(aParams);
	}

	cVector2f cLowLevelGraphicsSDL::GetScreenSize()
	{
		return cVector2f((float)mWindow.GetSize().x, (float)mWindow.GetSize().y);
	}

	cVector2f cLowLevelGraphicsSDL::GetRenderSize()
	{
		return mTargets.GetRenderSize();
	}

	cVector2f cLowLevelGraphicsSDL::GetVirtualSize()
	{
		return mvVirtualSize;
	}

	void cLowLevelGraphicsSDL::SetVirtualSize(cVector2f avSize)
	{
		mvVirtualSize = avSize;
	}

	cVector2f cLowLevelGraphicsSDL::GetVirtualOffset()
	{
		if(mWindow.GetSize().x <= 0 || mWindow.GetSize().y <= 0) return cVector2f(0, 0);

		// One virtual unit is the same number of pixels on both axes, so the
		// axis with pixels to spare gets a border on each side.
		float fScaleX = (float)mWindow.GetSize().x / mvVirtualSize.x;
		float fScaleY = (float)mWindow.GetSize().y / mvVirtualSize.y;
		float fScale = fScaleX < fScaleY ? fScaleX : fScaleY;

		return cVector2f(((float)mWindow.GetSize().x / fScale - mvVirtualSize.x) * 0.5f,
			((float)mWindow.GetSize().y / fScale - mvVirtualSize.y) * 0.5f);
	}

	void cLowLevelGraphicsSDL::BindVertexArray(GLuint alVertexArray)
	{
		mState.BindVertexArray(alVertexArray);
	}

	const void *cLowLevelGraphicsSDL::StreamIndices(const unsigned int *apIndices, int alCount)
	{
		return mStream.StreamIndices(apIndices, alCount);
	}

	void cLowLevelGraphicsSDL::DrawSpriteBatch(const cSpriteVertex *apVertices, size_t alCount,
		iTexture *apTexture, const cSpriteBlend &aBlend)
	{
		mStream.DrawSpriteBatch(apVertices, alCount, apTexture, aBlend, mTargets.GetDrawState(mShadows.IsActive()));
	}

	GLenum cLowLevelGraphicsSDL::GetGLTextureTargetEnum(eTextureTarget aType)
	{
		return TextureTargetToGL(aType);
	}

	void cLowLevelGraphicsSDL::SetFrameUniforms(const cMatrixf &a_mtxView, const cMatrixf &a_mtxProjection,
		const cVector3f &avEyePos, float afNear, float afFar,
		float afTime)
	{
		mState.SetFrameUniforms(a_mtxView, a_mtxProjection, avEyePos, afNear, afFar, afTime, mTargets.GetSize());
	}

	bool cLowLevelGraphicsSDL::PrepareDraw()
	{
		return mState.PrepareDraw(mTargets.GetDrawState(mShadows.IsActive()));
	}
} // namespace hpl
