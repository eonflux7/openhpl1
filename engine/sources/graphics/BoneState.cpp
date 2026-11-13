// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "graphics/BoneState.h"

#include "physics/PhysicsBody.h"

#include "math/Math.h"

#include "system/LowLevelSystem.h"

namespace hpl {

	cBoneState::cBoneState(const tString &asName,bool abAutoDeleteChildren) : cNode3D(asName,abAutoDeleteChildren)
	{
		mpBody = nullptr;
		mpColliderBody = nullptr;
	}

	void cBoneState::SetBody(iPhysicsBody *apBody)
	{
		mpBody = apBody;
	}
	iPhysicsBody* cBoneState::GetBody()
	{
		return mpBody;
	}

	void cBoneState::SetColliderBody(iPhysicsBody *apBody)
	{
		mpColliderBody = apBody;
	}
	iPhysicsBody* cBoneState::GetColliderBody()
	{
		return mpColliderBody;
	}

	void cBoneState::SetBodyMatrix(const cMatrixf &a_mtxBody)
	{
		m_mtxBody = a_mtxBody;
		m_mtxInvBody = cMath::MatrixInverse(m_mtxBody);
	}

	const cMatrixf& cBoneState::GetBodyMatrix()
	{
		return m_mtxBody;
	}

	const cMatrixf& cBoneState::GetInvBodyMatrix()
	{
		return m_mtxInvBody;
	}
}
