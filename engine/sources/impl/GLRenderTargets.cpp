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
#include "impl/GLRenderTargets.h"
#include "impl/GLGraphicsHelpers.h"
#include "impl/SDLTexture.h"
#include "impl/GpuProfiler.h"
#include "system/LowLevelSystem.h"
#include "math/Math.h"
namespace hpl
{
	bool cGLRenderTargets::Init(const cVector2l &avOutputSize, int alSamples)
	{
		mvOutputSize = mvRenderSize = avOutputSize;
		mlMultisampling = alSamples;
		return CreateOutputFramebuffer() && CreateSceneFramebuffers();
	}
	void cGLRenderTargets::FinishScene()
	{
		SetRenderTarget(nullptr);
		ResolveScene();
		mbInScene = false;
		mbSceneColorOnly = false;
		mState.Invalidate();
	}
	void cGLRenderTargets::RestoreAfterShadow()
	{
		if(mpRenderTarget)
			SetRenderTarget(mpRenderTarget);
		else if(mbInScene)
			BindFramebuffer(GetSceneFramebuffer(), mvRenderSize);
		else
			BindFramebuffer(mlOutputFramebuffer, mvOutputSize);
	}

	void cGLRenderTargets::SetMultisampling(int alSamples)
	{
		if(alSamples < 2) alSamples = 0;
		alSamples = cMath::Min(alSamples, mState.GetCaps(eGraphicCaps_MaxMultisampling));
		if(alSamples == mlMultisampling) return;

		mlMultisampling = alSamples;
		DeleteSceneFramebuffers();
		if(!CreateSceneFramebuffers()) Error("Couldn't create the scene framebuffer with %d samples!\n", alSamples);
	}

	bool cGLRenderTargets::CreateSceneFramebuffers()
	{
		GLint lLastTexture = 0;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &lLastTexture);

		const int lW = mvRenderSize.x, lH = mvRenderSize.y;
		bool bOK = true;
		// The output pass samples the single-sampled scene colour, and
		// filters it when it is scaled to the window.
		GLenum lFilter = mvRenderSize == mvOutputSize ? GL_NEAREST : GL_LINEAR;

		mlMultisampling = cMath::Min(mlMultisampling, mState.GetCaps(eGraphicCaps_MaxMultisampling));
		if(mlMultisampling < 2) mlMultisampling = 0;

		GLenum lTarget = GL_TEXTURE_2D;
		if(mlMultisampling > 0)
		{
			lTarget = GL_TEXTURE_2D_MULTISAMPLE;
			glGenTextures(1, &mlSceneColor);
			glGenTextures(1, &mlSceneDepthStencil);
			glBindTexture(lTarget, mlSceneColor);
			glTexStorage2DMultisample(lTarget, mlMultisampling, kSceneColorFormat, lW, lH, GL_TRUE);
			glBindTexture(lTarget, mlSceneDepthStencil);
			glTexStorage2DMultisample(lTarget, mlMultisampling, GL_DEPTH24_STENCIL8, lW, lH, GL_TRUE);
			glBindTexture(lTarget, 0);
		}
		else
		{
			mlSceneColor = CreateColorTexture(lW, lH, kSceneColorFormat, lFilter);
			mlSceneDepthStencil = CreateDepthStencilTexture(lW, lH);
		}

		glGenFramebuffers(1, &mlSceneFramebuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, mlSceneFramebuffer);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, lTarget, mlSceneColor, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, lTarget, mlSceneDepthStencil, 0);
		bOK = CheckFramebuffer("Scene") && bOK;
		cGpuProfiler::Label(GL_FRAMEBUFFER, mlSceneFramebuffer, "Scene");

		// The same colour alone, for the passes that read the depth of
		// the scene they draw over.
		glGenFramebuffers(1, &mlSceneColorFramebuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, mlSceneColorFramebuffer);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, lTarget, mlSceneColor, 0);
		bOK = CheckFramebuffer("Scene colour") && bOK;
		cGpuProfiler::Label(GL_FRAMEBUFFER, mlSceneColorFramebuffer, "Scene colour only");
		cGpuProfiler::Label(GL_TEXTURE, mlSceneColor, "Scene colour");
		cGpuProfiler::Label(GL_TEXTURE, mlSceneDepthStencil, "Scene depth/stencil");

		if(mlMultisampling > 0)
		{
			mlResolveColor = CreateColorTexture(lW, lH, kSceneColorFormat, lFilter);
			mlResolveDepthStencil = CreateDepthStencilTexture(lW, lH);
			glGenFramebuffers(1, &mlResolveFramebuffer);
			glBindFramebuffer(GL_FRAMEBUFFER, mlResolveFramebuffer);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mlResolveColor, 0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, mlResolveDepthStencil, 0);
			bOK = CheckFramebuffer("Resolve") && bOK;
			glGenFramebuffers(1, &mlResolveColorFramebuffer);
			glBindFramebuffer(GL_FRAMEBUFFER, mlResolveColorFramebuffer);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mlResolveColor, 0);
			bOK = CheckFramebuffer("Resolve colour") && bOK;
			cGpuProfiler::Label(GL_FRAMEBUFFER, mlResolveFramebuffer, "Resolve");
			cGpuProfiler::Label(GL_FRAMEBUFFER, mlResolveColorFramebuffer, "Resolve colour only");
			cGpuProfiler::Label(GL_TEXTURE, mlResolveColor, "Resolve colour");
			cGpuProfiler::Label(GL_TEXTURE, mlResolveDepthStencil, "Resolve depth/stencil");
		}

		// The depth with one sample a pixel stays bound to a unit past
		// those SetTexture() binds to, for the passes that read it while
		// they draw into something else (hpl1_scene_depth.glsl).
		glActiveTexture(GL_TEXTURE0 + cGLSLProgram::kSceneDepthUnit);
		glBindTexture(GL_TEXTURE_2D, mlMultisampling > 0 ? mlResolveDepthStencil : mlSceneDepthStencil);
		// And the scene's colour itself, for BeginSceneColorRead()
		glActiveTexture(GL_TEXTURE0 + cGLSLProgram::kSceneColorUnit);
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
		glBindTexture(lTarget, mlSceneColor);
		glActiveTexture(GL_TEXTURE0 + mState.GetActiveTextureUnit());

		glBindTexture(GL_TEXTURE_2D, lLastTexture);
		BindFramebuffer(mlOutputFramebuffer, mvOutputSize);

		Log(" Scene framebuffer: %d x %d, %d samples, 16-bit float\n", lW, lH, mlMultisampling);
		return bOK;
	}

	bool cGLRenderTargets::CreateOutputFramebuffer()
	{
		GLint lLastTexture = 0;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &lLastTexture);

		mlOutputColor = CreateColorTexture(mvOutputSize.x, mvOutputSize.y, GL_RGBA8, GL_NEAREST);
		glGenRenderbuffers(1, &mlOutputDepthStencil);
		glBindRenderbuffer(GL_RENDERBUFFER, mlOutputDepthStencil);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, mvOutputSize.x, mvOutputSize.y);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
		glGenFramebuffers(1, &mlOutputFramebuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, mlOutputFramebuffer);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mlOutputColor, 0);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, mlOutputDepthStencil);
		bool bOK = CheckFramebuffer("Output");
		cGpuProfiler::Label(GL_FRAMEBUFFER, mlOutputFramebuffer, "Output");
		cGpuProfiler::Label(GL_TEXTURE, mlOutputColor, "Output colour");
		cGpuProfiler::Label(GL_RENDERBUFFER, mlOutputDepthStencil, "Output depth/stencil");

		glBindTexture(GL_TEXTURE_2D, lLastTexture);
		BindFramebuffer(mlOutputFramebuffer, mvOutputSize);
		return bOK;
	}

	void cGLRenderTargets::DeleteSceneFramebuffers()
	{
		if(!mlSceneFramebuffer && !mlSceneColor && !mlResolveFramebuffer) return;
		GLuint vFramebuffers[] = {mlSceneFramebuffer, mlSceneColorFramebuffer, mlResolveFramebuffer,
			mlResolveColorFramebuffer};
		GLuint vTextures[] = {mlSceneColor, mlSceneDepthStencil, mlResolveColor, mlResolveDepthStencil};
		glDeleteFramebuffers(4, vFramebuffers);
		glDeleteTextures(4, vTextures);
		mlSceneFramebuffer = mlSceneColorFramebuffer = mlResolveFramebuffer = mlResolveColorFramebuffer = 0;
		mlSceneColor = mlSceneDepthStencil = mlResolveColor = mlResolveDepthStencil = 0;
		++mlSceneGeneration;
	}

	void cGLRenderTargets::DeleteFramebuffers()
	{
		DeleteSceneFramebuffers();
		if(!mlOutputFramebuffer && !mlTargetFramebuffer && !mlOutputColor && !mlOutputDepthStencil) return;
		glDeleteFramebuffers(1, &mlOutputFramebuffer);
		glDeleteFramebuffers(1, &mlTargetFramebuffer);
		glDeleteTextures(1, &mlOutputColor);
		glDeleteRenderbuffers(1, &mlOutputDepthStencil);
		mlOutputFramebuffer = mlTargetFramebuffer = mlOutputColor = mlOutputDepthStencil = 0;
		mlTargetNum = 1;
	}

	void cGLRenderTargets::SetRenderScale(float afScale)
	{
		mfRenderScale = cMath::Clamp(afScale, 0.25f, 2.0f);

		cVector2l vSize((int)(mvOutputSize.x * mfRenderScale + 0.5f), (int)(mvOutputSize.y * mfRenderScale + 0.5f));
		vSize.x = cMath::Max(vSize.x, 1);
		vSize.y = cMath::Max(vSize.y, 1);
		if(vSize == mvRenderSize) return;

		mvRenderSize = vSize;
		DeleteSceneFramebuffers();
		if(!CreateSceneFramebuffers()) Error("Couldn't create the scene framebuffer at %d x %d!\n", vSize.x, vSize.y);
	}

	float cGLRenderTargets::GetRenderScale()
	{
		return mfRenderScale;
	}

	void cGLRenderTargets::BindFramebuffer(GLuint alFramebuffer, const cVector2l &avSize)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, alFramebuffer);
		glViewport(0, 0, avSize.x, avSize.y);
		mvTargetSize = avSize;
	}

	void cGLRenderTargets::BeginScene()
	{
		SetRenderTarget(nullptr);
		mbInScene = true;
		mbSceneResolved = false;
		mbSceneColorOnly = false;
		mState.Invalidate();
		BindFramebuffer(mlSceneFramebuffer, mvRenderSize);
	}

	void cGLRenderTargets::BindCurrentFramebuffer()
	{
		if(mpRenderTarget)
			glBindFramebuffer(GL_FRAMEBUFFER, mlTargetFramebuffer);
		else if(mbInScene)
			glBindFramebuffer(GL_FRAMEBUFFER, GetSceneFramebuffer());
		else
			glBindFramebuffer(GL_FRAMEBUFFER, mlOutputFramebuffer);
	}

	void cGLRenderTargets::BlitScene(GLbitfield aMask, int alX0, int alY0, int alX1, int alY1)
	{
		GLboolean bScissor = glIsEnabled(GL_SCISSOR_TEST);
		GLboolean bDepthMask = GL_TRUE;
		GLboolean vColorMask[4];
		glGetBooleanv(GL_DEPTH_WRITEMASK, &bDepthMask);
		glGetBooleanv(GL_COLOR_WRITEMASK, vColorMask);
		if(bScissor) glDisable(GL_SCISSOR_TEST);
		glDepthMask(GL_TRUE);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

		glBindFramebuffer(GL_READ_FRAMEBUFFER, mlSceneFramebuffer);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mlResolveFramebuffer);
		glBlitFramebuffer(alX0, alY0, alX1, alY1, alX0, alY0, alX1, alY1, aMask, GL_NEAREST);

		if(bScissor) glEnable(GL_SCISSOR_TEST);
		glDepthMask(bDepthMask);
		glColorMask(vColorMask[0], vColorMask[1], vColorMask[2], vColorMask[3]);
	}

	void cGLRenderTargets::ResolveSceneDepth()
	{
		if(mlMultisampling <= 0 || mbInScene == false || mbSceneResolved) return;

		BlitScene(GL_DEPTH_BUFFER_BIT, 0, 0, mvRenderSize.x, mvRenderSize.y);
		BindCurrentFramebuffer();
	}

	void cGLRenderTargets::ResolveScene()
	{
		if(mbInScene == false || mbSceneResolved) return;

		if(mlMultisampling > 0) BlitScene(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, 0, 0, mvRenderSize.x, mvRenderSize.y);
		mbSceneResolved = true;
		mState.Invalidate();
		if(mlMultisampling > 0) BindCurrentFramebuffer();
	}

	bool cGLRenderTargets::BeginSceneColorRead()
	{
		if(!GLAD_GL_VERSION_4_5 || mbInScene == false || mbSceneResolved || mpRenderTarget) return false;

		glTextureBarrier();
		return true;
	}

	void cGLRenderTargets::SetRenderTarget(iTexture *pTex)
	{
		// A resized texture can have a new immutable storage handle even when
		// the resource pointer is unchanged. Reattach it and update the viewport.
		if(pTex == nullptr && mpRenderTarget == nullptr) return;
		mpRenderTarget = pTex;
		mState.Invalidate();

		if(pTex == nullptr)
		{
			if(mbInScene)
				BindFramebuffer(GetSceneFramebuffer(), mvRenderSize);
			else
				BindFramebuffer(mlOutputFramebuffer, mvOutputSize);
			return;
		}

		if(mlTargetFramebuffer == 0) glGenFramebuffers(1, &mlTargetFramebuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, mlTargetFramebuffer);
		cGpuProfiler::Label(GL_FRAMEBUFFER, mlTargetFramebuffer, "Render target");
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, TextureTargetToGL(pTex->GetTarget()),
			static_cast<cSDLTexture *>(pTex)->GetTextureHandle(), 0);
		// What SetRenderTargets() left attached
		if(mlTargetNum > 1)
		{
			for(int i = 1; i < mlTargetNum; ++i)
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, 0, 0);
			glDrawBuffer(GL_COLOR_ATTACHMENT0);
			mlTargetNum = 1;
		}
		BindFramebuffer(mlTargetFramebuffer, cVector2l(pTex->GetWidth(), pTex->GetHeight()));
	}

	void cGLRenderTargets::SetRenderTargets(iTexture **apTextures, int alNum)
	{
		const int lMaxTargets = 8;
		if(alNum <= 1)
		{
			SetRenderTarget(alNum == 1 ? apTextures[0] : nullptr);
			return;
		}
		if(alNum > lMaxTargets) alNum = lMaxTargets;

		mpRenderTarget = apTextures[0];
		mState.Invalidate();

		if(mlTargetFramebuffer == 0) glGenFramebuffers(1, &mlTargetFramebuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, mlTargetFramebuffer);
		cGpuProfiler::Label(GL_FRAMEBUFFER, mlTargetFramebuffer, "Render target");
		GLenum vBuffers[lMaxTargets];
		for(int i = 0; i < alNum; ++i)
		{
			iTexture *pTex = apTextures[i];
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, TextureTargetToGL(pTex->GetTarget()),
				static_cast<cSDLTexture *>(pTex)->GetTextureHandle(), 0);
			vBuffers[i] = GL_COLOR_ATTACHMENT0 + i;
		}
		for(int i = alNum; i < mlTargetNum; ++i)
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, 0, 0);
		glDrawBuffers(alNum, vBuffers);
		mlTargetNum = alNum;
		BindFramebuffer(mlTargetFramebuffer, cVector2l(apTextures[0]->GetWidth(), apTextures[0]->GetHeight()));
	}

	void cGLRenderTargets::SetSceneColorOnly(bool abX)
	{
		if(mbInScene == false || abX == mbSceneColorOnly) return;
		mbSceneColorOnly = abX;

		if(mpRenderTarget == nullptr) BindFramebuffer(GetSceneFramebuffer(), mvRenderSize);
	}

	cVector2f cGLRenderTargets::GetRenderSize()
	{
		return cVector2f((float)mvRenderSize.x, (float)mvRenderSize.y);
	}
} // namespace hpl
