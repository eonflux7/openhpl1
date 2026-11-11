/*
 * This file is part of OpenHPL1, licensed under the GNU GPL v3 or
 * later. See COPYING.
 */
#ifndef HPL_RENDER_INTERPOLATION_H
#define HPL_RENDER_INTERPOLATION_H

#include <vector>

#include "math/MathTypes.h"

namespace hpl {

	class iEntity3D;

	/**
	 * Shared state of render interpolation (see cGame::Run()).
	 *
	 * Enabled while frames are drawn between logic steps (the 60 FPS limit
	 * is off and the steps are not fixed). Only then is the state before a
	 * step kept, so with the limit on nothing is done or changed.
	 *
	 * The step count goes up before each logic step. Something that kept
	 * its state before a step records the count, and blends only while the
	 * count is still the same: it is not blended if it was not updated in
	 * the last step.
	 *
	 * Particles are blended while they are drawn. Entities that moved in the
	 * last step, and the bones and nodes of animated meshes, are registered
	 * here during the step: a frame between steps moves them part of the
	 * way back (ApplyEntities), so that skins and what is attached to them
	 * follow, and RestoreEntities puts them where the step left them, as is
	 * done with physics bodies.
	 */
	class cRenderInterpolation
	{
	public:
		static void SetEnabled(bool abX){ mbEnabled = abX; }
		static bool IsEnabled(){ return mbEnabled; }

		/** A new logic step: what was registered for the last one is dropped. */
		static void BeginStep(){ ++mlStep; mvEntities.clear(); }
		static unsigned int GetStep(){ return mlStep; }

		/** Set for the frame being drawn, afT is between the last two steps, 0 - 1. */
		static void BeginFrame(float afT){ mfT = afT; mbFrame = true; }
		static void EndFrame(){ mbFrame = false; }

		/** True while a frame between two steps is set up. */
		static bool IsFrame(){ return mbFrame && mbEnabled; }
		/** True while a frame between two steps is set up, and it is not at the last step. */
		static bool IsBlending(){ return mbFrame && mbEnabled && mfT < 1.0f; }
		static float GetT(){ return mfT; }

		/** True while what moves is to be recorded: in a logic step, with interpolation enabled. */
		static bool IsRecording(){ return mbEnabled && mbFrame==false; }

		/** Called by iEntity3D, once per step, when it keeps its state before the step. */
		static void AddEntity(iEntity3D *apEntity){ mvEntities.push_back(apEntity); }
		/** Called by an entity registered in this step when it is destroyed. */
		static void RemoveEntity(iEntity3D *apEntity);

		/** Blend the entities registered in the last step (with IsBlending()). */
		static void ApplyEntities(float afT);
		/** Put them back where the step left them. */
		static void RestoreEntities();

		/**
		 * Blend two transforms. A matrix with a scale is blended element
		 * by element, which keeps the scale; rigid ones are blended with
		 * cMath::MatrixInterpolate.
		 */
		static cMatrixf BlendMatrix(float afT, const cMatrixf& a_mtxA, const cMatrixf& a_mtxB);

	private:
		static inline bool mbEnabled = false;
		static inline bool mbFrame = false;
		static inline float mfT = 1.0f;
		// Starts above 0 so that 0 can mean "never".
		static inline unsigned int mlStep = 1;
		static inline std::vector<iEntity3D*> mvEntities;
	};

}

#endif // HPL_RENDER_INTERPOLATION_H
