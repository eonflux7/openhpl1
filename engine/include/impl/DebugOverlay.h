/*
 * This file is part of OpenHPL1, licensed under the GNU GPL v3 or
 * later. See COPYING.
 */
#ifndef HPL_DEBUG_OVERLAY_H
#define HPL_DEBUG_OVERLAY_H

#include <chrono>
#include <functional>
#include <string>

#include <SDL3/SDL.h>

#include "math/MathTypes.h"

namespace hpl {

	class cGpuProfiler;

	/**
	 * Dear ImGui overlay with the renderer's numbers, drawn over the
	 * finished frame (after gamma). ImGui is set up the first time something
	 * is shown.
	 *
	 * The game can add windows of its own with SetPanel(). The overlay takes
	 * the mouse and the keyboard only while SetInput(true): the pointer is
	 * then free and drawn by ImGui, and the game gets no mouse input and no
	 * key presses but the function keys (see cLowLevelInputSDL).
	 */
	class cDebugOverlay
	{
	public:
		cDebugOverlay();
		~cDebugOverlay();

		void SetActive(bool abX){ mbActive = abX; }
		bool IsActive(){ return mbActive; }

		/// Called every frame inside the ImGui frame; an empty function for none.
		void SetPanel(std::function<void()> aDraw){ mPanel = std::move(aDraw); }

		void SetInput(bool abX){ mbInput = abX; }
		/// Where the windows' places and sizes are kept; "" for nowhere
		/// (the default: the working directory is the game's data).
		void SetIniFile(const std::string &asFile);
		/// The overlay has the mouse and the keyboard.
		static bool HasInput(){ return mbInput && mbInitialized; }
		/// Every SDL event, while HasInput().
		static void ProcessEvent(const SDL_Event *apEvent);

		/// The pointer is held and hidden, its motion gathered (with HasInput()).
		static void SetMouseCapture(bool abX){ mbMouseCapture = abX; mvMouseMotion = cVector2f(0,0); }
		static bool IsMouseCaptured(){ return mbMouseCapture && HasInput(); }
		static cVector2f TakeMouseMotion(){ cVector2f vMotion = mvMouseMotion; mvMouseMotion = cVector2f(0,0); return vMotion; }

		/// Once per frame, before the swap: draws into the bound framebuffer if active.
		void Draw(SDL_Window *apWindow, cGpuProfiler *apProfiler, const cVector2l& avRenderSize,
				float afRenderScale);

		/// Needs the GL context.
		void Exit();

	private:
		bool Init(SDL_Window *apWindow);
		void DrawRenderer(cGpuProfiler *apProfiler, const cVector2l& avRenderSize, float afRenderScale);

		bool mbActive;
		bool mbFailed;
		std::function<void()> mPanel;
		std::string msIniFile;

		// There is one ImGui context, and the input code has no overlay object.
		static bool mbInitialized;
		static bool mbInput;
		static bool mbMouseCapture;
		static cVector2f mvMouseMotion;

		std::chrono::steady_clock::time_point mLastFrame;
		float mfFrameMs;
	};

};
#endif // HPL_DEBUG_OVERLAY_H
