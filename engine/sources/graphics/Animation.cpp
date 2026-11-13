// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "graphics/Animation.h"

#include "math/Math.h"
#include "graphics/AnimationTrack.h"

namespace hpl {

	cAnimation::cAnimation(const tString &asName, const tString &asFile) : iResourceBase(asName,0)
	{
		msAnimName = "";
		msFileName = asFile;
	}

	cAnimation::~cAnimation()
	{
		STLDeleteAll(mvTracks);
	}

	float cAnimation::GetLength()
	{
		return mfLength;
	}

	void cAnimation::SetLength(float afTime)
	{
		mfLength = afTime;
	}

	cAnimationTrack* cAnimation::CreateTrack(const tString &asName, tAnimTransformFlag aFlags)
	{
		cAnimationTrack *pTrack = new cAnimationTrack(asName, aFlags, this);

		mvTracks.push_back(pTrack);

		return pTrack;
	}

	cAnimationTrack* cAnimation::GetTrack(int alIndex)
	{
		return mvTracks[alIndex];
	}

	cAnimationTrack* cAnimation::GetTrackByName(const tString &asName)
	{
		for(size_t i=0; i< mvTracks.size(); ++i)
		{
			if(asName == tString(mvTracks[i]->GetName()))
			{
				return mvTracks[i];
			}
		}

		return nullptr;
	}

	void cAnimation::ResizeTracks(int alNum)
	{
		mvTracks.reserve(alNum);
	}

	int cAnimation::GetTrackNum()
	{
		return (int)mvTracks.size();
	}
}
