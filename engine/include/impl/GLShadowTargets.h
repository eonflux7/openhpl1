// This file is part of OpenHPL1, licensed under the GNU GPL v3 or later.
#ifndef HPL_GLSHADOWTARGETS_H
#define HPL_GLSHADOWTARGETS_H
#include <glad/gl.h>
#include <cstdint>
#include <map>
#include <vector>
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
		void NotifyDrawFailed() { mbDrawFailed = true; }
		// Select/bind a light's retained map. Returns the mask of valid faces;
		// zero identity or an over-budget map uses the uncached scratch target.
		unsigned SelectShadowMap(uint64_t alLightId, bool abCube, int alResolution,
			const std::vector<uint64_t> &avSignature, size_t alBudgetBytes);
		size_t GetCacheBytes() const { return mlCacheBytes; }
		size_t GetCacheEntries() const { return mCache.size(); }
		cGLShadowTargets(const cGLShadowTargets &) = delete;
		cGLShadowTargets &operator=(const cGLShadowTargets &) = delete;

		void BeginShadowMap(bool abCube, int alFace, int alSize);

		void EndShadowMap();

	private:
		struct cCachedMap
		{
			GLuint mlTexture = 0;
			bool mbCube = false;
			int mlResolution = 0;
			unsigned mlValidFaceMask = 0;
			uint64_t mlLastUseSerial = 0;
			size_t mlBytes = 0;
			std::vector<uint64_t> mvSignature;
		};
		GLuint AllocateShadowMap(bool abCube, int alSize);
		void BindShadowMap(bool abCube, GLuint alTexture);
		void Evict(std::map<uint64_t, cCachedMap>::iterator aEntry);
		void CreateShadowMap(bool abCube, int alSize);
		std::map<uint64_t, cCachedMap> mCache;
		cCachedMap *mpSelectedMap = nullptr;
		size_t mlCacheBytes = 0;
		uint64_t mlUseSerial = 0;
		unsigned mlPendingFaceMask = 0;
		cGLGraphicsState &mState;
		cGLRenderTargets &mTargets;
		GLuint mlShadowFramebuffer = 0;
		GLuint mvShadowMaps[2] = {};
		int mvShadowMapSizes[2] = {};
		GLuint mvShadowSamplers[2] = {};
		bool mbInShadowMap = false;
		bool mbDrawFailed = false;
	};
} // namespace hpl
#endif
