/*
 * This file is part of Overture Remade, licensed under the GNU GPL v3 or
 * later. See COPYING.
 */
#ifndef HPL_DEBUG_OVERLAY_H
#define HPL_DEBUG_OVERLAY_H

#include <chrono>

#include <SDL3/SDL.h>

#include "math/MathTypes.h"

namespace hpl {

	class cGpuProfiler;

	/**
	 * Dear ImGui overlay with the renderer's numbers, drawn over the
	 * finished frame (after gamma). Display only for now: it takes no input.
	 * ImGui is set up the first time the overlay is shown.
	 */
	class cDebugOverlay
	{
	public:
		cDebugOverlay();
		~cDebugOverlay();

		void SetActive(bool abX){ mbActive = abX; }
		bool IsActive(){ return mbActive; }

		/// Once per frame, before the swap: draws into the bound framebuffer if active.
		void Draw(SDL_Window *apWindow, cGpuProfiler *apProfiler, const cVector2l& avRenderSize,
				float afRenderScale);

		/// Needs the GL context.
		void Exit();

	private:
		bool Init(SDL_Window *apWindow);

		bool mbActive;
		bool mbInitialized;
		bool mbFailed;

		std::chrono::steady_clock::time_point mLastFrame;
		float mfFrameMs;
	};

};
#endif // HPL_DEBUG_OVERLAY_H
