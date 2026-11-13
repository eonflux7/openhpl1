// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_ANIMATION_TRACK_H
#define HPL_ANIMATION_TRACK_H

#include "math/MathTypes.h"
#include "graphics/GraphicsTypes.h"
#include "system/SystemTypes.h"

namespace hpl {

	class cAnimation;
	class cNode3D;

	class cAnimationTrack
	{
	public:
		cAnimationTrack(const tString &asName, tAnimTransformFlag aTransformFlags, cAnimation *apParent);
		~cAnimationTrack();

		void ResizeKeyFrames(int alSize);

		/**
		 * Creates a new key frame. These should be added in sequential order.
		 * \param afTime the time for the key frame.
		 */
		cKeyFrame* CreateKeyFrame(float afTime);

		inline cKeyFrame* GetKeyFrame(int alIndex){ return mvKeyFrames[alIndex];}
		inline int GetKeyFrameNum(){ return (int) mvKeyFrames.size();}

		inline tAnimTransformFlag GetTransformFlags(){ return mTransformFlags;}

		/**
		 * Apply the animation to a node. The method uses Node->AddXXX() so Update matrix must be called
		 * for the transformation to be applied.
		 * \param apNode The node with it's base pose
		 * \param afTime The time at which to apply the animation
		 * \param afWeight The weight of the animation, a value from 0 to 1.
		 */
		void ApplyToNode(cNode3D* apNode, float afTime, float afWeight);

		/**
		 * Get a KeyFrame that contains an interpolated value.
		 * \param afTime The time from wihcih to create the keyframe.
		 */
		cKeyFrame GetInterpolatedKeyFrame(float afTime);

		/**
		 * Gets key frames between for a specific time.
		 * \param afTime The time
		 * \param &apKeyFrameA The frame that is equal to or before time
		 * \param &apKeyFrameB The frame that is after time.
		 * \return Weight of the different frames. 0 = 100% A, 1 = 100% B 0.5 = 50% A and 50% B
		 */
		float GetKeyFramesAtTime(float afTime, cKeyFrame** apKeyFrameA,cKeyFrame** apKeyFrameB);

		const char* GetName(){ return msName.c_str();}

		void SetNodeIndex(int alIndex){ mlNodeIdx = alIndex;}
		int GetNodeIndex(){ return mlNodeIdx;}

	private:
		tString msName;

		int mlNodeIdx;

		tKeyFramePtrVec mvKeyFrames;
		tAnimTransformFlag mTransformFlags;

		float mfMaxFrameTime;

		cAnimation* mpParent;
	};

};
#endif // HPL_ANIMATION_TRACK_H
