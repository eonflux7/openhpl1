// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "graphics/Skeleton.h"

#include "graphics/Bone.h"
#include "system/MemoryManager.h"
#include "system/LowLevelSystem.h"

namespace hpl {

	cSkeleton::cSkeleton()
	{
		mpRootBone = hplNew( cBone, ("__root_bone",this) );
		mpRootBone->SetTransform(cMatrixf::Identity);
	}

	cSkeleton::~cSkeleton()
	{
		//All bones are deleted in this call.
		//No need to delete the bones in the containers.
		hplDelete(mpRootBone);
	}

	void cSkeleton::AddBone(cBone* apBone)
	{
		mvBones.push_back(apBone);

		m_mapBonesIdxByName.insert(tBoneIdxNameMap::value_type(apBone->GetName(), (int)mvBones.size()-1));
	}

	void cSkeleton::RemoveBone(cBone* apBone)
	{
		tBoneVecIt vecIt = mvBones.begin();
		for(;vecIt != mvBones.end(); ++vecIt)
		{
			if(*vecIt == apBone)
			{
				mvBones.erase(vecIt);
				break;
			}
		}

		//Rebuild the map
		m_mapBonesIdxByName.clear();

		for(int i=0;i< (int) mvBones.size(); i++)
		{
			m_mapBonesIdxByName.insert(tBoneIdxNameMap::value_type(
								mvBones[i]->GetName(),i));
		}
	}

	cBone* cSkeleton::GetRootBone()
	{
		return mpRootBone;
	}

	cBone* cSkeleton::GetBoneByIndex(int alIndex)
	{
		return mvBones[alIndex];
	}

	cBone* cSkeleton::GetBoneByName(const tString &asName)
	{
		int alIdx = GetBoneIndexByName(asName);
		if(alIdx <0) return nullptr;

		return mvBones[alIdx];
	}

	int cSkeleton::GetBoneIndexByName(const tString &asName)
	{
		tBoneIdxNameMapIt it = m_mapBonesIdxByName.find(asName);
		if(it == m_mapBonesIdxByName.end()) return -1;

		return it->second;
	}
	int cSkeleton::GetBoneNum()
	{
		return (int)mvBones.size();
	}
	}
