// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "physics/PhysicsJointHinge.h"

#include "scene/World3D.h"
#include "scene/Scene.h"
#include "game/Game.h"

#include "physics/PhysicsBody.h"
#include "physics/PhysicsWorld.h"

namespace hpl {

	kBeginSerialize(cSaveData_iPhysicsJointHinge, cSaveData_iPhysicsJoint)
	kSerializeVar(mfMaxAngle, eSerializeType_Float32)
	kSerializeVar(mfMinAngle, eSerializeType_Float32)
	kEndSerialize()

	iSaveObject* cSaveData_iPhysicsJointHinge::CreateSaveObject(cSaveObjectHandler *apSaveObjectHandler,cGame *apGame)
	{
		iPhysicsWorld *apWorld = apGame->GetScene()->GetWorld3D()->GetPhysicsWorld();

		cMatrixf mtxChildTemp, mtxParentTemp;

		iPhysicsBody *pChildBody = static_cast<iPhysicsBody*>(apSaveObjectHandler->Get(mlChildBodyId));
		if(pChildBody==nullptr) return nullptr;

		iPhysicsBody *pParentBody = nullptr;
		if(mlParentBodyId>0) pParentBody = static_cast<iPhysicsBody*>(apSaveObjectHandler->Get(mlParentBodyId));

		mtxChildTemp = pChildBody->GetLocalMatrix();
		if(pParentBody) mtxParentTemp = pParentBody->GetLocalMatrix();

		pChildBody->SetMatrix(m_mtxChildBodySetup);
		if(pParentBody) pParentBody->SetMatrix(m_mtxParentBodySetup);

		iPhysicsJointHinge *pJoint = apWorld->CreateJointHinge(msName,mvStartPivotPoint,mvPinDir,pParentBody,pChildBody);

		pChildBody->SetMatrix(mtxChildTemp);
		if(pParentBody) pParentBody->SetMatrix(mtxParentTemp);

		return pJoint;
	}

	int cSaveData_iPhysicsJointHinge::GetSaveCreatePrio()
	{
		return 1;
	}

	iSaveData* iPhysicsJointHinge::CreateSaveData()
	{
		return new cSaveData_iPhysicsJointHinge();
	}

	void iPhysicsJointHinge::SaveToSaveData(iSaveData *apSaveData)
	{
		kSaveData_SaveToBegin(iPhysicsJointHinge);

		kSaveData_SaveTo(mfMaxAngle);
		kSaveData_SaveTo(mfMinAngle);
	}

	void iPhysicsJointHinge::LoadFromSaveData(iSaveData *apSaveData)
	{
		kSaveData_LoadFromBegin(iPhysicsJointHinge);

		kSaveData_LoadFrom(mfMaxAngle);
		kSaveData_LoadFrom(mfMinAngle);
	}

	void iPhysicsJointHinge::SaveDataSetup(cSaveObjectHandler *apSaveObjectHandler, cGame *apGame)
	{
		kSaveData_SetupBegin(iPhysicsJointHinge);
	}
}
