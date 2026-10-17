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
#include "impl/GLFinalPasses.h"
#include "impl/GLGraphicsHelpers.h"
#include "impl/GpuProfiler.h"
#include "system/LowLevelSystem.h"
#include "math/Math.h"
#include "AreaTex.h"
#include "SearchTex.h"
namespace hpl
{
	cGLFinalPasses::cGLFinalPasses(cGLGraphicsState &aState, cGLRenderTargets &aTargets, cGpuProfiler &aProfiler)
	: mState(aState), mTargets(aTargets), mGpuProfiler(aProfiler),
	  mSceneCopyPass("scene_copy"), mGammaPass("gamma_correction"), mOutputPass("output"),
	  mEncodePass("output", "ENCODE_ONLY"), mSmaaEdgesPass("SMAA", "EDGES"),
	  mSmaaWeightsPass("SMAA", "WEIGHTS"),
	  mSmaaWeightsNoDiagPass("SMAA", "WEIGHTS SMAA_DISABLE_DIAG_DETECTION"),
	  mSmaaBlendPass("SMAA", "BLEND") {}
	void cGLFinalPasses::Reset()
	{
		for(cFullscreenPass *pass : {&mSceneCopyPass, &mGammaPass, &mOutputPass, &mEncodePass,
				&mSmaaEdgesPass, &mSmaaWeightsPass, &mSmaaWeightsNoDiagPass, &mSmaaBlendPass}) DeleteFullscreenPass(*pass);
		DeleteSmaaTextures();
		if(mlSmaaAreaTexture) glDeleteTextures(1, &mlSmaaAreaTexture);
		if(mlSmaaSearchTexture) glDeleteTextures(1, &mlSmaaSearchTexture);
		mlSmaaAreaTexture = mlSmaaSearchTexture = 0;
	}

	void cGLFinalPasses::SetGammaCorrection(float afX)
	{
		// SDL3 has no hardware gamma ramps. The value is applied to the
		// finished frame in SwapBuffers(), see DrawGammaPass().
		mfGammaCorrection = afX;
	}

	float cGLFinalPasses::GetGammaCorrection()
	{
		return mfGammaCorrection;
	}

	void cGLFinalPasses::SetPassUniform(cFullscreenPass &aPass, eProgramParam aParam,
		float afX, float afY, float afZ, float afW)
	{
		aPass.mpProgram->SetVec4f(aParam, afX, afY, afZ, afW);
	}

	void cGLFinalPasses::EndScene()
	{
		mTargets.FinishScene();

		// Turn scissor off through its owner, not with raw GL. Fullscreen
		// passes snapshot this cache; a raw disable would let the first pass
		// restore the old value and scissor the SMAA clears.
		const bool bScissor = mState.GetScissorActive();
		mState.SetScissorActive(false);

		const bool bMultisampled = mTargets.GetSamples() > 0;
		GLuint lColor = bMultisampled ? mTargets.GetResolvedColor() : mTargets.GetSceneColor();
		GLuint lColorFramebuffer = bMultisampled ? mTargets.GetResolvedColorFramebuffer() : mTargets.GetSceneColorFramebuffer();

		// SMAA leaves the scene as screen values
		bool bLinear = true;
		if(mSmaa.mbActive)
		{
			mGpuProfiler.Begin("SMAA");
			if(DrawSmaa(lColor, lColorFramebuffer)) bLinear = false;
			mGpuProfiler.End();
		}

		mTargets.BindFramebuffer(mTargets.GetOutputFramebuffer(), mTargets.GetOutputSize());
		if(CreateFullscreenPass(mOutputPass))
		{
			SetPassUniform(mOutputPass, eProgramParam::sceneSize, (float)mTargets.GetSize().x, (float)mTargets.GetSize().y,
				1.0f / mTargets.GetSize().x, 1.0f / mTargets.GetSize().y);
			SetPassUniform(mOutputPass, eProgramParam::invOutputSize, 1.0f / mTargets.GetOutputSize().x, 1.0f / mTargets.GetOutputSize().y);
			// The shoulder is set as a screen value; the curve works on light
			SetPassUniform(mOutputPass, eProgramParam::tonemap, bLinear ? 1.0f : 0.0f, mState.GetLinearLightingTuning().mfExposure,
				powf(cMath::Clamp(mState.GetLinearLightingTuning().mfShoulder, 0.0f, 1.0f), 2.2f),
				cMath::Max(mState.GetLinearLightingTuning().mfWhite, 1.0f));
			// The grain: its cells are as large on a picture of any size,
			// and all of it changes at its own rate
			const cFilmGrainSettings &grain = mFilmGrain;
			float fGrainSeed = grain.mfRate > 0 ? floorf(mState.GetFrameTime() * grain.mfRate) : mState.GetFrameTime() * 1000.0f;
			SetPassUniform(mOutputPass, eProgramParam::grain, (float)grain.mType, cMath::Clamp(grain.mfIntensity, 0.0f, 1.0f),
				cMath::Max(grain.mfSize * (float)mTargets.GetOutputSize().y / 1080.0f, 1.0f),
				fmodf(fGrainSeed, 65536.0f));
			SetPassUniform(mOutputPass, eProgramParam::grainTone, cMath::Clamp(grain.mfColor, 0.0f, 1.0f),
				cMath::Clamp(grain.mfShadows, 0.0f, 1.0f), cMath::Clamp(grain.mfHighlights, 0.0f, 1.0f),
				cMath::Clamp(grain.mfDispersion, 0.0f, 1.0f));
			const cPlayerEffectSettings &feedback = mPlayerEffectSettings;
			const cPlayerEffectState &state = mPlayerEffectState;
			auto Amount = [](float f)
			{ return cMath::Clamp(f, 0.0f, 1.0f); };
			SetPassUniform(mOutputPass, eProgramParam::playerEffects0, Amount(state.mfAdaptation) * Amount(feedback.mfAdaptationStrength),
				Amount(state.mfLowHealth) * Amount(feedback.mfInjuryStrength),
				Amount(state.mfBloodHit) * Amount(feedback.mfInjuryStrength),
				Amount(state.mfIceHit) * Amount(feedback.mfInjuryStrength));
			SetPassUniform(mOutputPass, eProgramParam::playerEffects1, Amount(state.mfPanic) * Amount(feedback.mfPanicStrength),
				Amount(state.mfFlash), Amount(state.mfSave), Amount(state.mfDeath));
			SetPassUniform(mOutputPass, eProgramParam::playerEffects2, Amount(state.mfDeathBlack), Amount(state.mfPanicPulse),
				Amount(state.mfUnderwater), Amount(feedback.mfWhiteoutGlow));
			SetPassUniform(mOutputPass, eProgramParam::playerDetails, state.mfBloodSeed, Amount(state.mfLowHealthVision) * Amount(feedback.mfInjuryStrength), 0, 0);
			SetPassUniform(mOutputPass, eProgramParam::playerWaterColor, state.mWaterColor.r, state.mWaterColor.g, state.mWaterColor.b, 0);
			DrawFullscreenPass(mOutputPass, lColor, mTargets.GetOutputSize());
		}
		else
		{
			// Without dithering
			bool bScaled = mTargets.GetSize() != mTargets.GetOutputSize();
			glBindFramebuffer(GL_READ_FRAMEBUFFER, lColorFramebuffer);
			glBlitFramebuffer(0, 0, mTargets.GetSize().x, mTargets.GetSize().y, 0, 0, mTargets.GetOutputSize().x, mTargets.GetOutputSize().y,
				GL_COLOR_BUFFER_BIT, bScaled ? GL_LINEAR : GL_NEAREST);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, mTargets.GetOutputFramebuffer());
		}

		mState.SetScissorActive(bScissor);
	}

	void cGLFinalPasses::DeleteSmaaTextures()
	{
		if(!mlSmaaColor && !mlSmaaStencil && !mvSmaaFramebuffers[0]) return;
		GLuint vTextures[] = {mlSmaaColor, mlSmaaEdges, mlSmaaWeights};
		glDeleteFramebuffers(3, mvSmaaFramebuffers);
		glDeleteTextures(3, vTextures);
		glDeleteRenderbuffers(1, &mlSmaaStencil);
		mvSmaaFramebuffers[0] = mvSmaaFramebuffers[1] = mvSmaaFramebuffers[2] = 0;
		mlSmaaColor = mlSmaaEdges = mlSmaaWeights = mlSmaaStencil = 0;
	}

	bool cGLFinalPasses::CreateSmaaTextures()
	{
		if(mlSceneGeneration != mTargets.GetSceneGeneration())
		{
			DeleteSmaaTextures();
			mlSceneGeneration = mTargets.GetSceneGeneration();
		}
		if(mlSmaaColor) return mvSmaaFramebuffers[0] != 0;

		GLint lLastTexture = 0, lLastAlignment = 4;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &lLastTexture);
		glGetIntegerv(GL_UNPACK_ALIGNMENT, &lLastAlignment);

		if(mlSmaaAreaTexture == 0)
		{
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			mlSmaaAreaTexture = CreateColorTexture(AREATEX_WIDTH, AREATEX_HEIGHT, GL_RG8, GL_LINEAR);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, AREATEX_WIDTH, AREATEX_HEIGHT, GL_RG, GL_UNSIGNED_BYTE, areaTexBytes);
			mlSmaaSearchTexture = CreateColorTexture(SEARCHTEX_WIDTH, SEARCHTEX_HEIGHT, GL_R8, GL_NEAREST);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, SEARCHTEX_WIDTH, SEARCHTEX_HEIGHT, GL_RED, GL_UNSIGNED_BYTE, searchTexBytes);
			glPixelStorei(GL_UNPACK_ALIGNMENT, lLastAlignment);
			cGpuProfiler::Label(GL_TEXTURE, mlSmaaAreaTexture, "SMAA area");
			cGpuProfiler::Label(GL_TEXTURE, mlSmaaSearchTexture, "SMAA search");
		}

		const int lW = mTargets.GetSize().x, lH = mTargets.GetSize().y;
		// The picture keeps the scene's precision: it is dithered to 8 bits later
		mlSmaaColor = CreateColorTexture(lW, lH, kSceneColorFormat, GL_LINEAR);
		mlSmaaEdges = CreateColorTexture(lW, lH, GL_RG8, GL_LINEAR);
		mlSmaaWeights = CreateColorTexture(lW, lH, GL_RGBA8, GL_LINEAR);
		glGenRenderbuffers(1, &mlSmaaStencil);
		glBindRenderbuffer(GL_RENDERBUFFER, mlSmaaStencil);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, lW, lH);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);

		bool bOK = true;
		const GLuint vTextures[3] = {mlSmaaColor, mlSmaaEdges, mlSmaaWeights};
		const char *vNames[3] = {"SMAA picture", "SMAA edges", "SMAA weights"};
		glGenFramebuffers(3, mvSmaaFramebuffers);
		for(int i = 0; i < 3; ++i)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, mvSmaaFramebuffers[i]);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, vTextures[i], 0);
			if(i > 0) glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, mlSmaaStencil);
			bOK = CheckFramebuffer(vNames[i]) && bOK;
			cGpuProfiler::Label(GL_FRAMEBUFFER, mvSmaaFramebuffers[i], vNames[i]);
			cGpuProfiler::Label(GL_TEXTURE, vTextures[i], vNames[i]);
		}

		glBindTexture(GL_TEXTURE_2D, lLastTexture);
		if(bOK == false)
		{
			// Not tried again until the scene's size changes
			glDeleteFramebuffers(3, mvSmaaFramebuffers);
			mvSmaaFramebuffers[0] = mvSmaaFramebuffers[1] = mvSmaaFramebuffers[2] = 0;
		}
		return bOK;
	}

	bool cGLFinalPasses::DrawSmaa(GLuint alColor, GLuint alColorFramebuffer)
	{
		const cSmaaSettings &settings = mSmaa;
		int lDiagonalSteps = cMath::Clamp(settings.mlDiagonalSteps, 0, 20);
		cFullscreenPass &weightsPass = lDiagonalSteps > 0 ? mSmaaWeightsPass : mSmaaWeightsNoDiagPass;

		if(!CreateFullscreenPass(mEncodePass) || !CreateFullscreenPass(mSmaaEdgesPass) ||
			!CreateFullscreenPass(weightsPass) || !CreateFullscreenPass(mSmaaBlendPass)) return false;
		if(CreateSmaaTextures() == false) return false;

		const cVector2l &vSize = mTargets.GetSize();
		GLfloat vClearColor[4];
		GLint lClearStencil = 0;
		glGetFloatv(GL_COLOR_CLEAR_VALUE, vClearColor);
		glGetIntegerv(GL_STENCIL_CLEAR_VALUE, &lClearStencil);
		glClearColor(0, 0, 0, 0);
		glClearStencil(0);
		glStencilMask(0xff);

		///////////////////////////////////////////
		// The picture
		mTargets.BindFramebuffer(mvSmaaFramebuffers[0], vSize);
		SetPassUniform(mEncodePass, eProgramParam::tonemap, 1.0f, mState.GetLinearLightingTuning().mfExposure,
			powf(cMath::Clamp(mState.GetLinearLightingTuning().mfShoulder, 0.0f, 1.0f), 2.2f),
			cMath::Max(mState.GetLinearLightingTuning().mfWhite, 1.0f));
		DrawFullscreenPass(mEncodePass, alColor, vSize);

		///////////////////////////////////////////
		// Edges: the stencil buffer is set where there is one
		for(cFullscreenPass *pPass : {&mSmaaEdgesPass, &weightsPass, &mSmaaBlendPass})
		{
			SetPassUniform(*pPass, eProgramParam::smaaMetrics, 1.0f / vSize.x, 1.0f / vSize.y, (float)vSize.x, (float)vSize.y);
			SetPassUniform(*pPass, eProgramParam::smaaEdge, (float)settings.mEdges, cMath::Clamp(settings.mfThreshold, 0.01f, 0.5f),
				cMath::Max(settings.mfDepthThreshold, 0.0001f), (float)settings.mlShow);
			SetPassUniform(*pPass, eProgramParam::smaaSearch, (float)cMath::Clamp(settings.mlSearchSteps, 1, 112),
				(float)lDiagonalSteps, (float)cMath::Clamp(settings.mlCornerRounding, 0, 100));
			SetPassUniform(*pPass, eProgramParam::smaaDepth, settings.mbDepthHelp ? 1.0f : 0.0f,
				cMath::Max(settings.mfDepthScale, 0.0f), cMath::Clamp(settings.mfDepthStrength, 0.0f, 1.0f));
		}

		mTargets.BindFramebuffer(mvSmaaFramebuffers[1], vSize);
		glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		glStencilFunc(GL_ALWAYS, 1, 0xff);
		glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
		DrawFullscreenPass(mSmaaEdgesPass, mlSmaaColor, vSize, 0, 0, true);

		///////////////////////////////////////////
		// Blend weights, only where there are edges
		mTargets.BindFramebuffer(mvSmaaFramebuffers[2], vSize);
		glClear(GL_COLOR_BUFFER_BIT);
		glStencilFunc(GL_EQUAL, 1, 0xff);
		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
		DrawFullscreenPass(weightsPass, mlSmaaEdges, vSize, mlSmaaAreaTexture, mlSmaaSearchTexture, true);

		///////////////////////////////////////////
		// The blend, back into the scene
		mTargets.BindFramebuffer(alColorFramebuffer, vSize);
		DrawFullscreenPass(mSmaaBlendPass, mlSmaaColor, vSize, mlSmaaWeights, mlSmaaEdges);

		// The renderer sets the stencil function and mask anew whenever it
		// uses the stencil buffer
		glClearColor(vClearColor[0], vClearColor[1], vClearColor[2], vClearColor[3]);
		glClearStencil(lClearStencil);
		return true;
	}

	void cGLFinalPasses::PresentFrame(SDL_Window *apWindow, cDebugOverlay &aOverlay)
	{
		mTargets.SetRenderTarget(nullptr);

		const bool bScissor = mState.GetScissorActive();
		mState.SetScissorActive(false);

		mTargets.BindFramebuffer(0, mTargets.GetOutputSize());
		if(DrawGammaPass() == false)
		{
			glBindFramebuffer(GL_READ_FRAMEBUFFER, mTargets.GetOutputFramebuffer());
			glBlitFramebuffer(0, 0, mTargets.GetOutputSize().x, mTargets.GetOutputSize().y, 0, 0, mTargets.GetOutputSize().x, mTargets.GetOutputSize().y,
				GL_COLOR_BUFFER_BIT, GL_NEAREST);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		}

		aOverlay.Draw(apWindow, &mGpuProfiler, mTargets.GetSize(), mTargets.GetRenderScale());

		// The renderer uses destination alpha as scratch space, so the frame
		// ends with arbitrary alpha. Wayland compositors treat the window as
		// translucent, so make it opaque.
		GLboolean vColorMask[4];
		GLfloat vClearColor[4];
		glGetBooleanv(GL_COLOR_WRITEMASK, vColorMask);
		glGetFloatv(GL_COLOR_CLEAR_VALUE, vClearColor);
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);
		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT);
		glColorMask(vColorMask[0], vColorMask[1], vColorMask[2], vColorMask[3]);
		glClearColor(vClearColor[0], vClearColor[1], vClearColor[2], vClearColor[3]);

		mState.SetScissorActive(bScissor);
	}

	bool cGLFinalPasses::CreateFullscreenPass(cFullscreenPass &aPass)
	{
		if(aPass.mpProgram) return aPass.mpProgram->GetHandle() != 0;
		aPass.mpProgram = hplNew(cGLSLProgram, (aPass.msName + " " + aPass.msDefines, false));
		// Retain a failed object too: its watched files allow a later edit
		// to recover it, and no incomplete program is ever drawn.
		if(!aPass.mpProgram->CreateFromFile(aPass.msName, aPass.msDefines)) return false;
		return true;
	}

	void cGLFinalPasses::DeleteFullscreenPass(cFullscreenPass &aPass)
	{
		if(aPass.mpProgram) hplDelete(aPass.mpProgram);
		aPass.mpProgram = nullptr;
	}

	void cGLFinalPasses::DrawFullscreenPass(cFullscreenPass &aPass, GLuint alTexture, const cVector2l &avSize,
		GLuint alTexture1, GLuint alTexture2, bool abStencil,
		const cVector2l &avPos)
	{
		const GLuint vTextures[3] = {alTexture, alTexture1, alTexture2};

		// The owners below are the authoritative source for engine state. The
		// few values they do not track (the stencil test and this pass's own
		// viewport) are read or restored explicitly. External clients that
		// change raw GL state call cGLGraphicsState::ResyncExternalState().
		cGLTextureUnitBinding vLastTextures[3];
		for(int i = 0; i < 3; ++i)
			vLastTextures[i] = mState.GetTextureUnitBinding(i);
		const unsigned int lLastUnit = mState.GetActiveTextureUnit();
		cGLSLProgram *pLastProgram = cGLSLProgram::GetAppliedProgram();
		const bool bLastStencil = glIsEnabled(GL_STENCIL_TEST) == GL_TRUE;
		const bool bLastDepthTest = mState.GetDepthTestActive();
		const bool bLastBlend = mState.GetBlendActive();
		const bool bLastScissor = mState.GetScissorActive();
		const bool bLastCull = mState.GetCullActive();
		const bool bLastCoverage = mState.GetAlphaToCoverageActive();
		const bool bLastDepthWrite = mState.GetDepthWriteActive();
		const bool *pLastColorWrite = mState.GetColorWriteMask();
		const bool vLastColorWrite[4] = {pLastColorWrite[0], pLastColorWrite[1],
			pLastColorWrite[2], pLastColorWrite[3]};
		const cVector2l vLastViewportPos = mTargets.GetViewportPos();
		const cVector2l vLastViewportSize = mTargets.GetViewportSize();

		for(int i = 2; i >= 0; --i)
		{
			if(vTextures[i] == 0) continue;
			glActiveTexture(GL_TEXTURE0 + i);
			glBindTexture(GL_TEXTURE_2D, vTextures[i]);
			glBindSampler(i, 0);
		}

		// The renderer never enables user clip planes; keep them off rather
		// than querying six capabilities for every pass.
		static const GLenum vClipPlanes[] = {GL_CLIP_DISTANCE0, GL_CLIP_DISTANCE1, GL_CLIP_DISTANCE2,
			GL_CLIP_DISTANCE3, GL_CLIP_DISTANCE4, GL_CLIP_DISTANCE5};
		for(GLenum plane : vClipPlanes) glDisable(plane);

		glDisable(GL_DEPTH_TEST);
		glDisable(GL_STENCIL_TEST);
		glDisable(GL_BLEND);
		glDisable(GL_SCISSOR_TEST);
		glDisable(GL_CULL_FACE);
		glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
		if(abStencil) glEnable(GL_STENCIL_TEST);
		glDepthMask(GL_FALSE);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glViewport(avPos.x, avPos.y, avSize.x, avSize.y);

		bool ready = aPass.mpProgram->Apply();

		// The vertex shader makes the triangle from the vertex numbers
		mGpuProfiler.CountDraw(GL_TRIANGLES, 3);
		mState.BindEmptyVertexArray();
		if(ready) glDrawArrays(GL_TRIANGLES, 0, 3);

		if(pLastProgram)
			pLastProgram->RebindCurrent();
		else
		{
			glUseProgram(0);
			cGLSLProgram::ClearAppliedProgram();
		}

		mState.SetDepthTestActive(bLastDepthTest);
		mState.SetBlendActive(bLastBlend);
		mState.SetScissorActive(bLastScissor);
		mState.SetCullActive(bLastCull);
		if(bLastCoverage)
			glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
		else
			glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
		if(bLastStencil)
			glEnable(GL_STENCIL_TEST);
		else
			glDisable(GL_STENCIL_TEST);
		mState.SetDepthWriteActive(bLastDepthWrite);
		mState.SetColorWriteActive(vLastColorWrite[0], vLastColorWrite[1],
			vLastColorWrite[2], vLastColorWrite[3]);
		glViewport(vLastViewportPos.x, vLastViewportPos.y, vLastViewportSize.x, vLastViewportSize.y);
		for(int i = 0; i < 3; ++i)
			mState.RestoreTextureUnit(i, vLastTextures[i]);
		glActiveTexture(GL_TEXTURE0 + lLastUnit);
	}

	bool cGLFinalPasses::DrawGammaPass()
	{
		if(mfGammaCorrection > 0.999f && mfGammaCorrection < 1.001f) return false;
		if(CreateFullscreenPass(mGammaPass) == false) return false;

		mGammaPass.mpProgram->SetFloat(eProgramParam::gamma,
			mfGammaCorrection);
		DrawFullscreenPass(mGammaPass, mTargets.GetOutputColor(), mTargets.GetOutputSize());
		return true;
	}

	bool cGLFinalPasses::DrawSceneCopy(iTexture *apTarget, const cVector2l &avPos, const cVector2l &avSize)
	{
		if(apTarget == nullptr || mTargets.IsInScene() == false || mTargets.GetRenderTarget()) return false;
		if(avSize.x <= 0 || avSize.y <= 0) return false;
		if(!CreateFullscreenPass(mSceneCopyPass)) return false;

		// GL counts rows from the bottom
		cVector2l vPos(avPos.x, mTargets.GetSize().y - avSize.y - avPos.y);

		GLuint lColor = mTargets.GetSceneColor();
		if(mTargets.GetSamples() > 0)
		{
			if(!mTargets.IsSceneResolved())
				mTargets.BlitScene(GL_COLOR_BUFFER_BIT, vPos.x, vPos.y, vPos.x + avSize.x, vPos.y + avSize.y);
			lColor = mTargets.GetResolvedColor();
		}

		mTargets.SetRenderTarget(apTarget);
		DrawFullscreenPass(mSceneCopyPass, lColor, avSize, 0, 0, false, vPos);
		mTargets.SetRenderTarget(nullptr);
		return true;
	}
} // namespace hpl
