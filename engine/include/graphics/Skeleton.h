// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_SKELETON_H
#define HPL_SKELETON_H

#include <map>

#include "math/MathTypes.h"
#include "graphics/GraphicsTypes.h"
#include "system/SystemTypes.h"

namespace hpl {

	class cBone;

	typedef std::vector<cBone*> tBoneVec;
	typedef tBoneVec::iterator tBoneVecIt;

	typedef std::map<tString,int> tBoneIdxNameMap;
	typedef tBoneIdxNameMap::iterator tBoneIdxNameMapIt;

	class cSkeleton
	{
	public:
		cSkeleton();
		~cSkeleton();

		void AddBone(cBone* apBone);
		void RemoveBone(cBone* apBone);

		cBone* GetRootBone();

		cBone* GetBoneByIndex(int alIndex);
		cBone* GetBoneByName(const tString &asName);
		int GetBoneIndexByName(const tString &asName);
		int GetBoneNum();

	private:
		cBone* mpRootBone;

		tBoneVec mvBones;
		tBoneIdxNameMap m_mapBonesIdxByName;
	};

};
#endif // HPL_SKELETON_H
