// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/StdAfx.h"
#include "entities/GameForceArea.h"

#include "core/Init.h"
#include "core/MapHandler.h"
#include "player/Player.h"
#include "player/PlayerState.h"
#include "enemies/GameEnemy.h"

#include "core/GlobalInit.h"

// LOADER

cAreaLoader_GameForceArea::cAreaLoader_GameForceArea(const tString &asName, cInit *apInit)
: iArea3DLoader(asName)
{
	mpInit = apInit;
}

cAreaLoader_GameForceArea::~cAreaLoader_GameForceArea()
{

}

iEntity3D* cAreaLoader_GameForceArea::Load(const tString &asName, const cVector3f &avSize, 
									  const cMatrixf &a_mtxTransform,cWorld3D *apWorld)
{
	cGameForceArea *pArea = new cGameForceArea(mpInit,asName);

	pArea->m_mtxOnLoadTransform = a_mtxTransform;

	//Create physics data
	iPhysicsWorld *pPhysicsWorld = apWorld->GetPhysicsWorld();
	iCollideShape* pShape = pPhysicsWorld->CreateBoxShape(avSize,nullptr);
	std::vector<iPhysicsBody*> vBodies;
	vBodies.push_back(pPhysicsWorld->CreateBody(asName,pShape));

	vBodies[0]->SetCollide(false);
	vBodies[0]->SetCollideCharacter(false);
	vBodies[0]->SetMatrix(a_mtxTransform);

	vBodies[0]->SetUserData(pArea);
	pArea->SetBodies(vBodies);

	mpInit->mpMapHandler->AddGameEntity(pArea);

	pArea->Setup();
	
	// The map handler owns the game entity; no scene entity is returned.
	return nullptr;
}

cGameForceArea::cGameForceArea(cInit *apInit,const tString& asName) : iGameEntity(apInit,asName)
{
	mType = eGameEntityType_ForceArea;

	mfMaxForce = 1;
	mfConstant = 1;
	mfDestSpeed = 1;
	mfMaxMass = 0;

	mbMulWithMass = false;
	mbAffectCharacters = true;
	mbAffectBodies = true;
	mbForceAtPoint = false;
}

cGameForceArea::~cGameForceArea(void)
{
}

void cGameForceArea::Setup()
{
	mvForceDirection = cMath::MatrixInverse(mvBodies[0]->GetWorldMatrix()).GetUp();
	mvForceDirection.Normalise();
}

void cGameForceArea::Update(float afTimeStep)
{
	iPhysicsBody *pAreaBody = mvBodies[0];
	cWorld3D *pWorld = mpInit->mpGame->GetScene()->GetWorld3D();
	iPhysicsWorld *pPhysicsWorld = pWorld->GetPhysicsWorld();

	cCollideData collideData;
	collideData.SetMaxSize(4);

	// Check candidate bodies overlapping the area bounds.
	cPortalContainerEntityIterator bodyIt = pWorld->GetPortalContainer()->GetEntityIterator(
																pAreaBody->GetBoundingVolume());
	while(bodyIt.HasNext())
	{
		iPhysicsBody *pBody = static_cast<iPhysicsBody*>(bodyIt.Next());

		iGameEntity *pEntity = (iGameEntity*)pBody->GetUserData();

		if(pBody->GetCollide() && pBody->IsActive())
		{
			if(pEntity)
			{
				if(pEntity->IsActive()==false) continue;
				if(pBody->GetMass() ==0 && pEntity->GetType() != eGameEntityType_Enemy) continue;
			}
			else if((pBody->GetMass()==0 && pBody->IsCharacter()==false) || 
					pBody->GetMass() > mfMaxMass)
			{
				continue;
			}

			//Bounding volume check
			if(cMath::CheckCollisionBV(*pBody->GetBV(), *pAreaBody->GetBV())==false) continue;
				
			//Check for collision
			int lCollideNum = mbForceAtPoint ? 4: 1;

			if(	pPhysicsWorld->CheckShapeCollision(pBody->GetShape(),pBody->GetLocalMatrix(),
				pAreaBody->GetShape(), pAreaBody->GetLocalMatrix(),collideData,1)==false)
			{
				continue;
			}

			//Add Character Force
			if(pBody->IsCharacter() && mbAffectCharacters)
			{
				iCharacterBody *pCharBody = pBody->GetCharacterBody();

				float fSpeedAlongForce = cMath::Vector3Dot(mvForceDirection, pCharBody->GetForceVelocity());
				float fSpeedError = mfDestSpeed - fSpeedAlongForce;

				//If speed is above wanted, do nothing.
				if(fSpeedError<=0) continue;

				float fForce = fSpeedError * mfConstant;
				if(mfMaxForce != 0 && fForce > mfMaxForce) fForce = mfMaxForce;
				if(mbMulWithMass) fForce *= pCharBody->GetMass();

				fForce *= 10; // Apply the character-specific force multiplier.

                pCharBody->AddForce(mvForceDirection * fForce);
			}
			//Add Body Force
			else if(mbAffectBodies)
			{
				cVector3f vContactWorldPos =0;
				if(mbForceAtPoint)
				{
					for(int i=0; i< collideData.mlNumOfPoints; ++i)
					{
						vContactWorldPos += collideData.mvContactPoints[i].mvPoint;
					}
					vContactWorldPos = vContactWorldPos / (float)collideData.mlNumOfPoints;
				}

				//Get relative speed
				cVector3f vBodyVelocity = mbForceAtPoint ?	pBody->GetVelocityAtPosition(vContactWorldPos) :
													pBody->GetLinearVelocity();

				float fSpeedAlongForce = cMath::Vector3Dot(mvForceDirection, vBodyVelocity);
				float fSpeedError = mfDestSpeed - fSpeedAlongForce;
				
				//If speed is above wanted, do nothing.
				if(fSpeedError<=0) continue;

                float fForce = fSpeedError * mfConstant;
				if(mfMaxForce != 0 && fForce > mfMaxForce) fForce = mfMaxForce;
				if(mbMulWithMass) fForce *= pBody->GetMass();

				if(mbForceAtPoint)
				{
					pBody->AddForceAtPosition(mvForceDirection * fForce,vContactWorldPos);
				}
				else
				{
					pBody->AddForce(mvForceDirection * fForce);
				}
			}
		}
	}
}

void cGameForceArea::OnPostSceneDraw()
{
	return;
	iLowLevelGraphics *pLowGfx = mpInit->mpGame->GetGraphics()->GetLowLevel();

	mvBodies[0]->RenderDebugGeometry(pLowGfx,cColor(1,1,1,1));
}

kBeginSerialize(cGameForceArea_SaveData,iGameEntity_SaveData)
kSerializeVar(mvSize,eSerializeType_Vector3f)

kSerializeVar(mfMaxForce,eSerializeType_Float32)
kSerializeVar(mfConstant,eSerializeType_Float32)
kSerializeVar(mfDestSpeed,eSerializeType_Float32)
kSerializeVar(mfMaxMass,eSerializeType_Float32)

kSerializeVar(mbMulWithMass,eSerializeType_Bool)
kSerializeVar(mbAffectCharacters,eSerializeType_Bool)
kSerializeVar(mbAffectBodies,eSerializeType_Bool)
kSerializeVar(mbForceAtPoint,eSerializeType_Bool)
kEndSerialize()

iGameEntity* cGameForceArea_SaveData::CreateEntity()
{
	return nullptr;	
}

iGameEntity_SaveData* cGameForceArea::CreateSaveData()
{
	return new cGameForceArea_SaveData();
}

void cGameForceArea::SaveToSaveData(iGameEntity_SaveData *apSaveData)
{
	__super::SaveToSaveData(apSaveData);
	cGameForceArea_SaveData *pData = static_cast<cGameForceArea_SaveData*>(apSaveData);

	kCopyToVar(pData,mfMaxForce);
	kCopyToVar(pData,mfConstant);
	kCopyToVar(pData,mfDestSpeed);
	kCopyToVar(pData,mfMaxMass);

	kCopyToVar(pData,mbMulWithMass);
	kCopyToVar(pData,mbAffectCharacters);
	kCopyToVar(pData,mbAffectBodies);
	kCopyToVar(pData,mbForceAtPoint);

	pData->mvSize = mvBodies[0]->GetShape()->GetSize();
}

void cGameForceArea::LoadFromSaveData(iGameEntity_SaveData *apSaveData)
{
	__super::LoadFromSaveData(apSaveData);
	cGameForceArea_SaveData *pData = static_cast<cGameForceArea_SaveData*>(apSaveData);

	kCopyFromVar(pData,mfMaxForce);
	kCopyFromVar(pData,mfConstant);
	kCopyFromVar(pData,mfDestSpeed);
	kCopyFromVar(pData,mfMaxMass);

	kCopyFromVar(pData,mbMulWithMass);
	kCopyFromVar(pData,mbAffectCharacters);
	kCopyFromVar(pData,mbAffectBodies);
	kCopyFromVar(pData,mbForceAtPoint);
}

void cGameForceArea::SetupSaveData(iGameEntity_SaveData *apSaveData)
{
	__super::SetupSaveData(apSaveData);
	cGameForceArea_SaveData *pData = static_cast<cGameForceArea_SaveData*>(apSaveData);
}
