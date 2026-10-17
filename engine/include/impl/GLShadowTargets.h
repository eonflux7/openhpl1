// This file is part of OpenHPL1, licensed under the GNU GPL v3 or later.
#ifndef HPL_GLSHADOWTARGETS_H
#define HPL_GLSHADOWTARGETS_H
#include <glad/gl.h>
#include "impl/GLGraphicsState.h"
#include "impl/GLRenderTargets.h"

namespace hpl
{
	// Owns shadow depth textures, samplers and framebuffer; restores the current draw target.
	// Construction needs no GL. Initialization/use and teardown of allocated resources
	// require the caller's current context; Reset/cleanup is idempotent.
	class cGLShadowTargets
	{
	public:
		cGLShadowTargets(cGLGraphicsState &aState, cGLRenderTargets &aTargets) : mState(aState), mTargets(aTargets) {}
		~cGLShadowTargets() { Reset(); }
		void Init();
		void Reset();
		bool IsActive() const { return mbInShadowMap; }
		cGLShadowTargets(const cGLShadowTargets &) = delete;
		cGLShadowTargets &operator=(const cGLShadowTargets &) = delete;

		void BeginShadowMap(bool abCube, int alFace, int alSize);

		void EndShadowMap();

	private:
		void CreateShadowMap(bool abCube, int alSize);
		cGLGraphicsState &mState;
		cGLRenderTargets &mTargets;
		GLuint mlShadowFramebuffer = 0;
		GLuint mvShadowMaps[2] = {};
		int mvShadowMapSizes[2] = {};
		GLuint mvShadowSamplers[2] = {};
		bool mbInShadowMap = false;
	};
} // namespace hpl
#endif
