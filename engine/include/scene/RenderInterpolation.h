/*
 * This file is part of OpenHPL1, licensed under the GNU GPL v3 or
 * later. See COPYING.
 */
#ifndef HPL_RENDER_INTERPOLATION_H
#define HPL_RENDER_INTERPOLATION_H

namespace hpl {

	/**
	 * Shared state of render interpolation (see cGame::Run()) for what
	 * blends itself while it is drawn rather than having its nodes moved:
	 * skinned meshes and particles.
	 *
	 * Enabled while frames are drawn between logic steps (the 60 FPS limit
	 * is off and the steps are not fixed). Only then is the state before a
	 * step kept, so with the limit on nothing is done or changed.
	 *
	 * The step count goes up before each logic step. Something that kept
	 * its state before a step records the count, and blends only while the
	 * count is still the same: it is not blended if it was not updated in
	 * the last step.
	 */
	class cRenderInterpolation
	{
	public:
		static void SetEnabled(bool abX){ mbEnabled = abX; }
		static bool IsEnabled(){ return mbEnabled; }

		static void BeginStep(){ ++mlStep; }
		static unsigned int GetStep(){ return mlStep; }

		/** Set for the frame being drawn, afT is between the last two steps, 0 - 1. */
		static void BeginFrame(float afT){ mfT = afT; mbFrame = true; }
		static void EndFrame(){ mbFrame = false; }

		/** True while a frame between two steps is set up. */
		static bool IsFrame(){ return mbFrame && mbEnabled; }
		/** True while a frame between two steps is set up, and it is not at the last step. */
		static bool IsBlending(){ return mbFrame && mbEnabled && mfT < 1.0f; }
		static float GetT(){ return mfT; }

	private:
		static inline bool mbEnabled = false;
		static inline bool mbFrame = false;
		static inline float mfT = 1.0f;
		// Starts above 0 so that 0 can mean "never".
		static inline unsigned int mlStep = 1;
	};

}

#endif // HPL_RENDER_INTERPOLATION_H
