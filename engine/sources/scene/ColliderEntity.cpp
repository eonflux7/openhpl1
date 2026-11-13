// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "scene/ColliderEntity.h"

#include "physics/PhysicsWorld.h"
#include "physics/PhysicsBody.h"
#include "physics/CollideShape.h"

namespace hpl {

	cColliderEntity::cColliderEntity(const tString &asName,iPhysicsBody* apBody, iPhysicsWorld *apWorld):
	iRenderable(asName)
	{
		mpPhysicsWorld = apWorld;
		mpBody = apBody;

		mBoundingVolume = apBody->GetShape()->GetBoundingVolume();
	}

	cColliderEntity::~cColliderEntity()
	{
		if(mpBody && mpPhysicsWorld)
			mpPhysicsWorld->DestroyBody(mpBody);
	}

	cBoundingVolume* cColliderEntity::GetBoundingVolume()
	{
		return &mBoundingVolume;
	}

	cMatrixf* cColliderEntity::GetModelMatrix(cCamera3D *apCamera)
	{
		return &GetWorldMatrix();
	}

	int cColliderEntity::GetMatrixUpdateCount()
	{
		return GetTransformUpdateCount();
	}
}
