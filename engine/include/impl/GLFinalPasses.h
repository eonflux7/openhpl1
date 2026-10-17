// This file is part of OpenHPL1, licensed under the GNU GPL v3 or later.
#ifndef HPL_GLFINALPASSES_H
#define HPL_GLFINALPASSES_H
#include <glad/gl.h>
#include "impl/GLGraphicsState.h"
#include "impl/GLRenderTargets.h"
#include "impl/GpuProfiler.h"
#include "impl/DebugOverlay.h"

namespace hpl
{
	// Owns fullscreen programs and SMAA resources; consumes target handles and draw state.
	// Construction needs no GL. Initialization/use and teardown of allocated resources
	// require the caller's current context; Reset/cleanup is idempotent.
	class cGLFinalPasses
	{
		struct cFullscreenPass
		{
			tString msName;
			cGLSLProgram *mpProgram = nullptr;
			tString msDefines;
			cFullscreenPass(const tString &asName, const tString &asDefines = "") : msName(asName), msDefines(asDefines) {}
		};

	public:
		cGLFinalPasses(cGLGraphicsState &aState, cGLRenderTargets &aTargets, cGpuProfiler &aProfiler);
		~cGLFinalPasses() { Reset(); }
		void Reset();
		void SetSmaa(const cSmaaSettings &value) { mSmaa = value; }
		const cSmaaSettings &GetSmaa() const { return mSmaa; }
		void SetFilmGrain(const cFilmGrainSettings &value) { mFilmGrain = value; }
		const cFilmGrainSettings &GetFilmGrain() const { return mFilmGrain; }
		void SetPlayerEffectSettings(const cPlayerEffectSettings &value) { mPlayerEffectSettings = value; }
		const cPlayerEffectSettings &GetPlayerEffectSettings() const { return mPlayerEffectSettings; }
		void SetPlayerEffectState(const cPlayerEffectState &value) { mPlayerEffectState = value; }
		const cPlayerEffectState &GetPlayerEffectState() const { return mPlayerEffectState; }
		cGLFinalPasses(const cGLFinalPasses &) = delete;
		cGLFinalPasses &operator=(const cGLFinalPasses &) = delete;
		void SetGammaCorrection(float afX);

		float GetGammaCorrection();

		void EndScene();

		void PresentFrame(SDL_Window *apWindow, cDebugOverlay &aOverlay);

		bool DrawSceneCopy(iTexture *apTarget, const cVector2l &avPos, const cVector2l &avSize);

	private:
		void DeleteSmaaTextures();
		void SetPassUniform(cFullscreenPass &aPass, eProgramParam aParam,
			float afX, float afY = 0, float afZ = 0, float afW = 0);

		bool CreateSmaaTextures();

		bool DrawSmaa(GLuint alColor, GLuint alColorFramebuffer);

		bool CreateFullscreenPass(cFullscreenPass &aPass);

		void DeleteFullscreenPass(cFullscreenPass &aPass);

		void DrawFullscreenPass(cFullscreenPass &aPass, GLuint alTexture, const cVector2l &avSize,
			GLuint alTexture1 = 0, GLuint alTexture2 = 0, bool abStencil = false,
			const cVector2l &avPos = 0);

		bool DrawGammaPass();
		cGLGraphicsState &mState;
		cGLRenderTargets &mTargets;
		cGpuProfiler &mGpuProfiler;
		float mfGammaCorrection = 1.0f;
		cFullscreenPass mSceneCopyPass;
		cFullscreenPass mGammaPass;
		cFullscreenPass mOutputPass;
		cFullscreenPass mEncodePass;
		cFullscreenPass mSmaaEdgesPass;
		cFullscreenPass mSmaaWeightsPass;
		cFullscreenPass mSmaaWeightsNoDiagPass;
		cFullscreenPass mSmaaBlendPass;
		cSmaaSettings mSmaa;
		cFilmGrainSettings mFilmGrain;
		cPlayerEffectSettings mPlayerEffectSettings;
		cPlayerEffectState mPlayerEffectState;
		// Rebuild render-sized SMAA targets lazily after scene resize or FSAA changes.
		unsigned int mlSceneGeneration = 0;
		GLuint mlSmaaColor = 0;
		GLuint mlSmaaEdges = 0;
		GLuint mlSmaaWeights = 0;
		GLuint mlSmaaStencil = 0;
		GLuint mvSmaaFramebuffers[3] = {};
		GLuint mlSmaaAreaTexture = 0;
		GLuint mlSmaaSearchTexture = 0;
	};
} // namespace hpl
#endif
