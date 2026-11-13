// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "graphics/Bone.h"

#include "graphics/Skeleton.h"
#include "math/Math.h"

#include "system/LowLevelSystem.h"


namespace hpl {

	cBone::cBone(const tString &asName, cSkeleton* apSkeleton)
	{
		msName = asName;
		mpSkeleton = apSkeleton;
		mpParent = nullptr;

		mbNeedsUpdate = true;

		mlValue = 0;
	}

	cBone::~cBone()
	{
		STLDeleteAll(mlstChildren);
	}

	cBone* cBone::CreateChildBone(const tString &asName)
	{
		cBone *pBone = hplNew(cBone,(asName, mpSkeleton) );
		pBone->mpParent = this;

		mlstChildren.push_back(pBone);
		mpSkeleton->AddBone(pBone);

		return pBone;
	}

	void cBone::SetTransform(const cMatrixf &a_mtxTransform)
	{
		m_mtxTransform = a_mtxTransform;

		NeedsUpdate();
	}

	const cMatrixf& cBone::GetLocalTransform()
	{
		return m_mtxTransform;
	}

	const cMatrixf& cBone::GetWorldTransform()
	{
		UpdateMatrix();

		return m_mtxWorldTransform;
	}

	const cMatrixf& cBone::GetInvWorldTransform()
	{
		UpdateMatrix();

		return m_mtxInvWorldTransform;
	}

	void cBone::Detach()
	{
		if(mpParent == nullptr) return;

		tBoneListIt it = mpParent->mlstChildren.begin();
		for(;it != mpParent->mlstChildren.end(); it++)
		{
			if(*it == this)
			{
				mpParent->mlstChildren.erase(it);
				break;
			}
		}

		mpSkeleton->RemoveBone(this);
	}

	cBoneIterator cBone::GetChildIterator()
	{
		return cBoneIterator(&mlstChildren);
	}

	void cBone::UpdateMatrix()
	{
		if(mbNeedsUpdate == false) return;

		if(mpParent==nullptr)
		{
			m_mtxWorldTransform = m_mtxTransform;
		}
		else
		{
			m_mtxWorldTransform = cMath::MatrixMul(mpParent->GetWorldTransform(),m_mtxTransform);
		}

		m_mtxInvWorldTransform = cMath::MatrixInverse(m_mtxWorldTransform);

		mbNeedsUpdate = false;
	}

	void cBone::NeedsUpdate()
	{
		mbNeedsUpdate = true;

		tBoneListIt it = mlstChildren.begin();
		for(;it != mlstChildren.end(); it++)
		{
			cBone* pBone = *it;

			pBone->NeedsUpdate();
		}
	}
}
