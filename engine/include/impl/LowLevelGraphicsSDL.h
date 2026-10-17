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
#ifndef HPL_LOWLEVELGRAPHICS_SDL_H
#define HPL_LOWLEVELGRAPHICS_SDL_H

#include <glad/gl.h>

#include <SDL3/SDL.h>

#include "graphics/LowLevelGraphics.h"
#include "impl/DebugOverlay.h"
#include "impl/GpuProfiler.h"
#include "impl/SDLPixelFormat.h"
#include "impl/SDLGraphicsWindow.h"
#include "impl/GLShadowTargets.h"
#include "impl/GLStreamDrawing.h"
#include "impl/GLFinalPasses.h"
#include "impl/GLRenderTargets.h"
#include "impl/GLGraphicsState.h"
#include "math/MathTypes.h"
#include "impl/GLGraphicsHelpers.h"



namespace hpl
{

	class cLowLevelGraphicsSDL : public iLowLevelGraphics
	{
	public:
		cLowLevelGraphicsSDL();
		~cLowLevelGraphicsSDL() override;

		bool Init(int alWidth, int alHeight, int alBpp, int abFullscreen, int alMultisampling,
			const tString &asWindowCaption, int alDisplay) override;

		int GetDisplayNum() override;
		tString GetDisplayName(int alDisplay) override;
		cVector2l GetDisplaySize(int alDisplay) override;
		void GetDisplayModes(int alDisplay, std::vector<cVector2l> &avModes) override;
		int GetDisplay() override;

		int GetCaps(eGraphicCaps aType) override;

		void ShowCursor(bool abX) override;

		void SetVsyncActive(bool abX) override;

		void SetGammaCorrection(float afX) override;
		float GetGammaCorrection() override;

		int GetMultisampling() override { return mTargets.GetSamples(); }
		void SetMultisampling(int alSamples) override;
		void SetTextureFilter(eTextureFilter aFilter, float afAnisotropy) override;


		cVector2f GetScreenSize() override;
		cVector2f GetRenderSize() override;
		void SetRenderScale(float afScale) override;
		float GetRenderScale() override;
		void SetLinearLightingTuning(const cLinearLightingTuning &aTuning) override;
		const cLinearLightingTuning &GetLinearLightingTuning() override { return mState.GetLinearLightingTuning(); }
		void SetShading(const cShadingSettings &aSettings) override { mState.SetShading(aSettings); }
		const cShadingSettings &GetShading() override { return mState.GetShading(); }
		void SetEffects(const cEffectSettings &aSettings) override { mState.SetEffects(aSettings); }
		const cEffectSettings &GetEffects() override { return mState.GetEffects(); }
		void SetFilmGrain(const cFilmGrainSettings &aSettings) override { mFinalPasses.SetFilmGrain(aSettings); }
		const cFilmGrainSettings &GetFilmGrain() override { return mFinalPasses.GetFilmGrain(); }
		void SetPlayerEffectSettings(const cPlayerEffectSettings &aSettings) override { mFinalPasses.SetPlayerEffectSettings(aSettings); }
		const cPlayerEffectSettings &GetPlayerEffectSettings() override { return mFinalPasses.GetPlayerEffectSettings(); }
		void SetPlayerEffectState(const cPlayerEffectState &aState) override { mFinalPasses.SetPlayerEffectState(aState); }
		const cPlayerEffectState &GetPlayerEffectState() override { return mFinalPasses.GetPlayerEffectState(); }
		void SetSmaa(const cSmaaSettings &aSettings) override { mFinalPasses.SetSmaa(aSettings); }
		const cSmaaSettings &GetSmaa() override { return mFinalPasses.GetSmaa(); }
		void BeginScene() override;
		void ResolveScene() override;
		void ResolveSceneDepth() override;
		bool BeginSceneColorRead() override;
		void EndScene() override;
		cVector2f GetVirtualSize() override;
		void SetVirtualSize(cVector2f avSize) override;
		cVector2f GetVirtualOffset() override;

		iBitmap2D *CreateBitmap2D(const cVector2l &avSize, unsigned int alBpp) override;
		iFontData *CreateFontData(const tString &asName) override;

		iTexture *CreateTexture(bool abUseMipMaps, eTextureType aType, eTextureTarget aTarget) override;
		iTexture *CreateTexture(const tString &asName, bool abUseMipMaps, eTextureType aType, eTextureTarget aTarget) override;
		iTexture *CreateTexture(iBitmap2D *apBmp, bool abUseMipMaps, eTextureType aType, eTextureTarget aTarget) override;
		iTexture *CreateFloatTexture(const cVector2l &avSize, int alChannels, eTextureTarget aTarget) override;
		iTexture *CreateTexture(const cVector2l &avSize, int alBpp, cColor aFillCol,
			bool abUseMipMaps, eTextureType aType, eTextureTarget aTarget) override;



		iGpuProgram *CreateGpuProgram(const tString &asName) override;

		void SaveScreenToBMP(const tString &asFile) override;
		void SaveNextFrameToPNG(const tString &asFile) override { mWindow.SaveNextFrameToPNG(asFile); }
		void CaptureNextFrame(tFrameCapture aCapture) override { mWindow.CaptureNextFrame(std::move(aCapture)); }

		/////////// MATRIX METHODS /////////////////////////

		using iLowLevelGraphics::SetDrawMatrices;
		void SetDrawMatrices(const cDrawMatrices &aMatrices) override;
		const cDrawMatrices &GetDrawMatrices() const override { return mState.GetDrawMatrices(); }

		void SetFrameUniforms(const cMatrixf &a_mtxView, const cMatrixf &a_mtxProjection,
			const cVector3f &avEyePos, float afNear, float afFar,
			float afTime) override;

		void BeginGpuSection(const char *asName) override { mGpuProfiler.Begin(asName); }
		void EndGpuSection() override { mGpuProfiler.End(); }
		void SetDebugOverlayActive(bool abX) override { mDebugOverlay.SetActive(abX); }
		bool GetDebugOverlayActive() override { return mDebugOverlay.IsActive(); }
		void SetDebugPanel(std::function<void()> aDraw) override { mDebugOverlay.SetPanel(std::move(aDraw)); }
		void SetDebugPanelInput(bool abX) override { mDebugOverlay.SetInput(abX); }

		cGpuProfiler *GetGpuProfiler() { return &mGpuProfiler; }

		/**
		 * Applies the bound linked program (or hpl1_Default) and updates
		 * shader state before a draw. Returns false for an unusable layout.
		 */
		bool PrepareDraw();

		/// Every draw binds its vertex array through this.
		void BindVertexArray(GLuint alVertexArray);
		/**
		 * Copies indices to the stream index buffer and binds it to the
		 * bound vertex array. Returns what glDrawElements takes as the
		 * pointer.
		 */
		void BindSheetIndices() { mStream.BindSheetIndices(); }
		const void *StreamIndices(const unsigned int *apIndices, int alCount);

		cMatrixf CreateOrthoProjection(const cVector2f &avSize, float afMin, float afMax) override;
		cMatrixf CreateVirtualOrthoProjection(float afMin, float afMax) override;

		/////////// DRAWING METHODS /////////////////////////

		// OCCLUSION

		// CLEARING THE FRAMEBUFFER
		void ClearScreen() override;

		void SetClearColor(const cColor &aCol) override;
		void SetClearDepth(float afDepth) override;

		void SetClearColorActive(bool abX) override;
		void SetClearDepthActive(bool abX) override;

		void SetColorWriteActive(bool abR, bool abG, bool abB, bool abA) override;
		void SetDepthWriteActive(bool abX) override;

		void SetCullActive(bool abX) override;
		void SetCullMode(eCullMode aMode) override;

		//DEPTH
		void SetDepthTestActive(bool abX) override;
		void SetDepthTestFunc(eDepthTestFunc aFunc) override;

		//ALPHA
		void SetAlphaCutoff(float afMin) override;

		//SCISSOR
		void SetScissorActive(bool abX) override;
		void SetScissorRect(const cRect2l &aRect) override;

		//BLENDING
		void SetBlendActive(bool abX) override;
		void SetBlendFunc(eBlendFunc aSrcFactor, eBlendFunc aDestFactor) override;


		// TEXTURE
		void SetTexture(unsigned int alUnit, iTexture *apTex) override;
		/**
		 * Called by a texture being destroyed, so that SetTexture does not
		 * take a new texture at the same address for the one still set.
		 */
		void ForgetTexture(iTexture *apTex);
		/**
		 * Called by a texture after it has bound itself to create or update
		 * its data: binds and enables the texture set with SetTexture on the
		 * active unit again.
		 */
		void RestoreSetTexture();
		/**
		 * The sampler object for textures with these wrap modes, with
		 * mipmaps or without. A texture is read through the sampler bound
		 * to its unit, so the filter of all textures is changed by
		 * changing a few samplers (SetTextureFilter()).
		 */
		GLuint GetSampler(bool abMipMaps, GLenum aWrapS, GLenum aWrapT, GLenum aWrapR);
		/// Binds a sampler object to a texture unit, 0 for none: the
		/// texture's own parameters are used then.
		void BindSampler(unsigned int alUnit, GLuint alSampler);
		/// Called by a texture whose sampler is another one now.
		void TextureSamplerChanged(iTexture *apTex);
		/// Rebind an immutable texture whose storage object was replaced.
		void TextureStorageChanged(iTexture *apTex);

		// POLYGONS
		iVertexBuffer *CreateVertexBuffer(tVertexFlag aFlags, eVertexBufferDrawType aDrawType,
			eVertexBufferUsageType aUsageType,
			int alReserveVtxSize = 0, int alReserveIdxSize = 0) override;



		void DrawSpriteBatch(const cSpriteVertex *apVertices, size_t alCount,
			iTexture *apTexture, const cSpriteBlend &aBlend) override;

		void DrawTriangles(const tVertexVec &avVertices) override;

		void DrawLine(const cVector3f &avBegin, const cVector3f &avEnd, cColor aCol) override;
		void DrawBoxMaxMin(const cVector3f &avMax, const cVector3f &avMin, cColor aCol) override;
		void DrawSphere(const cVector3f &avPos, float afRadius, cColor aCol) override;

		void DrawLine2D(const cVector2f &avBegin, const cVector2f &avEnd, float afZ, cColor aCol) override;


		//FRAMEBUFFER
		bool DrawSceneCopy(iTexture *apTarget, const cVector2l &avPos, const cVector2l &avSize) override;
		void SetRenderTarget(iTexture *pTex) override;
		void SetRenderTargets(iTexture **apTextures, int alNum) override;
		void SetSceneColorOnly(bool abX) override;
		void BeginShadowMap(bool abCube, int alFace, int alSize) override;
		void EndShadowMap() override;
		void SetShadowMapParams(const cShadowMapParams &aParams) override;
		void SetLightLook(const cLightLook &aLook) override;

		void FlushRendering() override;
		void SwapBuffers() override;

		///// SDL Specific ////////////////////////////

		iBitmap2D *CreateBitmap2DFromSurface(SDL_Surface *apSurface, const tString &asType);

		SDL_Window *GetWindow() { return mWindow.GetWindow(); }

		void SetupGL();

		GLenum GetGLTextureTargetEnum(eTextureTarget aType);

	private:
		// The context owner is first so it outlives all GL resources.
		cSDLGraphicsWindow mWindow;
		cGpuProfiler mGpuProfiler;
		cDebugOverlay mDebugOverlay;
		cGLGraphicsState mState;
		cGLRenderTargets mTargets;
		cGLStreamDrawing mStream;
		cGLFinalPasses mFinalPasses;
		cGLShadowTargets mShadows;
		cVector2f mvVirtualSize = cVector2f(800, 600);
		cSDLPixelFormat *mpPixelFormat;
	};
} // namespace hpl
#endif // HPL_LOWLEVELGRAPHICS_SDL_H
