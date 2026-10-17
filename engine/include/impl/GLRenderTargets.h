// This file is part of OpenHPL1, licensed under the GNU GPL v3 or later.
#ifndef HPL_GLRENDERTARGETS_H
#define HPL_GLRENDERTARGETS_H
#include <glad/gl.h>
#include "impl/GLGraphicsState.h"

namespace hpl
{
	// Owns scene, resolve, output and resource-target framebuffers and their transitions.
	// Construction needs no GL. Initialization/use and teardown of allocated resources
	// require the caller's current context; Reset/cleanup is idempotent.
	class cGLRenderTargets
	{
	public:
		explicit cGLRenderTargets(cGLGraphicsState &aState) : mState(aState) {}
		~cGLRenderTargets() { DeleteFramebuffers(); }
		// Leaves the output framebuffer bound; the caller owns the current context.
		bool Init(const cVector2l &avOutputSize, int alSamples);
		void FinishScene();
		void RestoreAfterShadow();
		const cVector2l &GetSize() const { return mvRenderSize; }
		const cVector2l &GetOutputSize() const { return mvOutputSize; }
		const cVector2l &GetTargetSize() const { return mvTargetSize; }
		const cVector2l &GetViewportPos() const { return mvViewportPos; }
		const cVector2l &GetViewportSize() const { return mvViewportSize; }
		int GetSamples() const { return mlMultisampling; }
		unsigned int GetSceneGeneration() const { return mlSceneGeneration; }
		bool IsInScene() const { return mbInScene; }
		bool IsSceneResolved() const { return mbSceneResolved; }
		bool IsSceneDepthResolved() const { return mbSceneDepthResolved; }
		unsigned int GetSceneDepthResolveCount() const { return mlSceneDepthResolveCount; }
		/// A clear or completed draw may have written the scene's depth.
		void InvalidateSceneDepth();
		iTexture *GetRenderTarget() const { return mpRenderTarget; }
		GLuint GetSceneColor() const { return mlSceneColor; }
		GLuint GetResolvedColor() const { return mlResolveColor; }
		GLuint GetSceneColorFramebuffer() const { return mlSceneColorFramebuffer; }
		GLuint GetResolvedColorFramebuffer() const { return mlResolveColorFramebuffer; }
		GLuint GetOutputFramebuffer() const { return mlOutputFramebuffer; }
		GLuint GetOutputColor() const { return mlOutputColor; }
		cGLDrawTargetState GetDrawState(bool abInShadowMap) const
		{
			return {mbInScene, mbSceneResolved, abInShadowMap, mpRenderTarget != nullptr, mlMultisampling};
		}
		cGLRenderTargets(const cGLRenderTargets &) = delete;
		cGLRenderTargets &operator=(const cGLRenderTargets &) = delete;
		void SetMultisampling(int alSamples);

		void DeleteFramebuffers();

		void SetRenderScale(float afScale);

		float GetRenderScale();

		void BindFramebuffer(GLuint alFramebuffer, const cVector2l &avSize);

		void BeginScene();

		// Partial resolve for scene copies. Caller subsequently restores/binds a target.
		void BlitScene(GLbitfield aMask, int alX0, int alY0, int alX1, int alY1);

		void ResolveSceneDepth();

		void ResolveScene();

		bool BeginSceneColorRead();

		void SetRenderTarget(iTexture *pTex);

		void SetRenderTargets(iTexture **apTextures, int alNum);

		void SetSceneColorOnly(bool abX);

		cVector2f GetRenderSize();

	private:
		bool CreateSceneFramebuffers();

		bool CreateOutputFramebuffer();

		void DeleteSceneFramebuffers();

		void BindCurrentFramebuffer();
		cGLGraphicsState &mState;
		cVector2l mvOutputSize = 0;
		cVector2l mvRenderSize = 0;
		cVector2l mvTargetSize = 0;
		cVector2l mvViewportPos = 0;
		cVector2l mvViewportSize = 0;
		float mfRenderScale = 1.0f;
		int mlMultisampling = 0;
		unsigned int mlSceneGeneration = 0;
		bool mbInScene = false;
		bool mbSceneResolved = false;
		bool mbSceneDepthResolved = false;
		bool mbSceneColorOnly = false;
		unsigned int mlSceneDepthResolveCount = 0;
		iTexture *mpRenderTarget = nullptr;
		GLuint mlSceneFramebuffer = 0;
		GLuint mlSceneColorFramebuffer = 0;
		GLuint mlSceneColor = 0;
		GLuint mlSceneDepthStencil = 0;
		GLuint mlResolveFramebuffer = 0;
		GLuint mlResolveColorFramebuffer = 0;
		GLuint mlResolveColor = 0;
		GLuint mlResolveDepthStencil = 0;
		GLuint mlOutputFramebuffer = 0;
		GLuint mlOutputColor = 0;
		GLuint mlOutputDepthStencil = 0;
		GLuint mlTargetFramebuffer = 0;
		int mlTargetNum = 1;
		GLuint GetSceneFramebuffer() const
		{
			if(mlMultisampling > 0 && mbSceneResolved)
				return mbSceneColorOnly ? mlResolveColorFramebuffer : mlResolveFramebuffer;
			return mbSceneColorOnly ? mlSceneColorFramebuffer : mlSceneFramebuffer;
		}
	};
} // namespace hpl
#endif
