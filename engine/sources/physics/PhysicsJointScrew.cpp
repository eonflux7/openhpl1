// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "physics/PhysicsJointScrew.h"

#include "scene/World3D.h"
#include "scene/Scene.h"
#include "game/Game.h"

#include "physics/PhysicsBody.h"
#include "physics/PhysicsWorld.h"

namespace hpl {

	kBeginSerialize(cSaveData_iPhysicsJointScrew, cSaveData_iPhysicsJoint)
	kSerializeVar(mfMaxDistance, eSerializeType_Float32)
	kSerializeVar(mfMinDistance, eSerializeType_Float32)
	kSerializeVar(mvPin, eSerializeType_Vector3f)
	kEndSerialize()

	iSaveObject* cSaveData_iPhysicsJointScrew::CreateSaveObject(cSaveObjectHandler *apSaveObjectHandler,cGame *apGame)
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

		iPhysicsJointScrew *pJoint = apWorld->CreateJointScrew(msName,mvStartPivotPoint,mvPinDir,pParentBody,pChildBody);

		pChildBody->SetMatrix(mtxChildTemp);
		if(pParentBody) pParentBody->SetMatrix(mtxParentTemp);

		return pJoint;
	}

	int cSaveData_iPhysicsJointScrew::GetSaveCreatePrio()
	{
		return 1;
	}

	iSaveData* iPhysicsJointScrew::CreateSaveData()
	{
		return new cSaveData_iPhysicsJointScrew();
	}

	void iPhysicsJointScrew::SaveToSaveData(iSaveData *apSaveData)
	{
		kSaveData_SaveToBegin(iPhysicsJointScrew);

		kSaveData_SaveTo(mfMaxDistance);
		kSaveData_SaveTo(mfMinDistance);
		kSaveData_SaveTo(mvPin);
	}

	void iPhysicsJointScrew::LoadFromSaveData(iSaveData *apSaveData)
	{
		kSaveData_LoadFromBegin(iPhysicsJointScrew);

		kSaveData_LoadFrom(mfMaxDistance);
		kSaveData_LoadFrom(mfMinDistance);
		kSaveData_LoadFrom(mvPin);
	}

	void iPhysicsJointScrew::SaveDataSetup(cSaveObjectHandler *apSaveObjectHandler, cGame *apGame)
	{
		kSaveData_SetupBegin(iPhysicsJointScrew);
	}
}
