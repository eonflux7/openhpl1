// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/StdAfx.h"
#include "entities/GameDamageArea.h"

#include "core/Init.h"
#include "core/MapHandler.h"
#include "player/Player.h"
#include "player/PlayerState.h"
#include "enemies/GameEnemy.h"

#include "core/GlobalInit.h"

// LOADER

cAreaLoader_GameDamageArea::cAreaLoader_GameDamageArea(const tString &asName, cInit *apInit)
: iArea3DLoader(asName)
{
	mpInit = apInit;
}

cAreaLoader_GameDamageArea::~cAreaLoader_GameDamageArea()
{

}

iEntity3D* cAreaLoader_GameDamageArea::Load(const tString &asName, const cVector3f &avSize, 
									  const cMatrixf &a_mtxTransform,cWorld3D *apWorld)
{
	cGameDamageArea *pArea = hplNew( cGameDamageArea, (mpInit,asName) );

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
	
	// The map handler owns the game entity; no scene entity is returned.
	return nullptr;
}

cGameDamageArea::cGameDamageArea(cInit *apInit,const tString& asName) : iGameEntity(apInit,asName)
{
	mType = eGameEntityType_DamageArea;

	mfDamage = 1;
	mfUpdatesPerSec = 1;
	mlStrength = 1;
	mbDisableObjects = false;
	mbDisableEnemies = false;

	mbHasInteraction = false;

	mfDamageDelaySeconds =0;
}

cGameDamageArea::~cGameDamageArea(void)
{
}

void cGameDamageArea::OnPlayerPick()
{
}

void cGameDamageArea::Update(float afTimeStep)
{
	if(mfDamageDelaySeconds <=0)
	{
		mfDamageDelaySeconds = 1/mfUpdatesPerSec;

		iPhysicsBody *pAreaBody = mvBodies[0];
		cWorld3D *pWorld = mpInit->mpGame->GetScene()->GetWorld3D();
		iPhysicsWorld *pPhysicsWorld = pWorld->GetPhysicsWorld();

		cCollideData collideData;
		collideData.SetMaxSize(1);
		
		// Snapshot candidates in case damage changes the portal container.
		std::list<iPhysicsBody*> lstBodies;
		cPortalContainerEntityIterator bodyIt = pWorld->GetPortalContainer()->GetEntityIterator(
																	pAreaBody->GetBoundingVolume());
		while(bodyIt.HasNext())
		{
			iPhysicsBody *pBody = static_cast<iPhysicsBody*>(bodyIt.Next());
			lstBodies.push_back(pBody);
		}

		// Check candidate bodies overlapping the area bounds.
		std::list<iPhysicsBody*>::iterator it = lstBodies.begin();
        for(; it != lstBodies.end(); ++it)
		{
			iPhysicsBody *pBody = *it;
			iGameEntity *pEntity = (iGameEntity*)pBody->GetUserData();

			if(pBody->GetCollide() && pBody->IsActive())
			{
				if(pEntity)
				{
					if(pEntity->GetHealth() <=0 || pEntity->IsActive()==false) continue;
				}
				else if(mpInit->mpPlayer->GetCharacterBody()->GetBody() != pBody)
				{
					continue;
				}

				//Bounding volume check
				if(cMath::CheckCollisionBV(*pBody->GetBV(), *pAreaBody->GetBV())==false) continue;
				
				//Check for collision
				if(pPhysicsWorld->CheckShapeCollision(pBody->GetShape(),pBody->GetLocalMatrix(),
					pAreaBody->GetShape(), pAreaBody->GetLocalMatrix(),collideData,1)==false)
				{
					continue;
				}

				//Player
				if(mpInit->mpPlayer->GetCharacterBody()->GetBody() == pBody)
				{
					mpInit->mpPlayer->Damage(mfDamage,(ePlayerDamageType)mDamageType);

					if(msDamageSound != "")
						mpInit->mpGame->GetSound()->GetSoundHandler()->PlayGui(msDamageSound,false,1);
				}
				else if(pEntity)
				{
					if( (pEntity->GetType() == eGameEntityType_Object && mbDisableObjects) ||
						(pEntity->GetType() == eGameEntityType_Enemy && mbDisableEnemies))
					{
						if( pEntity->GetType() == eGameEntityType_Enemy && 
							pEntity->GetSubType() == "Worm")
						{
							return;
						}

						pEntity->SetActive(false);
					}
					else
					{
						pEntity->Damage(mfDamage,mlStrength);
					}
				}
			}
		}
	}
	else
	{
		mfDamageDelaySeconds -= afTimeStep;
	}

}

kBeginSerialize(cGameDamageArea_SaveData,iGameEntity_SaveData)
kSerializeVar(mvSize,eSerializeType_Vector3f)
kSerializeVar(mfDamage, eSerializeType_Float32)
kSerializeVar(mfUpdatesPerSec, eSerializeType_Float32)
kSerializeVar(mlStrength, eSerializeType_Int32)
kSerializeVar(mbDisableObjects, eSerializeType_Bool)
kSerializeVar(mbDisableEnemies, eSerializeType_Bool)
kSerializeVar(mDamageType, eSerializeType_Int32)
kSerializeVar(msDamageSound, eSerializeType_String)
kEndSerialize()

iGameEntity* cGameDamageArea_SaveData::CreateEntity()
{
	return nullptr;	
}

iGameEntity_SaveData* cGameDamageArea::CreateSaveData()
{
	return hplNew( cGameDamageArea_SaveData, () );
}

void cGameDamageArea::SaveToSaveData(iGameEntity_SaveData *apSaveData)
{
	__super::SaveToSaveData(apSaveData);
	cGameDamageArea_SaveData *pData = static_cast<cGameDamageArea_SaveData*>(apSaveData);

	kCopyToVar(pData, mfDamage);
	kCopyToVar(pData,mfUpdatesPerSec);
	kCopyToVar(pData,mlStrength);
	kCopyToVar(pData,mbDisableObjects);
	kCopyToVar(pData,mbDisableEnemies);
	kCopyToVar(pData,mDamageType);
	kCopyToVar(pData,msDamageSound);

	pData->mvSize = mvBodies[0]->GetShape()->GetSize();
}

void cGameDamageArea::LoadFromSaveData(iGameEntity_SaveData *apSaveData)
{
	__super::LoadFromSaveData(apSaveData);
	cGameDamageArea_SaveData *pData = static_cast<cGameDamageArea_SaveData*>(apSaveData);

	kCopyFromVar(pData,mfDamage);
	kCopyFromVar(pData,mfUpdatesPerSec);
	kCopyFromVar(pData,mlStrength);
	kCopyFromVar(pData,mbDisableObjects);
	kCopyFromVar(pData,mbDisableEnemies);
	kCopyFromVar(pData,mDamageType);
	kCopyFromVar(pData,msDamageSound);

}

void cGameDamageArea::SetupSaveData(iGameEntity_SaveData *apSaveData)
{
	__super::SetupSaveData(apSaveData);
	cGameDamageArea_SaveData *pData = static_cast<cGameDamageArea_SaveData*>(apSaveData);
}
