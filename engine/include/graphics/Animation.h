// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_ANIMATION_H
#define HPL_ANIMATION_H

#include "math/MathTypes.h"
#include "graphics/GraphicsTypes.h"
#include "system/SystemTypes.h"
#include "resources/ResourceBase.h"

namespace hpl {

	class cAnimationTrack;

	typedef std::vector<cAnimationTrack*> tAnimationTrackVec;
	typedef tAnimationTrackVec::iterator tAnimationTrackVecIt;

	class cAnimation : public iResourceBase
	{
	public:
		cAnimation(const tString &asName, const tString &asFile);
		~cAnimation() override;

		float GetLength();
		void SetLength(float afTime);

		cAnimationTrack* CreateTrack(const tString &asName, tAnimTransformFlag aFlags);
		cAnimationTrack* GetTrack(int alIndex);
		cAnimationTrack* GetTrackByName(const tString &asName);
		void ResizeTracks(int alNum);
		int GetTrackNum();

		const char* GetAnimationName(){ return msAnimName.c_str();}
		void SetAnimationName(const tString &asName){ msAnimName =asName;}

		tString& GetFileName(){ return msFileName;}

		//Resources implementation
		bool Reload() override{ return false;}
		void Unload() override{}
		void Destroy() override{}

	private:
		tString msAnimName;
		tString msFileName;

		float mfLength;

		tAnimationTrackVec mvTracks;
	};

};
#endif // HPL_ANIMATION_H
