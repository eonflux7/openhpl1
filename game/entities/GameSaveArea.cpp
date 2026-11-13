// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/StdAfx.h"
#include "entities/GameSaveArea.h"

#include "core/Init.h"
#include "core/MapHandler.h"
#include "player/Player.h"
#include "systems/EffectHandler.h"

#include "core/GlobalInit.h"

// LOADER

cAreaLoader_GameSaveArea::cAreaLoader_GameSaveArea(const tString &asName, cInit *apInit)
: iArea3DLoader(asName)
{
	mpInit = apInit;
}

cAreaLoader_GameSaveArea::~cAreaLoader_GameSaveArea()
{

}

iEntity3D* cAreaLoader_GameSaveArea::Load(const tString &asName, const cVector3f &avSize, 
									  const cMatrixf &a_mtxTransform,cWorld3D *apWorld)
{
	cGameSaveArea *pSaveArea = new cGameSaveArea(mpInit,asName);

	pSaveArea->m_mtxOnLoadTransform = a_mtxTransform;

	//Create physics data
	iPhysicsWorld *pPhysicsWorld = apWorld->GetPhysicsWorld();
	iCollideShape* pShape = pPhysicsWorld->CreateBoxShape(avSize,nullptr);
	std::vector<iPhysicsBody*> vBodies;
	vBodies.push_back(pPhysicsWorld->CreateBody(asName,pShape));

	vBodies[0]->SetCollide(false);
	vBodies[0]->SetCollideCharacter(false);
	vBodies[0]->SetMatrix(a_mtxTransform);

	vBodies[0]->SetUserData(pSaveArea);
	pSaveArea->SetBodies(vBodies);

	mpInit->mpMapHandler->AddGameEntity(pSaveArea);

	return nullptr;
}

cGameSaveArea::cGameSaveArea(cInit *apInit,const tString& asName) : iGameEntity(apInit,asName)
{
	mType = eGameEntityType_SaveArea;

	msDescription = kTranslate("Save", "DefaultDesc");

	msMessageCat = "";
	msMessageEntry = "";

	msSound = "";

	mbHasBeenUsed = false;
}

cGameSaveArea::~cGameSaveArea(void)
{
}

void cGameSaveArea::OnPlayerPick()
{
	if(mpInit->mpPlayer->GetPickedDist() <= 2.1)
	{
		mpInit->mpPlayer->SetCrossHairState(eCrossHairState_Active);
	}
}

void cGameSaveArea::OnPlayerInteract()
{
	if(mpInit->mpPlayer->GetPickedDist() <= 2.1)
	{
		mpInit->mpEffectHandler->GetSaveEffect()->NormalSave(mvBodies[0]->GetWorldPosition(),this);
	}
}

kBeginSerialize(cGameSaveArea_SaveData,iGameEntity_SaveData)
kSerializeVar(mvSize,eSerializeType_Vector3f)
kSerializeVar(msMessageCat, eSerializeType_String)
kSerializeVar(msMessageEntry, eSerializeType_String)

kSerializeVar(msSound, eSerializeType_String)

kSerializeVar(mbHasBeenUsed, eSerializeType_Bool)
kEndSerialize()

iGameEntity* cGameSaveArea_SaveData::CreateEntity()
{
	return nullptr;	
}

iGameEntity_SaveData* cGameSaveArea::CreateSaveData()
{
	return new cGameSaveArea_SaveData();
}

void cGameSaveArea::SaveToSaveData(iGameEntity_SaveData *apSaveData)
{
	__super::SaveToSaveData(apSaveData);
	cGameSaveArea_SaveData *pData = static_cast<cGameSaveArea_SaveData*>(apSaveData);

	pData->mvSize = mvBodies[0]->GetShape()->GetSize();

	kCopyToVar(pData,msMessageCat);
	kCopyToVar(pData,msMessageEntry);

	kCopyToVar(pData,msSound);

	kCopyToVar(pData,mbHasBeenUsed);
}

void cGameSaveArea::LoadFromSaveData(iGameEntity_SaveData *apSaveData)
{
	__super::LoadFromSaveData(apSaveData);
	cGameSaveArea_SaveData *pData = static_cast<cGameSaveArea_SaveData*>(apSaveData);

	kCopyFromVar(pData,msMessageCat);
	kCopyFromVar(pData,msMessageEntry);

	kCopyFromVar(pData,msSound);

	kCopyFromVar(pData,mbHasBeenUsed);
}
